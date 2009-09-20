/**************************************************************************
** Filename: shareddataeditor.h
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

#ifndef MVD_SHAREDDATAEDITOR_H
#define MVD_SHAREDDATAEDITOR_H

#include "ui_shareddataeditor.h"

#include <QtGui/QWidget>

class MvdTreeView;
class QAbstractItemModel;
class QContextMenuEvent;
class QModelIndex;

class MvdSharedDataEditor : public QWidget, protected Ui::MvdSharedDataEditor
{
    Q_OBJECT

public:
    MvdSharedDataEditor(QWidget *parent = 0);
    MvdSharedDataEditor(QAbstractItemModel *model, QWidget *parent = 0);
    virtual ~MvdSharedDataEditor();

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    QAbstractItemView *view() const;

signals:
    void itemActivated(int id, bool replace);
    void contextMenuRequested(const QModelIndex &index);

protected:
    void contextMenuEvent(QContextMenuEvent *event);

private slots:
    void updateDataRole();

private:
    void init();
};

#endif // MVD_SHAREDDATAEDITOR_H
