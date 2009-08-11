/**************************************************************************
** Filename: treeview.h
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

#ifndef MVD_TREEVIEW_H
#define MVD_TREEVIEW_H

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QTreeView>

class QKeyEvent;
class QPoint;
class QString;

class MvdTreeView : public QTreeView
{
    Q_OBJECT

public:
    MvdTreeView(QWidget *parent = 0);
    virtual ~MvdTreeView();

    bool isHeaderContextMenuDisabled() const;
    void setHeaderContextMenuDisabled(bool disable);

    void setModel(QAbstractItemModel *model);

    bool hasSelectedRows() const;
    QModelIndexList selectedRows() const;

    QModelIndex selectedIndex() const;
    QModelIndexList selectedIndexes() const;

    void selectIndex(const QModelIndex &idx);

public slots:
signals:
    void contextMenuRequested(const QModelIndex &index, QContextMenuEvent::Reason);
    void itemSelectionChanged();

protected:
    virtual void showHeaderContext(const QPoint &p);

    bool eventFilter(QObject *o, QEvent *e);
    void contextMenuEvent(QContextMenuEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void dragEnterEvent(QDragEnterEvent *e);
    void dragLeaveEvent(QDragLeaveEvent *e);
    void dragMoveEvent(QDragMoveEvent *e);
    void dropEvent(QDropEvent *e);

private:
    bool mDisableHeaderContextMenu;
};

#endif // MVD_TREEVIEW_H
