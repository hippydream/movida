/**************************************************************************
** Filename: sdtreewidget.cpp
**
** Copyright (C) 2007-2009 Angius Fabrizio. All rights reserved.
**
** This file is part of the Movida project (http://movida.42cows.org/).
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See the file LICENSE.GPL that came with this software distribution or
** visit http://www.gnu.org/copyleft/gpl.html for GPL licensing information.
**
**************************************************************************/

#include "sdtreewidget.h"

#include "guiglobal.h"

#include "mvdcore/core.h"
#include "mvdcore/logger.h"
#include "mvdcore/settings.h"
#include "mvdcore/utils.h"

#include "mvdshared/clearedit.h"
#include "mvdshared/combobox.h"
#include "mvdshared/completer.h"

#include <QtCore/QList>
#include <QtCore/QUrl>
#include <QtCore/QtDebug>
#include <QtGui/QDesktopServices>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QStringListModel>

/*!
    \class MvdSDTreeWidget sdtreewidget.h
    \ingroup Movida

    \brief MvdTreeWidget for display and editing of a movie's shared data.
    \todo Override tab order when a new item is being edited!
*/


namespace {
    enum Columns {
        RolesColumn = 1
    };

    const bool PlaceHolderAtBottom = true;
}

/************************************************************************
    MvdSDTreeWidget
 *************************************************************************/

/*!
    Creates a new empty view with Movida::DataRole set to Movida::NoRole.
*/
MvdSDTreeWidget::MvdSDTreeWidget(QWidget *parent) :
    MvdTreeWidget(parent),
    mCollection(0),
    mDataRole(Movida::NoRole)
{
    init();
}

/*!
    Creates a new view for the specified movie.
*/
MvdSDTreeWidget::MvdSDTreeWidget(Movida::DataRole ds, const MvdMovie &movie,
    MvdMovieCollection *c, QWidget *parent) :
    MvdTreeWidget(parent),
    mCollection(0),
    mDataRole(Movida::NoRole)
{
    init();
    setMovieCollection(c);
    setMovie(movie);
    setDataRole(ds);
}

//! \internal
void MvdSDTreeWidget::init()
{
    setSortingEnabled(false);
    setDragDropMode(MvdSDTreeWidget::InternalMove);
    setEditTriggers((EditTriggers)(AllEditTriggers & ~CurrentChanged));

    mModified = false;
    header()->setMovable(false);

    connect(this, SIGNAL(contextMenuRequested(QTreeWidgetItem *, int)), this, SLOT(showContextMenu(QTreeWidgetItem *, int)));

    connect(model(), SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(updatedModifiedStatus()));
    connect(model(), SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(updatedModifiedStatus()));

    setItemDelegate(new MvdSDDelegate(this));
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    appendPlaceHolder();
}

/*!
    Changes the data role for this view and clears the modified status.
    Note: The view is updated even if the new data role is the same as the
    old one.
*/
void MvdSDTreeWidget::setDataRole(Movida::DataRole ds)
{
    mDataRole = ds;

    clear();
    appendPlaceHolder();
    setModified(false);

    QStringList labels;
    QList<Movida::SharedDataAttribute> attributes =
        Movida::sharedDataAttributes(mDataRole, Movida::MainAttributeFilter);

    for (int i = 0; i < attributes.size(); ++i)
        labels << Movida::sharedDataAttributeString(attributes.at(i));

    setHeaderLabels(labels);

    if (attributes.size() > 1) {
        // Attempt to adjust column sizes - future releases might require some more logic here
        header()->setResizeMode(0, QHeaderView::Stretch);
        header()->setResizeMode(1, QHeaderView::Stretch);
        header()->setStretchLastSection(false);
    }

    if (mCollection == 0 || !mMovie.isValid())
        return;

    switch (ds) {
        case Movida::ActorRole:
            setPersonRoleData(mMovie.actors());
            break;

        case Movida::CrewMemberRole:
            setPersonRoleData(mMovie.crewMembers());
            break;

        case Movida::DirectorRole:
            setSimpleData(mMovie.directors());
            break;

        case Movida::ProducerRole:
            setSimpleData(mMovie.producers());
            break;

        case Movida::GenreRole:
            setSimpleData(mMovie.genres());
            break;

        case Movida::TagRole:
            setSimpleData(mMovie.tags());
            break;

        case Movida::CountryRole:
            setSimpleData(mMovie.countries());
            break;

        case Movida::LanguageRole:
            setSimpleData(mMovie.languages());
            break;

        default:
            ;
    }
}

