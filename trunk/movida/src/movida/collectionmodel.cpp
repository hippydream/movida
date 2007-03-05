/**************************************************************************
** Filename: collectionmodel.cpp
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

#include "collectionmodel.h"
#include "moviecollection.h"
#include "movie.h"
#include "settings.h"
#include "shareddata.h"

#include <QCoreApplication>

/*!
	\class MvdCollectionModel collectionmodel.h
	\ingroup movida

	\brief Model for a movie collection.
*/

static const unsigned short MVD_ColTitle = 0;
static const unsigned short MVD_ColOriginalTitle = 1;
static const unsigned short MVD_ColEdition = 2;
static const unsigned short MVD_ColProductionYear = 3;
static const unsigned short MVD_ColReleaseYear = 4;
static const unsigned short MVD_ColDirectors = 5;
static const unsigned short MVD_ColProducers = 6;
static const unsigned short MVD_ColCast = 7;
static const unsigned short MVD_ColCrew = 8;
static const unsigned short MVD_ColRunningTime = 9;
static const unsigned short MVD_ColStorageId = 10;
static const unsigned short MVD_ColGenres = 11;
static const unsigned short MVD_ColCountries = 12;
static const unsigned short MVD_ColLanguages = 13;
static const unsigned short MVD_ColTags = 14;
static const unsigned short MVD_ColColorMode = 15;
static const unsigned short MVD_ColImdbId = 16;
static const unsigned short MVD_ColRating = 17;

//! \internal This is the number of columns. \todo Dynamically add/remove columns.
static const unsigned short MVD_ColumnCount = 18;


/************************************************************************
 MvdCollectionModel_P
*************************************************************************/

//! \internal
class MvdCollectionModel_P
{
public:
	typedef struct SortWrapper
	{
		SortWrapper() { movieId = 0; }
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

		movieid movieId;
		QVariant data;
	};

	MvdCollectionModel_P() : collection(0) {}

	MvdSharedData& smd(smdid id) const;

	QString resolveSectionLabel(unsigned short s) const;
	QString dataList(const QList<smdid>& list, Movida::SmdDataRole dt) const;
	QString initials(const QString& s) const;
	void sort(int column, Qt::SortOrder order);

	MvdMovieCollection* collection;
	QList<movieid> movies;
};

//! \internal Returns a reference to the global or to the collection specific smd(id).
MvdSharedData& MvdCollectionModel_P::smd(smdid id) const
{
	if (MvdSharedData::isHardcoded(id))
		return Movida::globalSD();
	return collection == 0 ? Movida::globalSD() : collection->smd();
}

//! \internal Maps section identifiers to section labels.
QString MvdCollectionModel_P::resolveSectionLabel(unsigned short s) const
{
	switch (s)
	{
	case MVD_ColTitle: return QCoreApplication::translate("MvdCollectionModel", "Title");
	case MVD_ColOriginalTitle: return QCoreApplication::translate("MvdCollectionModel", "Original title");
	case MVD_ColEdition: return QCoreApplication::translate("MvdCollectionModel", "Version");
	case MVD_ColProductionYear: return QCoreApplication::translate("MvdCollectionModel", "Prod. year");
	case MVD_ColReleaseYear: return QCoreApplication::translate("MvdCollectionModel", "Rel. year");
	case MVD_ColDirectors: return QCoreApplication::translate("MvdCollectionModel", "Directed by");
	case MVD_ColProducers: return QCoreApplication::translate("MvdCollectionModel", "Produced by");
	case MVD_ColCast: return QCoreApplication::translate("MvdCollectionModel", "Cast");
	case MVD_ColCrew: return QCoreApplication::translate("MvdCollectionModel", "Crew");
	case MVD_ColRunningTime: return QCoreApplication::translate("MvdCollectionModel", "Running time");
	case MVD_ColStorageId: return QCoreApplication::translate("MvdCollectionModel", "Storage ID");
	case MVD_ColGenres: return QCoreApplication::translate("MvdCollectionModel", "Genres");
	case MVD_ColCountries: return QCoreApplication::translate("MvdCollectionModel", "Countries");
	case MVD_ColLanguages: return QCoreApplication::translate("MvdCollectionModel", "Languages");
	case MVD_ColTags: return QCoreApplication::translate("MvdCollectionModel", "Tags");
	case MVD_ColColorMode: return QCoreApplication::translate("MvdCollectionModel", "Color mode");
	case MVD_ColImdbId: return QCoreApplication::translate("MvdCollectionModel", "IMDb ID");
	case MVD_ColRating: return QCoreApplication::translate("MvdCollectionModel", "Rating");
	}
	return QString();
}

