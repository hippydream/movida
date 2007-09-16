/**************************************************************************
** Filename: treewidget.h
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

#ifndef MVD_TREEWIDGET_H
#define MVD_TREEWIDGET_H

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QList>
#include <QHash>
#include <QContextMenuEvent>

class MvdTreeWidget_P;
class QPoint;
class QString;

class MvdTreeWidget : public QTreeWidget
{
	Q_OBJECT
  
public:
	MvdTreeWidget(QWidget* parent = 0);
	virtual ~MvdTreeWidget();

	bool isHeaderContextMenuDisabled() const;
	void setHeaderContextMenuDisabled(bool disable);

signals:
	void contextMenuRequested(QTreeWidgetItem* item, int column);

protected:
	virtual bool eventFilter(QObject* o, QEvent* e);
	virtual void showHeaderContext(const QPoint& p);
	virtual void contextMenuEvent(QContextMenuEvent* e);

private:
	MvdTreeWidget_P* d;
};

#endif // MVD_TREEWIDGET_H