/*!
    Convenience method, same as calling setDataRole(Movida::DataRole) with the
    same old data role. Clears the modified status.
*/
void MvdSDTreeWidget::resetToDefaults()
{
    setDataRole(mDataRole);
}

/*!
    Sets an existing movie as data source. You still need to call either
    setDataRole() or resetToDefaults() to load the data into the view.
*/
void MvdSDTreeWidget::setMovie(const MvdMovie &m)
{
    mMovie = m;
}

/*!
    Stores the current values to a movie object, replacing existing ones.
    Sets the modified status to false (without triggering any signal).
*/
void MvdSDTreeWidget::store(MvdMovie &m)
{
    if (!mCollection)
        return;

    QList<MvdRoleItem> roleData;
    QList<mvdid> stringData;

    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = topLevelItem(i);
        if (isPlaceHolder(item))
            continue;

        mvdid id = item->data(0, Movida::IdRole).toUInt();
        if (id == MvdNull) {
            // Register a new shared item.

            MvdSdItem sdItem;
            sdItem.role = mDataRole;
            sdItem.value = item->text(0);

            id = mCollection->sharedData().addItem(sdItem);
        }

        // Re-check id as we might have added a new item to the SD
        if (id != MvdNull) {
            if (mDataRole == Movida::ActorRole || mDataRole == Movida::CrewMemberRole)
                roleData.append(MvdRoleItem(id, splitString(item->text(::RolesColumn))));
            else stringData.append(id);
        }
    }

    switch (mDataRole) {
        case Movida::ActorRole:
            m.setActors(roleData);
            break;

        case Movida::CrewMemberRole:
            m.setCrewMembers(roleData);
            break;

        case Movida::DirectorRole:
            m.setDirectors(stringData);
            break;

        case Movida::ProducerRole:
            m.setProducers(stringData);
            break;

        case Movida::GenreRole:
            m.setGenres(stringData);
            break;

        case Movida::TagRole:
            m.setTags(stringData);
            break;

        case Movida::CountryRole:
            m.setCountries(stringData);
            break;

        case Movida::LanguageRole:
            m.setLanguages(stringData);
            break;

        default:
            ;
    }

    mModified = false;
}

/*!
    Sets the movie collection used to resolve collection-related shared data.
    You still need to call either setDataRole() or reset() to load the data into the view.
*/
void MvdSDTreeWidget::setMovieCollection(MvdMovieCollection *mc)
{
    mCollection = mc;
}

//! Returns the current values in the tree view, eventually excluding \p excluded if it is not 0.
QList<quint32> MvdSDTreeWidget::currentValues(quint32 excluded, bool excludeNewItems) const
{
    QList<quint32> list;
    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = topLevelItem(i);
        if (isPlaceHolder(item))
            continue;

        mvdid id = item->data(0, Movida::IdRole).toUInt();
        if ((id == MvdNull && !excludeNewItems) || (id != MvdNull && id != excluded))
            list << id;
    }

    return list;
}

/*!
    Returns true if any data has been modified.
*/
bool MvdSDTreeWidget::isModified() const
{
    return mModified;
}

//! \internal Sets the modified status and evtl. triggers the modifiedStatusChanged() signal.
void MvdSDTreeWidget::setModified(bool m)
{
    if (m == mModified)
        return;

    mModified = m;
    emit modifiedStatusChanged(m);
}

