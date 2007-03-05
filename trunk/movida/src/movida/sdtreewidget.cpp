/**************************************************************************
** Filename: sdtreewidget.cpp
** Revision: 1
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

#include <QtDebug> //! \todo debug only include
#include <QComboBox>
#include <QList>
#include <QTreeWidgetItemIterator>
#include <QMenu>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QComboBox>
#include <QLineEdit>
#include <QHeaderView>

#define MVD_MST_ITEM_ID Qt::UserRole + 1
#define MVD_MST_ITEM_NEW Qt::UserRole + 2

#define MVD_MST_MAX_MENU_ITEMS -1

/*!
	\class MvdSDTreeWidget sdtreewidget.h
	\ingroup gui

	\brief MvdTreeWidget for display and editing of a movie's shared data.
*/


/************************************************************************
MvdSDTreeWidget
*************************************************************************/

/*!
	Creates a new empty view with Movida::SmdDataRole set to Movida::ActorRole.
*/
MvdSDTreeWidget::MvdSDTreeWidget(QWidget* parent)
: MvdTreeWidget(parent), mCollection(0), mDS(Movida::ActorRole)
{
	init();
}

/*!
	Creates a new view for the specified movie.
*/
MvdSDTreeWidget::MvdSDTreeWidget(Movida::SmdDataRole ds, const MvdMovie& movie, 
	MvdMovieCollection* c, QWidget* parent)
: MvdTreeWidget(parent), mMovie(movie), mCollection(c), mDS(ds)
{
	init();
}

/*!
	Changes the data source for this view and clears the modified status.
	Note: The view is updated even if the new data source is the same as the
	old one.
*/
void MvdSDTreeWidget::setDataSource(Movida::SmdDataRole ds)
{
	clear();
	mMaybeModified = false;

	mDS = ds;

	QStringList labels;

	switch (ds)
	{
	case Movida::ActorRole:
	case Movida::CrewMemberRole:
		labels << tr("First name") << tr("Last name") << tr("Role(s)");
		break;
	case Movida::DirectorRole:
	case Movida::ProducerRole:
		labels << tr("First name") << tr("Last name");
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
	case Movida::UrlRole:
		labels << tr("Description") << tr("URL");
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
	case Movida::UrlRole:
		setSimpleData(mMovie.urls(), ds);
		break;
	default: ;
	}
}

/*!
	Convenience method, same as calling setDataSource(Movida::SmdDataRole) with the
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
*/
void MvdSDTreeWidget::store(MvdMovie& m)
{
	QHash<smdid,QStringList> roleData;
	QList<smdid> stringData;

	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (item == 0)
			continue;

		quint32 id = item->data(0, MVD_MST_ITEM_ID).toUInt();
		if (id == 0 && mCollection != 0)
		{
			// Record new item in SMD.
			switch (mDS)
			{
			case Movida::ActorRole:
			case Movida::CrewMemberRole:
			case Movida::DirectorRole:
			case Movida::ProducerRole:
				id = mCollection->smd().addPerson(item->text(0), item->text(1));				
				break;
			case Movida::GenreRole:
				id = mCollection->smd().addGenre(item->text(0));
				break;
			case Movida::CountryRole:
				id = mCollection->smd().addCountry(item->text(0));
				break;
			case Movida::LanguageRole:
				id = mCollection->smd().addLanguage(item->text(0));
				break;
			case Movida::TagRole:
				id = mCollection->smd().addTag(item->text(0));
				break;
			case Movida::UrlRole:
				id = mCollection->smd().addUrl(item->text(1), item->text(0));
				break;			
			default: ;
			}	
		}

		// Re-check id as we might have added a new item to the smd
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
	case Movida::UrlRole:
		m.setUrls(stringData);
		break;
	default: ;
	}
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
		if (item == 0)
			continue;

		quint32 id = item->data(0, MVD_MST_ITEM_ID).toUInt();
		if ( (id == 0 && !excludeNewItems) || (id != 0 && id != excluded) )
			list << id;
	}

	return list;
}

