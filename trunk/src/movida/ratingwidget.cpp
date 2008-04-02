/**************************************************************************
** Filename: ratingwidget.cpp
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

#include "ratingwidget.h"
#include <QPaintEvent>
#include <QPainter>
#include <QIcon>
#include <math.h>

/*!
	\class MvdRatingWidget ratingwidget.h
	\\ingroup Movida

	\brief QWidget subclass, used to draw a 'rating'-like image sequence.
*/

/************************************************************************
MvdRatingWidget_P
*************************************************************************/

//! \internal
class MvdRatingWidget_P
{
public:
	MvdRatingWidget_P() : value(0), minimum(0), maximum(10),
		size(QSize(22, 22)),
		mousePressed(false), hoveredIndex(-1), active(false)
	{}

	//! \internal Returns the index (starting from 0) of the highlighted pixmap or -1.
	inline void updateHoveredIndex(const QPoint& p, const QSize& sz)
	{
		int idx = -1;

		if (p.x() >= 0 &&
			p.y() >= 0 &&
			p.x() < sz.width() &&
			p.y() < sz.height())
		{
			int itemWidth = sz.width() / maximum;
			idx = (int) floor((double)p.x() / itemWidth);
		}

		hoveredIndex = idx;
	}

	int value;
	int minimum;
	int maximum;

	QIcon icon;
	QSize size;

	bool mousePressed;
	int hoveredIndex;
	bool active;
};


/************************************************************************
MvdRatingWidget
*************************************************************************/

/*!
	Builds a new rating label with a default star-like pixmap.
*/
MvdRatingWidget::MvdRatingWidget(QWidget* parent)
: QWidget(parent), d(new MvdRatingWidget_P)
{
	setFixedSize(d->size);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setMouseTracking(true);
}

//! Destructor.
MvdRatingWidget::~MvdRatingWidget()
{
	delete d;
}

//! Returns the current rating.
int MvdRatingWidget::value() const
{
	return d->value;
}

/*!
	Sets the current value (if it is not lower than minimum() or higher than maximum())
	and updates the label.
*/
void MvdRatingWidget::setValue(int value)
{
	if (value < d->minimum || value > d->maximum)
		return;

	d->value = value;

	update();

	emit valueChanged(value);
}

//! Returns the current minimum value. Remember that the minimum is used for a missing rating.
int MvdRatingWidget::minimum() const
{
	return d->minimum;
}

/*!
	Sets the current minimum value (if it is a positive and lower that maximum() value).
	Remember that the minimum is used for a missing rating.
*/
void MvdRatingWidget::setMinimum(int value)
{
	if (value < 0 || value > d->maximum)
		return;

	d->minimum = value;

	if (d->value < value)
		setValue(value);
}

//! Returns the current maximum value.
int MvdRatingWidget::maximum() const
{
	return d->maximum;
}

/*!
	Sets the new maximum value (if it is a positive and lower than minimum() value).
*/
void MvdRatingWidget::setMaximum(int value)
{
	if (value < d->minimum || value == d->maximum)
		return;

	QWidget::updateGeometry();

	d->maximum = value;

	if (d->value > value)
		setValue(value);

	QSize sz = sizeHint();
	setMinimumSize(sz);
	setMaximumSize(sz);
}

//! Returns the icon used for the rating.
QIcon MvdRatingWidget::icon() const
{
	return d->icon;
}

/*!
	Sets an icon to be used for the rating.
	The QIcon::Selected mode is used when the mouse is hovered, and the QIcon::Disabled
	state for values > than the set rating.
*/
void MvdRatingWidget::setIcon(const QIcon& i, QSize size)
{
	d->icon = i;
	d->size = size.isValid() ? size : QSize(22, 22);
	setFixedSize(d->size);
	QWidget::updateGeometry();
}

//! \internal Returns the size hint for this widget.
QSize MvdRatingWidget::sizeHint() const
{
	return QSize(d->size.width() * d->maximum, d->size.height());
}

//! Returns the minimum size hint for this widget.
QSize MvdRatingWidget::minimumSizeHint() const
{
	return sizeHint();
}

//! \internal
void MvdRatingWidget::mouseMoveEvent(QMouseEvent* e)
{
	Q_UNUSED(e);

	int oldHovered = d->hoveredIndex;
	d->active = false;

	QPoint p = this->mapFromGlobal(QCursor::pos());
	QSize sz = sizeHint();
	d->updateHoveredIndex(p, sz);

	if (oldHovered != d->hoveredIndex) {
		update();
		emit hovered(d->hoveredIndex < 0 ? -1 : d->hoveredIndex + 1);
	}
}

//! \internal
void MvdRatingWidget::leaveEvent(QEvent* e)
{
	Q_UNUSED(e);

	int oldHovered = d->hoveredIndex;
	d->hoveredIndex = -1;
	d->active = false;

	update();

	if (oldHovered != d->hoveredIndex)
		emit hovered(-1);
}

//! \internal
void MvdRatingWidget::mousePressEvent(QMouseEvent* e)
{
	if ((e->button() & Qt::LeftButton) == 0) {
		QWidget::mousePressEvent(e);
		return;
	}

	QPoint p = this->mapFromGlobal(QCursor::pos());
	QSize sz = sizeHint();
	d->updateHoveredIndex(p, sz);

	if (d->hoveredIndex >= 0) {
		d->value = d->minimum + d->hoveredIndex + 1;
		d->mousePressed = true;
		d->active = true;
		update();
		emit valueChanged(d->value);
	}
}

//! \internal
void MvdRatingWidget::mouseReleaseEvent(QMouseEvent* e)
{
	Q_UNUSED(e);
	d->mousePressed = false;
	update();
}

//! \internal
void MvdRatingWidget::paintEvent(QPaintEvent* e)
{
	Q_UNUSED(e);
	QPainter p(this);

	if (d->icon.isNull()) {
		p.drawText(rect(), tr("Rating: %1").arg(d->value));
		p.end();
		return;
	}

	for (int i = 0; i < d->maximum - d->minimum; ++i) {
		if (d->hoveredIndex >= 0 && !d->active) {
			// Highlight items up to the hovered index
			if (i <= d->hoveredIndex)
				p.drawPixmap(d->size.width() * i, 0, d->icon.pixmap(d->size, QIcon::Selected));
			else p.drawPixmap(d->size.width() * i, 0, d->icon.pixmap(d->size, QIcon::Disabled));
		} else {
			// Draw normal rating
			if (i < d->value)
				p.drawPixmap(d->size.width() * i, 0, d->icon.pixmap(d->size, QIcon::Normal));
			else p.drawPixmap(d->size.width() * i, 0, d->icon.pixmap(d->size, QIcon::Disabled));
		}
	}

	p.end();
}