/*!
    \internal
*/
void MvdSDTreeWidget::updatedModifiedStatus()
{
    // We are editing a new movie
    if (!mCollection || !mMovie.isValid()) {
        setModified(this->topLevelItemCount() != 0);
        return;
    }

    // We are editing an existing movie: check for changed values
    QList<quint32> current = currentValues(0, false);
    QList<mvdid> original;

    switch (mDataRole) {
        case Movida::ActorRole:
            original = mMovie.actorIDs(); break;

        case Movida::CrewMemberRole:
            original = mMovie.crewMemberIDs(); break;

        case Movida::DirectorRole:
            original = mMovie.directors(); break;

        case Movida::ProducerRole:
            original = mMovie.producers(); break;

        case Movida::GenreRole:
            original = mMovie.genres(); break;

        case Movida::TagRole:
            original = mMovie.tags(); break;

        case Movida::CountryRole:
            original = mMovie.countries(); break;

        case Movida::LanguageRole:
            original = mMovie.languages(); break;

        default:
            ;
    }

    if (current.size() != original.size()) {
        setModified(true);
        return;
    }

    // implicit ASSERT(current.size() == original.size()) follows from above statement
    for (int i = 0; i < current.size(); ++i) {
        if (current.at(i) != original.at(i)) {
            setModified(true);
            return;
        }
    }

    // Compare role info: we can't do it in the prev. loop because we have IDs only
    if (mDataRole == Movida::ActorRole || mDataRole == Movida::CrewMemberRole) {
        for (int i = 0; i < topLevelItemCount(); ++i) {
            QTreeWidgetItem *item = topLevelItem(i);
            if (isPlaceHolder(item))
                continue;

            mvdid id = item->data(0, Movida::IdRole).toUInt();
            QStringList roles = splitString(item->text(::RolesColumn));

            QStringList originalRoles = mDataRole == Movida::ActorRole ?
                                        mMovie.actorRoles(id) : mMovie.crewMemberRoles(id);

            if (roles.size() != originalRoles.size()) {
                setModified(true);
                return;
            }

            qSort(roles.begin(), roles.end(), Movida::LocaleAwareSorter());
            qSort(originalRoles.begin(), originalRoles.end(), Movida::LocaleAwareSorter());

            // implicit ASSERT(roles.size() == originalRoles.size()) follows from above statement
            for (int j = 0; j < roles.size(); ++j) {
                if (roles.at(j) != originalRoles.at(j)) {
                    setModified(true);
                    return;
                }
            }
        }
    }

    setModified(false);
}

/*!
    \internal Populates the view with person+role descriptions.
*/
void MvdSDTreeWidget::setPersonRoleData(const QList<MvdRoleItem> &d)
{
    if (!mCollection)
        return;

    for (int i = 0; i < d.size(); ++i) {
        const MvdRoleItem &ri = d.at(i);
        mvdid id = ri.first;
        QStringList roles = ri.second;

        MvdSdItem pd = mCollection->sharedData().item(id);

        MvdTreeWidgetItem *item = new MvdTreeWidgetItem(this);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setData(0, Movida::IdRole, id);
        item->setText(0, pd.value);
        item->setText(::RolesColumn, joinStringList(roles, "; "));
    }
}

//! \internal
QString MvdSDTreeWidget::joinStringList(const QStringList &list,
    const QString &sep, const QString &def) const
{
    QString s;

    for (int i = 0; i < list.size(); ++i) {
        QString x = list.at(i);

        if (x.isEmpty())
            x = def;

        if (!x.isEmpty()) {
            if (!s.isEmpty())
                s.append(sep);
            s.append(x);
        }
    }

    return s.isEmpty() ? def : s;
}

QStringList MvdSDTreeWidget::splitString(const QString &s) const
{
    QStringList dirty = s.split(QRegExp("[;,/]"), QString::SkipEmptyParts);
    QStringList clean;

    for (int i = 0; i < dirty.size(); ++i) {
        QString item = dirty.at(i).trimmed();
        if (!item.isEmpty())
            clean.append(item);
    }

    return clean;
}

//! \internal Populates the view with simple data descriptions.
void MvdSDTreeWidget::setSimpleData(const QList<mvdid> &d)
{
    for (int i = 0; i < d.size(); ++i) {
        mvdid id = d.at(i);
        MvdSdItem sdItem = mCollection->sharedData().item(id);

        MvdTreeWidgetItem *item = new MvdTreeWidgetItem(this);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setData(0, Movida::IdRole, id);
        item->setText(0, sdItem.value);
    }
}

