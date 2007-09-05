/**************************************************************************
** Filename: ratingwidget.h
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

#ifndef MVD_RATINGWIDGET_H
#define MVD_RATINGWIDGET_H

#include <QWidget>
#include <QPixmap>

class MvdRatingWidget_P;
class QPaintEvent;
class QMouseEvent;

class MvdRatingWidget : public QWidget
{
	Q_OBJECT

public:
	enum PixmapRole { RatedRole, UnratedRole, HoveredRole };

	MvdRatingWidget(QWidget* parent = 0);
	virtual ~MvdRatingWidget();

	int value() const;
	int minimum() const;
	int maximum() const;

	QPixmap pixmap(PixmapRole role) const;
	void setPixmap(PixmapRole role, const QPixmap& pm);

	QSize sizeHint() const;
	QSize minimumSizeHint() const;

protected:
	virtual void paintEvent(QPaintEvent*);
	virtual void mouseMoveEvent(QMouseEvent*);
	virtual void leaveEvent(QEvent*);
	virtual void mousePressEvent(QMouseEvent*);
	virtual void mouseReleaseEvent(QMouseEvent*);

	public slots:
		void setValue(int value);
		void setMinimum(int value);
		void setMaximum(int value);

signals:
		void valueChanged(int value);
		//! Emitted when a pixmap is hovered or un-hovered (-1).
		void hovered(int value);

private:
	MvdRatingWidget_P* d;
};

#endif // MVD_RATINGWIDGET_H
