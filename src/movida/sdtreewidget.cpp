/**************************************************************************
** Filename: sdtreewidget.cpp
** Revision: 3
**
** Copyright (C) 2007 Angius Fabrizio. All rights reserved.
**
** This file is part of the Movida project (http://movida.sourceforge.net/).
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
#include "settings.h"
#include "logger.h"
#include "core.h"
#include "guiglobal.h"
#include "expandinglineedit.h"
#include <QtDebug>
#include <QComboBox>
#include <QList>
#include <QTreeWidgetItemIterator>
#include <QMenu>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QComboBox>
#include <QHeaderView>

/*!
	\class MvdSDTreeWidget sdtreewidget.h
	\ingroup Movida

	\brief MvdTreeWidget for display and editing of a movie's shared data.
	\todo Add a "add new" special item to the "add existing" combo box.
	\todo Rename "data source" to "role" to be consistent within the whole app.
*/


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
: MvdTreeWidget(parent), mMovie(movie), mCollection(c), mDS(ds)
{
	init();
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
}

/*!
	Changes the data source for this view and clears the modified status.
	Note: The view is updated even if the new data source is the same as the
	old one.
*/
void MvdSDTreeWidget::setDataSource(Movida::DataRole ds)
{
	clear();
	setModified(false);

	mDS = ds;

	QStringList labels;

	switch (ds)
	{
	case Movida::ActorRole:
	case Movida::CrewMemberRole:
		labels << tr("Name") << tr("IMDb ID") << tr("Role(s)");
		break;
	case Movida::DirectorRole:
	case Movida::ProducerRole:
		labels << tr("Name") << tr("IMDb ID");
		break;
	case Movida::GenreRole:
		labels << tr("Genre");
		break;
	case Movida::CountryRole:
		labels << tr("Country");
		break;
	case Movida::LanguageRole:
		labels << tr("Language");
		break;
	case Movida::TagRole:
		labels << tr("Tag");
		break;
	default: ;
	}

	setHeaderLabels(labels);

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
		setSimpleData(mMovie.directors(), ds);
		break;
	case Movida::ProducerRole:
		setSimpleData(mMovie.producers(), ds);
		break;
	case Movida::GenreRole:
		setSimpleData(mMovie.genres(), ds);
		break;
	case Movida::TagRole:
		setSimpleData(mMovie.tags(), ds);
		break;
	case Movida::CountryRole:
		setSimpleData(mMovie.countries(), ds);
		break;
	case Movida::LanguageRole:
		setSimpleData(mMovie.languages(), ds);
		break;
	default: ;
	}
}

