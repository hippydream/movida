/**************************************************************************
** Filename: collectionmodel.cpp
**
** Copyright (C) 2007-2008 Angius Fabrizio. All rights reserved.
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

#include "collectionmodel.h"
#include "mvdcore/moviecollection.h"
#include "mvdcore/movie.h"
#include "mvdcore/settings.h"
#include "mvdcore/shareddata.h"
#include <QCoreApplication>

/*!
	\class MvdCollectionModel collectionmodel.h
	\ingroup Movida

	\brief Model for a movie collection.
*/


/************************************************************************
 MvdCollectionModel_P
*************************************************************************/

//! \internal
class MvdCollectionModel_P
{
public:
	typedef struct SortWrapper
	{
		SortWrapper(mvdid aid = MvdNull) : id(aid) { }

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
			else if (data.type() == QVariant::Bool)
				return (int)data.toBool() < (int)o.data.toBool();

			return false;
		}

		mvdid id;
		QVariant data;
	};

	MvdCollectionModel_P(MvdCollectionModel* c) : 
		q(c), collection(0), sortOrder(Qt::AscendingOrder), sortAttribute(Movida::TitleAttribute) {}

	MvdSharedData& smd(mvdid id) const;

	QString dataList(const QList<mvdid>& list, Movida::DataRole dt, int role = Qt::DisplayRole) const;
	void sort(Movida::MovieAttribute attribute, Qt::SortOrder order);

	MvdCollectionModel* q;
	
	MvdMovieCollection* collection;
	QList<mvdid> movies;
	Qt::SortOrder sortOrder;
	Movida::MovieAttribute sortAttribute;
};

//! \internal Converts a list of IMDb IDs to a list of semi-colon separated strings
QString MvdCollectionModel_P::dataList(const QList<mvdid>& list, Movida::DataRole dt, int role) const
{
	Q_UNUSED(dt);

	QString s;
	if (!collection)
		return s;

	for (int i = 0; i < list.size(); ++i)
	{
		mvdid id = list.at(i);
		MvdSdItem sdItem = collection->smd().item(id);
		
		if (!s.isEmpty()) {
			if (role == Movida::SmartViewDisplayRole) {
				if (i == list.size() - 1)
					s.append(QCoreApplication::translate("Movie collection model", " and ", "Smart view: joining multiple item elements (e.g. 'Starring Robert De Niro *AND* Wesley Snipes')"));
				else s.append(", ");
			} else s.append("; ");
		}
		
		s.append(sdItem.value);
	}

	return s;
}