//! \internal
void MvdSDTreeWidget::showContextMenu(QTreeWidgetItem *item, int col)
{
    Q_UNUSED(col);
    Q_UNUSED(item);

    if (!mCollection)
        return;

    int itemCount;
    int maxMenuItems = Movida::core().parameter("movida/max-menu-items").toInt();

    QList<QTreeWidgetItem *> selected = filteredSelectedItems();
    ActionItemList actions = generateActions(0, &itemCount, maxMenuItems);

    // Create context menu
    QMenu menu;

    // ************* ADD/MANAGE
    if (itemCount != 0) {
        if (itemCount <= maxMenuItems && !actions.isEmpty()) {
            QMenu *addMenu = createItemMenu(tr("Add existing"), actions, AddItemAction);
            menu.addMenu(addMenu);
        } else {
            QAction *addAction = menu.addAction(tr("Add existing"));
            addAction->setData(qVariantFromValue(ActionDescriptor(ShowItemSelectorAction)));
        }
    }

    QAction *editorAction = menu.addAction(tr("Manage lists..."));
    editorAction->setData(qVariantFromValue(ActionDescriptor(ShowEditorAction)));

    menu.addSeparator();

    // ************* REMOVE
    if (!selected.isEmpty()) {
        QAction *removeAction = menu.addAction(tr("Delete selected item(s)", "", selected.size()));
        removeAction->setShortcut(tr("Del", "Delete selected items"));
        removeAction->setData(qVariantFromValue(ActionDescriptor(RemoveItemAction, selected)));
    }

    QAction *res = menu.exec(QCursor::pos());
    if (res == 0)
        return;

    executeAction(res->data().value<ActionDescriptor>());
}

/*!
    Extracts action descriptions from the SD.
    Returns an empty map if there are more than \p max items (if \p max is not negative).
*/
MvdSDTreeWidget::ActionItemList MvdSDTreeWidget::generateActions(quint32 sel, int *_itemCount, int max)
{
    Q_ASSERT(mCollection);

    ActionItemList actions;
    QList<quint32> current = currentValues();

    int itemCount = 0;
    Movida::DataRole role = mDataRole;

    switch (mDataRole) {
        case Movida::ActorRole:
        case Movida::DirectorRole:
        case Movida::ProducerRole:
        case Movida::CrewMemberRole:
            role = Movida::PersonRole;

        default:
            ;
    }

    itemCount = mCollection->sharedData().countItems(role);
    Q_ASSERT(itemCount >= current.size());

    itemCount = itemCount - current.size();

    if (max < 0 || itemCount <= max) {
        MvdSharedData::ItemList items = mCollection->sharedData().items(role);
        generateActions(items, current, actions, sel);
    }

    if (_itemCount != 0)
        *_itemCount = itemCount;

    return actions;
}

//! \internal Creates a map of labels and action descriptors suitable for a menu.
void MvdSDTreeWidget::generateActions(const MvdSharedData::ItemList &itemList,
    const QList<quint32> &current,
    ActionItemList &actions, quint32 selected)
{
    MvdSharedData::ItemPairVector d = MvdSharedData::sortedItemVector(itemList);
    for (int i = 0; i < d.size(); ++i) {
        const MvdSharedData::ItemPair& p = d[i];
        const mvdid& id = p.first;
        const MvdSdItem& item = p.second;

        if ((selected != 0 && id == selected) || !current.contains(id)) {
            const QString& value = item.value;
            if (!value.isEmpty()) {
                ActionDescriptor ad;
                ad.itemIds << id;
                ad.current = (id == selected);
                actions.append(ActionItem(value, ad));
            }
        }
    }
}

//! \internal Creates a menu with items of given type.
QMenu *MvdSDTreeWidget::createItemMenu(const QString &label,
    const ActionItemList &actions, ActionType type)
{
    QMenu *menu = new QMenu(label);

    ActionItemList::ConstIterator begin = actions.constBegin();
    ActionItemList::ConstIterator end = actions.constEnd();
    while (begin != end) {
        const ActionItem& aitem = *begin;
        const QString& name = aitem.first;
        const ActionDescriptor& _ad = aitem.second;
        if (!_ad.current) {
            ActionDescriptor ad = _ad;
            ad.type = type;

            QAction *a = new QAction(name, this);
            a->setData(qVariantFromValue(ad));

            menu->addAction(a);
        }
        ++begin;
    }

    return menu;
}

