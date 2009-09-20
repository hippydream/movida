/**************************************************************************
** Filename: expandingcombobox.cpp
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

#include "expandingcombobox.h"

#include <QtGui/QLineEdit>

/*!
    \class MvdExpandingComboBox expandingcombobox.h
    \ingroup MovidaShared

    \brief MvdExpandingComboBox subclass that adapts to the text width. It should be used as editor
    widget in item views.

    Source code comes from the private QExpandingLineEdit found in the Qt 4.3
    QItemEditorFactory.
*/

MvdExpandingComboBox::MvdExpandingComboBox(QWidget *parent) :
    MvdComboBox(parent),
    mOriginalWidth(-1)
{
    if (QLineEdit *le = lineEdit())
        connect(le, SIGNAL(textChanged(QString)), this, SLOT(resizeToContents()));
}

//! \internal
void MvdExpandingComboBox::resizeToContents()
{
    QLineEdit *le = lineEdit();
    if (!le)
        return;

    const int diff = width() - le->width();

    if (mOriginalWidth == -1)
        mOriginalWidth = width();
    if (QWidget * parent = parentWidget()) {
        QPoint position = pos();
        QFontMetrics fm(le->font());
        const int hintWidth = sizeHint().width() - (fm.width(QLatin1Char('x')) * 17) + fm.width(le->text());
        const int parentWidth = parent->width();
        const int maxWidth = isRightToLeft() ? position.x() + width() : parentWidth - position.x();
        const int newWidth = qBound(mOriginalWidth, hintWidth, maxWidth) + diff;
        if (isRightToLeft())
            setGeometry(position.x() - newWidth + width(), position.y(), newWidth, height());
        else
            resize(newWidth, height());
    }
}
