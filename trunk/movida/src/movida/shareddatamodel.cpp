/**************************************************************************
** Filename: shareddatamodel.cpp
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

#include "shareddatamodel.h"
#include "settings.h"
#include "moviecollection.h"
#include "shareddata.h"

#include <QCoreApplication>

/*!
	\class MvdSharedDataModel shareddatamodel.h
	\ingroup movida

	\brief Model for a movie collection's shared data.
	The model only displays global data unless you set a movie collection
	using setMovieCollection(). The model displays both global and
	collection-specific data, unless you call setFilter().
*/

static const unsigned short MVD_SDM_FirstName = 0;
static const unsigned short MVD_SDM_LastName = 1;
static const unsigned short MVD_SDM_Genre = 2;
static const unsigned short MVD_SDM_Country = 3;
static const unsigned short MVD_SDM_Language = 4;
static const unsigned short MVD_SDM_Tag = 5;
static const unsigned short MVD_SDM_Url = 6;
static const unsigned short MVD_SDM_Description = 7;
static const unsigned short MVD_SDM_LinkCount = 8;

//! \internal This is the number of columns. \todo Dynamically add/remove columns.
static const unsigned short MVD_SDM_ColumnCount = 9;


/************************************************************************
MvdSharedDataModel_P
*************************************************************************/

//! \internal
class MvdSharedDataModel_P
{
public:
	typedef struct SortWrapper
	{
		SortWrapper() { dataid = -1; }
		bool operator< (const SortWrapper& o) const
		{
			if (data.type() == QVariant::String)
				return data.toString() < o.data.toString();
			else if (data.type() == QVariant::Double)
				return data.toDouble() < o.data.toDouble();
			else if (data.type() == QVariant::LongLong)
				return data.toLongLong() < o.data.toLongLong();
			else if (data.type() == QVariant::Int)
				return data.toInt() < o.data.toInt();

			return false;
		}

		int dataid;
		QVariant data;
	};

	MvdSharedDataModel_P() :
		collection(0), mode(Movida::PersonRole), 
		filter(MvdSharedDataModel::DefaultScope)
		{}

	MvdSharedData& smd(smdid id) const;

	QString resolveSectionLabel(unsigned short s) const;
	bool sort(int column, Qt::SortOrder order);

	MvdMovieCollection* collection;
	QList<int> idmap;
	Movida::SmdDataRole mode;
	MvdSharedDataModel::DataScope filter;
};

//! \internal Returns a reference to the global or to the collection specific smd(id).
MvdSharedData& MvdSharedDataModel_P::smd(smdid id) const
{
	if (MvdSharedData::isHardcoded(id))
		return Movida::globalSD();
	return collection == 0 ? Movida::globalSD() : collection->smd();
}

//! \internal Maps section identifiers to section labels.
QString MvdSharedDataModel_P::resolveSectionLabel(unsigned short s) const
{
	switch (s)
	{
	case MVD_SDM_FirstName: return QCoreApplication::translate("MvdSharedDataModel", "First Name");
	case MVD_SDM_LastName: return QCoreApplication::translate("MvdSharedDataModel", "Last Name");
	case MVD_SDM_Genre: return QCoreApplication::translate("MvdSharedDataModel", "Genre");
	case MVD_SDM_Country: return QCoreApplication::translate("MvdSharedDataModel", "Country");
	case MVD_SDM_Language: return QCoreApplication::translate("MvdSharedDataModel", "Language");
	case MVD_SDM_Tag: return QCoreApplication::translate("MvdSharedDataModel", "Tag");
	case MVD_SDM_Url: return QCoreApplication::translate("MvdSharedDataModel", "URL");
	case MVD_SDM_Description: return QCoreApplication::translate("MvdSharedDataModel", "Description");
	case MVD_SDM_LinkCount: return QCoreApplication::translate("MvdSharedDataModel", "MvdMovie Count");
	}
	return QString();
}

