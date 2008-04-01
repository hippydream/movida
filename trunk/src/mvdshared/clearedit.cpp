/**************************************************************************
** Filename: clearedit.cpp
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

#include "clearedit.h"
#include <QToolButton>
#include <QStyle>
#include <QPainter>
#include <QStyleOptionFrameV2>

#define verticalMargin 1
#define horizontalMargin 2

class MvdClearEdit::Private
{
public:
	Private() : clearButton(0) {}

	QToolButton* clearButton;
	QString placeHolder;
};

MvdClearEdit::MvdClearEdit(QWidget* parent)
: QLineEdit(parent), d(new Private)
{
	QPixmap pixmap(":/images/clear-edit.png");

	d->clearButton = new QToolButton(this);
	d->clearButton->setToolTip(tr("Click to clear the text."));
	d->clearButton->setIcon(QIcon(pixmap));
	d->clearButton->setIconSize(pixmap.size());
	d->clearButton->setCursor(Qt::PointingHandCursor);
	d->clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
	d->clearButton->hide();
	connect(d->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

	connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateClearButton(const QString&)));
	
	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(d->clearButton->sizeHint().width() + frameWidth + 1));
	
	QSize msz = minimumSizeHint();
	setMinimumSize(qMax(msz.width(), d->clearButton->sizeHint().height() + frameWidth * 2 + 2),
		qMax(msz.height(), d->clearButton->sizeHint().height() + frameWidth * 2 + 2));
}

void MvdClearEdit::resizeEvent(QResizeEvent*)
{
	QSize sz = d->clearButton->sizeHint();
	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	d->clearButton->move(rect().right() - frameWidth - sz.width(),
		(rect().bottom() + 1 - sz.height())/2);
}

void MvdClearEdit::updateClearButton(const QString& text)
{
	d->clearButton->setVisible(!text.isEmpty());
}

//! Sets a string to be displayed as place holder when the widget contains no text.
void MvdClearEdit::setPlaceHolder(const QString& s)
{
	d->placeHolder = s.trimmed();
}

//! Returns the current place holder string, if any.
QString MvdClearEdit::placeHolder() const
{
	return d->placeHolder;
}

void MvdClearEdit::paintEvent(QPaintEvent* e)
{
	QLineEdit::paintEvent(e);
	if (d->placeHolder.isEmpty() || !text().isEmpty() || hasFocus())
		return;

	QPainter p(this);

	QRect r;
	const QPalette& pal = palette();

	QStyleOptionFrameV2 panel;
	initStyleOption(&panel);
	r = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
	p.setClipRect(r);

	int vscroll;
	QFontMetrics fm = fontMetrics();
	Qt::Alignment va = QStyle::visualAlignment(layoutDirection(), QFlag(alignment()));
	switch (va & Qt::AlignVertical_Mask) {
	case Qt::AlignBottom:
		vscroll = r.y() + r.height() - fm.height() - verticalMargin;
		break;
	case Qt::AlignTop:
		vscroll = r.y() + verticalMargin;
		break;
	default:
		//center
		vscroll = r.y() + (r.height() - fm.height() + 1) / 2;
		break;
	}

	QRect lineRect(r.x() + horizontalMargin * 3, vscroll, r.width() - 2*horizontalMargin, fm.height());

	p.setPen(pal.color(QPalette::Disabled, QPalette::Text));
	p.drawText(lineRect, d->placeHolder);
}
