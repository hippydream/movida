/**************************************************************************
** Filename: shareddatamodel.h
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

#ifndef MVD_SHAREDDATAMODEL_H
#define MVD_SHAREDDATAMODEL_H

#include "mvdcore/global.h"
#include <QAbstractTableModel>

class MvdSharedDataModel_P;
class MvdSharedData;
class MvdMovieCollection;
class QMimeData;

// Movida::DataRole -> Data (e.g. Movida::PersonRole -> 3424)
typedef QHash<int, QList<mvdid> > MvdSharedDataAttributes;

class MvdSharedDataModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	MvdSharedDataModel(Movida::DataRole mode, QObject* parent = 0);
	MvdSharedDataModel(Movida::DataRole mode, MvdMovieCollection* collection, 
		QObject* parent = 0);
	virtual ~MvdSharedDataModel();

	// Item Data Handling
	virtual Qt::ItemFlags flags(const QModelIndex& index) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;

	// Navigation and Model Index Creation
	virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex& index) const;

	// Editable Items & Resizable Models
	bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
	bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());

	virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
	virtual Qt::SortOrder sortOrder() const;

	virtual void setMovieCollection(MvdMovieCollection* c);

	void setRole(Movida::DataRole role);
	Movida::DataRole role() const;

	Qt::DropActions supportedDropActions() const;
	QMimeData* mimeData(const QModelIndexList& indexes) const;

protected:
	void reset();

private:
	MvdSharedDataModel_P* d;
	void setData(MvdSharedData& smd);

private slots:
	void itemAdded(mvdid id);
	void itemRemoved(mvdid id);
	void itemUpdated(mvdid id);
	void itemReferenceChanged(mvdid id);
	void sharedDataCleared();
	void removeCollection() { setMovieCollection(0); }
};

#endif // MVD_SHAREDDATAMODEL_H
