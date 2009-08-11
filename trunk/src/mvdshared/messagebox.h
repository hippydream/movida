/**************************************************************************
** Filename: messagebox.cpp
**
** Copyright (C) 2007-2009-2008 Angius Fabrizio. All rights reserved.
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

#ifndef MVD_MESSAGEBOX_H
#define MVD_MESSAGEBOX_H

#include "sharedglobal.h"

#include <QtGui/QMessageBox>

class MVD_EXPORT_SHARED MvdMessageBox : public QMessageBox
{
    Q_OBJECT

public:
    explicit MvdMessageBox(QWidget *parent = 0);
    MvdMessageBox(Icon icon, const QString &title, const QString &text,
        StandardButtons buttons = NoButton, QWidget * parent = 0,
        Qt::WindowFlags f = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    virtual ~MvdMessageBox();

    static StandardButton critical(QWidget *parent, const QString &title,
        const QString &text, StandardButtons buttons = Ok,
        StandardButton defaultButton = NoButton);

    static StandardButton information(QWidget *parent, const QString &title,
        const QString &text, StandardButtons buttons = Ok,
        StandardButton defaultButton = NoButton);

    static StandardButton question(QWidget *parent, const QString &title,
        const QString &text, StandardButtons buttons = Ok,
        StandardButton defaultButton = NoButton);

    static StandardButton warning(QWidget *parent, const QString &title,
        const QString &text, StandardButtons buttons = Ok,
        StandardButton defaultButton = NoButton);
};

#endif // MVD_MESSAGEBOX_H
