/**************************************************************************
** Filename: treewidget.h
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

#ifndef MVD_TREEWIDGET_H
#define MVD_TREEWIDGET_H

#include "guiglobal.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QList>
#include <QHash>
#include <QContextMenuEvent>
#include <QHeaderView>

class MvdTreeWidget_P;
class QPoint;
class QString;
class QKeyEvent;

class MvdTreeWidget : public QTreeWidget
{
	Q_OBJECT

public:
	enum CursorMovement {
		MoveByRow = 0x01, MoveByColumn = 0x02, MoveByIndex = MoveByRow | MoveByColumn
	};

	MvdTreeWidget(QWidget* parent = 0);
	virtual ~MvdTreeWidget();

	bool isHeaderContextMenuDisabled() const;
	void setHeaderContextMenuDisabled(bool disable);

	CursorMovement cursorMovement() const;
	void setCursorMovement(CursorMovement m);

	bool isPlaceHolder(QTreeWidgetItem* item) const;

	virtual QList<QTreeWidgetItem*> filteredSelectedItems() const;
	virtual QList<mvdid> filteredSelectedIds() const;
	virtual QList<QTreeWidgetItem*> itemsById(const QList<mvdid>& ids) const;
	
signals:
	void contextMenuRequested(QTreeWidgetItem* item, int column);

protected:
	virtual bool eventFilter(QObject* o, QEvent* e);
	virtual void showHeaderContext(const QPoint& p);
	virtual void contextMenuEvent(QContextMenuEvent* e);
	virtual void keyPressEvent(QKeyEvent* e);
	virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);

private:
	MvdTreeWidget_P* d;
};

class MvdTreeWidgetItem : public QTreeWidgetItem
{
public:
	MvdTreeWidgetItem ( int type = Type )
		: QTreeWidgetItem (type) {}
	MvdTreeWidgetItem ( const QStringList & strings, int type = Type ) 
		: QTreeWidgetItem (strings, type) {}
	MvdTreeWidgetItem ( MvdTreeWidget * parent, int type = Type ) 
		: QTreeWidgetItem (parent, type) {}
	MvdTreeWidgetItem ( MvdTreeWidget * parent, const QStringList & strings, int type = Type ) 
		: QTreeWidgetItem (parent, strings, type) {}
	MvdTreeWidgetItem ( MvdTreeWidget * parent, MvdTreeWidgetItem * preceding, int type = Type ) 
		: QTreeWidgetItem (parent, preceding, type) {}
	MvdTreeWidgetItem ( MvdTreeWidgetItem * parent, int type = Type ) 
		: QTreeWidgetItem (parent, type) {}
	MvdTreeWidgetItem ( MvdTreeWidgetItem * parent, const QStringList & strings, int type = Type ) 
		: QTreeWidgetItem (parent, strings, type) {}
	MvdTreeWidgetItem ( MvdTreeWidgetItem * parent, MvdTreeWidgetItem * preceding, int type = Type ) 
		: QTreeWidgetItem (parent, preceding, type) {}
	MvdTreeWidgetItem ( const MvdTreeWidgetItem & other ) 
		: QTreeWidgetItem (other) {}

	virtual bool operator<(const QTreeWidgetItem& other) const
	{
		if (!treeWidget())
			return QTreeWidgetItem::operator <(other);

		if (QHeaderView* h = treeWidget()->header())
		{
			bool invert = h->sortIndicatorOrder() == Qt::DescendingOrder;

			QVariant v = data(0, Movida::PlaceholderRole);
			if (!v.isNull() && v.toBool())
				return invert ? true : false;
			v = other.data(0, Movida::PlaceholderRole);
			if (!v.isNull() && v.toBool())
				return invert ? false : true;
		}

		return QTreeWidgetItem::operator <(other);
	}
};

#endif // MVD_TREEWIDGET_H
