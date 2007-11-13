/**************************************************************************
** Filename: clearedit.cpp
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

#include "clearedit.h"
#include <QToolButton>
#include <QStyle>

MvdClearEdit::MvdClearEdit(QWidget* parent)
: QLineEdit(parent)
{
	QPixmap pixmap(":/images/misc/clear-edit");

	clearButton = new QToolButton(this);
	clearButton->setToolTip(tr("Click to clear the text."));
	clearButton->setIcon(QIcon(pixmap));
	clearButton->setIconSize(pixmap.size());
	clearButton->setCursor(Qt::PointingHandCursor);
	clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
	clearButton->hide();
	connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));

	connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateClearButton(const QString&)));
	
	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(clearButton->sizeHint().width() + frameWidth + 1));
	
	QSize msz = minimumSizeHint();
	setMinimumSize(qMax(msz.width(), clearButton->sizeHint().height() + frameWidth * 2 + 2),
		qMax(msz.height(), clearButton->sizeHint().height() + frameWidth * 2 + 2));
}

void MvdClearEdit::resizeEvent(QResizeEvent*)
{
	QSize sz = clearButton->sizeHint();
	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	clearButton->move(rect().right() - frameWidth - sz.width(),
		(rect().bottom() + 1 - sz.height())/2);
}

void MvdClearEdit::updateClearButton(const QString& text)
{
	clearButton->setVisible(!text.isEmpty());
}