//! \internal Handles an action. \p data may contain optional parameters.
void MvdSDTreeWidget::executeAction(ActionDescriptor ad, const QVariant &data)
{
    Q_UNUSED(data);

    switch (ad.type) {
        case AddItemAction:
            if (ad.current)
                break;
            Q_ASSERT(!ad.itemIds.isEmpty());
            if (mDataRole == Movida::ActorRole || mDataRole == Movida::CrewMemberRole) {
                QList<MvdRoleItem> l;
                l.append(MvdRoleItem(ad.itemIds.first(), QStringList()));
                setPersonRoleData(l);
            } else {
                QList<mvdid> l;
                l << ad.itemIds.first();
                setSimpleData(l);
            }
            break;

        case RemoveItemAction:
            qDeleteAll(ad.items);
            break;

        case ShowEditorAction:
            QMessageBox::warning(this, MVD_CAPTION, "Not implemented");
            break;

        case ShowItemSelectorAction:
            /*
               {
               MvdTreeWidgetItem* item = new MvdTreeWidgetItem(this);
               item->setFlags(item->flags() | Qt::ItemIsEditable);
               item->setData(0, Movida::IdRole, 0);
               setCurrentItem(item);
               editItem(item);
               }
             */
            QMessageBox::warning(this, MVD_CAPTION, "Not implemented");
            break;

        default:
            ;
    }

    updatedModifiedStatus();
}

/*!
    Handle keyboard shortcuts.
*/
void MvdSDTreeWidget::keyPressEvent(QKeyEvent *event)
{
    QTreeWidgetItem *item = currentItem();

    bool ok = true;
    mvdid currentId = item ? item->data(0, Movida::IdRole).toUInt(&ok) : MvdNull;

    if (!ok)
        currentId = MvdNull;

    QList<QTreeWidgetItem *> selected = filteredSelectedItems();

    switch (event->key()) {
        case Qt::Key_Delete:
            if (!selected.isEmpty()) {
                executeAction(ActionDescriptor(RemoveItemAction, selected));
                return;
            }
    }

    MvdTreeWidget::keyPressEvent(event);
}

MvdTreeWidgetItem *MvdSDTreeWidget::appendPlaceHolder()
{
    if (mDataRole == Movida::NoRole)
        return 0;

    int phIndex = 0;
    if (PlaceHolderAtBottom) {
        phIndex = topLevelItemCount();
    }

    MvdTreeWidgetItem *item = new MvdTreeWidgetItem;
    insertTopLevelItem(phIndex, item);
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    QFont f = item->font(0);
    item->setData(0, Movida::FontBackupRole, f);
    f.setItalic(true);
    item->setFont(0, f);

    f = item->font(1);
    item->setData(1, Movida::FontBackupRole, f);
    f.setItalic(true);
    item->setFont(1, f);

    switch (mDataRole) {
        case Movida::ActorRole:
            item->setText(0, tr("New actor"));
            item->setData(0, Qt::ToolTipRole, tr("Edit here to add a new actor"));
            item->setData(::RolesColumn, Qt::ToolTipRole, tr("Separate roles with a semi colon (\";\"), comma (\",\") or slash (\"/\")"));
            break;

        case Movida::DirectorRole:
            item->setText(0, tr("New director"));
            item->setData(0, Qt::ToolTipRole, tr("Edit here to add a new director"));
            item->setData(::RolesColumn, Qt::ToolTipRole, tr("Separate roles with a semi colon (\";\"), comma (\",\") or slash (\"/\")"));
            break;

        case Movida::ProducerRole:
            item->setText(0, tr("New producer"));
            item->setData(0, Qt::ToolTipRole, tr("Edit here to add a new producer"));
            item->setData(::RolesColumn, Qt::ToolTipRole, tr("Separate roles with a semi colon (\";\"), comma (\",\") or slash (\"/\")"));
            break;

        case Movida::CrewMemberRole:
            item->setText(0, tr("New crew member"));
            item->setData(0, Qt::ToolTipRole, tr("Edit here to add a new crew member"));
            item->setData(::RolesColumn, Qt::ToolTipRole, tr("Separate roles with a semi colon (\";\"), comma (\",\") or slash (\"/\")"));
            break;

        case Movida::GenreRole:
            item->setText(0, tr("New genre"));
            item->setData(0, Qt::ToolTipRole, tr("Edit here to add a new genre"));
            break;

        case Movida::LanguageRole:
            item->setText(0, tr("New language"));
            item->setData(0, Qt::ToolTipRole, tr("Edit here to add a new language"));
            break;

        case Movida::CountryRole:
            item->setText(0, tr("New country"));
            item->setData(0, Qt::ToolTipRole, tr("Edit here to add a new country"));
            break;

        case Movida::TagRole:
            item->setText(0, tr("New tag"));
            item->setData(0, Qt::ToolTipRole, tr("Edit here to add a new tag"));
            break;

        default:
            item->setText(0, tr("New item"));
            item->setData(0, Qt::ToolTipRole, tr("Edit here to add a new item"));
    }

    item->setData(0, Movida::PlaceholderRole, true);

    item->setData(0, Movida::TextColorBackupRole, item->data(0, Qt::TextColorRole));
    item->setData(0, Qt::TextColorRole, qVariantFromValue<QColor>(QColor("#585858")));
    item->setData(1, Movida::TextColorBackupRole, item->data(1, Qt::TextColorRole));
    item->setData(1, Qt::TextColorRole, qVariantFromValue<QColor>(QColor("#585858")));

    return item;
}

