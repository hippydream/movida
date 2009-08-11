/**************************************************************************
** Filename: clearedit.cpp
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

#include "clearedit.h"

#include <QtGui/QApplication>
#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtGui/QStyleOptionFrameV2>
#include <QtGui/QToolButton>

#include <cmath>

// From QLineEdit
#define verticalMargin 1
#define horizontalMargin 2

namespace {
//! Check out style issues with Oxygen
static int spacing()
{
    QStyle *s = QApplication::style();

    if (s->inherits("OxygenStyle"))
        return 3;
    return 2;
}

static QSize &adjustSizeHint(QSize &sz)
{
    QStyle *s = QApplication::style();

    // oxygen style will cause a wrong vertical size hint
    // if the line edit has a stylesheet
    if (s->inherits("OxygenStyle"))
        sz.rheight() = 27;
    return sz;
}

}

class MvdClearEdit::Private
{
public:
    Private() :
        clearButton(0) { }

    QToolButton *clearButton;
    QString placeHolder;
    QSize pixmapSize;
};

MvdClearEdit::MvdClearEdit(QWidget *parent) :
    QLineEdit(parent),
    d(new Private)
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
    connect(d->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    connect(this, SIGNAL(textChanged(const QString &)), this, SLOT(updateClearButton(const QString &)));

    QString styleSheet = QLatin1String("QLineEdit {");
    styleSheet += QString(" padding-right: %1;").arg(d->pixmapSize.width() + 2 * ::spacing());

    if (style()->inherits("OxygenStyle"))   // Focus effect is too close to the text! Add more spacing.
        styleSheet += QString(" padding-left: %1;").arg(::spacing());

    styleSheet += QLatin1String("}");
    setStyleSheet(styleSheet);
}

void MvdClearEdit::resizeEvent(QResizeEvent *e)
{
    QLineEdit::resizeEvent(e);

    QStyleOptionFrameV2 opt;
    initStyleOption(&opt);

    QRect r = style()->subElementRect(QStyle::SE_LineEditContents, &opt, this);

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

    QRect lineRect(r.x() + horizontalMargin * 3, vscroll, r.width() - 2 * horizontalMargin, fm.height());

    d->clearButton->move(
        lineRect.right(),
        (int)ceil((lineRect.height() - d->pixmapSize.height()) / 2.0) + ::spacing()
        );
}

QSize MvdClearEdit::sizeHint() const
{
    QSize sz = QLineEdit::sizeHint();

    return adjustSizeHint(sz);
}

void MvdClearEdit::updateClearButton(const QString &text)
{
    d->clearButton->setVisible(!text.isEmpty());
}

//! Sets a string to be displayed as place holder when the widget contains no text.
void MvdClearEdit::setPlaceHolder(const QString &s)
{
    d->placeHolder = s.trimmed();
}

//! Returns the current place holder string, if any.
QString MvdClearEdit::placeHolder() const
{
    return d->placeHolder;
}

void MvdClearEdit::paintEvent(QPaintEvent *e)
{
    QLineEdit::paintEvent(e);

    // Draw place holder
    if (d->placeHolder.isEmpty() || !text().isEmpty() || hasFocus())
        return;

    QPainter p(this);

    QRect r;
    const QPalette &pal = palette();

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

    QRect lineRect(r.x() + horizontalMargin * 3, vscroll, r.width() - 2 * horizontalMargin, fm.height());

    p.setPen(pal.color(QPalette::Disabled, QPalette::Text));
    p.drawText(lineRect, d->placeHolder);
}
