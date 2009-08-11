/**************************************************************************
** Filename: actionlabel.h
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

#ifndef MVD_ACTIONLABEL_H
#define MVD_ACTIONLABEL_H

#include "sharedglobal.h"

#include <QtGui/QLabel>

class MVD_EXPORT_SHARED MvdActionLabel : public QLabel
{
Q_OBJECT

public:
    MvdActionLabel(QWidget * parent = 0);
    virtual ~MvdActionLabel();

    int addControl(const QString &text, bool enabled);
    void setControlEnabled(int control, bool enabled);
    bool controlEnabled(int control);

signals:
    void controlTriggered(int);

private:
    void emit_controlTriggered(int id) { emit controlTriggered(id); }

    class Private;
    friend class Private;
    Private *d;
};

#endif // MVD_ACTIONLABEL_H