void MvdSDTreeWidget::removePlaceHolder(QTreeWidgetItem *item)
{
    if (!item || !isPlaceHolder(item))
        return;

    item->setData(0, Movida::PlaceholderRole, QVariant());

    QVariant c = item->data(0, Movida::TextColorBackupRole);
    item->setData(0, Movida::TextColorBackupRole, QVariant());
    item->setData(0, Qt::TextColorRole, c);
    c = item->data(1, Movida::TextColorBackupRole);
    item->setData(1, Movida::TextColorBackupRole, QVariant());
    item->setData(1, Qt::TextColorRole, c);

    QVariant f = item->data(0, Movida::FontBackupRole);
    item->setData(0, Movida::FontBackupRole, QVariant());
    item->setFont(0, qVariantValue<QFont>(f));
    f = item->data(1, Movida::FontBackupRole);
    item->setData(1, Movida::FontBackupRole, QVariant());
    item->setFont(1, qVariantValue<QFont>(f));
}

void MvdSDTreeWidget::startDrag(Qt::DropActions supportedActions)
{
    // Ensure no placeholder is involved in a drag
    QTreeWidgetItem *item = topLevelItem(0);

    if (item && isPlaceHolder(item)) {
        setItemSelected(item, false);
    }
    MvdTreeWidget::startDrag(supportedActions);
}

void MvdSDTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    MvdTreeWidget::dragMoveEvent(event);     // This will update the drop indicator
    if (dropIndicatorPosition() == MvdSDTreeWidget::OnItem) {
        // Only accept real move operations and not replacing items!
        event->setAccepted(false);
        return;
    }
}

/************************************************************************
    MvdSDDelegate
 *************************************************************************/

//! \internal
MvdSDDelegate::MvdSDDelegate(MvdSDTreeWidget *parent) :
    QItemDelegate(parent)
{ }

//! \internal Convenience method.
MvdSDTreeWidget *MvdSDDelegate::tree() const
{
    return qobject_cast<MvdSDTreeWidget *>(this->parent());
}

//! \internal Returns the validator associated to a column and/or row (if any).
Movida::ItemValidator MvdSDDelegate::validatorType(const QModelIndex &index,
    const MvdSDTreeWidget &tree, QVariant *data, ValidatorUse use) const
{
    Q_UNUSED(use);
    Q_UNUSED(data);

    //! \todo use columns as defined in Movida namespace.
    /*
       Movida::DataRole ds = tree.dataRole();
       if ((Movida::PersonRole & ds) && index.column() == 1)
       {
       if (data)
       {
       if (use == ValidationUse)
       data->setValue(Movida::core().parameter("mvdcore/imdb-id-regexp").toString());
       else data->setValue(Movida::core().parameter("mvdcore/imdb-id-mask").toString());
       }
       return Movida::RegExpValidator;
       }
     */

    if (tree.isPlaceHolder(tree.topLevelItem(index.row())))
        return Movida::UndoEmptyValitator;

    return Movida::NoValidator;
}

