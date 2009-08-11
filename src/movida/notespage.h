/**************************************************************************
** Filename: notespage.h
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

#ifndef MVD_NOTESPAGE_H
#define MVD_NOTESPAGE_H

#include "ui_notespage.h"

#include "movieeditorpage.h"

class QIcon;

class MvdNotesPage : public MvdMovieEditorPage, private Ui::MvdNotesPage
{
    Q_OBJECT

public:
    MvdNotesPage(MvdMovieCollection *c, MvdMovieEditor *parent = 0);

    QString label();
    QIcon icon();

    void setMovieImpl(const MvdMovie &movie);

    bool store(MvdMovie &movie);

private slots:
    void linkActivated(const QString &url);
    void updateModifiedStatus();

private:
    QString mDefaultPlot;
    QString mDefaultNotes;
};

#endif // MVD_NOTESPAGE_H