/*!
	Returns true if any data has been modified.
*/
bool MvdSDTreeWidget::isModified() const
{
	if (!mMaybeModified)
		return false;

	// We are editing a new movie
	if (mCollection == 0 || !mMovie.isValid())
		return this->topLevelItemCount() != 0;

	// We are editing an existing movie: check for changed values
	QList<quint32> current = currentValues(0, false);

	QList<smdid> original;

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
	case Movida::UrlRole: original = mMovie.urls(); break;
	default: ;
	}

	if (current.size() != original.size())
		return true;

	qSort(current);
	qSort(original);

	// implicit ASSERT(current.size() == original.size()) follows from above statement
	for (int i = 0; i < current.size(); ++i)
	{
		if (current.at(i) != original.at(i))
			return true;
	}

	// Compare role info: we can't do it in the prev. loop because we have IDs only
	if (mDS == Movida::ActorRole || mDS == Movida::CrewMemberRole)
	{
		for (int i = 0; i < topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* item = topLevelItem(i);
			if (item == 0)
				continue;

			quint32 id = item->data(0, MVD_MST_ITEM_ID).toUInt();
			QStringList roles = splitString(item->text(2));

			QStringList originalRoles = mDS == Movida::ActorRole ? 
				mMovie.actorRoles(id) : mMovie.crewMemberRoles(id);

			if (roles.size() != originalRoles.size())
				return true;

			qSort(roles);
			qSort(originalRoles);

			// implicit ASSERT(roles.size() == originalRoles.size()) follows from above statement
			for (int j = 0; j < roles.size(); ++j)
			{
				if (roles.at(j) != originalRoles.at(j))
					return true;
			}
		}
	}

	return false;
}

//! \internal
void MvdSDTreeWidget::init()
{
	mMaybeModified = false;
	header()->setMovable(false);

	connect( this, SIGNAL(contextMenuRequested(QTreeWidgetItem*, int)), this, SLOT(showContextMenu(QTreeWidgetItem*, int)) );
	connect( this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem*)) );

	setItemDelegate(new MvdSDDelegate(this));
}