//! \internal
void MvdCollectionModel_P::sort(Movida::MovieAttribute attribute, Qt::SortOrder order)
{
	sortOrder = order;
	sortAttribute = attribute;

	QList<SortWrapper> l;

	for (int i = 0; i < movies.size(); ++i)
	{
		mvdid id = movies.at(i);
		MvdMovie movie = collection->movie(id);

		SortWrapper w(id);

		switch (attribute)
		{
		case Movida::TitleAttribute: w.data = movie.title(); l << w; break;
		case Movida::OriginalTitleAttribute: w.data = movie.originalTitle(); l << w; break;
		case Movida::ProductionYearAttribute: w.data = movie.productionYear(); l << w; break;
		case Movida::ReleaseYearAttribute: w.data = movie.releaseYear(); l << w; break;
		case Movida::DirectorsAttribute: w.data = dataList(movie.directors(), Movida::PersonRole); l << w; break;
		case Movida::ProducersAttribute: w.data = dataList(movie.producers(), Movida::PersonRole); l << w; break;
		case Movida::CastAttribute: w.data = dataList(movie.actorIDs(), Movida::PersonRole); l << w; break;
		case Movida::CrewAttribute: w.data = dataList(movie.crewMemberIDs(), Movida::PersonRole); l << w; break;
		case Movida::RunningTimeAttribute: w.data = QVariant(movie.runningTime()); l << w; break;
		case Movida::StorageIdAttribute: w.data = movie.storageId(); l << w; break;
		case Movida::GenresAttribute: dataList(movie.genres(), Movida::GenreRole); l << w; break;
		case Movida::TagsAttribute: dataList(movie.tags(), Movida::TagRole); l << w; break;
		case Movida::CountriesAttribute: dataList(movie.countries(), Movida::CountryRole); l << w; break;
		case Movida::LanguagesAttribute: dataList(movie.languages(), Movida::LanguageRole); l << w; break;
		case Movida::ColorModeAttribute: w.data = movie.colorModeString(); l << w; break;
		case Movida::ImdbIdAttribute: w.data = movie.imdbId(); l << w; break;
		case Movida::RatingAttribute: w.data = QVariant(movie.rating()); l << w; break;
		case Movida::SeenAttribute: w.data = QVariant(movie.hasSpecialTagEnabled(MvdMovie::SeenTag)); l << w; break;
		case Movida::SpecialAttribute: w.data = QVariant(movie.hasSpecialTagEnabled(MvdMovie::SpecialTag)); l << w; break;
		case Movida::LoanedAttribute: w.data = QVariant(movie.hasSpecialTagEnabled(MvdMovie::LoanedTag)); l << w; break;
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
			movies.append( l.at(i).id );
	}
	else
	{
		for (int i = sz - 1; i >= 0; --i)
			movies.append( l.at(i).id );
	}
	
	q->emit_sorted();
}


/************************************************************************
 MvdCollectionModel
*************************************************************************/

/*!
	Creates a new empty model. A movie collection needs to be set for the item
	to work correctly.
*/
MvdCollectionModel::MvdCollectionModel(QObject* parent)
: QAbstractTableModel(parent), d(new MvdCollectionModel_P(this))
{
}

/*!
	Creates a new model for the specified movie collection.
*/
MvdCollectionModel::MvdCollectionModel(MvdMovieCollection* collection, QObject* parent)
: QAbstractTableModel(parent), d(new MvdCollectionModel_P(this))
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

QModelIndex MvdCollectionModel::findMovie(mvdid id) const
{
	if (id == MvdNull)
		return QModelIndex();
	
	int idx = d->movies.indexOf(id);
	return idx < 0 ? QModelIndex() : createIndex(idx, 0);
}

/*!
	Sets a movie collection for this movie.
*/
void MvdCollectionModel::setMovieCollection(MvdMovieCollection* c)
{
	emit layoutAboutToBeChanged();

	if (d->collection) {
		disconnect(this, SLOT(movieAdded(mvdid)));
		disconnect(this, SLOT(movieChanged(mvdid)));
		disconnect(this, SLOT(movieRemoved(mvdid)));
		disconnect(this, SLOT(collectionCleared()));
	}

	d->collection = c;
	d->movies.clear();

	if (c) {
		connect( c, SIGNAL(destroyed(QObject*)), this, SLOT(removeCollection()) );
		connect( c, SIGNAL(movieAdded(mvdid)), this, SLOT(movieAdded(mvdid)) );
		connect( c, SIGNAL(movieChanged(mvdid)), this, SLOT(movieChanged(mvdid)) );
		connect( c, SIGNAL(movieRemoved(mvdid)), this, SLOT(movieRemoved(mvdid)) );
		connect( c, SIGNAL(cleared()), this, SLOT(collectionCleared()) );

		// Insert existing movies - new movies will be added through 
		// the signal/slot mechanism
		QList<mvdid> list = c->movieIds();
	
		for (int i = 0; i < list.size(); ++i)
			d->movies.append(list.at(i));
	}

	emit layoutChanged();
}

//!
MvdMovieCollection* MvdCollectionModel::movieCollection() const
{
	return d->collection;
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

	if (role == (int) Movida::IdRole)
		return d->movies.at(row);

	if (role == Qt::DecorationRole && col == 0)
	{
		mvdid id = d->movies.at(row);
		if (id == 0)
			return QVariant();
		MvdMovie movie = d->collection->movie(id);
		QString poster = movie.poster();
		if (poster.isEmpty())
			return poster;
		return d->collection->metaData(MvdMovieCollection::DataPathInfo)
			.append("/images/").append(poster);
	}

	if (role != Qt::DisplayRole && role != Movida::SmartViewDisplayRole)
		return QVariant();

	if (row >= d->movies.size())
		return QVariant();

	mvdid id = d->movies.at(row);
	MvdMovie movie = d->collection->movie(id);

	switch ( (Movida::MovieAttribute) col )
	{
	case Movida::CastAttribute: return d->dataList(movie.actorIDs(), Movida::PersonRole, role);
	case Movida::ColorModeAttribute: return movie.colorModeString();
	case Movida::CountriesAttribute: return d->dataList(movie.countries(), Movida::CountryRole, role);
	case Movida::CrewAttribute: return d->dataList(movie.crewMemberIDs(), Movida::PersonRole, role);
	case Movida::DirectorsAttribute: return d->dataList(movie.directors(), Movida::PersonRole, role);
	case Movida::GenresAttribute: return d->dataList(movie.genres(), Movida::GenreRole, role);
	case Movida::ImdbIdAttribute: return movie.imdbId();
	case Movida::LanguagesAttribute: return d->dataList(movie.languages(), Movida::LanguageRole, role);
	case Movida::OriginalTitleAttribute: return movie.originalTitle();
	case Movida::ProducersAttribute: return d->dataList(movie.producers(), Movida::PersonRole, role);
	case Movida::ProductionYearAttribute: return movie.productionYear();
	case Movida::RatingAttribute: return QVariant(movie.rating());
	case Movida::ReleaseYearAttribute: return movie.releaseYear();
	case Movida::RunningTimeAttribute: return movie.runningTime() ? movie.runningTimeString() : QString();
	case Movida::StorageIdAttribute: return movie.storageId();
	case Movida::TagsAttribute: return d->dataList(movie.tags(), Movida::TagRole, role);
	case Movida::TitleAttribute: return movie.title();
	case Movida::SeenAttribute: return movie.hasSpecialTagEnabled(MvdMovie::SeenTag);
	case Movida::SpecialAttribute: return movie.hasSpecialTagEnabled(MvdMovie::SpecialTag);
	case Movida::LoanedAttribute: return movie.hasSpecialTagEnabled(MvdMovie::LoanedTag);
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
		return Movida::movieAttributeString((Movida::MovieAttribute)section);
	return QVariant();
}

/*!
	Provides the number of rows of data exposed by the model.
*/
int MvdCollectionModel::rowCount(const QModelIndex& p) const
{
	return p.isValid() ? 0 : d->movies.size();
}

/*!
	Provides the number of columns of data exposed by the model.
*/
int MvdCollectionModel::columnCount(const QModelIndex& p) const
{
	return p.isValid() ? 0 : Movida::movieAttributes(Movida::NoAttributeFilter).size();
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
void MvdCollectionModel::movieAdded(mvdid id)
{
	int row = d->movies.size();
	insertRows(row, 1);
	d->movies[row] = id;
	emit dataChanged( createIndex(row, 0), createIndex(row, columnCount() - 1) );
}

//! \internal
bool MvdCollectionModel::insertRows(int row, int count, const QModelIndex& p)
{
	beginInsertRows(p, row, row + count - 1);
	for (int i = 0; i < count; ++i)
		d->movies.insert(row + i, 0);
	endInsertRows();
	return true;
}

//! \internal
void MvdCollectionModel::movieRemoved(mvdid id)
{
	int row = d->movies.indexOf(id);
	removeRows(row, 1, QModelIndex());
}

//! \internal
bool MvdCollectionModel::removeRows(int row, int count, const QModelIndex& p)
{
	beginRemoveRows(p, row, row + count - 1);
	for (int i = 0; i < count; ++i)
		if (row < d->movies.size())
			d->movies.removeAt(row);
	endRemoveRows();
	return true;
}

//! \internal
void MvdCollectionModel::movieChanged(mvdid id)
{
	int row = d->movies.indexOf(id);
	emit dataChanged( createIndex(row, 0), createIndex(row, columnCount() - 1) );
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
	d->sort((Movida::MovieAttribute)column, order);
	emit layoutChanged();
}

//!
void MvdCollectionModel::sortByAttribute(Movida::MovieAttribute attribute, Qt::SortOrder order)
{
	if (d->movies.isEmpty())
		return;

	emit layoutAboutToBeChanged();
	d->sort(attribute, order);
	emit layoutChanged();
}

//! Returns the current sort order or Qt::AscendingOrder if the model is not sorted.
Qt::SortOrder MvdCollectionModel::sortOrder() const
{
	return d->sortOrder;
}

//! Convenience method. Returns the current sort column or the title column id if the model is not sorted.
int MvdCollectionModel::sortColumn() const
{
	return (int) d->sortAttribute;
}

//! Returns the current sort attribute or the title attribute if the model is not sorted.
Movida::MovieAttribute MvdCollectionModel::sortAttribute() const
{
	return d->sortAttribute;
}
