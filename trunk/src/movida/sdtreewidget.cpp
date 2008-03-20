/**************************************************************************
** Filename: sdtreewidget.cpp
**
** Copyright (C) 2007 Angius Fabrizio. All rights reserved.
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

#include "guiglobal.h"
#include "sdtreewidget.h"
#include "mvdcore/core.h"
#include "mvdcore/logger.h"
#include "mvdcore/settings.h"
#include "mvdshared/expandinglineedit.h"
#include <QComboBox>
#include <QComboBox>
#include <QCompleter>
#include <QDesktopServices>
#include <QHeaderView>
#include <QHeaderView>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QStringListModel>
#include <QUrl>
#include <QtDebug>

/*!
	\class MvdSDTreeWidget sdtreewidget.h
	\ingroup Movida

	\brief MvdTreeWidget for display and editing of a movie's shared data.
	\todo Add a "add new" special item to the "add existing" combo box.
	\todo Rename "data source" to "role" to be consistent within the whole app.
	\todo Override tab order when a new item is being edited!
*/


namespace MvdSDTW {
	static const int RolesColumn = 1;
}

/************************************************************************
MvdSDTreeWidget
*************************************************************************/

/*!
	Creates a new empty view with Movida::DataRole set to Movida::NoRole.
*/
MvdSDTreeWidget::MvdSDTreeWidget(QWidget* parent)
: MvdTreeWidget(parent), mCollection(0), mDS(Movida::NoRole)
{
	init();
}

/*!
	Creates a new view for the specified movie.
*/
MvdSDTreeWidget::MvdSDTreeWidget(Movida::DataRole ds, const MvdMovie& movie, 
	MvdMovieCollection* c, QWidget* parent)
: MvdTreeWidget(parent), mCollection(0), mDS(Movida::NoRole)
{
	init();
	setMovieCollection(c);
	setMovie(movie);
	setDataSource(ds);
}

//! \internal
void MvdSDTreeWidget::init()
{
	mModified = false;
	header()->setMovable(false);

	connect( this, SIGNAL(contextMenuRequested(QTreeWidgetItem*, int)), this, SLOT(showContextMenu(QTreeWidgetItem*, int)) );

	connect (model(), SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(updatedModifiedStatus()));
	connect (model(), SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(updatedModifiedStatus()));

	setItemDelegate(new MvdSDDelegate(this));
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	appendPlaceHolder();
}

/*!
	Changes the data source for this view and clears the modified status.
	Note: The view is updated even if the new data source is the same as the
	old one.
*/
void MvdSDTreeWidget::setDataSource(Movida::DataRole ds)
{
	mDS = ds;

	clear();
	appendPlaceHolder();
	setModified(false);

	QStringList labels;
	QList<Movida::SharedDataAttribute> attributes = 
		Movida::sharedDataAttributes(mDS, Movida::MainAttributeFilter);

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

	switch (ds)
	{
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
	default: ;
	}
}

/*!
	Convenience method, same as calling setDataSource(Movida::DataRole) with the
	same old data source. Clears the modified status.
*/
void MvdSDTreeWidget::resetToDefaults()
{
	setDataSource(mDS);
}

/*!
	Sets an existing movie as data source. You still need to call either
	setDatasource() or resetToDefaults() to load the data into the view.
*/
void MvdSDTreeWidget::setMovie(const MvdMovie& m)
{
	mMovie = m;
}

/*!
	Stores the current values to a movie object, replacing existing ones.
	Sets the modified status to false (without triggering any signal).
*/
void MvdSDTreeWidget::store(MvdMovie& m)
{
	if (!mCollection)
		return;

	QHash<mvdid,QStringList> roleData;
	QList<mvdid> stringData;

	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (isPlaceHolder(item))
			continue;

		mvdid id = item->data(0, Movida::IdRole).toUInt();
		if (id == MvdNull)
		{
			// Register a new shared item.

			MvdSdItem sdItem;
			sdItem.role = mDS;
			sdItem.value = item->text(0);
			
			id = mCollection->smd().addItem(sdItem);
		}

		// Re-check id as we might have added a new item to the SD
		if (id != MvdNull)
		{
			if (mDS == Movida::ActorRole || mDS == Movida::CrewMemberRole)
				roleData.insert(id, splitString(item->text(MvdSDTW::RolesColumn)));
			else stringData.append(id);
		}
	}

	switch (mDS)
	{
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
	default: ;
	}

	mModified = false;
}

