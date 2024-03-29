/**************************************************************************
** Filename: expandinglineedit.cpp
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

#include "expandinglineedit.h"

/*!
    \class MvdExpandingLineEdit expandinglineedit.h
    \ingroup MovidaShared

    \brief QLineEdit subclass that adapts to the text width. It should be used as editor
    widget in item views.

    Source code comes from the private QExpandingLineEdit found in the Qt 4.3
    QItemEditorFactory.
*/

MvdExpandingLineEdit::MvdExpandingLineEdit(QWidget *parent) :
    MvdLineEdit(parent),
    mOriginalWidth(-1)
{
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(resizeToContents()));
}

MvdExpandingLineEdit::MvdExpandingLineEdit(const QString &contents, QWidget *parent) :
    MvdLineEdit(contents, parent),
    mOriginalWidth(-1)
{
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(resizeToContents()));
}

//! \internal
void MvdExpandingLineEdit::resizeToContents()
{
    if (mOriginalWidth == -1)
        mOriginalWidth = width();
    if (QWidget * parent = parentWidget()) {
        QPoint position = pos();
        QFontMetrics fm(font());
        const int hintWidth = sizeHint().width() - (fm.width(QLatin1Char('x')) * 17) + fm.width(displayText());
        const int parentWidth = parent->width();
        const int maxWidth = isRightToLeft() ? position.x() + width() : parentWidth - position.x();
        const int newWidth = qBound(mOriginalWidth, hintWidth, maxWidth);
        if (isRightToLeft())
            setGeometry(position.x() - newWidth + width(), position.y(), newWidth, height());
        else
            resize(newWidth, height());
    }
}