/*!
	Convenience method, same as calling setDataSource(Movida::DataRole) with the
	same old data source. Clears the modified status.

	\todo add a RESET_DEFAULTS action to the context menu
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
	QHash<mvdid,QStringList> roleData;
	QList<mvdid> stringData;

	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (!item || isPlaceHolder(item))
			continue;

		mvdid id = item->data(0, Movida::IdRole).toUInt();
		if (id == MvdNull && mCollection != 0)
		{
			MvdSdItem sdItem;
			sdItem.role = mDS;

			// Record new item in SMD.
			switch (mDS)
			{
			case Movida::ActorRole:
			case Movida::CrewMemberRole:
			case Movida::DirectorRole:
			case Movida::ProducerRole:
				sdItem.value = item->text(0);
				sdItem.id = item->text(1);
				break;
			case Movida::GenreRole:
			case Movida::CountryRole:
			case Movida::LanguageRole:
			case Movida::TagRole:
				sdItem.value = item->text(0);
				break;
			default: ;
			}
			
			id = mCollection->smd().addItem(sdItem);
		}

		// Re-check id as we might have added a new item to the SD
		if (id != 0)
		{
			if (mDS == Movida::ActorRole || mDS == Movida::CrewMemberRole)
				roleData.insert(id, splitString(item->text(2)));
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
		if (!item || isPlaceHolder(item))
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
	if (mCollection == 0 || !mMovie.isValid())
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
			if (!item || isPlaceHolder(item))
				continue;

			mvdid id = item->data(0, Movida::IdRole).toUInt();
			QStringList roles = splitString(item->text(2));

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

		QTreeWidgetItem* item = new QTreeWidgetItem(this);
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		item->setData(0, Movida::IdRole, id);

		item->setText(0, pd.value);
		item->setText(1, pd.id);
		item->setText(2, joinStringList(roles, "; "));
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
	QStringList dirty = s.split(";", QString::SkipEmptyParts);
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
void MvdSDTreeWidget::setSimpleData(const QList<mvdid>& d, Movida::DataRole ds)
{
	for (int i = 0; i < d.size(); ++i)
	{
		mvdid id = d.at(i);
		MvdSdItem sdItem = mCollection->smd().item(id);
		
		QTreeWidgetItem* item = new QTreeWidgetItem(this);
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		item->setData(0, Movida::IdRole, id);

		switch (ds)
		{
		case Movida::DirectorRole:
		case Movida::ProducerRole:
				item->setText(0, sdItem.value);
				item->setText(1, sdItem.id);
			break;
		case Movida::GenreRole:
		case Movida::TagRole:
		case Movida::CountryRole:
		case Movida::LanguageRole:
				item->setText(0, sdItem.value);
			break;
		default: ;
		}
	}
}

//! \internal
void MvdSDTreeWidget::showContextMenu(QTreeWidgetItem* item, int col)
{
	Q_UNUSED(col);
	Q_UNUSED(item);

	if (mCollection == 0)
		return;

	int itemCount;
	int maxMenuItems = MvdCore::parameter("mvdp://movida/max-menu-items").toInt();

	QList<mvdid> selected = filteredSelectedIds();
	QMap<QString, ActionDescriptor> actions = generateActions(0, &itemCount, maxMenuItems);

	// Create context menu
	QMenu menu;

	// ************* EXECUTE/PROPERTIES


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
	Q_ASSERT(mCollection != 0);

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
			setSimpleData(l, mDS);
		}
		break;
	case RemoveItemAction:
		qDeleteAll(itemsById(ad.itemIds));
		break;
	case ShowEditorAction:
		QMessageBox::warning(this, MVD_CAPTION, "Not implemented");
		break;
	case ShowItemSelectorAction:
		/*
		{
			QTreeWidgetItem* item = new QTreeWidgetItem(this);
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
	if (!item)
		return;

	bool ok;
	mvdid currentId = item->data(0, Movida::IdRole).toUInt(&ok);
	if (!ok)
		currentId = MvdNull;

	QList<mvdid> selectedIds = filteredSelectedIds();

	switch (event->key())
	{
	case Qt::Key_Delete:
		if (!selectedIds.isEmpty())
		{
			ActionDescriptor ad;
			ad.type = RemoveItemAction;
			ad.itemIds = selectedIds;
			executeAction(ad);
			return;
		}
	}

	MvdTreeWidget::keyPressEvent(event);
}


/************************************************************************
MvdSDDelegate
*************************************************************************/

//! \internal
MvdSDDelegate::MvdSDDelegate(MvdSDTreeWidget* parent)
: QItemDelegate(parent)
{
}

//! Returns the type of delegate to be used for the current index.
MvdSDDelegate::DelegateType MvdSDDelegate::delegateType(const QModelIndex& index, 
	MvdSDTreeWidget** _tree) const
{
	MvdSDTreeWidget* tree = qobject_cast<MvdSDTreeWidget*>(this->parent());
	if (tree == 0)
		return UnsupportedDelegate;

	if (_tree != 0)
		*_tree = tree;

	QTreeWidgetItem* currentItem = tree->itemFromIndex(index);
	if (currentItem != 0)
	{
		QVariant v = currentItem->data(0, Movida::NewItemRole);
		bool isNew = v.isNull() ? false : v.toBool();
		
		// New custom item
		if (isNew)
			return TextDelegate;
	}

	Movida::DataRole ds = tree->dataSource();
	if (Movida::PersonRole & ds)
	{
		// ID column
		if (index.column() == 1)
			return NullDelegate;
		// Role column
		if (index.column() > 1)
			return TextDelegate;

		return PersonDelegate;
	}
	return StandardDelegate;
}