//! \internal Converts a list of IMDb IDs to a list of semi-colon separated strings
QString MvdCollectionModel_P::dataList(const QList<smdid>& list, Movida::SmdDataRole dt) const
{
	QString s;

	//! \todo Some names may have a non trivial way to be transformed into initials: what if "De Niro" would be a name? Are the initials "D.N." or just "D."?
	bool truncate = Movida::settings().getBool("initials", "movie_list");

	for (int i = 0; i < list.size(); ++i)
	{
		smdid id = list.at(i);
		
		switch (dt)
		{
		case Movida::PersonRole:
			{
				const MvdSharedData::PersonData* data = smd(id).person(id);
				if (data != 0)
				{
					if (!s.isEmpty())
						s.append("; ");

					if (data->firstName.isEmpty())
						s.append(data->lastName);
					else if (data->lastName.isEmpty())
						s.append(data->firstName);
					else if (truncate)
						s.append( QString("%1 %2").arg(initials(data->firstName)).arg(data->lastName) );
					else s.append( QString("%1 %2").arg(data->firstName).arg(data->lastName) );
				}
			}
			break;
		case Movida::GenreRole:
			{
				const MvdSharedData::StringData* data = smd(id).genre(id);
				if (data != 0)
				{
					if (!s.isEmpty())
						s.append("; ");

					s.append(data->name);
				}
			}
			break;
		case Movida::TagRole:
			{
				const MvdSharedData::StringData* data = smd(id).tag(id);
				if (data != 0)
				{
					if (!s.isEmpty())
						s.append("; ");

					s.append(data->name);
				}
			}
			break;
		case Movida::CountryRole:
			{
				const MvdSharedData::StringData* data = smd(id).country(id);
				if (data != 0)
				{
					if (!s.isEmpty())
						s.append("; ");

					s.append(data->name);
				}
			}
			break;
		case Movida::LanguageRole:
			{
				const MvdSharedData::StringData* data = smd(id).language(id);
				if (data != 0)
				{
					if (!s.isEmpty())
						s.append("; ");

					s.append(data->name);
				}
			}
			break;
		default: ;
		}
	}

	return s;
}

//! \internal
QString MvdCollectionModel_P::initials(const QString& s) const
{
	QString final;
	QStringList list = s.split(" ", QString::SkipEmptyParts);

	for (int i = 0; i < list.size(); ++i)
	{
		QString p = list.at(i);

		Q_ASSERT(!p.isEmpty()); // We use skipEmptyParts in split()

		final.append( p.at(0).toUpper() );
		final.append( "." );
	}

	return final;
}

//! \internal
void MvdCollectionModel_P::sort(int column, Qt::SortOrder order)
{
	QList<SortWrapper> l;

	for (int i = 0; i < movies.size(); ++i)
	{
		movieid movieId = movies.at(i);
		MvdMovie movie = collection->movie(movieId);

		SortWrapper w;
		w.movieId = movieId;

		switch (column)
		{
		case MVD_ColTitle: w.data = movie.title(); l << w; break;
		case MVD_ColOriginalTitle: w.data = movie.originalTitle(); l << w; break;
		case MVD_ColEdition: w.data = movie.edition(); l << w; break;
		case MVD_ColProductionYear: w.data = movie.productionYear(); l << w; break;
		case MVD_ColReleaseYear: w.data = movie.releaseYear(); l << w; break;
		case MVD_ColDirectors: w.data = dataList(movie.directors(), Movida::PersonRole); l << w; break;
		case MVD_ColProducers: w.data = dataList(movie.producers(), Movida::PersonRole); l << w; break;
		case MVD_ColCast: w.data = dataList(movie.actorIDs(), Movida::PersonRole); l << w; break;
		case MVD_ColCrew: w.data = dataList(movie.crewMemberIDs(), Movida::PersonRole); l << w; break;
		case MVD_ColRunningTime: w.data = QVariant(movie.runningTime()); l << w; break;
		case MVD_ColStorageId: w.data = movie.storageId(); l << w; break;
		case MVD_ColGenres: dataList(movie.genres(), Movida::GenreRole); l << w; break;
		case MVD_ColTags: dataList(movie.tags(), Movida::TagRole); l << w; break;
		case MVD_ColCountries: dataList(movie.countries(), Movida::CountryRole); l << w; break;
		case MVD_ColLanguages: dataList(movie.languages(), Movida::LanguageRole); l << w; break;
		case MVD_ColColorMode: w.data = movie.colorModeString(); l << w; break;
		case MVD_ColImdbId: w.data = movie.imdbId(); l << w; break;
		case MVD_ColRating: w.data = QVariant(movie.rating()); l << w; break;
		default: ;
		}
	}

	if (l.isEmpty())
		 return;

	/*
	Sorts the items in ascending order using the heap sort algorithm.
	The sort algorithm is efficient on large data sets. It operates in linear-logarithmic time, O(n log n).
	This function requires the item type (in the example above, int) to implement operator<().
	If of two items neither is less than the other, the items are taken to be equal. 
	The item that appeared before the other in the original container will still appear first after the sort. 
	This property is often useful when sorting user-visible data.
	*/
	qStableSort(l);

	movies.clear();

	int sz = l.size();

	if (order == Qt::AscendingOrder)
	{	
		for (int i = 0; i < sz; ++i)
			movies.append( l.at(i).movieId );
	}
	else
	{
		for (int i = sz - 1; i >= 0; --i)
			movies.append( l.at(i).movieId );
	}
}


