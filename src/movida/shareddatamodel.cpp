/**************************************************************************
** Filename: shareddatamodel.cpp
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

#include "shareddatamodel.h"
#include "guiglobal.h"
#include "mvdcore/settings.h"
#include "mvdcore/moviecollection.h"
#include "mvdcore/shareddata.h"
#include "mvdcore/sditem.h"
#include <QCoreApplication>

/*!
	\class MvdSharedDataModel shareddatamodel.h
	\ingroup Movida

	\brief Model for a movie collection's shared data.
*/

/************************************************************************
MvdSharedDataModel_P
*************************************************************************/

//! \internal
class MvdSharedDataModel_P
{
public:
	struct SortWrapper
	{
		SortWrapper(int id = -1) : dataid(id) {}

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

	struct RowWrapper {
		RowWrapper(mvdid aid = MvdNull) : id(aid) {}

		mvdid id;
	};

	MvdSharedDataModel_P(Movida::DataRole r) : collection(0), role(r) {}

	void populate();
	int rowIndex(mvdid id) const;

	MvdMovieCollection* collection;
	QList<RowWrapper> rows;
	Movida::DataRole role;
};

//! \internal Fills the internal id list.
void MvdSharedDataModel_P::populate()
{
	if (!collection)
		return;

	MvdSharedData::ItemList data = collection->smd().items(role);
	for (MvdSharedData::ItemList::ConstIterator it = data.constBegin(); it != data.constEnd(); ++it)
	{
		mvdid id = it.key();
		RowWrapper rw(id);
		rows.append(rw);
	}
}

int MvdSharedDataModel_P::rowIndex(mvdid id) const
{
	for (int i = 0; i < rows.size(); ++i)
		if (rows.at(i).id == id)
			return i;
	return -1;
}

/************************************************************************
MvdSharedDataModel
*************************************************************************/

/*!
	Creates a new empty model. A movie collection needs to be set for the item
	to work correctly.
*/
MvdSharedDataModel::MvdSharedDataModel(Movida::DataRole role, QObject* parent)
: QAbstractTableModel(parent), d(new MvdSharedDataModel_P(role))
{

}

/*!
	Creates a new model for the specified movie collection.
*/
MvdSharedDataModel::MvdSharedDataModel(Movida::DataRole role, 
	MvdMovieCollection* collection, QObject* parent)
: QAbstractTableModel(parent), d(new MvdSharedDataModel_P(role))
{
	setMovieCollection(collection);
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
		disconnect(d->collection, 0, this, 0);

	d->collection = c;
	d->rows.clear();

	if (c)
	{
		MvdSharedData* sd = &(c->smd());
		connect( sd, SIGNAL(itemAdded(int)), this, SLOT(itemAdded(int)) );
		connect( sd, SIGNAL(itemRemoved(int)), this, SLOT(itemRemoved(int)) );
		connect( sd, SIGNAL(itemUpdated(int)), this, SLOT(itemUpdated(int)) );
		connect( sd, SIGNAL(itemReferenceChanged(int)), this, SLOT(itemReferenceChanged(int)) );
		connect( sd, SIGNAL(cleared()), this, SLOT(sharedDataCleared()) );

		// Insert existing data - new items will be added through the signal/slot mechanism
		d->populate();
	}

	reset();
}

/*!
	Sets a new role for this model. The role determines what kind of 
	SD data is displayed by the model.
*/
void MvdSharedDataModel::setRole(Movida::DataRole role)
{
	if (d->role == role)
		return;

	d->role = role;
	reset();
}

/*!
	Returns the current role (i.e. tells what kind of SD data is currently
	being displayed by the model.
*/
Movida::DataRole MvdSharedDataModel::role() const
{
	return d->role;
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
	if (!d->collection || role != Qt::DisplayRole)
		return QVariant();

	int row = index.row();
	int col = index.column();

	if (row < 0 || row >= d->rows.size())
		return QVariant();

	QList<Movida::SharedDataAttribute> columns = Movida::sharedDataAttributes(d->role);
	if (col < 0 || col >= columns.size())
		return QVariant();

	const MvdSharedDataModel_P::RowWrapper& rw = d->rows.at(row);
	MvdSdItem sdItem = d->collection->smd().item(rw.id);

	// NameSDA, GenreSDA, CountrySDA, LanguageSDA, TagSDA, [UrlSDA, DescriptionSDA,] ImdbIdSDA
	switch (columns.at(col))
	{
	case Movida::NameSDA:
	case Movida::GenreSDA:
	case Movida::CountrySDA:
	case Movida::LanguageSDA:
	case Movida::TagSDA:
		return sdItem.value;
	case Movida::ImdbIdSDA:
		return sdItem.id;
	default: ;
	}

	return QVariant();
}

/*!
	Provides views with information to show in their headers. The information is only retrieved by views
	that can display header information.
*/
QVariant MvdSharedDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(orientation);

	QList<Movida::SharedDataAttribute> columns = Movida::sharedDataAttributes(d->role);
	if (section >= 0 && section < columns.size() && role == Qt::DisplayRole)
		return Movida::sharedDataAttributeString(columns.at(section));

	return QVariant();
}

/*!
	Provides the number of rows of data exposed by the model.
*/
int MvdSharedDataModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;

	return d->rows.count();
}

/*!
	Provides the number of columns of data exposed by the model.
*/
int MvdSharedDataModel::columnCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;

	return Movida::sharedDataAttributes(d->role).count();
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
	if (parent.isValid() || !d->collection)
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
	d->rows.append(MvdSharedDataModel_P::RowWrapper(id));
	insertRows(d->rows.size() - 1, 1, QModelIndex());
}

//! \internal
void MvdSharedDataModel::itemRemoved(int id)
{
	int pos = d->rowIndex(id);
	if (pos >= 0)
	{
		d->rows.removeAt(pos);
		removeRows(pos, 1, QModelIndex());
	}
}

//! \internal
void MvdSharedDataModel::itemUpdated(int id)
{
	int pos = d->rowIndex(id);
	if (pos >= 0)
	{
		emit dataChanged( createIndex(pos, 0), createIndex(pos, columnCount()) );
	}
}

//! \internal
void MvdSharedDataModel::itemReferenceChanged(int id)
{
	Q_UNUSED(id);
}

//! \internal
void MvdSharedDataModel::sharedDataCleared()
{
	d->rows.clear();
	reset();
}

//! \internal
void MvdSharedDataModel::sort(int column, Qt::SortOrder order)
{
	if (rowCount() == 0)
		return;

	Q_ASSERT(d->collection);

	QList<MvdSharedDataModel_P::SortWrapper> l;

	for (int i = 0; i < rowCount(); ++i)
	{
		int id = d->rows.at(i).id;

		MvdSharedDataModel_P::SortWrapper w(id);
		w.data = data(index(i, column, QModelIndex()));
		l << w;
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

	d->rows.clear();

	int sz = l.size();

	if (order == Qt::AscendingOrder)
	{	
		for (int i = 0; i < sz; ++i)
			d->rows.append(MvdSharedDataModel_P::RowWrapper(l.at(i).dataid));
	}
	else
	{
		for (int i = sz - 1; i >= 0; --i)
			d->rows.append(MvdSharedDataModel_P::RowWrapper(l.at(i).dataid));
	}

	emit layoutChanged();
}

