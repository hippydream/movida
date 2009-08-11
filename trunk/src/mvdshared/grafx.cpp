/**************************************************************************
** Filename: grafx.cpp
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

#include "grafx.h"

#include "mvdcore/core.h"

#include <QtGui/QFont>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QPixmapCache>
#include <QtGui/QRegion>
#include <QtGui/QTextLayout>

#include <cmath>

namespace {
const int Border = 1;
const int TipPadding = 2;

const float PosterAspectRatio = 0.66f;
const int PosterDisplacement = 5;
const int PosterPadding = 1;
const int PosterTextPadding = 2;

const int SharedDataPadding = 2;

bool comparePixmapByWidth(const QPixmap *p1, const QPixmap *p2)
{
    return p1->width() > p2->width();
}

} // anonymous namespace

QPixmap MvdGrafx::moviesDragPixmap(const QStringList &imagePaths, QString message, QFont font)
{
    QSize cachedSize = MvdCore::parameter("movida/movie-poster/cached-size").toSize();

    if (cachedSize.isNull()) {
        cachedSize.setHeight(75);
        cachedSize.setWidth((int)(PosterAspectRatio * 75));
    }

    QPixmap pm;

    QList<QPixmap *> pixmaps;
    int posterAreaW = 0;
    int posterAreaH = 0;

    const int MaxPosters = MvdCore::parameter("movida/d&d/max-pixmaps").toInt();

    foreach(QString s, imagePaths)
    {
        QString key = QString("%1x%2/").arg(cachedSize.width()).arg(cachedSize.height()).append(s);
        QPixmap *p = new QPixmap;

        QPixmapCache::find(key, *p);
        if (p->isNull()) {
            p->load(s);
            if (!p->isNull()) {
                // Resize and cache poster pixmap
                QPixmap *sp = new QPixmap(p->scaled(cachedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                delete p;
                p = sp;
                QPixmapCache::insert(key, *p);
            }
        }

        if (p->isNull()) {
            delete p;
            continue;
        }

        posterAreaW = qMax(p->width(), posterAreaW);
        posterAreaH = qMax(p->height(), posterAreaH);
        pixmaps.append(p);

        if (pixmaps.size() >= MaxPosters)
            break;
    }

    // Sort pixmaps by width - first is the largest in the set
    qSort(pixmaps.begin(), pixmaps.end(), ::comparePixmapByWidth);

    if (pixmaps.isEmpty()) {
        // Add a default poster pixmap
        QPixmap defaultPoster(":/images/default-poster.png");
        QPixmap *p = new QPixmap(defaultPoster.scaled(cachedSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        pixmaps.append(p);
        posterAreaW = p->width();
        posterAreaH = p->height();
    }

    const int pmCount = pixmaps.size();
    posterAreaW += (pmCount - 1) * PosterDisplacement + (pmCount + 1) * (Border + PosterPadding);
    posterAreaH += 2 * (Border + PosterPadding);

    int imageAreaH = posterAreaH;
    int imageAreaW = posterAreaW;
    QRect textBR;

    if (!message.isEmpty()) {
        static const int MaxMessageWidth = 150;
        QFontMetrics fm(font);
        textBR = fm.boundingRect(message);
        if (textBR.width() > MaxMessageWidth) {
            message = fm.elidedText(message, Qt::ElideRight, MaxMessageWidth);
            textBR = fm.boundingRect(message);
        }
        textBR.setWidth(textBR.width() + 2 * (Border + PosterTextPadding));
        textBR.setHeight(textBR.height() + 2 * (Border + PosterTextPadding));
        textBR.moveLeft(PosterDisplacement);
        textBR.moveTop(posterAreaH - PosterDisplacement);
        posterAreaW = qMax(posterAreaW, textBR.right() + Border);
        posterAreaH += (textBR.height() - PosterDisplacement);
    }

    pm = QPixmap(posterAreaW, posterAreaH);
    pm.fill(Qt::transparent);

    QPainter painter(&pm);

    QPen pen = painter.pen();
    pen.setWidth(Border);
    pen.setColor(QColor("#222222")); //! \todo Style d&d pixmaps
    painter.setPen(pen);

    painter.save();

    QRegion clipRegion(QRect(0, 0, imageAreaW, imageAreaH));
    painter.setClipRegion(clipRegion);
    painter.setBrush(Qt::white);

    int offsetX = 0;

    for (int i = 0; i < pixmaps.size(); ++i) {
        const QPixmap *p = pixmaps.at(i);
        QRect rCurrent(
            i == 0 ? 0 : (offsetX + PosterDisplacement - p->width() - Border - PosterPadding), // Right align (+ displacement)
            imageAreaH - (p->height() + 2 * Border + 2 * PosterPadding), // Bottom align
            p->width() + 2 * PosterPadding + Border,
            p->height() + 2 * PosterPadding + Border);

        painter.drawRect(rCurrent);

        painter.drawPixmap(
            rCurrent.x() + PosterPadding + Border,
            rCurrent.y() + PosterPadding + Border,
            *p);

        // Right border was not included in rect because added by pen width
        rCurrent.setRight(rCurrent.right() + Border);

        // +1 to index the first leftmost pixel of the next poster
        offsetX = rCurrent.right() + 1;

        if (i != pixmaps.size() - 1) {
            clipRegion = clipRegion.subtracted(rCurrent);
            painter.setClipRegion(clipRegion);
        }
    }

    painter.restore();

    if (!message.isEmpty()) {
        painter.save();
        painter.setBrush(Qt::white);
        painter.drawRect(textBR.adjusted(0, 0, -Border, -Border));
        painter.drawText(textBR.adjusted(Border + PosterTextPadding, Border + PosterTextPadding, 0, 0), message);
        painter.restore();
    }

    qDeleteAll(pixmaps);

    return pm;
}

QPixmap MvdGrafx::sharedDataDragPixmap(const QString &values, QFont font)
{
    if (values.isEmpty())
        return QPixmap();

    QFontMetrics fontMetrics(font);
    QTextLayout textLayout(values, font);
    QTextOption textOption;
    textOption.setAlignment(Qt::AlignLeft);
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textLayout.setTextOption(textOption);

    int lines = 0;
    int leading = fontMetrics.leading();
    int height = 0;
    qreal widthUsed = 0;
    textLayout.beginLayout();
    static const int LineWidth = 250;
    while (1) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;

        ++lines;
        line.setLineWidth(LineWidth);
        height += leading;
        line.setPosition(QPoint(0, height));
        height += ceil(line.height());
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    textLayout.endLayout();

    QRect textRect(0, 0, ceil(widthUsed), height);

    int areaW = textRect.width() + 2 * Border + 2 * SharedDataPadding;
    int areaH = textRect.height() + 2 * Border + 2 * SharedDataPadding;

    QPixmap pm(areaW, areaH);
    pm.fill(Qt::transparent);

    QPainter painter(&pm);

    painter.setBrush(Qt::white);
    QPen pen = painter.pen();
    pen.setWidth(Border);
    pen.setColor(QColor("#222222")); //! \todo Style d&d pixmaps
    painter.setPen(pen);

    painter.drawRect(0, 0, textRect.width() + 2 * SharedDataPadding + Border,
        textRect.height() + 2 * SharedDataPadding + Border);

    textLayout.draw(&painter, QPointF(SharedDataPadding + Border, SharedDataPadding + Border));

    return pm;
}