/************************************************************************
 MvdCollectionModel
*************************************************************************/

/*!
	Creates a new empty model. A movie collection needs to be set for the item
	to work correctly.
*/
MvdCollectionModel::MvdCollectionModel(QObject* parent)
: QAbstractTableModel(parent), d(new MvdCollectionModel_P)
{
}

/*!
	Creates a new model for the specified movie collection.
*/
MvdCollectionModel::MvdCollectionModel(MvdMovieCollection* collection, QObject* parent)
: QAbstractTableModel(parent), d(new MvdCollectionModel_P)
{
	setMovieCollection(collection);
}

/*!
	Destroys this model.
*/
MvdCollectionModel::~MvdCollectionModel()
{
	delete d;
}

/*!
	Sets a movie collection for this movie.
*/
void MvdCollectionModel::setMovieCollection(MvdMovieCollection* c)
{
	if (d->collection != 0)
	{
		disconnect(this, SLOT(movieAdded(movieid)));
		disconnect(this, SLOT(movieChanged(movieid)));
		disconnect(this, SLOT(movieRemoved(movieid)));
		disconnect(this, SLOT(collectionCleared()));
	}

	d->collection = c;

	emit layoutAboutToBeChanged();

	d->movies.clear();

	if (c != 0)
	{
		connect( c, SIGNAL(movieAdded(movieid)), this, SLOT(movieAdded(movieid)) );
		connect( c, SIGNAL(movieChanged(movieid)), this, SLOT(movieChanged(movieid)) );
		connect( c, SIGNAL(movieRemoved(movieid)), this, SLOT(movieRemoved(movieid)) );
		connect( c, SIGNAL(cleared()), this, SLOT(collectionCleared()) );

		// Insert existing movies - new movies will be added through 
		// the signal/slot mechanism
		QList<movieid> list = c->movieIds();
	
		for (int i = 0; i < list.size(); ++i)
			d->movies.append(list.at(i));
	}

	emit layoutChanged();
}

