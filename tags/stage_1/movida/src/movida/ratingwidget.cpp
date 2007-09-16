/**************************************************************************
** Filename: ratingwidget.cpp
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

#include "ratingwidget.h"

#include <QPaintEvent>
#include <QPainter>
#include <QRect>

#include <math.h>

/*!
	\class MvdRatingWidget ratingwidget.h
	\ingroup movida

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
		sizeCheckOk(true), sizeOk(true), mousePressed(false), hoveredIndex(-1),
		fullHover(true)
	{}

	inline QPixmap findValidPixmap() const
	{
		return rated.isNull() ? unrated.isNull() ? hovered.isNull() ?
			QPixmap() : hovered : unrated : rated;
	}

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

	QPixmap rated;
	QPixmap unrated;
	QPixmap hovered;

	bool sizeCheckOk;
	bool sizeOk;

	bool mousePressed;
	int hoveredIndex;

	// If true items lower than the currently hovered item are painted as "rated"
	bool fullHover;

	static const int defaultSize = 20;
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
	QSize defaultSize(MvdRatingWidget_P::defaultSize * d->maximum, MvdRatingWidget_P::defaultSize);
	setMinimumSize(defaultSize);
	setMaximumSize(defaultSize);
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

//! Returns the current pixmap for the specific role.
QPixmap MvdRatingWidget::pixmap(PixmapRole role) const
{
	return (role == HoveredRole) ? d->hovered :
		(role == RatedRole) ? d->rated :
		(role == UnratedRole) ? d->unrated : QPixmap();
}

/*!
	Sets a pixmap for a specific role.
	Note that the set pixmaps should have the same size or they won't be
	rendered. Mixed size pixmaps may be supported in a future version.
	If a pixmap has not been set for a specific role when the label is rendered,
	it will be replaced by the next valid pixmap (if any).
*/
void MvdRatingWidget::setPixmap(PixmapRole role, const QPixmap& pm)
{
	switch (role)
	{
	case RatedRole: d->rated = pm; break;
	case UnratedRole: d->unrated = pm; break;
	case HoveredRole: d->hovered = pm; break;
	default: ;
	}

	d->sizeCheckOk = false;

	QSize sz = sizeHint();
	setMinimumSize(sz);
	setMaximumSize(sz);
	QWidget::updateGeometry();
}

//! \internal Returns the size hint for this widget.
QSize MvdRatingWidget::sizeHint() const
{
	QPixmap pm = d->findValidPixmap();
	int w = pm.isNull() ? MvdRatingWidget_P::defaultSize : pm.width();
	int h = pm.isNull() ? MvdRatingWidget_P::defaultSize : pm.height();

	return QSize(w * d->maximum, h);
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

	QPoint p = this->mapFromGlobal(QCursor::pos());
	QSize sz = sizeHint();
	d->updateHoveredIndex(p, sz);

	if (oldHovered != d->hoveredIndex)
	{
		repaint();
		emit hovered(d->hoveredIndex < 0 ? -1 : d->hoveredIndex + 1);
	}
}

//! \internal
void MvdRatingWidget::leaveEvent(QEvent* e)
{
	Q_UNUSED(e);

	int oldHovered = d->hoveredIndex;
	d->hoveredIndex = -1;

	repaint();

	if (oldHovered != d->hoveredIndex)
		emit hovered(-1);
}

//! \internal
void MvdRatingWidget::mousePressEvent(QMouseEvent* e)
{
	if ((e->button() & Qt::LeftButton) == 0)
	{
		QWidget::mousePressEvent(e);
		return;
	}

	QPoint p = this->mapFromGlobal(QCursor::pos());
	QSize sz = sizeHint();
	d->updateHoveredIndex(p, sz);

	if (d->hoveredIndex >= 0)
	{
		d->value = d->minimum + d->hoveredIndex + 1;
		d->mousePressed = true;
		repaint();
	}
}

//! \internal
void MvdRatingWidget::mouseReleaseEvent(QMouseEvent* e)
{
	Q_UNUSED(e);
	d->mousePressed = false;
}

//! \internal
void MvdRatingWidget::paintEvent(QPaintEvent* e)
{
	Q_UNUSED(e);

	QPixmap validPm = d->findValidPixmap();
	QPainter p(this);

	if (validPm.isNull())
	{
		p.drawText(rect(), tr("Rating: %1").arg(d->value));
		p.end();
		return;
	}

	if (!d->sizeCheckOk)
	{
		if (d->rated.isNull())
		{
			if (d->unrated.isNull() || d->hovered.isNull())
				d->sizeOk = true; // Only one valid pixmap
			else d->sizeOk = d->unrated.size() == d->hovered.size();
		}
		else if (d->unrated.isNull())
		{
			if (d->rated.isNull() || d->hovered.isNull())
				d->sizeOk = true; // Only one valid pixmap
			else d->sizeOk = d->rated.size() == d->hovered.size();
		}
		else if (d->hovered.isNull())
		{
			if (d->rated.isNull() || d->unrated.isNull())
				d->sizeOk = true; // Only one valid pixmap
			else d->sizeOk = d->rated.size() == d->unrated.size();
		}
		else
		{
			d->sizeOk = (d->rated.size() == d->unrated.size() && d->rated.size() == d->hovered.size());
		}

		d->sizeCheckOk = true;
	}

	if (!d->sizeOk)
	{
		p.drawText(rect(), tr("Rating: %1").arg(d->value));
		p.end();
		return;
	}

	for (int i = 0; i < d->maximum - d->minimum; ++i)
	{
		if (d->hoveredIndex == i && !d->mousePressed)
		{
			// Hovered
			QPixmap& pm = d->hovered.isNull() ? validPm : d->hovered;
			p.drawPixmap(pm.width() * i, 0, pm);
		}
		else if (i < d->value || (d->fullHover && i < d->hoveredIndex))
		{
			// Rated
			QPixmap& pm = d->rated.isNull() ? d->hovered.isNull() ? d->unrated : d->hovered : d->rated;
			p.drawPixmap(pm.width() * i, 0, pm);
		}
		else
		{
			// Unrated
			QPixmap& pm = d->unrated.isNull() ? d->rated.isNull() ? d->hovered : d->rated : d->unrated;
			p.drawPixmap(pm.width() * i, 0, pm);
		}
	}

	p.end();
}
