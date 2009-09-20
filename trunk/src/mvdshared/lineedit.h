/**************************************************************************
** Filename: lineedit.h
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

#ifndef MVD_LINEEDIT_H
#define MVD_LINEEDIT_H

#include "sharedglobal.h"

#include <QtGui/QLineEdit>

class MvdCompleter;

class MVD_EXPORT_SHARED MvdLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    MvdLineEdit(QWidget *parent = 0);
    MvdLineEdit(const QString &contents, QWidget *parent = 0);
    virtual ~MvdLineEdit();

    virtual QSize sizeHint() const;

    void setPlaceHolder(const QString &s);
    QString placeHolder() const;

    void setAdvancedCompleter(MvdCompleter *completer);
    MvdCompleter *advancedCompleter() const;

protected:
    virtual bool event(QEvent *e);
    virtual void paintEvent(QPaintEvent *);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void focusInEvent(QFocusEvent *e);

    virtual void setTextCalled();

protected slots:
    virtual void completionActivated(const QString &text);
    virtual void completionHighlighted(const QString &text);

private slots:
    void on_textEdited();
    void on_textChanged();

private:
    void init();

    class Private;
    Private *d;
};

#endif // MVD_LINEEDIT_H
