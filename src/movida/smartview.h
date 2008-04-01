/**************************************************************************
** Filename: smartviewwidget.h
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

#ifndef MVD_SMARTVIEW_H
#define MVD_SMARTVIEW_H

#include "listview.h"

class MvdSmartView : public MvdListView
{
	Q_OBJECT

public:
	enum ItemSize {
		SmallItemSize,
		MediumItemSize,
		LargeItemSize
	};

	MvdSmartView(QWidget* parent = 0);
	MvdSmartView(QAbstractItemModel* model, QWidget* parent = 0);
	virtual ~MvdSmartView();

	ItemSize itemSize() const;
	void setItemSize(ItemSize s);

signals:
	void contextMenuRequested(const QModelIndex& index);

protected:
	bool event(QEvent* e);
	void keyPressEvent(QKeyEvent* e);
	void contextMenuEvent(QContextMenuEvent* e);
	void startDrag(Qt::DropActions supportedActions);
	void dragEnterEvent(QDragEnterEvent* e);

private:
	void init();
};

#endif // MVD_SMARTVIEW_H
