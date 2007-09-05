/**************************************************************************
** Filename: dockwidget.h
** Revision: 3
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

#ifndef MVD_DOCKWIDGET_H
#define MVD_DOCKWIDGET_H

#include <QDockWidget>

class QHideEvent;
class QShowEvent;

class MvdDockWidget : public QDockWidget {
	Q_OBJECT
		
public:
	MvdDockWidget(const QString & title, QWidget * parent = 0)
	: QDockWidget(title, parent) {
	}
	
	MvdDockWidget(QWidget * parent = 0)
	: QDockWidget(parent) {
	}
	
protected:
	virtual void hideEvent(QHideEvent* event)
	{
		emit toggled(false);
		QDockWidget::hideEvent(event);
	}
	
	virtual void showEvent(QShowEvent* event)
	{
		emit toggled(true);
		QDockWidget::showEvent(event);
	}
	
	
signals:
	void toggled(bool visible);
};

#endif // MVD_DOCKWIDGET_H
