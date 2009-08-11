/**************************************************************************
** Filename: posterlabel.cpp
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

#include "posterlabel.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QPixmapCache>

/*!
    \class MvdPosterLabel posterlabel.h
    \ingroup Movida

    \brief MvdLinkLabel subclass with custom appearance for use with movie posters.
*/

const qreal MvdPosterLabel::IconAspectRatio = 0.7;
const QColor MvdPosterLabel::BorderColor = QColor(164, 164, 164);
const QColor MvdPosterLabel::ShadowColor = QColor(127, 127, 127);
const int MvdPosterLabel::InnerIconBorderWidth = 2;
const int MvdPosterLabel::BorderWidth = 1;
const int MvdPosterLabel::ShadowWidth = 4;

MvdPosterLabel::MvdPosterLabel(QWidget *parent) :
    MvdLinkLabel(parent),
    mDirty(true)
{ }

MvdPosterLabel::~MvdPosterLabel()
{ }

bool MvdPosterLabel::setPoster(const QString &path)
{
    mPosterPath.clear();

    mPoster = QPixmap(path);
    if (!mPoster.isNull()) {
        mPosterPath = path;
        mDirty = true;
        update();
        return true;
    }
    return false;
}

QString MvdPosterLabel::poster() const
{
    return mPosterPath;
}

void MvdPosterLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QRect pixmapRect(rect().adjusted(InnerIconBorderWidth, InnerIconBorderWidth,
                         -(InnerIconBorderWidth + ShadowWidth), -(InnerIconBorderWidth + ShadowWidth)));

    QRect borderRect(rect().adjusted(0, 0, -ShadowWidth, -ShadowWidth));

    QString pixmapKey = QString("%1x%2/%3")
                            .arg(pixmapRect.width()).arg(pixmapRect.height())
                            .arg(mPosterPath);

    QPixmap pm;

    if (!QPixmapCache::find(pixmapKey, pm)) {
        pm = mPoster;
        if (!pm.isNull()) {
            pm = pm.scaled(pixmapRect.size(),
                Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPixmapCache::insert(pixmapKey, pm);
        }
    }

    // Adjust rectangles to take aspect ratio into account
    pixmapRect = QRect(
        (width() - pm.width()) / 2 + InnerIconBorderWidth,
        (height() - pm.height()) / 2 + InnerIconBorderWidth,
        pm.width() - ShadowWidth,
        pm.height() - ShadowWidth
        );

    borderRect = QRect(
        (width() - pm.width()) / 2,
        (height() - pm.height()) / 2,
        pm.width() + 2 * InnerIconBorderWidth - ShadowWidth,
        pm.height() + 2 * InnerIconBorderWidth - ShadowWidth
        );

    QPainter painter(this);

    // Init painter
    QPen pen = painter.pen();
    pen.setColor(BorderColor);
    pen.setWidth(BorderWidth);
    painter.setPen(pen);

    painter.setRenderHint(QPainter::Antialiasing, true);

    // Draw border
    painter.setBrush(Qt::white);
    painter.drawRect(borderRect);

    painter.setBrush(Qt::transparent);

    // Icon shadow
    painter.setPen(ShadowColor);

    qreal opacityDelta = qreal(1) / qreal(ShadowWidth);
    QRect shadowRect = borderRect;

    for (int i = 1; i <= ShadowWidth; ++i) {
        shadowRect.translate(1, 1);
        painter.setOpacity(1 - opacityDelta * (i - 1));
        painter.drawLine(shadowRect.topRight(), shadowRect.bottomRight());
        painter.drawLine(shadowRect.bottomLeft(), shadowRect.bottomRight());
    }

    painter.setOpacity(1);

    // Draw pixmap
    if (!pm.isNull())
        painter.drawPixmap(pixmapRect, pm);

    // Update viewport area
    if (mDirty) {
        QRect r = borderRect;
        r.setWidth(r.width() + ShadowWidth);
        r.setHeight(r.height() + ShadowWidth);
        setActiveAreaRect(r);
    }
}
