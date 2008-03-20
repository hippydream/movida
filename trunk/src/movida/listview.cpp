/**************************************************************************
** Filename: listview.cpp
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

#include "listview.h"
#include "smartviewdelegate.h"
#include <QMouseEvent>
#include <QApplication>
#include <QScrollBar>

class MvdListView::MvdListView_P
{
public:
	QPersistentModelIndex lastDirtyIndex;
};

MvdListView::MvdListView(QWidget* parent)
: QListView(parent), d(new MvdListView_P)
{
	setSpacing(10);
}

MvdListView::~MvdListView()
{

}

void MvdListView::mouseMoveEvent(QMouseEvent* e)
{
	if (hasMouseTracking() && !QApplication::mouseButtons()) {
		QModelIndex index = indexAt(e->pos());
		if (index.isValid()) {
			if (d->lastDirtyIndex.isValid())
				setDirtyRegion(visualRect(d->lastDirtyIndex));
			setDirtyRegion(visualRect(index));
			repaint(visualRect(index));
			d->lastDirtyIndex = index;

			// Show some hint
			MvdSmartViewDelegate* d = qobject_cast<MvdSmartViewDelegate*>(itemDelegate(index));
			if (d) d->showHoveredControlHint();
		}
		else if (d->lastDirtyIndex.isValid()) {
			setDirtyRegion(visualRect(d->lastDirtyIndex));
			repaint(visualRect(index));
			d->lastDirtyIndex = QPersistentModelIndex();
		}
	}
	QListView::mouseMoveEvent(e);
}

void MvdListView::mouseReleaseEvent(QMouseEvent* e)
{
	QModelIndex index = indexAt(e->pos());
	if (index.isValid()) {
		MvdSmartViewDelegate* d = qobject_cast<MvdSmartViewDelegate*>(itemDelegate(index));
		if (d) d->mousePressed(visualRect(index), index);
	
	}
	QListView::mouseReleaseEvent(e);
}

void MvdListView::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command)
{
	if (!selectionModel())
		return;

	// FIX: QListView won't clear the selection after a click outside of the contents area when 
	// wrapping is enabled.
	if (command == QItemSelectionModel::Clear && isWrapping() && !QRect(QPoint(0, 0), contentsSize()).intersects(rect)) {
		selectionModel()->clear();
		return;
	}
	
	QListView::setSelection(rect, command);
}
