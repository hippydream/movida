/**************************************************************************
** Filename: clearspin.cpp
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

#include "clearspin.h"
#include <QApplication>
#include <QToolButton>
#include <QStyle>
#include <QPainter>
#include <QStyleOptionFrameV2>
#include <math.h>

namespace {
    //! Check out style issues with Oxygen
    static int spacing() {
        QStyle* s = QApplication::style();
        if (s->inherits("OxygenStyle"))
            return 3;
        return 2;
    }
};

class MvdClearSpin::Private
{
public:
        Private() : clearButton(0) {}

        QToolButton* clearButton;
        QSize pixmapSize;
};

MvdClearSpin::MvdClearSpin(QWidget* parent)
: QSpinBox(parent), d(new Private)
{
        QPixmap pixmap(":/images/clear-edit.png");
        d->pixmapSize = pixmap.size();

        d->clearButton = new QToolButton(this);
        d->clearButton->setToolTip(tr("Click to clear the text."));
        d->clearButton->setIcon(QIcon(pixmap));
        d->clearButton->setIconSize(pixmap.size());
        d->clearButton->setCursor(Qt::PointingHandCursor);
        d->clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
        d->clearButton->setFocusPolicy(Qt::NoFocus);
        d->clearButton->hide();
        connect(d->clearButton, SIGNAL(clicked()), this, SLOT(clearButtonClicked()));

        connect(this, SIGNAL(valueChanged(const QString&)), this, SLOT(updateClearButton(const QString&)));

        setStyleSheet(QString("QSpinBox { padding-right: %1; }").arg(d->pixmapSize.width() + 2 * ::spacing() + 1));
}

QSize MvdClearSpin::sizeHint() const
{
        QSize sz = QSpinBox::sizeHint();
//        sz.rwidth() += d->pixmapSize.width() + 2 * ::spacing();
//        sz.setHeight(qMax(sz.height(), d->pixmapSize.height()));
        return sz;
}

void MvdClearSpin::resizeEvent(QResizeEvent* e)
{
        QSpinBox::resizeEvent(e);

        QStyleOptionSpinBox opt;
        initStyleOption(&opt);

        QRect r = style()->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxEditField, this);
        d->clearButton->move(
                r.right(),
                (int)ceil((r.height() - d->pixmapSize.height()) / 2.0) + ::spacing()
        );
}

void MvdClearSpin::paintEvent(QPaintEvent* e)
{
        QSpinBox::paintEvent(e);
}

void MvdClearSpin::updateClearButton(const QString& text)
{
        d->clearButton->setVisible(!text.isEmpty() && text != specialValueText());
}

void MvdClearSpin::clearButtonClicked()
{
        setValue(minimum());
}

void MvdClearSpin::showEvent(QShowEvent* e)
{
        QSpinBox::showEvent(e);
        updateClearButton(text());
}
