/**************************************************************************
** Filename: smartviewdelegate.h
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

#ifndef MVD_SMARTVIEWDELEGATE_H
#define MVD_SMARTVIEWDELEGATE_H

#include "guiglobal.h"

#include <QtGui/QItemDelegate>
#include <QtGui/QTextLayout>
#include <QtGui/QTextOption>

class MvdSmartView;

class MvdSmartViewDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    enum ItemSize {
        InvalidItemSize = 0,
        SmallItemSize,
        MediumItemSize,
        LargeItemSize
    };

    MvdSmartViewDelegate(MvdSmartView *parent = 0);

    void setItemSize(ItemSize size = MediumItemSize);
    ItemSize itemSize() const;

    void forcedUpdate();
    void mousePressed(const QRect &rect, const QModelIndex &index);
    void showHoveredControlHint();

protected:
    virtual void paint(QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const;

    virtual QSize sizeHint(const QStyleOptionViewItem &option,
    const QModelIndex &index) const;

    virtual QSize maximumIconSize() const;

private:
    static const int Margin;
    static const int Padding;
    static const int CornerWidth;
    static const int ControlSize;
    static const int BorderWidth;
    static const int HalfPadding;
    static const int IconMarginRight;
    static const int IconPadding;

    static const float ItemAspectRatio;
    static const float IconAspectRatio;

    static const bool UseTitleSeparator;

    static const QColor BorderColor;
    static const QColor SelectionColor;
    static const QColor InactiveSelectionColor;

    static const Qt::Alignment IconAlignment;


    enum Control {
        NoControl = 0,
        RatingControl,
        SeenControl,
        SpecialControl
    };

    struct TextOptions {
        enum HeadingLevel { H1_HeadingLevel, H2_HeadingLevel, H3_HeadingLevel } headingLevel;
        TextOptions() :
            headingLevel(H1_HeadingLevel) { }

    };

    ItemSize mItemSize;
    QPixmap mDefaultPoster;
    QIcon mRatingIcon;
    QIcon mSpecialIcon;
    QIcon mLoanedIcon;
    QIcon mSeenIcon;
    mutable QTextLayout mTextLayout;
    mutable QTextOption mTextOption;
    MvdSmartView *mView;

    // Metrics - anything else can be computed easily starting with these values
    QSize mSize; // Whole item
    QSize mIconSize; // Icon (including border & everything else)

    // Convenience only:
    QSize mControlsSize;
    QSize mTextSize;

    void drawItemText(QPainter *painter, const QStyleOptionViewItem &option,
    QRect rect, QString text, const TextOptions &options, QRect *boundingRect = 0, int maxLines = 1) const;
    QSizeF doTextLayout(int lineWidth, int maxHeight, int *lineCount = 0) const;
    void rebuildDefaultIcon();
    inline bool hasMouseOver(const QRect &itemRect) const;
    inline Control hoveredControl(const QRect &itemRect, int *index) const;
};

#endif // MVD_SMARTVIEWDELEGATE_H
