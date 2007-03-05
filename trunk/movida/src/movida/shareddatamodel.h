/**************************************************************************
** Filename: shareddatamodel.h
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

#ifndef MVD_SHAREDDATAMODEL_H
#define MVD_SHAREDDATAMODEL_H

#include "global.h"
#include <QAbstractTableModel>

class MvdSharedDataModel_P;
class MvdSharedData;
class MvdMovieCollection;

class MvdSharedDataModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	enum DataScope { GlobalScope, CollectionScope, DefaultScope };

	MvdSharedDataModel(Movida::SmdDataRole mode, QObject* parent = 0);
	MvdSharedDataModel(Movida::SmdDataRole mode, MvdMovieCollection* collection, 
		QObject* parent = 0);
	virtual ~MvdSharedDataModel();

	// Item Data Handling
	virtual Qt::ItemFlags flags(const QModelIndex& index) const;
	virtual QVariant data(const QModelIndex& index, 
		int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, 
		int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;

	// Navigation and Model Index Creation
	virtual QModelIndex index(int row, int column, 
		const QModelIndex& parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex& index) const;

	virtual void sort(int column, Qt::SortOrder order);

	virtual void setMovieCollection(MvdMovieCollection* c);

	virtual void setMode(Movida::SmdDataRole mode);
	virtual Movida::SmdDataRole mode() const;

	virtual void setFilter(DataScope filter);
	virtual DataScope filter() const;

private:
	MvdSharedDataModel_P* d;
	void setData(MvdSharedData& smd);

private slots:
	void itemAdded(int id);
	void itemRemoved(int id);
	void itemChanged(int id);
	void itemLinkChanged(int id);
	void smdCleared();
};

#endif // MVD_SHAREDDATAMODEL_H
