/**************************************************************************
** Filename: combobox.cpp
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

#include "combobox.h"

#include "completer.h"
#include "lineedit.h"

#include <QtGui/QAbstractItemView>
#include <QtGui/QKeyEvent>

MvdComboBox::MvdComboBox(QWidget *parent) :
    QComboBox(parent)
{
    setLineEdit(new MvdLineEdit(this));
}

MvdComboBox::~MvdComboBox()
{

}

bool MvdComboBox::event(QEvent *e)
{
    return QComboBox::event(e);
}

/*!
    Sets the \a completer to use instead of the current completer.
    If \a completer is 0, auto completion is disabled.

    By default, for an editable combo box, a MvdCompleter that
    performs case insensitive inline completion is automatically created.

    Please note that any MvdCompleter set with setCompleter() will be disabled
    if an advanced completer is available.

    \sa MvdCompleter
*/
void MvdComboBox::setAdvancedCompleter(MvdCompleter *c)
{
    MvdLineEdit* mvdEdit = qobject_cast<MvdLineEdit*>(lineEdit());
    if (!mvdEdit)
        return;

    mvdEdit->setAdvancedCompleter(c);
    
    if (mvdEdit->advancedCompleter()) {
        QObject::disconnect(mvdEdit->advancedCompleter(), 0, this, 0);
    }

    if (c) {
        QObject::connect(c, SIGNAL(activated(QModelIndex)), this, SLOT(completerActivated()));
        c->setWidget(this);
        c->setModel(model());
    }
}

/*!
    Returns the completer that is used to auto complete text input for the
    combobox.

    \sa editable
*/
MvdCompleter *MvdComboBox::advancedCompleter() const
{
    MvdLineEdit* mvdEdit = qobject_cast<MvdLineEdit*>(lineEdit());
    return mvdEdit ? mvdEdit->advancedCompleter() : 0;
}

void MvdComboBox::completerActivated()
{
    
}

void MvdComboBox::focusInEvent(QFocusEvent *e)
{
    QComboBox::focusInEvent(e);

    MvdLineEdit* mvdEdit = qobject_cast<MvdLineEdit*>(lineEdit());
    if (mvdEdit && mvdEdit->advancedCompleter()) {
        mvdEdit->advancedCompleter()->setWidget(this);
    }
}

void MvdComboBox::keyPressEvent(QKeyEvent *e)
{
    MvdCompleter* c = advancedCompleter();
    if (c) {
        setCompleter(0); // Ensure no QCompleter will be used
    }

    if (c 
        && c->popup()
        && c->popup()->isVisible()) {
            // provide same autocompletion support as line edit
            lineEdit()->event(e);
            return;
    }

    QComboBox::keyPressEvent(e);
}
