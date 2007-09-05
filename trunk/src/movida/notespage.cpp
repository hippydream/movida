/**************************************************************************
** Filename: notespage.cpp
** Revision: 3
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
#include "core.h"
#include <QIcon>
#include <QTextEdit>

/*!
	\class MvdNotesPage notespage.h
	\ingroup Movida

	\brief Plot and notes editing widget for movie editing dialog.
*/

/*!
	Creates a new page.
*/
MvdNotesPage::MvdNotesPage(MvdMovieCollection* c, MvdMovieEditor* parent)
: MvdMovieEditorPage(c, parent)
{
	setupUi(this);
	
	linkActivated("movida://toggle-view/plot");
	connect(toggleLabel, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)));

	connect(plot->editor(), SIGNAL(textChanged()), this, SLOT(updateModifiedStatus()));
	connect(notes->editor(), SIGNAL(textChanged()), this, SLOT(updateModifiedStatus()));
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

void MvdNotesPage::setMovieImpl(const MvdMovie& movie)
{
	plot->editor()->setHtml(movie.plot());
	mDefaultPlot = plot->editor()->toHtml();
	notes->editor()->setHtml(movie.notes());
	mDefaultNotes = notes->editor()->toHtml();
}

void MvdNotesPage::setMoviesImpl(const QList<MvdMovie>& movies)
{
}

bool MvdNotesPage::store(MvdMovie& movie)
{
	movie.setNotes(notes->editor()->toHtml());
	movie.setPlot(plot->editor()->toHtml());

	return true;
}

//! \internal
void MvdNotesPage::linkActivated(const QString& url)
{
	//! \bug Temporary bug fix. Trolltech task tracker #172324, scheduled for Qt 4.3.2 (http://trolltech.com/developer/task-tracker/index_html?method=entry&id=172324)
	Q_ASSERT(QMetaObject::invokeMethod(this, "doLinkActivated", Qt::QueuedConnection, Q_ARG(QString, url)));
}

void MvdNotesPage::doLinkActivated(const QString& url)
{
	MvdCore::LabelAction a = MvdCore::parseLabelAction(url);
	if (!a.isValid())
		return;

	int index = -1;

	if (a.action == "toggle-view")
	{
		if (a.parameter == "plot")
		{
			toggleLabel->setText(tr("Currently showing the movie <b>plot</b>. Click to show the <a href=\"movida://toggle-view/notes\">notes</a>."));
			index = 0;
		}
		else if (a.parameter == "notes")
		{
			toggleLabel->setText(tr("Currently showing the movie <b>notes</b>. Click to show the <a href=\"movida://toggle-view/plot\">plot</a>."));
			index = 1;
		}

		stack->setCurrentIndex(index);
	}
}

//!
void MvdNotesPage::updateModifiedStatus()
{
	if (notes->editor()->toHtml().trimmed() != mDefaultNotes
		|| plot->editor()->toHtml().trimmed() != mDefaultPlot)
		setModified(true);
	else setModified(false);
}
