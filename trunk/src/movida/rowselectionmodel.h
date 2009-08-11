/**************************************************************************
** Filename: rowselectionmodel.h
** Review: 3
**
** Copyright (C) 2007-2009 Angius Fabrizio. All rights reserved.
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

#ifndef MVD_ROWSELECTIONMODEL_H
#define MVD_ROWSELECTIONMODEL_H

#include <QtGui/QItemSelectionModel>

class MvdRowSelectionModel : public QItemSelectionModel
{
    Q_OBJECT

public:
    MvdRowSelectionModel(QAbstractItemModel *model) :
        QItemSelectionModel(model)
    { }

    MvdRowSelectionModel(QAbstractItemModel *model, QObject *parent) :
        QItemSelectionModel(model, parent)
    { }

public slots:
    virtual void select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
    {
        QItemSelectionModel::select(index, command | QItemSelectionModel::Rows);
    }

    virtual void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command)
    {
        QItemSelectionModel::select(selection, command | QItemSelectionModel::Rows);
    }

};

#endif // MVD_ROWSELECTIONMODEL_H
