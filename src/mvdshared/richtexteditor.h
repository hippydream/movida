/**************************************************************************
** Filename: richtexteditor.h
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

#ifndef MVD_RICHTEXTEDITOR_H
#define MVD_RICHTEXTEDITOR_H

#include "sharedglobal.h"

#include <QtGui/QWidget>

class QTextEdit;

class MVD_EXPORT_SHARED MvdRichTextEditor : public QWidget
{
    Q_OBJECT

public:
    enum ControlsPosition { Top, Bottom };

    MvdRichTextEditor(QWidget *parent = 0);
    MvdRichTextEditor(ControlsPosition pos, QWidget *parent = 0);
    virtual ~MvdRichTextEditor();

    QTextEdit *editor() const;

    ControlsPosition position() const;

private:
    void init();

    class Private;
    Private *d;
};

#endif // MVD_RICHTEXTEDITOR_H
