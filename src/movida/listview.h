/**************************************************************************
** Filename: listview.h
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

#ifndef MVD_LISTVIEW_H
#define MVD_LISTVIEW_H

#include <QListView>

class QMouseEvent;

class MvdListView : public QListView
{
public:
	MvdListView(QWidget* parent = 0);
	virtual ~MvdListView();

protected:
	void mouseMoveEvent(QMouseEvent* e);
	void mouseReleaseEvent(QMouseEvent* e);
	void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command);

private:
	class MvdListView_P;
	MvdListView_P* d;
};

#endif // MVD_LISTVIEW_H