//! \internal Returns a combo box for selecting a shared item.
QWidget *MvdSDDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
    Q_UNUSED(option);

    MvdSDTreeWidget *t = tree();
    Q_ASSERT(t);

    QTreeWidgetItem *item = t->currentItem();
    Q_ASSERT(item);

    const int maxInputLength = Movida::core().parameter("mvdcore/max-edit-length").toInt();

    MvdLineEdit *le = 0;
    MvdComboBox *cb = 0;
    MvdCompleter *completer = 0;
    QWidget *editor = 0;


    // id might be MvdNull if we are adding a new item. generateActions() behaves
    // correctly in this case too.
    mvdid id = item->data(0, Movida::IdRole).toUInt();

    // Only column 0 needs a completion-enabled widget
    QStringList completionData;
    if (index.column() == 0) {
        MvdSDTreeWidget::ActionItemList actions = t->generateActions(id);
        MvdSDTreeWidget::ActionItemList::ConstIterator begin = actions.constBegin();
        MvdSDTreeWidget::ActionItemList::ConstIterator end = actions.constEnd();
        while (begin != end) {
            completionData << (*begin).first;
            ++begin;
        }
    }

    if (!completionData.isEmpty()) {
        cb = new MvdComboBox(parent);
        cb->setLineEdit(new MvdResetEdit(cb));
        cb->setEditable(true);
        cb->setFrame(false);
        if (QLineEdit* le = cb->lineEdit()) {
            le->setMaxLength(maxInputLength);
        }
        cb->setInsertPolicy(QComboBox::NoInsert);

        cb->addItems(completionData);
        cb->setCurrentIndex(-1); // Clear line edit

        completer = new MvdCompleter(cb);
        completer->setCompletionMode(MvdCompleter::PopupCompletion);
        completer->setModelSorting(MvdCompleter::CaseInsensitivelySortedModel);
        cb->setAdvancedCompleter(completer);
        editor = cb;

    } else {
        le = new MvdResetEdit(parent);
        le->setFrame(false);
        le->setMaxLength(maxInputLength);
        editor = le;
    }

    if (cb)
        le = qobject_cast<MvdLineEdit*>(cb->lineEdit());

    if (le) {
        QVariant v;
        Movida::ItemValidator val = validatorType(index, *t, &v, MaskUse);
        if (val == Movida::RegExpValidator) {
            QString mask = v.toString();
            if (!mask.isEmpty())
                le->setInputMask(mask);
        }
    }

    mCurrentEditor = editor;
    return editor;
}

//! \internal
void MvdSDDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    MvdSDTreeWidget *t = tree();

    Q_ASSERT(t);

    QTreeWidgetItem *item = t->currentItem();
    Q_ASSERT(item);

    MvdComboBox *combo = qobject_cast<MvdComboBox *>(editor);
    MvdLineEdit *le = combo ? qobject_cast<MvdLineEdit *>(combo->lineEdit()) : qobject_cast<MvdLineEdit *>(editor);
    Q_ASSERT(le);

    if (!t->isPlaceHolder(item)) {
        QString text = item->text(index.column());
        if (MvdResetEdit *re = qobject_cast<MvdResetEdit* >(le))
            re->setDefaultValue(text);
        le->setText(text);
        le->selectAll();
    }
}

