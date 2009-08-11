/**************************************************************************
** Filename: movietreeviewdelegate.cpp
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

#include "movietreeviewdelegate.h"

#include "mvdcore/core.h"

#include <QtCore/QModelIndex>
#include <QtGui/QIcon>
#include <QtGui/QPainter>

#include <cmath>

/*!
    \class MvdMovieTreeViewDelegate movietreeviewdelegate.h

    \brief Tree view delegate for a movie view - adds custom rendering for rating and other attributes.
*/

const int MvdMovieTreeViewDelegate::Margin = 1;

MvdMovieTreeViewDelegate::MvdMovieTreeViewDelegate(QObject *parent) :
    QItemDelegate(parent)
{
    mRatingIcon = QIcon(":/images/rating.svgz");
    mSeenIcon = QIcon(":/images/seen.svgz");
    mSpecialIcon = QIcon(":/images/special.svgz");
    mLoanedIcon = QIcon(":/images/loaned.svgz");
}

//! \internal
void MvdMovieTreeViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    bool useDefaultDelegate = true;

    painter->save();
    painter->setClipRect(option.rect);
    if (index.column() == Movida::RatingAttribute) {
        QItemDelegate::drawBackground(painter, option, index);
        QItemDelegate::drawFocus(painter, option, option.rect.adjusted(0, 0, -1, -1));
        useDefaultDelegate = false;
        int rating = index.data().toInt();
        int n = sizeHint(option, index).height() - 2 * Margin;
        QSize sz(n, n);
        QRect r(option.rect.x(), option.rect.y(), sz.width() * rating, sz.height());
        painter->drawTiledPixmap(r, mRatingIcon.pixmap(sz));
        int maxRating = MvdCore::parameter("mvdcore/max-rating").toInt();
        rating = maxRating - rating;
        r.setX(r.x() + r.width());
        r.setWidth(sz.width() * rating);
        painter->drawTiledPixmap(r, mRatingIcon.pixmap(sz, QIcon::Disabled));

    } else if (index.column() == Movida::SpecialAttribute ||
               index.column() == Movida::SeenAttribute ||
               index.column() == Movida::LoanedAttribute) {
        QItemDelegate::drawBackground(painter, option, index);
        QItemDelegate::drawFocus(painter, option, option.rect.adjusted(0, 0, -1, -1));
        useDefaultDelegate = false;
        QIcon icon;
        bool enabled = index.data().toBool();
        switch (index.column()) {
            case Movida::SeenAttribute:
                icon = mSeenIcon; break;

            case Movida::SpecialAttribute:
                icon = mSpecialIcon; break;

            case Movida::LoanedAttribute:
                icon = mLoanedIcon; break;

            default:
                ;
        }
        QSize sz = sizeHint(option, index) - QSize(2 * Margin, 2 * Margin);
        QRect r(option.rect);
        r.setWidth(sz.width());
        r.setHeight(sz.height());
        painter->drawPixmap(r, icon.pixmap(sz, enabled ? QIcon::Normal : QIcon::Disabled));
    }
    painter->restore();

    if (useDefaultDelegate)
        QItemDelegate::paint(painter, option, index);
}

//! \internal
QSize MvdMovieTreeViewDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize sz = QItemDelegate::sizeHint(option, index);
    int w = sz.width();
    int h = sz.height();

    if (index.column() == Movida::RatingAttribute) {
        int maxRating = MvdCore::parameter("mvdcore/max-rating").toInt();
        h = sz.height();
        w = h * maxRating;

    } else if (index.column() == Movida::SeenAttribute ||
               index.column() == Movida::LoanedAttribute ||
               index.column() == Movida::SpecialAttribute) {
        h = w = sz.height();
    }

    return QSize(w, h);
}