//! \internal
bool MvdSharedDataModel_P::sort(int column, Qt::SortOrder order)
{
	if (idmap.isEmpty())
		return false;

	QList<SortWrapper> l;

	for (int i = 0; i < idmap.size(); ++i)
	{
		int id = idmap.at(i);

		SortWrapper w;
		w.dataid = id;

		switch (column)
		{
			case MVD_SDM_FirstName:
			{
				const MvdSharedData::PersonData* pd = smd(id).person(id);
				if (pd != 0)
				{
					w.data = pd->firstName;
					l << w;
				}
				break;
			}
			case MVD_SDM_LastName:
			{
				const MvdSharedData::PersonData* pd = smd(id).person(id);
				if (pd != 0)
				{
					w.data = pd->lastName;
					l << w;
				}
				break;
			}
			case MVD_SDM_Genre:
			{
				const MvdSharedData::StringData* sd = smd(id).genre(id);
				if (sd != 0)
				{
					w.data = sd->name;
					l << w;
				}
				break;
			}
			case MVD_SDM_Country:
			{
				w.data = QLocale::countryToString((QLocale::Country)id);
				l << w;
				break;
			}
			case MVD_SDM_Language:
			{
				w.data = QLocale::languageToString((QLocale::Language)id);
				l << w;
				break;
			}
			case MVD_SDM_Tag:
			{
				const MvdSharedData::StringData* sd = smd(id).tag(id);
				if (sd != 0)
				{
					w.data = sd->name;
					l << w;
				}
				break;
			}
			case MVD_SDM_Url:
			{
				const MvdSharedData::UrlData* ud = smd(id).url(id);
				if (ud != 0)
				{
					w.data = ud->url;
					l << w;
				}
				break;
			}
			case MVD_SDM_Description:
			{
				const MvdSharedData::UrlData* ud = smd(id).url(id);
				if (ud != 0)
				{
					w.data = ud->description;
					l << w;
				}
				break;
			}
			case MVD_SDM_LinkCount:
			{
				switch (mode)
				{
				case Movida::PersonRole:
					w.data = smd(id).personUsageCount(id); break;
				case Movida::GenreRole:
					w.data = smd(id).genreUsageCount(id); break;
				case Movida::CountryRole:
					w.data = smd(id).countryUsageCount(id); break;
				case Movida::LanguageRole:
					w.data = smd(id).languageUsageCount(id); break;
				case Movida::TagRole:
					w.data = smd(id).tagUsageCount(id); break;
				case Movida::UrlRole:
					w.data = smd(id).urlUsageCount(id); break;
				default:
					break;
				}
			}
			default:
			;
		}
	}

	if (l.isEmpty())
		 return false;

	/*
	Sorts the items in ascending order using the heap sort algorithm.
	The sort algorithm is efficient on large data sets. It operates in linear-logarithmic time, O(n log n).
	This function requires the item type (in the example above, int) to implement operator<().
	If of two items neither is less than the other, the items are taken to be equal. 
	The item that appeared before the other in the original container will still appear first after the sort. 
	This property is often useful when sorting user-visible data.
	*/
	qStableSort(l);

	idmap.clear();

	int sz = l.size();

	if (order == Qt::AscendingOrder)
	{	
		for (int i = 0; i < sz; ++i)
			idmap.append( l.at(i).dataid );
	}
	else
	{
		for (int i = sz - 1; i >= 0; --i)
			idmap.append( l.at(i).dataid );
	}

	return true;
}


/************************************************************************
MvdSharedDataModel
*************************************************************************/