//! \internal Returns the validator associated to a column and/or row (if any).
Movida::ItemValidator MvdSDDelegate::validatorType(const QModelIndex& index, 
	const MvdSDTreeWidget& tree, QVariant* data, ValidatorUse use) const
{
	//! \todo use columns as defined in Movida namespace.
	Movida::DataRole ds = tree.dataSource();
	if ((Movida::PersonRole & ds) && index.column() == 1)
	{
		if (data)
		{
			if (use == ValidationUse)
				data->setValue(MvdCore::parameter("mvdp://mvdcore/imdb-id-regexp").toString());
			else data->setValue(MvdCore::parameter("mvdp://mvdcore/imdb-id-mask").toString());
		}
		return Movida::RegExpValidator;
	}

	return Movida::NoValidator;
}

//! \internal Returns a combo box for selecting a shared item.
QWidget* MvdSDDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, 
	const QModelIndex& index) const
{
	MvdSDTreeWidget* tree = 0;
	DelegateType dt = delegateType(index, &tree);
	
	if (dt == UnsupportedDelegate)
		return QItemDelegate::createEditor(parent, option, index);

	if (dt == NullDelegate)
		return 0;
	
	Q_ASSERT(tree != 0);
	// Movida::DataRole ds = tree->dataSource();

	QWidget* editor = 0;

	if (dt == TextDelegate)
	{
		MvdExpandingLineEdit* le = new MvdExpandingLineEdit(parent);
		le->setFrame(false);
		editor = le;

		QVariant v;
		Movida::ItemValidator val = validatorType(index, *tree, &v, MaskUse);
		if (val == Movida::RegExpValidator)
		{
			QString mask = v.toString();
			if (!mask.isEmpty())
				le->setInputMask(mask);
		}
	}
	else
	{
		QComboBox* cb = new QComboBox(parent);
		cb->setFrame(false);
		editor = cb;
	}
	
	return editor;
}

//! \internal Populates the combobox editor with smd data.
void MvdSDDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	MvdSDTreeWidget* tree = 0;
	DelegateType dt = delegateType(index, &tree);

	if (dt == UnsupportedDelegate)
	{
		QItemDelegate::setEditorData(editor, index);
		return;
	}

	if (dt == NullDelegate)
		return;

	Q_ASSERT(tree != 0);

	QTreeWidgetItem* item = tree->currentItem();
	Q_ASSERT(item != 0);

	if (dt == TextDelegate)
	{
		QLineEdit* le = qobject_cast<QLineEdit*>(editor);
		Q_ASSERT(le != 0);

		le->setText( item->text(index.column()) );
	}
	else
	{
		QComboBox* cb = qobject_cast<QComboBox*>(editor);
		Q_ASSERT(cb != 0);

		// ID might be 0 if we are adding a new item. generateAction behaves 
		// correctly in this case too.
		mvdid id = item->data(0, Movida::IdRole).toUInt();
		
		QMap<QString,MvdSDTreeWidget::ActionDescriptor> actions = 
			tree->generateActions(id);
		int currentIndex = 0;

		for (QMap<QString,MvdSDTreeWidget::ActionDescriptor>::ConstIterator it = 
			actions.constBegin(); it != actions.constEnd(); ++it)
		{
			const MvdSDTreeWidget::ActionDescriptor& ad = it.value();
			mvdid currentId = ad.itemIds.first();

			cb->addItem(it.key());
			cb->setItemData(cb->count() - 1, currentId, Movida::IdRole);
			if (currentId == id)
				currentIndex = cb->count() - 1;
		}

		cb->setCurrentIndex(currentIndex);
	}
}

