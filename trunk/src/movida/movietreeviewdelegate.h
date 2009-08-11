/**************************************************************************
** Filename: movietreeviewdelegate.h
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

#ifndef MVD_MOVIETREEVIEWDELEGATE_H
#define MVD_MOVIETREEVIEWDELEGATE_H

#include "guiglobal.h"

#include <QtGui/QItemDelegate>
#include <QtGui/QTextLayout>
#include <QtGui/QTextOption>

class MvdMovieTreeViewDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    MvdMovieTreeViewDelegate(QObject *parent = 0);

protected:
    virtual void paint(QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const;

    virtual QSize sizeHint(const QStyleOptionViewItem &option,
    const QModelIndex &index) const;

private:
    static const int Margin;

    QIcon mRatingIcon;
    QIcon mSeenIcon;
    QIcon mSpecialIcon;
    QIcon mLoanedIcon;
};

#endif // MVD_MOVIETREEVIEWDELEGATE_H
