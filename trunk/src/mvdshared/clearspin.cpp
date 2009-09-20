/**************************************************************************
** Filename: clearspin.cpp
**
** Copyright (C) 2007-2009 Angius Fabrizio. All rights reserved.
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

#include "lineedit.h"

#include <QtGui/QApplication>
#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtGui/QStyleOptionFrameV2>
#include <QtGui/QToolButton>

#include <cmath>

namespace {
//! Check out style issues with Oxygen
static int spacing()
{
    QStyle *s = QApplication::style();

    if (s->inherits("OxygenStyle"))
        return 3;
    return 2;
}

}

class MvdClearSpin::Private
{
public:
    Private() :
        clearButton(0) { }

    QToolButton *clearButton;
    QSize pixmapSize;

    QSize cachedSizeHint;
    QString cachedSpecialText;
};

MvdClearSpin::MvdClearSpin(QWidget *parent) :
    QSpinBox(parent),
    d(new Private)
{
    setLineEdit(new MvdLineEdit(this));

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

    connect(this, SIGNAL(valueChanged(const QString &)), this, SLOT(updateClearButton(const QString &)));

    setStyleSheet(QString("QSpinBox { padding-right: %1; }").arg(d->pixmapSize.width() + 2 * ::spacing() + 1));
}

void MvdClearSpin::setRange(int min, int max)
{
    d->cachedSizeHint = QSize();
    QSpinBox::setRange(min, max);
}

bool MvdClearSpin::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::FontChange:
    case QEvent::StyleChange:
    d->cachedSizeHint = QSize();
    break;
    default: ;
    }

    return QSpinBox::event(event);
}

QSize MvdClearSpin::sizeHint() const
{
    // Same as QAbstractSpinBox::sizeHint() but using QApplication's QStyle
    // to avoid issues with the style sheet style (it will often return a 
    // smaller size)

    // setSpecialValueText() is not virtual so we use a trick to reset
    // the cached size hint when the specialValueText has been changed
    if (d->cachedSizeHint.isEmpty() || d->cachedSpecialText != specialValueText()) {
        
        ensurePolished();

        const QFontMetrics fm(fontMetrics());
        int h = lineEdit()->sizeHint().height();
        int w = 0;
        QString s;
        s = prefix() + textFromValue(minimum()) + suffix() + QLatin1Char(' ');
        s.truncate(18);
        w = qMax(w, fm.width(s));
        s = prefix() + textFromValue(maximum()) + suffix() + QLatin1Char(' ');
        s.truncate(18);
        w = qMax(w, fm.width(s));
        if (specialValueText().size()) {
            s = specialValueText();
            w = qMax(w, fm.width(s));
        }
        w += 2; // cursor blinking space

        QStyleOptionSpinBox opt;
        initStyleOption(&opt);
        QSize hint(w, h);
        QSize extra(35, 6);
        opt.rect.setSize(hint + extra);
        extra += hint - QApplication::style()->subControlRect(QStyle::CC_SpinBox, &opt,
            QStyle::SC_SpinBoxEditField, this).size();
        // get closer to final result by repeating the calculation
        opt.rect.setSize(hint + extra);
        extra += hint - QApplication::style()->subControlRect(QStyle::CC_SpinBox, &opt,
            QStyle::SC_SpinBoxEditField, this).size();
        hint += extra;  

        opt.rect = rect();
        d->cachedSizeHint = QApplication::style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this)
            .expandedTo(QApplication::globalStrut());
        d->cachedSizeHint.rwidth() += d->pixmapSize.width() + 2 * ::spacing() + 2;
        d->cachedSpecialText = specialValueText();
    }
    return d->cachedSizeHint;
}

void MvdClearSpin::resizeEvent(QResizeEvent *e)
{
    QSpinBox::resizeEvent(e);

    QStyleOptionSpinBox opt;
    initStyleOption(&opt);

    QRect r = style()->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxEditField, this);
    d->clearButton->move(
        r.right(),
        (int)ceil((r.height() - d->pixmapSize.height()) / 2.0)
        );
}

void MvdClearSpin::paintEvent(QPaintEvent *e)
{
    QSpinBox::paintEvent(e);
}

void MvdClearSpin::updateClearButton(const QString &text)
{
    d->clearButton->setVisible(!text.isEmpty() && text != specialValueText());
}

void MvdClearSpin::clearButtonClicked()
{
    setValue(minimum());
}

void MvdClearSpin::showEvent(QShowEvent *e)
{
    QSpinBox::showEvent(e);
    updateClearButton(text());
}