/*!
	\internal Populates the view with person+role descriptions.
*/
void MvdSDTreeWidget::setPersonRoleData(const QHash<smdid, QStringList>& d)
{
	for (QHash<smdid, QStringList>::ConstIterator it = d.constBegin();
		it != d.constEnd(); ++it)
	{
		smdid id = it.key();
		QStringList roles = it.value();

		const MvdSharedData::PersonData* pd = smd(id).person(id);
		if (pd == 0)
			continue;

		QTreeWidgetItem* item = new QTreeWidgetItem(this);
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		item->setData(0, MVD_MST_ITEM_ID, id);

		item->setText(0, pd->firstName);
		item->setText(1, pd->lastName);
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
void MvdSDTreeWidget::setSimpleData(const QList<smdid>& d, Movida::SmdDataRole ds)
{
	for (int i = 0; i < d.size(); ++i)
	{
		smdid id = d.at(i);
		switch (ds)
		{
		case Movida::DirectorRole:
		case Movida::ProducerRole:
			{
				const MvdSharedData::PersonData* pd = smd(id).person(id);
				if (pd == 0)
					continue;
				QTreeWidgetItem* item = new QTreeWidgetItem(this);
				item->setFlags(item->flags() | Qt::ItemIsEditable);
				item->setData(0, MVD_MST_ITEM_ID, id);

				item->setText(0, pd->firstName);
				item->setText(1, pd->lastName);
			}
			break;
		case Movida::GenreRole:
			{
				const MvdSharedData::StringData* sd = smd(id).genre(id);
				if (sd == 0)
					continue;
				QTreeWidgetItem* item = new QTreeWidgetItem(this);
				item->setFlags(item->flags() | Qt::ItemIsEditable);
				item->setData(0, MVD_MST_ITEM_ID, id);
				item->setText(0, sd->name);
			}
			break;
		case Movida::TagRole:
			{
				const MvdSharedData::StringData* sd = smd(id).tag(id);
				if (sd == 0)
					continue;
				QTreeWidgetItem* item = new QTreeWidgetItem(this);
				item->setFlags(item->flags() | Qt::ItemIsEditable);
				item->setData(0, MVD_MST_ITEM_ID, id);
				item->setText(0, sd->name);
			}
			break;
		case Movida::CountryRole:
			{
				const MvdSharedData::StringData* sd = smd(id).country(id);
				if (sd == 0)
					continue;
				QTreeWidgetItem* item = new QTreeWidgetItem(this);
				item->setFlags(item->flags() | Qt::ItemIsEditable);
				item->setData(0, MVD_MST_ITEM_ID, id);
				item->setText(0, sd->name);
			}
			break;
		case Movida::LanguageRole:
			{
				const MvdSharedData::StringData* sd = smd(id).language(id);
				if (sd == 0)
					continue;
				QTreeWidgetItem* item = new QTreeWidgetItem(this);
				item->setFlags(item->flags() | Qt::ItemIsEditable);
				item->setData(0, MVD_MST_ITEM_ID, id);
				item->setText(0, sd->name);
			}
			break;
		case Movida::UrlRole:
			{
				const MvdSharedData::UrlData* ud = smd(id).url(id);
				if (ud == 0)
					continue;
				QTreeWidgetItem* item = new QTreeWidgetItem(this);
				item->setFlags(item->flags() | Qt::ItemIsEditable);
				item->setData(0, MVD_MST_ITEM_ID, id);
				item->setText(0, ud->description);
				item->setText(1, ud->url);
			}
			break;
		default: ;
		}
	}
}

//! \internal
void MvdSDTreeWidget::showContextMenu(QTreeWidgetItem* item, int col)
{
	Q_UNUSED(col);

	if (mCollection == 0)
		return;

	quint32 currentId = item == 0 ? 0 : item->data(0, MVD_MST_ITEM_ID).toUInt();

	int itemCount;
	QMap<QString,ActionDescriptor> actions = generateActions(0, &itemCount, 
		MVD_MST_MAX_MENU_ITEMS);
	

	// Create context menu
	QMenu menu;

	// ************* EXECUTE/PROPERTIES
	if (item != 0)
	{
		if (mDS == Movida::UrlRole)
		{
			QAction* execAction = menu.addAction(tr("Open in browser"));
			execAction->setData(qVariantFromValue(ActionDescriptor(ExecuteItemAction, currentId)));
			menu.addSeparator();
		}

		if (mDS == Movida::ActorRole || mDS == Movida::CrewMemberRole)
		{
			QAction* editorAction = menu.addAction(tr("Edit roles..."));
			editorAction->setData(qVariantFromValue(ActionDescriptor(ShowPropertyEditorAction, currentId)));
			menu.addSeparator();
		}
	}

	// ************* ADD/MANAGE
	if (itemCount != 0)
	{
		if (itemCount <= MVD_MST_MAX_MENU_ITEMS && !actions.isEmpty())
		{
			QMenu* addMenu = createItemMenu(tr("Add existing"), actions, AddItemAction);
			menu.addMenu(addMenu);
		}
		else
		{
			QAction* addAction = menu.addAction(tr("Add existing"));
			addAction->setData(qVariantFromValue(ActionDescriptor(ShowItemSelectorAction, currentId)));
		}
	}

	QAction* addAction = menu.addAction(tr("Add new"));
	addAction->setData(qVariantFromValue(ActionDescriptor(AddNewItemAction, currentId)));

	QAction* editorAction = menu.addAction(tr("Manage lists..."));
	editorAction->setData(qVariantFromValue(ActionDescriptor(ShowEditorAction, currentId)));

	menu.addSeparator();

	// ************* REMOVE
	if (item != 0)
	{
		QAction* removeAction = menu.addAction(tr("Remove"));
		removeAction->setShortcut(Qt::Key_Delete);
		removeAction->setData(qVariantFromValue(ActionDescriptor(RemoveItemAction, currentId)));
	}

	if (topLevelItemCount() != 0)
	{
		QAction* clearAction = menu.addAction(tr("Remove all"));
		clearAction->setData(qVariantFromValue(ActionDescriptor(ClearAction, currentId)));
	}

	QAction* res = menu.exec(QCursor::pos());
	if (res == 0)
		return;

	executeAction(res->data().value<ActionDescriptor>(), item);
}

/*!
	Extracts action descriptions from both the global and the collection SMD.
	Returns an empty map if there are more than \p max items (if \p max is not negative).
*/
QMap<QString,MvdSDTreeWidget::ActionDescriptor> MvdSDTreeWidget::generateActions(
	quint32 sel, int* _itemCount, int max)
{
	Q_ASSERT(mCollection != 0);

	QMap<QString,ActionDescriptor> actions;

	QList<quint32> current = currentValues();

	int itemCount = 0;
	
	switch (mDS)
	{
	case Movida::ActorRole:
	case Movida::DirectorRole:
	case Movida::ProducerRole:
	case Movida::CrewMemberRole:
		{
			itemCount = Movida::globalSD().countPersons();
			itemCount += mCollection->smd().countPersons();
			Q_ASSERT(itemCount >= current.size());

			itemCount = itemCount - current.size();

			if (max < 0 || itemCount <= max)
			{
				const QHash<smdid, MvdSharedData::PersonData>* smdData;

				smdData = Movida::globalSD().persons();
				if (smdData != 0)
					generateActions(*smdData, current, actions, sel);

				smdData = mCollection->smd().persons();
				if (smdData != 0)
					generateActions(*smdData, current, actions, sel);
			}
		}
		break;
	case Movida::GenreRole:
		{
			itemCount = Movida::globalSD().countGenres();
			itemCount += mCollection->smd().countGenres();
			Q_ASSERT(itemCount >= current.size());

			itemCount = itemCount - current.size();

			if (max < 0 || itemCount <= max)
			{
				const QHash<smdid, MvdSharedData::StringData>* smdData;

				smdData = Movida::globalSD().genres();
				if (smdData != 0)
					generateActions(*smdData, current, actions, sel);

				smdData = mCollection->smd().genres();
				if (smdData != 0)
					generateActions(*smdData, current, actions, sel);
			}
		}
		break;
	case Movida::TagRole:
		{
			itemCount = Movida::globalSD().countTags();
			itemCount += mCollection->smd().countTags();
			Q_ASSERT(itemCount >= current.size());

			itemCount = itemCount - current.size();

			if (max < 0 || itemCount <= max)
			{
				const QHash<smdid, MvdSharedData::StringData>* smdData;

				smdData = Movida::globalSD().tags();
				if (smdData != 0)
					generateActions(*smdData, current, actions, sel);

				smdData = mCollection->smd().tags();
				if (smdData != 0)
					generateActions(*smdData, current, actions, sel);
			}
		}
		break;
	case Movida::CountryRole:
		{
			itemCount = Movida::globalSD().countCountries();
			itemCount += mCollection->smd().countCountries();
			Q_ASSERT(itemCount >= current.size());

			itemCount = itemCount - current.size();

			if (max < 0 || itemCount <= max)
			{
				const QHash<smdid, MvdSharedData::StringData>* smdData;

				smdData = Movida::globalSD().countries();
				if (smdData != 0)
					generateActions(*smdData, current, actions, sel);

				smdData = mCollection->smd().countries();
				if (smdData != 0)
					generateActions(*smdData, current, actions, sel);
			}
		}
		break;
	case Movida::LanguageRole:
		{
			itemCount = Movida::globalSD().countLanguages();
			itemCount += mCollection->smd().countLanguages();
			Q_ASSERT(itemCount >= current.size());

			itemCount = itemCount - current.size();

			if (max < 0 || itemCount <= max)
			{
				const QHash<smdid, MvdSharedData::StringData>* smdData;

				smdData = Movida::globalSD().languages();
				if (smdData != 0)
					generateActions(*smdData, current, actions, sel);

				smdData = mCollection->smd().languages();
				if (smdData != 0)
					generateActions(*smdData, current, actions, sel);
			}
		}
		break;
	case Movida::UrlRole:
		{
			itemCount = Movida::globalSD().countUrls();
			itemCount += mCollection->smd().countUrls();
			Q_ASSERT(itemCount >= current.size());

			itemCount = itemCount - current.size();

			if (max < 0 || itemCount <= max)
			{
				const QHash<smdid, MvdSharedData::UrlData>* smdData;

				smdData = Movida::globalSD().urls();
				if (smdData != 0)
					generateActions(*smdData, current, actions, sel);

				smdData = mCollection->smd().urls();
				if (smdData != 0)
					generateActions(*smdData, current, actions, sel);
			}
		}
		break;
	default: ;
	}

	if (_itemCount != 0)
		*_itemCount = itemCount;

	return actions;
}

//! Returns a string in "LastName, FirstName" form.
QString MvdSDTreeWidget::generateDisplayName(const QString& fn, const QString& ln) const
{
	QString displayName = ln;
	if (!ln.isEmpty() && !fn.isEmpty())
		displayName.append(", ");
	displayName.append(fn);
	return displayName;
}

//! \internal Creates a map of labels and action descriptors suitable for a menu.
void MvdSDTreeWidget::generateActions(
   const QHash<smdid, MvdSharedData::StringData>& d, 
   const QList<quint32>& current,
   QMap<QString,ActionDescriptor>& actions, quint32 selected)
{
	for (QHash<smdid, MvdSharedData::StringData>::ConstIterator it = 
		d.constBegin(); it != d.constEnd(); ++it)
	{
		quint32 id = (quint32) it.key();

		if ((selected != 0 && id == selected) || !current.contains(id))
		{
			QString value = it.value().name;
			if (!value.isEmpty())
			{
				ActionDescriptor ad;
				ad.itemId = id;
				ad.current = (id == selected);
				actions.insert(value, ad);
			}
		}
	}
}

//! \internal Creates a map of labels and action descriptors suitable for a menu.
void MvdSDTreeWidget::generateActions(
	const QHash<smdid, MvdSharedData::PersonData>& d, 
	const QList<quint32>& current,
	QMap<QString,ActionDescriptor>& actions, quint32 selected)
{
	for (QHash<smdid, MvdSharedData::PersonData>::ConstIterator it = 
		d.constBegin(); it != d.constEnd(); ++it)
	{
		quint32 id = (quint32) it.key();

		if ((selected != 0 && id == selected) || !current.contains(id))
		{
			QString displayName = generateDisplayName(it.value().firstName, it.value().lastName);

			if (! displayName.isEmpty() )
			{
				ActionDescriptor ad;
				ad.itemId = id;
				ad.current = (id == selected);
				actions.insert(displayName, ad);				
			}
		}
	}
}

//! \internal Creates a map of labels and action descriptors suitable for a menu.
void MvdSDTreeWidget::generateActions(
   const QHash<smdid, MvdSharedData::UrlData>& d, 
   const QList<quint32>& current,
   QMap<QString,ActionDescriptor>& actions, quint32 selected)
{
	for (QHash<smdid, MvdSharedData::UrlData>::ConstIterator it = 
		d.constBegin(); it != d.constEnd(); ++it)
	{
		quint32 id = (quint32) it.key();

		if ((selected != 0 && id == selected) || !current.contains(id))
		{
			QString value = it.value().description.isEmpty() ?
				it.value().url : it.value().description;

			if (!value.isEmpty())
			{
				ActionDescriptor ad;
				ad.itemId = id;
				ad.current = (id == selected);
				actions.insert(value, ad);
			}
		}
	}
}

//! \internal
void MvdSDTreeWidget::itemDoubleClicked(QTreeWidgetItem* item)
{
	if (item == 0)
		return;

	// quint32 id = item->data(0, MVD_MST_ITEM_ID).toUInt();
	// executeAction(ActionDescriptor(ExecuteItemAction, id), item);
}

//! \internal Creates a menu (and evtl. submenus) with items of given type.
QMenu* MvdSDTreeWidget::createItemMenu(const QString& label, 
	const QMap<QString,ActionDescriptor>& actions, ActionType type)
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
void MvdSDTreeWidget::executeAction(ActionDescriptor ad, QTreeWidgetItem* item, 
	const QVariant& data)
{
	Q_UNUSED(data);

	switch(ad.type)
	{
	case RemoveItemAction:
		delete item;
		mMaybeModified = true;
		break;
	case AddItemAction:
		if (ad.current)
			break;
		if (mDS == Movida::ActorRole || mDS == Movida::CrewMemberRole)
		{
			QHash<smdid,QStringList> l;
			l.insert(ad.itemId, QStringList());
			setPersonRoleData(l);			
		}
		else
		{
			QList<smdid> l;
			l << ad.itemId;
			setSimpleData(l, mDS);
		}
		mMaybeModified = true;
		break;
	case AddNewItemAction:
		{
			QTreeWidgetItem* item = new QTreeWidgetItem(this);
			item->setFlags(item->flags() | Qt::ItemIsEditable);
			item->setData(0, MVD_MST_ITEM_ID, 0);
			item->setData(0, MVD_MST_ITEM_NEW, true);
			setCurrentItem(item);
			editItem(item, mDS == Movida::UrlRole ? 1 : 0);
		}
		mMaybeModified = true;
		break;
	case ChangeItemAction:
		if (ad.current)
			break;
		if (mDS == Movida::ActorRole || mDS == Movida::CrewMemberRole)
		{
			QStringList roles = splitString(item->text(2));
			delete item;
			QHash<smdid,QStringList> l;
			l.insert(ad.itemId, roles);
			setPersonRoleData(l);
		}
		else
		{
			delete item;
			QList<smdid> l;
			l << ad.itemId;
			setSimpleData(l, mDS);
		}
		mMaybeModified = true;
		break;
	case ShowEditorAction:
		QMessageBox::warning(this, "", "Not implemented");
		//mMaybeModified = true;
		break;
	case ShowPropertyEditorAction:
		return;
		//mMaybeModified = true;
		break;
	case ExecuteItemAction:
		if (mDS == Movida::UrlRole)
			QDesktopServices::openUrl(item->text(1));
		//mMaybeModified = true;
		break;
	case ClearAction:
		{
			bool clearData = true;
			if (topLevelItemCount() > 2)
			{
				int res = QMessageBox::question(this, _CAPTION_, tr("Are you sure?"), 
					QMessageBox::Yes, QMessageBox::No);
				clearData = res == QMessageBox::Yes;
			}
			if (clearData)
			{
				clear();
				mMaybeModified = true;
			}
		}
		break;
	case ShowItemSelectorAction:
		{
			QTreeWidgetItem* item = new QTreeWidgetItem(this);
			item->setFlags(item->flags() | Qt::ItemIsEditable);
			item->setData(0, MVD_MST_ITEM_ID, 0);
			setCurrentItem(item);
			editItem(item);
		}
		mMaybeModified = true;
		break;
	default:
	    break;
	}
}

/*!
	Handle keyboard shortcuts.
*/
void MvdSDTreeWidget::keyPressEvent(QKeyEvent* event)
{
	QTreeWidgetItem* item = currentItem();
	if (item != 0)
	{
		switch (event->key())
		{
		case Qt::Key_Delete:
			delete item;
			return;
		}
	}

	QTreeWidget::keyPressEvent(event);
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
	MvdSDTreeWidget* tree = dynamic_cast<MvdSDTreeWidget*>(this->parent());
	if (tree == 0)
		return UnsupportedDelegate;

	if (_tree != 0)
		*_tree = tree;

	QTreeWidgetItem* currentItem = tree->itemFromIndex(index);
	if (currentItem != 0)
	{
		QVariant v = currentItem->data(0, MVD_MST_ITEM_NEW);
		bool isNew = v.isNull() ? false : v.toBool();
		
		// New custom item
		if (isNew)
			return TextDelegate;
	}

	Movida::SmdDataRole ds = tree->dataSource();
	if (ds == Movida::ActorRole || 
		ds == Movida::CrewMemberRole || 
		ds == Movida::DirectorRole || 
		ds == Movida::ProducerRole)
	{
		// Role column
		if (index.column() > 1)
			return TextDelegate;
		else return PersonDelegate;
	}

	if (ds == Movida::UrlRole && index.column() < 2)
		return PersonDelegate;

	return StandardDelegate;
}

//! \internal Returns a combo box for selecting a smd item.
QWidget* MvdSDDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, 
	const QModelIndex& index) const
{
	MvdSDTreeWidget* tree = 0;
	DelegateType dt = delegateType(index, &tree);
	
	if (dt == UnsupportedDelegate)
		return QItemDelegate::createEditor(parent, option, index);
	
	Q_ASSERT(tree != 0);
	Movida::SmdDataRole ds = tree->dataSource();

	QWidget* editor = 0;

	if (dt == TextDelegate)
	{
		editor = new QLineEdit(parent);

		QTreeWidgetItem* currentItem = tree->currentItem();
		if (currentItem != 0)
		{
			QVariant v = currentItem->data(0, MVD_MST_ITEM_NEW);
			bool isNew = v.isNull() ? false : v.toBool();
			if (isNew)
			{
				// Do not install validator on role text edits
				if (! ((ds == Movida::ActorRole || ds == Movida::CrewMemberRole) && 
					index.column() > 1) )
				{
					QLineEdit* le = static_cast<QLineEdit*>(editor);
					Q_ASSERT(le != 0);
					// le->setValidator(new SmdValidator(ds));
				}
			}
		}
	}
	else
		editor = new QComboBox(parent);
	
	editor->installEventFilter(const_cast<MvdSDDelegate*>(this));
	
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
	Q_ASSERT(tree != 0);

	QTreeWidgetItem* item = tree->currentItem();
	Q_ASSERT(item != 0);

	if (dt == TextDelegate)
	{
		QLineEdit* le = dynamic_cast<QLineEdit*>(editor);
		Q_ASSERT(le != 0);

		le->setText( item->text(index.column()) );
	}
	else
	{
		QComboBox* cb = dynamic_cast<QComboBox*>(editor);
		Q_ASSERT(cb != 0);

		// ID might be 0 if we are adding a new item. generateAction behaves 
		// correctly in this case too.
		smdid id = item->data(0, MVD_MST_ITEM_ID).toUInt();
		
		QMap<QString,MvdSDTreeWidget::ActionDescriptor> actions = 
			tree->generateActions(id);
		int currentIndex = 0;

		for (QMap<QString,MvdSDTreeWidget::ActionDescriptor>::ConstIterator it = 
			actions.constBegin(); it != actions.constEnd(); ++it)
		{
			cb->addItem(it.key());
			cb->setItemData(cb->count() - 1, it.value().itemId, MVD_MST_ITEM_ID);
			if (it.value().itemId == id)
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
	Q_ASSERT(tree != 0);

	QTreeWidgetItem* item = tree->currentItem();
	Q_ASSERT(item != 0);

	MvdMovieCollection* mc = tree->movieCollection();
	Q_ASSERT(mc != 0);

	if (dt == TextDelegate)
	{
		QLineEdit* le = dynamic_cast<QLineEdit*>(editor);
		Q_ASSERT(le != 0);

		QVariant v = item->data(0, MVD_MST_ITEM_NEW);
		bool isNew = v.isNull() ? false : v.toBool();

		//! \todo validate if new shared item
		QString oldText = item->text(index.column());
		item->setText(index.column(), le->text());

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
		QComboBox* cb = dynamic_cast<QComboBox*>(editor);
		Q_ASSERT(cb != 0);
	
		// The ID in the combo should never be 0.
		smdid id = cb->itemData(cb->currentIndex(), MVD_MST_ITEM_ID).toUInt();
		Q_ASSERT(id != 0);

		item->setData(0, MVD_MST_ITEM_ID, id);

		// Change display role data too. depends on data source.
		Movida::SmdDataRole ds = tree->dataSource();
		if (ds == Movida::ActorRole || ds == Movida::DirectorRole || 
			ds == Movida::ProducerRole || ds == Movida::CrewMemberRole)
		{
			// Person data: change both first and last name
			const MvdSharedData::PersonData* pd = MvdSharedData::isHardcoded(id) ?
				Movida::globalSD().person(id) : mc->smd().person(id);
			Q_ASSERT(pd != 0);

			item->setText(0, pd->firstName);
			item->setText(1, pd->lastName);
		}
		else
		{
			switch(ds)
			{
			case Movida::GenreRole:
				{
					const MvdSharedData::StringData* pd = MvdSharedData::isHardcoded(id) ?
						Movida::globalSD().genre(id) : mc->smd().genre(id);
					Q_ASSERT(pd != 0);
					item->setText(0, pd->name);
				}
				break;
			case Movida::CountryRole:
				{
					const MvdSharedData::StringData* pd = MvdSharedData::isHardcoded(id) ?
						Movida::globalSD().country(id) : mc->smd().country(id);
					Q_ASSERT(pd != 0);
					item->setText(0, pd->name);
				}
				break;
			case Movida::LanguageRole:
				{
					const MvdSharedData::StringData* pd = MvdSharedData::isHardcoded(id) ?
						Movida::globalSD().language(id) : mc->smd().language(id);
					Q_ASSERT(pd != 0);
					item->setText(0, pd->name);
				}
				break;
			case Movida::TagRole:
				{
					const MvdSharedData::StringData* pd = MvdSharedData::isHardcoded(id) ?
						Movida::globalSD().tag(id) : mc->smd().tag(id);
					Q_ASSERT(pd != 0);
					item->setText(0, pd->name);
				}
				break;
			case Movida::UrlRole:
				{
					const MvdSharedData::UrlData* pd = MvdSharedData::isHardcoded(id) ?
						Movida::globalSD().url(id) : mc->smd().url(id);
					Q_ASSERT(pd != 0);
					item->setText(0, pd->description);
					item->setText(1, pd->url);
				}
				break;
			default: ;
			}
		}
	}
}

//! \internal
void MvdSDDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, 
	const QModelIndex& index) const
{
	MvdSDTreeWidget* tree = 0;
	DelegateType dt = delegateType(index, &tree);
	if (dt == UnsupportedDelegate)
	{
		QItemDelegate::updateEditorGeometry(editor, option, index);
		return;
	}
	Q_ASSERT(tree != 0);

	QTreeWidgetItem* item = tree->currentItem();
	Q_ASSERT(item != 0);

	QRect r = option.rect;
	// r.setY(r.y() - 2);
	// r.setHeight(r.height() + 4);

	if (dt == PersonDelegate)
	{
		// Join first name and last name columns
		int sz = tree->header()->sectionSize( 0 )+ tree->header()->sectionSize( 1 );

		r.setX(0);
		r.setWidth(sz);
	}

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

	if (event->type() == QEvent::KeyPress)
	{
		switch (static_cast<QKeyEvent *>(event)->key())
		{
		// Intercept ESC key when editing an item and delete unused new items
		case Qt::Key_Escape:
		{
			MvdSDTreeWidget* tree = dynamic_cast<MvdSDTreeWidget*>(this->parent());
			if (tree != 0)
			{
				QTreeWidgetItem* item = tree->currentItem();
				if (item != 0)
				{
					QVariant v = item->data(0, MVD_MST_ITEM_NEW);
					bool isNew = v.isNull() ? false : v.toBool();
					quint32 id = item->data(0, MVD_MST_ITEM_ID).toUInt();

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
		}
	}
	
	return QItemDelegate::eventFilter(object, event);
}

//! \internal Returns true if an item is valid (all required fields are not empty).
bool MvdSDDelegate::isItemValid(Movida::SmdDataRole ds, const QTreeWidgetItem& item) const
{
	bool valid = false;

	switch (ds)
	{
	case Movida::ActorRole:
	case Movida::CrewMemberRole:
	case Movida::DirectorRole:
	case Movida::ProducerRole:
		// First and last name not empty
		valid = !(item.text(0).isEmpty() && item.text(1).isEmpty());
		break;
	case Movida::UrlRole:
		// Url not empty
		valid = !item.text(1).isEmpty();
		break;
	default:
		// First column not empty
		valid = !item.text(0).isEmpty();
	}

	return valid;
}

bool MvdSDDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
	const QStyleOptionViewItem& option, const QModelIndex& index)
{
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent* e = dynamic_cast<QKeyEvent*>(event);
		Q_ASSERT(e != 0);

		// Intercept ESC key when editing an item and delete unused new items
		if (e->key() == Qt::Key_Escape)
		{
			MvdSDTreeWidget* tree = 0;
			DelegateType dt = delegateType(index, &tree);
			if (dt == UnsupportedDelegate)
				return QItemDelegate::editorEvent(event, model, option, index);
			Q_ASSERT(tree != 0);

			QTreeWidgetItem* item = tree->currentItem();
			if (item != 0)
			{
				QVariant v = item->data(0, MVD_MST_ITEM_NEW);
				bool isNew = v.isNull() ? false : v.toBool();
				quint32 id = item->data(0, MVD_MST_ITEM_ID).toUInt();

				//! \todo Delete empty new items (possibly check first and last name)
				if (id == 0)
				{
					if (! (isNew && isItemValid(tree->dataSource(), *item)) )
					{
						delete item;
					}
				}
			}
		}
	}

	return QItemDelegate::editorEvent(event, model, option, index);
}
