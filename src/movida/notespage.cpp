/**************************************************************************
** Filename: notespage.cpp
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

#include "notespage.h"

#include "mvdcore/core.h"

#include <QtGui/QIcon>
#include <QtGui/QTextEdit>

/*!
    \class MvdNotesPage notespage.h
    \ingroup Movida

    \brief Plot and notes editing widget for movie editing dialog.
*/

/*!
    Creates a new page.
*/
MvdNotesPage::MvdNotesPage(MvdMovieCollection *c, MvdMovieEditor *parent) :
    MvdMovieEditorPage(c, parent)
{
    setupUi(this);

    linkActivated("movida://toggle-view/plot");
    connect(toggleLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(linkActivated(const QString &)));

    connect(plot, SIGNAL(textChanged()), this, SLOT(updateModifiedStatus()));
    connect(notes, SIGNAL(textChanged()), this, SLOT(updateModifiedStatus()));
}

/*!
    Returns the title to be used for this page.
*/
QString MvdNotesPage::label()
{
    return tr("Plot and notes");
}

/*!
    Returns the icon to be used for this page.
*/
QIcon MvdNotesPage::icon()
{
    return QIcon();
}

void MvdNotesPage::setMovieImpl(const MvdMovie &movie)
{
    plot->setPlainText(movie.plot());
    mDefaultPlot = plot->toPlainText();
    notes->setPlainText(movie.notes());
    mDefaultNotes = notes->toPlainText();
}

bool MvdNotesPage::store(MvdMovie &movie)
{
    movie.setNotes(notes->toPlainText());
    movie.setPlot(plot->toPlainText());

    return true;
}

//! \internal
void MvdNotesPage::linkActivated(const QString &url)
{
    MvdActionUrl a = MvdCore::parseActionUrl(url);

    if (!a.isValid())
        return;

    int index = -1;

    if (a.action == "toggle-view") {
        if (a.parameter == "plot") {
            toggleLabel->setText(tr("Currently showing the movie <b>plot</b>. Click to show the <a href=\"movida://toggle-view/notes\">notes</a>."));
            index = 0;
        } else if (a.parameter == "notes") {
            toggleLabel->setText(tr("Currently showing the movie <b>notes</b>. Click to show the <a href=\"movida://toggle-view/plot\">plot</a>."));
            index = 1;
        }

        stack->setCurrentIndex(index);
    }
}

//!
void MvdNotesPage::updateModifiedStatus()
{
    if (notes->toPlainText().trimmed() != mDefaultNotes
        || plot->toPlainText().trimmed() != mDefaultPlot)
        setModified(true);
    else setModified(false);
}