/*!
	Used by other components to obtain information about each item provided
	by the model.
*/
Qt::ItemFlags MvdCollectionModel::flags(const QModelIndex& index) const
{
	Q_UNUSED(index);
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

/*!
	Used to supply item data to views and delegates.
*/
QVariant MvdCollectionModel::data(const QModelIndex& index, int role) const
{
	if (d->movies.isEmpty())
		return QVariant();

	int row = index.row();
	int col = index.column();

	if (row < 0 || row > (d->movies.size() - 1) || col < 0)
		return QVariant();

	if (role == (int) MvdCollectionModel::MovieIdRole)
		return d->movies.at(row);

	if (role == Qt::DecorationRole && col == 0)
	{
		movieid id = d->movies.at(row);
		if (id == 0)
			return QVariant();
		MvdMovie movie = d->collection->movie(id);
		return movie.poster();
	}

	if (role != Qt::DisplayRole)
		return QVariant();	

	if (row >= d->movies.size())
		return QVariant();

	movieid id = d->movies.at(row);
	MvdMovie movie = d->collection->movie(id);

	switch (col)
	{
	case MVD_ColTitle: return movie.title();
	case MVD_ColOriginalTitle: return movie.originalTitle();
	case MVD_ColEdition: return movie.edition();
	case MVD_ColProductionYear: return movie.productionYear();
	case MVD_ColReleaseYear: return movie.releaseYear();
	case MVD_ColDirectors: return d->dataList(movie.directors(), Movida::PersonRole);
	case MVD_ColProducers: return d->dataList(movie.producers(), Movida::PersonRole);
	case MVD_ColCast: return d->dataList(movie.actorIDs(), Movida::PersonRole);
	case MVD_ColCrew: return d->dataList(movie.crewMemberIDs(), Movida::PersonRole);
	case MVD_ColRunningTime: return movie.runningTimeString();
	case MVD_ColStorageId: return movie.storageId();
	case MVD_ColGenres: return d->dataList(movie.genres(), Movida::GenreRole);
	case MVD_ColTags: return d->dataList(movie.tags(), Movida::TagRole);
	case MVD_ColCountries: return d->dataList(movie.countries(), Movida::CountryRole);
	case MVD_ColLanguages: return d->dataList(movie.languages(), Movida::LanguageRole);
	case MVD_ColColorMode: return movie.colorModeString();
	case MVD_ColImdbId: return movie.imdbId();
	case MVD_ColRating: return QVariant(movie.rating());
	}

	return QVariant();
}

/*!
	Provides views with information to show in their headers. The information 
	is only retrieved by views that can display header information.
*/
QVariant MvdCollectionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(orientation);
	if (role == Qt::DisplayRole)
		return d->resolveSectionLabel(section);
	return QVariant();
}

/*!
	Provides the number of rows of data exposed by the model.
*/
int MvdCollectionModel::rowCount(const QModelIndex&) const
{
	return d->movies.size();
}

/*!
	Provides the number of columns of data exposed by the model.
*/
int MvdCollectionModel::columnCount(const QModelIndex&) const
{
	return MVD_ColumnCount;
}

/*!
	Given a model index for a parent item, this function allows views and 
	delegates to access children of that item. 
	If no valid child item can be found that corresponds to the specified 
	row, column, and parent model index, the function must return QModelIndex() 
	- an invalid model index.
*/
QModelIndex MvdCollectionModel::index(int row, int column, const QModelIndex& parent) const
{
	if (parent.isValid() || d->collection == 0)
		return QModelIndex();

	return createIndex(row, column);
}

/*!
	Provides a model index corresponding to the parent of any given child item. 
	If the model index specified corresponds to a top-level item in the model, 
	or if there is no valid parent item in the model, and the function must 
	return an invalid model index, created with the empty QModelIndex() constructor.
*/
QModelIndex MvdCollectionModel::parent(const QModelIndex&) const
{
	return QModelIndex();
}

//! \internal
void MvdCollectionModel::movieAdded(movieid id)
{
	int row = d->movies.size();
	insertRows(row, 1);
	d->movies[row] = id;
}

//! \internal
bool MvdCollectionModel::insertRows(int row, int count, const QModelIndex& p)
{
	beginInsertRows(p, row, row + count);
	for (int i = 0; i < count; ++i)
		d->movies.insert(row + i, 0);
	endInsertRows();
	return true;
}

//! \internal
void MvdCollectionModel::movieRemoved(movieid id)
{
	int row = d->movies.indexOf(id);
	removeRows(row, 1, QModelIndex());
}

//! \internal
bool MvdCollectionModel::removeRows(int row, int count, const QModelIndex& p)
{
	beginRemoveRows(p, row, row + count);
	for (int i = 0; i < count; ++i)
		if (row < d->movies.size())
			d->movies.removeAt(row);
	endRemoveRows();
	return true;
}

//! \internal
void MvdCollectionModel::movieChanged(movieid id)
{
	int row = d->movies.indexOf(id);
	emit dataChanged( createIndex(row, 0), createIndex(row, columnCount()) );
}

//! \internal
void MvdCollectionModel::collectionCleared()
{
	d->movies.clear();
	reset();
}

//! \internal
void MvdCollectionModel::sort(int column, Qt::SortOrder order)
{
	if (d->movies.isEmpty())
		return;

	emit layoutAboutToBeChanged();
	d->sort(column, order);
	emit layoutChanged();
}
