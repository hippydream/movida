/**************************************************************************
** Filename: shareddataview.h
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

#ifndef MVD_SHAREDDATAVIEW_H
#define MVD_SHAREDDATAVIEW_H

#include "treeview.h"

class MvdSharedDataView : public MvdTreeView
{
    Q_OBJECT

public:
    MvdSharedDataView(QWidget *parent = 0);
    virtual ~MvdSharedDataView();

signals:
    void itemActivated(int id, bool replace);

protected:
    virtual void startDrag(Qt::DropActions supportedActions);

protected slots:
    virtual void onItemActivated(const QModelIndex &index);
};

#endif // MVD_SHAREDDATAVIEW_H