//! \internal Stores changes in the model.
void MvdSDDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
    const QModelIndex &index) const
{
    Q_UNUSED(model);

    MvdSDTreeWidget *t = tree();

    Q_ASSERT(t);

    QTreeWidgetItem *item = t->topLevelItem(index.row());
    Q_ASSERT(item);

    MvdMovieCollection *mc = t->movieCollection();
    Q_ASSERT(mc);

    MvdComboBox *combo = qobject_cast<MvdComboBox *>(editor);
    MvdLineEdit *le = combo ? qobject_cast<MvdLineEdit *>(combo->lineEdit()) : qobject_cast<MvdLineEdit *>(editor);
    Q_ASSERT(le);

    QString text = le->text().trimmed();

    // Discard empty required fields
    if (text.isEmpty() && index.column() == 0)
        return;

    if (MvdResetEdit *re = qobject_cast<MvdResetEdit* >(le)) {
        // Nothing to do
        if (re->defaultValue() == re->text())
            return;
    }

    const bool isNew = !combo || combo->findText(text) == -1;
    const bool isPH = t->isPlaceHolder(item);

    // Ensure this is not a duplicate
    if (isNew && index.column() == 0) {
        // \todo add dynamic validation by coloring the editor when a duplicate or empty string is entered
        for (int i = 0; i < t->topLevelItemCount(); ++i) {
            QTreeWidgetItem *it = t->topLevelItem(i);
            if (item == it || t->isPlaceHolder(it))
                continue;

            // Discard duplicate!
            if (it->text(0).toLower() == text.toLower())
                return;
        }
    } else if (!isNew && index.column() == 0) {
        // Store ID
        mvdid id = mc->sharedData().findItemByValue(text);
        Q_ASSERT(id != MvdNull);
        item->setData(0, Movida::IdRole, id);
    }

    QVariant v;
    Movida::ItemValidator validator = validatorType(index, *t, &v);

    bool accept = true;
    if (validator == Movida::RegExpValidator && !text.isEmpty()) {
        QString pattern = v.toString();
        if (!pattern.isEmpty()) {
            QRegExp rx(pattern);
            accept = rx.exactMatch(text);
        }
    } else if (validator == Movida::UndoEmptyValitator) {
        accept = !text.isEmpty();
    }

    if (accept) {
        item->setText(index.column(), text);
        if (isNew) {
            t->updatedModifiedStatus();
            item->setData(0, Movida::IdRole, MvdNull);
        }
    }

    if (isPH) {
        if (accept) {
            // Remove placeholder property
            t->removePlaceHolder(item);
            if (index.column() != 0)
                item->setText(0, tr("Undefined item", "New shared data item with no name"));

            // Move item to the bottom
            QTreeWidget *tree = item->treeWidget();
            tree->takeTopLevelItem(tree->indexOfTopLevelItem(item));
            tree->insertTopLevelItem(tree->topLevelItemCount(), item);

            // Add new placeholder
            t->setCurrentItem(t->appendPlaceHolder());

            // Move focus to the view as it will have lost it
            tree->setFocus(Qt::ActiveWindowFocusReason);
        } else {
            // Reset placeholder
            t->closeEditor(editor, QAbstractItemDelegate::RevertModelCache);
            delete item;
            t->setCurrentItem(t->appendPlaceHolder());

            // Move focus to the view as it will have lost it
            QTreeWidget *tree = item->treeWidget();
            tree->setFocus(Qt::ActiveWindowFocusReason);
        }
    }
}

//! \internal
void MvdSDDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
    Q_UNUSED(index);

    if (editor)
        editor->setGeometry(option.rect);
}

/*!
    Handle some of the editor widget's events.
*/
bool MvdSDDelegate::eventFilter(QObject *object, QEvent *event)
{
    QWidget *editor = qobject_cast<QWidget *>(object);

    if (editor == 0)
        return false;

    switch (event->type()) {
        case QEvent::KeyPress:
        {
            switch (static_cast<QKeyEvent *>(event)->key()) {
                // Intercept ESC key when editing an item and delete unused new items
                case Qt::Key_Escape:
                {
                    MvdSDTreeWidget *t = tree();
                    Q_ASSERT(t);

                    QTreeWidgetItem *item = t->currentItem();
                    if (item) {
                        if (t->isPlaceHolder(item) && !isItemValid(t->dataRole(), *item)) {
                            // Reset place holder
                            t->closeEditor(editor, QAbstractItemDelegate::RevertModelCache);
                            delete item;
                            t->setCurrentItem(t->appendPlaceHolder());

                            // Filter (block!) the event.
                            return true;
                        }
                    }
                }

                default:
                    ;
            }
        }
        break;

        default:
            ;
    }

    return QItemDelegate::eventFilter(object, event);
}

//! \internal Returns true if an item is valid (all required fields are not empty).
bool MvdSDDelegate::isItemValid(Movida::DataRole ds, const QTreeWidgetItem &item) const
{
    Q_UNUSED(ds);

    bool valid = !item.text(0).isEmpty();
    return valid;
}

QSize MvdSDDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize sz = QItemDelegate::sizeHint(option, index);
    sz.rheight() += 4;
    return sz;
}