//! \internal Stores changes in the model.
void MvdSDDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, 
	const QModelIndex& index) const
{
	MvdSDTreeWidget* tree = 0;
	DelegateType dt = delegateType(index, &tree);

	if (dt == UnsupportedDelegate)
	{
		QItemDelegate::setModelData(editor, model, index);
		return;
	}

	if (dt == NullDelegate)
		return;

	Q_ASSERT(tree);

	QTreeWidgetItem* item = tree->topLevelItem(index.row());
	Q_ASSERT(item);

	MvdMovieCollection* mc = tree->movieCollection();
	Q_ASSERT(mc);

	if (dt == TextDelegate)
	{
		QLineEdit* le = qobject_cast<QLineEdit*>(editor);
		Q_ASSERT(le);

		QVariant v = item->data(0, Movida::NewItemRole);
		bool isNew = v.isNull() ? false : v.toBool();

		Movida::ItemValidator validator = validatorType(index, *tree, &v);

		QString text = le->text();
				
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

		if (accept)
			item->setText(index.column(), text);

		if (isNew && !isItemValid(tree->dataSource(), *item))
		{
			// Close editor so we can delete the item safely.
			tree->closeEditor(editor, QAbstractItemDelegate::RevertModelCache);
			delete item;
			return;
		}	
	}
	else
	{
		QComboBox* cb = qobject_cast<QComboBox*>(editor);
		Q_ASSERT(cb);
	
		// The ID in the combo should never be 0.
		mvdid id = cb->itemData(cb->currentIndex(), Movida::IdRole).toUInt();
		Q_ASSERT(id != MvdNull);

		item->setData(0, Movida::IdRole, id);

		// Change display role data too. depends on data source.
		Movida::DataRole ds = tree->dataSource();
		MvdSdItem sdItem = mc->smd().item(id);

		item->setText(0, sdItem.value);

		if (Movida::PersonRole & ds)
			item->setText(1, sdItem.id);
	}
}

//! \internal
void MvdSDDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, 
	const QModelIndex& index) const
{
	Q_UNUSED(index);

	MvdSDTreeWidget* tree = 0;
	DelegateType dt = delegateType(index, &tree);

	if (dt == UnsupportedDelegate)
	{
		QItemDelegate::updateEditorGeometry(editor, option, index);
		return;
	}

	if (dt == NullDelegate)
		return;

	Q_ASSERT(tree != 0);

	QTreeWidgetItem* item = tree->currentItem();
	Q_ASSERT(item != 0);

	QRect r = option.rect;
	/*if (r.y() > 0)
	{
		r.setY(r.y() - 2);
		r.setHeight(r.height() + 4);
	}
	else
	{
		r.setHeight(r.height() + 6);
	}*/

	editor->setGeometry(r);
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
			MvdSDTreeWidget* tree = qobject_cast<MvdSDTreeWidget*>(this->parent());
			if (tree != 0)
			{
				QTreeWidgetItem* item = tree->currentItem();
				if (item != 0)
				{
					QVariant v = item->data(0, Movida::NewItemRole);
					bool isNew = v.isNull() ? false : v.toBool();
					quint32 id = item->data(0, Movida::IdRole).toUInt();

					//! \todo Delete empty new items (possibly check first and last name)
					if (id == 0)
					{
						if (! (isNew && isItemValid(tree->dataSource(), *item)) )
						{
							// Don't commit data but ensure the item won't be used 
							// any more so we can delete it.
							tree->closeEditor(editor, QAbstractItemDelegate::RevertModelCache);

							delete item;

							// Filter (block!) the event.
							return true;
						}
					}
				}
			}
		}
		default: ;
		}
	}
	break;
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
