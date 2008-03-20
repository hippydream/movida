/**************************************************************************
** Filename: collectionmodel.h
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

#ifndef MVD_COLLECTIONMODEL_H
#define MVD_COLLECTIONMODEL_H

#include "guiglobal.h"
#include "mvdcore/global.h"
#include <QAbstractTableModel>

class MvdCollectionModel_P;
class MvdMovieCollection;

class MvdCollectionModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	MvdCollectionModel(QObject* parent = 0);
	MvdCollectionModel(MvdMovieCollection* collection, QObject* parent = 0);
	virtual ~MvdCollectionModel();

	QModelIndex findMovie(mvdid id) const;

	// Item Data Handling
	virtual Qt::ItemFlags flags(const QModelIndex& index) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, 
		int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;

	// Navigation and Model Index Creation
	virtual QModelIndex index(int row, int column,
		const QModelIndex& parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex& index) const;

	// Editable Items & Resizable Models
	bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
	bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());

	virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
	virtual void sortByAttribute(Movida::MovieAttribute attribute, Qt::SortOrder order);

	virtual void setMovieCollection(MvdMovieCollection* c);
	virtual MvdMovieCollection* movieCollection() const;
	
	virtual Qt::SortOrder sortOrder() const;
	virtual int sortColumn() const;
	virtual Movida::MovieAttribute sortAttribute() const;

	void emit_sorted() { emit sorted(); }
	
signals:
	void sorted();

private:
	MvdCollectionModel_P* d;

private slots:
	void movieAdded(mvdid);
	void movieRemoved(mvdid);
	void movieChanged(mvdid);
	void collectionCleared();
	void removeCollection() { setMovieCollection(0); }
};

#endif // MVD_COLLECTIONMODEL_H