/*!
	Creates a new empty model. A movie collection needs to be set for the item
	to work correctly.
*/
MvdSharedDataModel::MvdSharedDataModel(Movida::SmdDataRole mode, QObject* parent)
: QAbstractTableModel(parent)
{
	d = new MvdSharedDataModel_P;
	d->mode = mode;

	MvdSharedData* globalSmd = &Movida::globalSD();
	setData(*globalSmd);

	connect( globalSmd, SIGNAL(personAdded(int)), this, SLOT(itemAdded(int)) );
	connect( globalSmd, SIGNAL(genreAdded(int)), this, SLOT(itemAdded(int)) );
	connect( globalSmd, SIGNAL(tagAdded(int)), this, SLOT(itemAdded(int)) );
	connect( globalSmd, SIGNAL(urlAdded(int)), this, SLOT(itemAdded(int)) );

	connect( globalSmd, SIGNAL(personRemoved(int)), this, SLOT(itemRemoved(int)) );
	connect( globalSmd, SIGNAL(genreRemoved(int)), this, SLOT(itemRemoved(int)) );
	connect( globalSmd, SIGNAL(tagRemoved(int)), this, SLOT(itemRemoved(int)) );
	connect( globalSmd, SIGNAL(urlRemoved(int)), this, SLOT(itemRemoved(int)) );

	connect( globalSmd, SIGNAL(personChanged(int)), this, SLOT(itemChanged(int)) );
	connect( globalSmd, SIGNAL(genreChanged(int)), this, SLOT(itemChanged(int)) );
	connect( globalSmd, SIGNAL(tagChanged(int)), this, SLOT(itemChanged(int)) );
	connect( globalSmd, SIGNAL(urlChanged(int)), this, SLOT(itemChanged(int)) );

	connect( globalSmd, SIGNAL(personLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
	connect( globalSmd, SIGNAL(genreLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
	connect( globalSmd, SIGNAL(tagLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
	connect( globalSmd, SIGNAL(urlLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
	connect( globalSmd, SIGNAL(countryLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
	connect( globalSmd, SIGNAL(languageLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );

	connect( globalSmd, SIGNAL(dataCleared()), this, SLOT(smdCleared()) );
}

/*!
	Creates a new model for the specified movie collection.
*/
MvdSharedDataModel::MvdSharedDataModel(Movida::SmdDataRole mode, 
	MvdMovieCollection* collection, QObject* parent)
: QAbstractTableModel(parent)
{
	d = new MvdSharedDataModel_P;
	d->mode = mode;

	MvdSharedData* globalSmd = &Movida::globalSD();
	setData(*globalSmd);
	
	setMovieCollection(collection);

	connect( globalSmd, SIGNAL(personAdded(int)), this, SLOT(itemAdded(int)) );
	connect( globalSmd, SIGNAL(genreAdded(int)), this, SLOT(itemAdded(int)) );
	connect( globalSmd, SIGNAL(tagAdded(int)), this, SLOT(itemAdded(int)) );
	connect( globalSmd, SIGNAL(urlAdded(int)), this, SLOT(itemAdded(int)) );

	connect( globalSmd, SIGNAL(personRemoved(int)), this, SLOT(itemRemoved(int)) );
	connect( globalSmd, SIGNAL(genreRemoved(int)), this, SLOT(itemRemoved(int)) );
	connect( globalSmd, SIGNAL(tagRemoved(int)), this, SLOT(itemRemoved(int)) );
	connect( globalSmd, SIGNAL(urlRemoved(int)), this, SLOT(itemRemoved(int)) );

	connect( globalSmd, SIGNAL(personChanged(int)), this, SLOT(itemChanged(int)) );
	connect( globalSmd, SIGNAL(genreChanged(int)), this, SLOT(itemChanged(int)) );
	connect( globalSmd, SIGNAL(tagChanged(int)), this, SLOT(itemChanged(int)) );
	connect( globalSmd, SIGNAL(urlChanged(int)), this, SLOT(itemChanged(int)) );

	connect( globalSmd, SIGNAL(personLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
	connect( globalSmd, SIGNAL(genreLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
	connect( globalSmd, SIGNAL(tagLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
	connect( globalSmd, SIGNAL(urlLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
	connect( globalSmd, SIGNAL(countryLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
	connect( globalSmd, SIGNAL(languageLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );

	connect( globalSmd, SIGNAL(dataCleared()), this, SLOT(smdCleared()) );
}

/*!
	Destroys this model.
*/
MvdSharedDataModel::~MvdSharedDataModel()
{
	delete d;
}

/*!
	Sets a movie collection for this movie.
*/
void MvdSharedDataModel::setMovieCollection(MvdMovieCollection* c)
{
	if (d->collection != 0)
	{
		disconnect(d->collection, 0, this, 0);
	}

	d->collection = c;
	d->idmap.clear();

	if (c != 0 && d->filter != MvdSharedDataModel::GlobalScope)
	{
		connect( &(c->smd()), SIGNAL(personAdded(int)), this, SLOT(itemAdded(int)) );
		connect( &(c->smd()), SIGNAL(genreAdded(int)), this, SLOT(itemAdded(int)) );
		connect( &(c->smd()), SIGNAL(tagAdded(int)), this, SLOT(itemAdded(int)) );
		connect( &(c->smd()), SIGNAL(urlAdded(int)), this, SLOT(itemAdded(int)) );

		connect( &(c->smd()), SIGNAL(personRemoved(int)), this, SLOT(itemRemoved(int)) );
		connect( &(c->smd()), SIGNAL(genreRemoved(int)), this, SLOT(itemRemoved(int)) );
		connect( &(c->smd()), SIGNAL(tagRemoved(int)), this, SLOT(itemRemoved(int)) );
		connect( &(c->smd()), SIGNAL(urlRemoved(int)), this, SLOT(itemRemoved(int)) );

		connect( &(c->smd()), SIGNAL(personChanged(int)), this, SLOT(itemChanged(int)) );
		connect( &(c->smd()), SIGNAL(genreChanged(int)), this, SLOT(itemChanged(int)) );
		connect( &(c->smd()), SIGNAL(tagChanged(int)), this, SLOT(itemChanged(int)) );
		connect( &(c->smd()), SIGNAL(urlChanged(int)), this, SLOT(itemChanged(int)) );

		connect( &(c->smd()), SIGNAL(personLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
		connect( &(c->smd()), SIGNAL(genreLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
		connect( &(c->smd()), SIGNAL(tagLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
		connect( &(c->smd()), SIGNAL(urlLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
		connect( &(c->smd()), SIGNAL(countryLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );
		connect( &(c->smd()), SIGNAL(languageLinkChanged(int)), this, SLOT(itemLinkChanged(int)) );

		connect( &(c->smd()), SIGNAL(dataCleared()), this, SLOT(smdCleared()) );

		// Insert existing data - new items will be added through the signal/slot mechanism
		setData(c->smd());
	}
}

/*!
	Sets a new mode for this model. The mode determines what kind of 
	SMD data is displayed by the model.
*/
void MvdSharedDataModel::setMode(Movida::SmdDataRole mode)
{
	if (d->mode == mode)
		return;

	d->mode = mode;
	reset();
}

/*!
	Returns the current mode (i.e. tells what kind of SMD data is currently
	displayed by the model.
*/
Movida::SmdDataRole MvdSharedDataModel::mode() const
{
	return d->mode;
}

/*!
	Sets a new filter for this model. The filter determines if the global,
	the collection-related or both SMDs should be used as data sources.
	The CollectionScope has obviously no effect if no collection has 
	been set.
*/
void MvdSharedDataModel::setFilter(DataScope filter)
{
	if (d->filter == filter)
		return;

	d->filter = filter;

	if (filter == GlobalScope && d->collection != 0)
		disconnect(d->collection, 0, this, 0);

	if (filter == CollectionScope)
		disconnect(&Movida::globalSD(), 0, this, 0);

	reset();
}

/*!
	Returns the current filter.
*/
MvdSharedDataModel::DataScope MvdSharedDataModel::filter() const
{
	return d->filter;
}

/*!
	Used by other components to obtain information about each item provided by the model.
*/
Qt::ItemFlags MvdSharedDataModel::flags(const QModelIndex& index) const
{
	Q_UNUSED(index);
	return Qt::ItemIsSelectable;
}

/*!
	Used to supply item data to views and delegates.
*/
QVariant MvdSharedDataModel::data(const QModelIndex& index, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (d->idmap.isEmpty())
		return QVariant();

	int row = index.row();
	int col = index.column();

	if (row >= d->idmap.size())
		return QVariant();

	int id = d->idmap.at(row);

	switch (col)
	{
		case MVD_SDM_FirstName:
		{
			const MvdSharedData::PersonData* pd = d->smd(id).person(id);
			return pd == 0 ? QString() : pd->firstName;
		}
		case MVD_SDM_LastName:
		{
			const MvdSharedData::PersonData* pd = d->smd(id).person(id);
			return pd == 0 ? QString() : pd->lastName;
		}
		case MVD_SDM_Genre:
		{
			const MvdSharedData::StringData* sd = d->smd(id).genre(id);
			return sd == 0 ? QString() : sd->name;
		}
		case MVD_SDM_Country:
		{
			return QLocale::countryToString((QLocale::Country)id);
		}
		case MVD_SDM_Language:
		{
			return QLocale::languageToString((QLocale::Language)id);
		}
		case MVD_SDM_Tag:
		{
			const MvdSharedData::StringData* sd = d->smd(id).tag(id);
			return sd == 0 ? QString() : sd->name;
		}
		case MVD_SDM_Url:
		{
			const MvdSharedData::UrlData* ud = d->smd(id).url(id);
			return ud == 0 ? QString() : ud->url;
		}
		case MVD_SDM_Description:
		{
			const MvdSharedData::UrlData* ud = d->smd(id).url(id);
			return ud == 0 ? QString() : ud->description;
		}
		case MVD_SDM_LinkCount:
		{
			switch (d->mode)
			{
			case Movida::PersonRole:
				return QVariant( d->smd(id).personUsageCount(id) ); break;
			case Movida::GenreRole:
				return QVariant( d->smd(id).genreUsageCount(id) ); break;
			case Movida::CountryRole:
				return QVariant( d->smd(id).countryUsageCount(id) ); break;
			case Movida::LanguageRole:
				return QVariant( d->smd(id).languageUsageCount(id) ); break;
			case Movida::TagRole:
				return QVariant( d->smd(id).tagUsageCount(id) ); break;
			case Movida::UrlRole:
				return QVariant( d->smd(id).urlUsageCount(id) ); break;
			default:
				break;
			}
		}
	}

	return QVariant();
}

/*!
	Provides views with information to show in their headers. The information is only retrieved by views that can 
	display header information.
*/
QVariant MvdSharedDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(orientation);
	if (role == Qt::DisplayRole)
		return d->resolveSectionLabel(section);
	return QVariant();
}

/*!
	Provides the number of rows of data exposed by the model.
*/
int MvdSharedDataModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return d->idmap.count();
}

/*!
	Provides the number of columns of data exposed by the model.
*/
int MvdSharedDataModel::columnCount(const QModelIndex& parent) const
{
	if (parent.column() > 0)
		return 0;

	return MVD_SDM_ColumnCount;
}

/*!
	Given a model index for a parent item, this function allows views and 
	delegates to access children of that item. 
	If no valid child item can be found that corresponds to the specified 
	row, column, and parent model index, the function must return QModelIndex() 
	- an invalid model index.
*/
QModelIndex MvdSharedDataModel::index(int row, int column, const QModelIndex& parent) const
{
	if (parent.isValid() || d->collection == 0)
		return QModelIndex();

	return createIndex(row, column);
}

/*!
	Provides a model index corresponding to the parent of any given child item. 
	If the model index specified corresponds to a top-level item in the model, 
	or if there is no valid parent item in the model, and the function must 
	return an invalid model index, created with the empty QModelIndex() 
	constructor.
*/
QModelIndex MvdSharedDataModel::parent(const QModelIndex& index) const
{
	Q_UNUSED(index);
	return QModelIndex();
}

//! \internal
void MvdSharedDataModel::itemAdded(int id)
{
	d->idmap.append(id);
	insertRows(d->idmap.size() - 1, 1, QModelIndex());
}

//! \internal
void MvdSharedDataModel::itemRemoved(int id)
{
	int pos = d->idmap.indexOf(id);
	if (pos < 0)
	{
		//qDebug("MvdSharedDataModel: Unable to retrieve item index.");
	}
	else 
	{
		d->idmap.removeAt(pos);
		removeRows(pos, 1, QModelIndex());
	}
}

//! \internal
void MvdSharedDataModel::itemChanged(int id)
{
	int pos = d->idmap.indexOf(id);
	if (pos < 0)
	{
		// qDebug("MvdSharedDataModel: Unable to retrieve item index.");
	}
	else 
	{
		emit dataChanged( createIndex(pos, 0), createIndex(pos, columnCount()) );
	}
}

//! \internal
void MvdSharedDataModel::itemLinkChanged(int id)
{
	int pos = d->idmap.indexOf(id);
	if (pos < 0)
	{
		// qDebug("MvdSharedDataModel: Unable to retrieve item index.");
	}
	else 
	{
		// Only update the last MMLinkCount column
		emit dataChanged( createIndex(pos, columnCount()-1), createIndex(pos, columnCount()) );
	}
}

//! \internal
void MvdSharedDataModel::smdCleared()
{
	d->idmap.clear();
	reset();
}

//! \internal
void MvdSharedDataModel::sort(int column, Qt::SortOrder order)
{
	if (d->idmap.isEmpty())
		return;

	if (d->sort(column, order))
		emit layoutChanged();
}

//! \internal Fills the internal id list with data from a specific smd.
void MvdSharedDataModel::setData(MvdSharedData& smd)
{
	switch (d->mode)
	{
		case Movida::PersonRole:
		{
			const QHash<smdid, MvdSharedData::PersonData>* pd = smd.persons();
			if (pd == 0)
				return;

			for (QHash<smdid, MvdSharedData::PersonData>::ConstIterator it = pd->constBegin();
				it != pd->constEnd(); ++it)
			{
				d->idmap.append(it.key());
				insertRows(d->idmap.size() - 1, 1, QModelIndex());
			}
		}
		break;
		case Movida::GenreRole:
		{
			const QHash<smdid, MvdSharedData::StringData>* pd = smd.genres();
			if (pd == 0)
				return;

			for (QHash<smdid, MvdSharedData::StringData>::ConstIterator it = pd->constBegin();
				it != pd->constEnd(); ++it)
			{
				d->idmap.append(it.key());
				insertRows(d->idmap.size() - 1, 1, QModelIndex());
			}
		}
		break;
		case Movida::CountryRole:
		{
			const QHash<smdid, MvdSharedData::StringData>* pd = smd.countries();
			if (pd == 0)
				return;

			for (QHash<smdid, MvdSharedData::StringData>::ConstIterator it = pd->constBegin();
				it != pd->constEnd(); ++it)
			{
				d->idmap.append(it.key());
				insertRows(d->idmap.size() - 1, 1, QModelIndex());
			}
		}
		break;
		case Movida::LanguageRole:
		{
			const QHash<smdid, MvdSharedData::StringData>* pd = smd.languages();
			if (pd == 0)
				return;

			for (QHash<smdid, MvdSharedData::StringData>::ConstIterator it = pd->constBegin();
				it != pd->constEnd(); ++it)
			{
				d->idmap.append(it.key());
				insertRows(d->idmap.size() - 1, 1, QModelIndex());
			}
		}
		break;
		case Movida::TagRole:
		{
			const QHash<smdid, MvdSharedData::StringData>* pd = smd.tags();
			if (pd == 0)
				return;

			for (QHash<smdid, MvdSharedData::StringData>::ConstIterator it = pd->constBegin();
				it != pd->constEnd(); ++it)
			{
				d->idmap.append(it.key());
				insertRows(d->idmap.size() - 1, 1, QModelIndex());
			}
		}
		break;
		case Movida::UrlRole:
		{
			const QHash<smdid, MvdSharedData::UrlData>* pd = smd.urls();
			if (pd == 0)
				return;

			for (QHash<smdid, MvdSharedData::UrlData>::ConstIterator it = pd->constBegin();
				it != pd->constEnd(); ++it)
			{
				d->idmap.append(it.key());
				insertRows(d->idmap.size() - 1, 1, QModelIndex());
			}
		}
		break;
		default:
			;
	}
}