/*!
	Sets the movie collection used to resolve collection-related shared data.
	You still need to call either setDatasource() or reset() to load the data into the view.
*/
void MvdSDTreeWidget::setMovieCollection(MvdMovieCollection* mc)
{
	mCollection = mc;
}

//! Returns the current values in the tree view, eventually excluding \p excluded if it is not 0.
QList<quint32> MvdSDTreeWidget::currentValues(quint32 excluded, bool excludeNewItems) const
{
	QList<quint32> list;
	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (isPlaceHolder(item))
			continue;

		mvdid id = item->data(0, Movida::IdRole).toUInt();
		if ( (id == MvdNull && !excludeNewItems) || (id != MvdNull && id != excluded) )
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
	if (!mCollection || !mMovie.isValid())
	{
		setModified(this->topLevelItemCount() != 0);
		return;
	}

	// We are editing an existing movie: check for changed values
	QList<quint32> current = currentValues(0, false);
	QList<mvdid> original;

	switch (mDS)
	{
	case Movida::ActorRole: original = mMovie.actorIDs(); break;
	case Movida::CrewMemberRole: original = mMovie.crewMemberIDs(); break;
	case Movida::DirectorRole: original = mMovie.directors(); break;
	case Movida::ProducerRole: original = mMovie.producers(); break;
	case Movida::GenreRole: original = mMovie.genres(); break;
	case Movida::TagRole: original = mMovie.tags(); break;
	case Movida::CountryRole: original = mMovie.countries(); break;
	case Movida::LanguageRole: original = mMovie.languages(); break;
	default: ;
	}

	if (current.size() != original.size())
	{
		setModified(true);
		return;
	}

	qSort(current);
	qSort(original);

	// implicit ASSERT(current.size() == original.size()) follows from above statement
	for (int i = 0; i < current.size(); ++i)
	{
		if (current.at(i) != original.at(i))
		{
			setModified(true);
			return;
		}
	}

	// Compare role info: we can't do it in the prev. loop because we have IDs only
	if (mDS == Movida::ActorRole || mDS == Movida::CrewMemberRole)
	{
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* item = topLevelItem(i);
			if (isPlaceHolder(item))
				continue;

			mvdid id = item->data(0, Movida::IdRole).toUInt();
			QStringList roles = splitString(item->text(MvdSDTW::RolesColumn));

			QStringList originalRoles = mDS == Movida::ActorRole ? 
				mMovie.actorRoles(id) : mMovie.crewMemberRoles(id);

			if (roles.size() != originalRoles.size())
			{
				setModified(true);
				return;
			}

			qSort(roles);
			qSort(originalRoles);

			// implicit ASSERT(roles.size() == originalRoles.size()) follows from above statement
			for (int j = 0; j < roles.size(); ++j)
			{
				if (roles.at(j) != originalRoles.at(j))
				{
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
void MvdSDTreeWidget::setPersonRoleData(const QHash<mvdid, QStringList>& d)
{
	if (!mCollection)
		return;

	for (QHash<mvdid, QStringList>::ConstIterator it = d.constBegin();
		it != d.constEnd(); ++it)
	{
		mvdid id = it.key();
		QStringList roles = it.value();

		MvdSdItem pd = mCollection->smd().item(id);

		MvdTreeWidgetItem* item = new MvdTreeWidgetItem(this);
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		item->setData(0, Movida::IdRole, id);
		item->setText(0, pd.value);
		item->setText(MvdSDTW::RolesColumn, joinStringList(roles, "; "));
	}
}

//! \internal
QString MvdSDTreeWidget::joinStringList(const QStringList& list, 
	const QString& sep, const QString& def) const
{
	QString s;

	for (int i = 0; i < list.size(); ++i)
	{
		QString x = list.at(i);

		if (x.isEmpty())
			x = def;

		if (!x.isEmpty())
		{
			if (!s.isEmpty())
				s.append(sep);
			s.append(x);
		}
	}

	return s.isEmpty() ? def : s;
}

QStringList MvdSDTreeWidget::splitString(const QString& s) const
{
	QStringList dirty = s.split(QRegExp("[;,/]"), QString::SkipEmptyParts);
	QStringList clean;

	for (int i = 0; i < dirty.size(); ++i)
	{
		QString item = dirty.at(i).trimmed();
		if (!item.isEmpty())
			clean.append(item);
	}

	return clean;
}

//! \internal Populates the view with simple data descriptions.
void MvdSDTreeWidget::setSimpleData(const QList<mvdid>& d)
{
	for (int i = 0; i < d.size(); ++i)
	{
		mvdid id = d.at(i);
		MvdSdItem sdItem = mCollection->smd().item(id);
		
		MvdTreeWidgetItem* item = new MvdTreeWidgetItem(this);
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		item->setData(0, Movida::IdRole, id);
		item->setText(0, sdItem.value);
	}
}

//! \internal
void MvdSDTreeWidget::showContextMenu(QTreeWidgetItem* item, int col)
{
	Q_UNUSED(col);
	Q_UNUSED(item);

	if (!mCollection)
		return;

	int itemCount;
	int maxMenuItems = MvdCore::parameter("movida/max-menu-items").toInt();

	QList<QTreeWidgetItem*> selected = filteredSelectedItems();
	QMap<QString, ActionDescriptor> actions = generateActions(0, &itemCount, maxMenuItems);

	// Create context menu
	QMenu menu;

	// ************* ADD/MANAGE
	if (itemCount != 0)
	{
		if (itemCount <= maxMenuItems && !actions.isEmpty())
		{
			QMenu* addMenu = createItemMenu(tr("Add existing"), actions, AddItemAction);
			menu.addMenu(addMenu);
		}
		else
		{
			QAction* addAction = menu.addAction(tr("Add existing"));
			addAction->setData(qVariantFromValue(ActionDescriptor(ShowItemSelectorAction)));
		}
	}

	QAction* editorAction = menu.addAction(tr("Manage lists..."));
	editorAction->setData(qVariantFromValue(ActionDescriptor(ShowEditorAction)));

	menu.addSeparator();

	// ************* REMOVE
	if (!selected.isEmpty())
	{
		QAction* removeAction = menu.addAction(tr("Delete selected item(s)", "", selected.size()));
		removeAction->setShortcut(tr("Del", "Delete selected items"));
		removeAction->setData(qVariantFromValue(ActionDescriptor(RemoveItemAction, selected)));
	}

	QAction* res = menu.exec(QCursor::pos());
	if (res == 0)
		return;

	executeAction(res->data().value<ActionDescriptor>());
}

/*!
	Extracts action descriptions from the SD.
	Returns an empty map if there are more than \p max items (if \p max is not negative).
*/
QMap<QString, MvdSDTreeWidget::ActionDescriptor> MvdSDTreeWidget::generateActions(
	quint32 sel, int* _itemCount, int max)
{
	Q_ASSERT(mCollection);

	QMap<QString,ActionDescriptor> actions;
	QList<quint32> current = currentValues();

	int itemCount = 0;
	Movida::DataRole role = mDS;

	switch (mDS)
	{
	case Movida::ActorRole:
	case Movida::DirectorRole:
	case Movida::ProducerRole:
	case Movida::CrewMemberRole:
		role = Movida::PersonRole;
	default: ;
	}

	itemCount = mCollection->smd().countItems(role);
	Q_ASSERT(itemCount >= current.size());

	itemCount = itemCount - current.size();

	if (max < 0 || itemCount <= max)
	{
		MvdSharedData::ItemList items = mCollection->smd().items(role);
		generateActions(items, current, actions, sel);
	}

	if (_itemCount != 0)
		*_itemCount = itemCount;

	return actions;
}

//! \internal Creates a map of labels and action descriptors suitable for a menu.
void MvdSDTreeWidget::generateActions(
	const MvdSharedData::ItemList& d, 
	const QList<quint32>& current,
	QMap<QString,ActionDescriptor>& actions, quint32 selected)
{
	for (MvdSharedData::ItemList::ConstIterator it = 
		d.constBegin(); it != d.constEnd(); ++it)
	{
		quint32 id = (quint32) it.key();

		if ((selected != 0 && id == selected) || !current.contains(id))
		{
			QString value = it.value().value;
			if (!value.isEmpty())
			{
				ActionDescriptor ad;
				ad.itemIds << id;
				ad.current = (id == selected);
				actions.insert(value, ad);
			}
		}
	}
}

//! \internal Creates a menu with items of given type.
QMenu* MvdSDTreeWidget::createItemMenu(const QString& label, 
	const QMap<QString, ActionDescriptor>& actions, ActionType type)
{
	QMenu* menu = new QMenu(label);

	for (QMap<QString,ActionDescriptor>::ConstIterator it = actions.constBegin();
		it != actions.constEnd(); ++it)
	{
		ActionDescriptor ad = it.value();
		if (ad.current)
			continue;

		QString name = it.key();

		ad.type = type;

		QAction* a = new QAction(name, this);
		a->setData(qVariantFromValue(ad));

		menu->addAction(a);
	}

	return menu;
}

//! \internal Handles an action. \p data may contain optional parameters.
void MvdSDTreeWidget::executeAction(ActionDescriptor ad, const QVariant& data)
{
	Q_UNUSED(data);

	switch(ad.type)
	{
	case AddItemAction:
		if (ad.current)
			break;
		Q_ASSERT(!ad.itemIds.isEmpty());
		if (mDS == Movida::ActorRole || mDS == Movida::CrewMemberRole)
		{
			QHash<mvdid,QStringList> l;
			l.insert(ad.itemIds.first(), QStringList());
			setPersonRoleData(l);
		}
		else
		{
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
void MvdSDTreeWidget::keyPressEvent(QKeyEvent* event)
{
	QTreeWidgetItem* item = currentItem();

	bool ok = true;
	mvdid currentId = item ? item->data(0, Movida::IdRole).toUInt(&ok) : MvdNull;
	if (!ok)
		currentId = MvdNull;

	QList<QTreeWidgetItem*> selected = filteredSelectedItems();

	switch (event->key())
	{
	case Qt::Key_Delete:
		if (!selected.isEmpty())
		{
			executeAction(ActionDescriptor(RemoveItemAction, selected));
			return;
		}
	}

	MvdTreeWidget::keyPressEvent(event);
}

MvdTreeWidgetItem* MvdSDTreeWidget::appendPlaceHolder()
{
	if (mDS == Movida::NoRole)
		return 0;

	MvdTreeWidgetItem* item = new MvdTreeWidgetItem(this);
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	
	QFont f = item->font(0);
	item->setData(0, Movida::FontBackupRole, f);
	f.setItalic(true);
	item->setFont(0, f);

	f = item->font(1);
	item->setData(1, Movida::FontBackupRole, f);
	f.setItalic(true);
	item->setFont(1, f);

	switch (mDS)
	{
	case Movida::ActorRole:
		item->setText(0, tr("New actor"));
		item->setData(0, Qt::ToolTipRole, tr("Edit here to add a new actor"));
		item->setData(MvdSDTW::RolesColumn, Qt::ToolTipRole, tr("Separate roles with a semi colon (\";\"), comma (\",\") or slash (\"/\")"));
		break;
	case Movida::DirectorRole:
		item->setText(0, tr("New director"));
		item->setData(0, Qt::ToolTipRole, tr("Edit here to add a new director"));
		item->setData(MvdSDTW::RolesColumn, Qt::ToolTipRole, tr("Separate roles with a semi colon (\";\"), comma (\",\") or slash (\"/\")"));
		break;
	case Movida::ProducerRole:
		item->setText(0, tr("New producer"));
		item->setData(0, Qt::ToolTipRole, tr("Edit here to add a new producer"));
		item->setData(MvdSDTW::RolesColumn, Qt::ToolTipRole, tr("Separate roles with a semi colon (\";\"), comma (\",\") or slash (\"/\")"));
		break;
	case Movida::CrewMemberRole:
		item->setText(0, tr("New crew member"));
		item->setData(0, Qt::ToolTipRole, tr("Edit here to add a new crew member"));
		item->setData(MvdSDTW::RolesColumn, Qt::ToolTipRole, tr("Separate roles with a semi colon (\";\"), comma (\",\") or slash (\"/\")"));
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

void MvdSDTreeWidget::removePlaceHolder(QTreeWidgetItem* item)
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


/************************************************************************
MvdSDDelegate
*************************************************************************/

//! \internal
MvdSDDelegate::MvdSDDelegate(MvdSDTreeWidget* parent)
: QItemDelegate(parent)
{
}

//! \internal Convenience method.
MvdSDTreeWidget* MvdSDDelegate::tree() const
{
	return qobject_cast<MvdSDTreeWidget*>(this->parent());
}

//! \internal Returns the validator associated to a column and/or row (if any).
Movida::ItemValidator MvdSDDelegate::validatorType(const QModelIndex& index, 
	const MvdSDTreeWidget& tree, QVariant* data, ValidatorUse use) const
{
	Q_UNUSED(use);
	Q_UNUSED(data);

	//! \todo use columns as defined in Movida namespace.
	/*
	Movida::DataRole ds = tree.dataSource();
	if ((Movida::PersonRole & ds) && index.column() == 1)
	{
		if (data)
		{
			if (use == ValidationUse)
				data->setValue(MvdCore::parameter("mvdcore/imdb-id-regexp").toString());
			else data->setValue(MvdCore::parameter("mvdcore/imdb-id-mask").toString());
		}
		return Movida::RegExpValidator;
	}
	*/

	if (tree.isPlaceHolder(tree.topLevelItem(index.row())))
		return Movida::UndoEmptyValitator;

	return Movida::NoValidator;
}

//! \internal Returns a combo box for selecting a shared item.
QWidget* MvdSDDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, 
	const QModelIndex& index) const
{
	Q_UNUSED(option);

	MvdSDTreeWidget* t = tree();
	Q_ASSERT(t);

	int maxInputLength = MvdCore::parameter("mvdcore/max-edit-length").toInt();

	MvdExpandingLineEdit* le = new MvdExpandingLineEdit(parent);
	le->setFrame(false);
	le->setMaxLength(maxInputLength);

	QStringList completionData;
	
	QTreeWidgetItem* item = t->currentItem();
	Q_ASSERT(item);
	
	// id might be MvdNull if we are adding a new item. generateActions() behaves 
	// correctly in this case too.
	mvdid id = item->data(0, Movida::IdRole).toUInt();
	
	QMap<QString,MvdSDTreeWidget::ActionDescriptor> actions = 
		t->generateActions(id);

	for (QMap<QString,MvdSDTreeWidget::ActionDescriptor>::ConstIterator it = 
		actions.constBegin(); it != actions.constEnd(); ++it)
	{
		// const MvdSDTreeWidget::ActionDescriptor& ad = it.value();
		// mvdid currentId = ad.itemIds.first();

		completionData << it.key();
	}

	QCompleter* completer = new QCompleter(completionData, le);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	le->setCompleter(completer);

	QVariant v;
	Movida::ItemValidator val = validatorType(index, *t, &v, MaskUse);
	if (val == Movida::RegExpValidator)
	{
		QString mask = v.toString();
		if (!mask.isEmpty())
			le->setInputMask(mask);
	}

	return le;
}

//! \internal
void MvdSDDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	MvdSDTreeWidget* t = tree();
	Q_ASSERT(t);

	QTreeWidgetItem* item = t->currentItem();
	Q_ASSERT(item);

	QLineEdit* le = qobject_cast<QLineEdit*>(editor);
	Q_ASSERT(le);

	if (!t->isPlaceHolder(item))
		le->setText( item->text(index.column()) );
}

//! \internal Stores changes in the model.
void MvdSDDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, 
	const QModelIndex& index) const
{
	MvdSDTreeWidget* t = tree();
	Q_ASSERT(t);

	QTreeWidgetItem* item = t->topLevelItem(index.row());
	Q_ASSERT(item);

	MvdMovieCollection* mc = t->movieCollection();
	Q_ASSERT(mc);

	QLineEdit* le = qobject_cast<QLineEdit*>(editor);
	Q_ASSERT(le);

	QStringListModel* slModel = static_cast<QStringListModel*>(le->completer()->model());
	Q_ASSERT(model);

	QString text = le->text().trimmed();
	
	// Discard empty required fields
	if (text.isEmpty() && index.column() == 0)
		return;

	QStringList completionData = slModel->stringList();
	bool isNew = !completionData.contains(text);
	bool isPH = t->isPlaceHolder(item);

	// Ensure this is not a duplicate
	if (isNew && index.column() == 0)
	{
		// \todo add dynamic validation by coloring the editor when a duplicate or empty string is entered
		for (int i = 0; i < t->topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* it = t->topLevelItem(i);
			if (item == it || t->isPlaceHolder(it))
				continue;

			// Discard duplicate!
			if (it->text(0).toLower() == text.toLower())
				return;
		}
	}
	else if (!isNew && index.column() == 0)
	{
		// Store ID
		mvdid id = mc->smd().findItemByValue(text);
		Q_ASSERT(id != MvdNull);
		item->setData(0, Movida::IdRole, id);
	}

	QVariant v;
	Movida::ItemValidator validator = validatorType(index, *t, &v);

	bool accept = true;
	if (validator == Movida::RegExpValidator && !text.isEmpty())
	{
		QString pattern = v.toString();
		if (!pattern.isEmpty())
		{
			QRegExp rx(pattern);
			accept = rx.exactMatch(text);
		}
	}
	else if (validator == Movida::UndoEmptyValitator)
	{
		accept = !text.isEmpty();
	}

	if (accept) {
		item->setText(index.column(), text);
		if (isNew) {
			t->updatedModifiedStatus();
			item->setData(0, Movida::IdRole, MvdNull);
		}
	}

	if (isPH)
	{
		if (accept)
		{
			// Remove placeholder
			t->removePlaceHolder(item);
			if (index.column() != 0)
				item->setText(0, tr("Undefined item", "New shared data item with no name"));
			t->setCurrentItem(t->appendPlaceHolder());
		}
		else
		{
			// Reset placeholder
			t->closeEditor(editor, QAbstractItemDelegate::RevertModelCache);
			delete item;
			t->setCurrentItem(t->appendPlaceHolder());
		}
	}
}

//! \internal
void MvdSDDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, 
	const QModelIndex& index) const
{
	Q_UNUSED(index);

	if (editor)
		editor->setGeometry(option.rect);
}

/*!
	Handle some of the editor widget's events.
*/
bool MvdSDDelegate::eventFilter(QObject* object, QEvent* event)
{
	QWidget* editor = qobject_cast<QWidget*>(object);
	if (editor == 0)
		return false;

	switch (event->type())
	{
	case QEvent::KeyPress:
	{
		switch (static_cast<QKeyEvent *>(event)->key())
		{
		// Intercept ESC key when editing an item and delete unused new items
		case Qt::Key_Escape:
		{
			MvdSDTreeWidget* t = tree();
			Q_ASSERT(t);

			QTreeWidgetItem* item = t->currentItem();
			if (item)
			{
				if (t->isPlaceHolder(item) && !isItemValid(t->dataSource(), *item))
				{
					// Reset place holder
					t->closeEditor(editor, QAbstractItemDelegate::RevertModelCache);
					delete item;
					t->setCurrentItem(t->appendPlaceHolder());
					
					// Filter (block!) the event.
					return true;
				}
			}
		}
		default: ;
		}
	}
	break;
	default: ;
	}
	
	return QItemDelegate::eventFilter(object, event);
}

//! \internal Returns true if an item is valid (all required fields are not empty).
bool MvdSDDelegate::isItemValid(Movida::DataRole ds, const QTreeWidgetItem& item) const
{
	Q_UNUSED(ds);

	bool valid = !item.text(0).isEmpty();
	return valid;
}
