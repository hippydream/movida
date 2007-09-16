/**************************************************************************
** Filename: notespage.cpp
** Revision: 1
**
** Copyright (C) 2007 Angius Fabrizio. All rights reserved.
**
** This file is part of the Movida project (http://movida.sourceforge.net/).
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

#include <QIcon>

/*!
	\class MvdNotesPage notespage.h

	\brief Plot and notes editing widget for movie editing dialog.
*/

/*!
	Creates a new page.	
*/
MvdNotesPage::MvdNotesPage(MvdMovieCollection* c, QWidget* parent)
: MvdMovieEditorPage(c, parent)
{
	setupUi(this);
}

/*!
	Reset to default values.
*/
void MvdNotesPage::reset()
{
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
	return QIcon(":/images/preferences/log.png");
}

bool MvdNotesPage::isModified() const
{
	return plot->document()->isModified() || notes->document()->isModified();
}

void MvdNotesPage::setMovieImpl(const MvdMovie& movie)
{
	plot->setHtml(movie.plot());
	notes->setHtml(movie.notes());
}

void MvdNotesPage::setMoviesImpl(const QList<MvdMovie>& movies)
{
}

bool MvdNotesPage::store(MvdMovie& movie)
{
	movie.setNotes(notes->toHtml());
	movie.setPlot(plot->toHtml());
	return true;
}
