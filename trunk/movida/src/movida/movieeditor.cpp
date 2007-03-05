/**************************************************************************
** Filename: movieeditor.cpp
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

#include "movieeditor.h"
#include "movieeditorpage.h"
#include "movie.h"
#include "moviecollection.h"

#include "maininfopage.h"
#include "extendedinfopage.h"
#include "crewpage.h"
#include "notespage.h"
#include "linkspage.h"

#include <QMessageBox>
#include <QCloseEvent>
#include <QPushButton>

/*!
	\class MvdMovieEditor movieeditor.h

	\brief Movie editor dialog.
*/


/*!
	Creates a new dialog for editing of movie descriptions.
*/
MvdMovieEditor::MvdMovieEditor(MvdMovieCollection* c, QWidget* parent)
: MvdMultiPageDialog(parent), mCollection(c), mMovieId(0)
{
	//! \todo set a better title (and update on setMovie())
	setWindowTitle(tr("Movie editor"));

	QDialogButtonBox* box = MvdMultiPageDialog::buttonBox();
	box->addButton(QDialogButtonBox::Help);
	box->addButton(QDialogButtonBox::Reset);
	previousButton = box->addButton(tr("&Previous"), QDialogButtonBox::ActionRole);
	previousButton->setEnabled(false);
	nextButton = box->addButton(tr("&Next"), QDialogButtonBox::ActionRole);
	nextButton->setEnabled(false);
	box->addButton(QDialogButtonBox::Ok);
	box->addButton(QDialogButtonBox::Cancel);

	MvdMovieEditorPage* page;

	page = new MvdMainInfoPage(c, this);
	mPages.append(page);
	addPage(page);

	page = new MvdExtendedInfoPage(c, this);
	mPages.append(page);
	addPage(page);

	page = new MvdCrewPage(c, this);
	mPages.append(page);
	addPage(page);

	page = new MvdNotesPage(c, this);
	mPages.append(page);
	addPage(page);

	page = new MvdLinksPage(c, this);
	mPages.append(page);
	addPage(page);

	connect( box, SIGNAL(rejected()), this, SLOT(cancelTriggered()) );
	connect( box, SIGNAL(accepted()), this, SLOT(storeTriggered()) );

	connect( previousButton, SIGNAL(clicked()), this, SIGNAL(previousRequested()) );
	connect( nextButton, SIGNAL(clicked()), this, SIGNAL(nextRequested()) );
}

/*!
	Sets data from a new movie removing any previously set info.
	Returns false if the movie has not been set (i.e. there was another
	movie being edited and the user didn't want to lose it).
*/
bool MvdMovieEditor::setMovie(movieid movieID, bool confirmIfModified)
{
	if (mCollection == 0)
		return false;

	if (confirmIfModified && isModified())
	{
		if (!confirmDiscardMovie())
			return false;
	}

	mMovieId = movieID;

	MvdMovie movie = mCollection->movie(movieID);

	for (int i = 0; i < mPages.size(); ++i)
		mPages.at(i)->setMovie(movie);

	return true;
}

bool MvdMovieEditor::setMovies(const QList<movieid>& movies, bool confirmIfModified)
{
	if (mCollection == 0)
		return false;

	if (confirmIfModified && isModified())
	{
		if (!confirmDiscardMovie())
			return false;
	}

	return true;
}

void MvdMovieEditor::cancelTriggered()
{
	bool discard = confirmDiscardMovie();

	if (!discard)
		return;

	reject();
}

/*!
	\internal Returns true if the movie is not modified or asks the user
	if the changes should be discarded or stored otherwise.
*/
bool MvdMovieEditor::confirmDiscardMovie()
{
	bool modified = isModified();
	bool discard = true;

	//! \todo validate first
	if (modified)
	{
		int res = QMessageBox::question(this, _CAPTION_, 
			tr("The movie has been modified. Do you want to save the movie, discard the changes or cancel the operation?"), 
			QMessageBox::Save, QMessageBox::Discard, QMessageBox::Cancel);
		if (res == QMessageBox::Save)
			discard = storeMovie();
		else if (res == QMessageBox::Cancel)
			discard = false;
	}

	return discard;
}

//! \internal \todo Handle ESC key
void MvdMovieEditor::closeEvent(QCloseEvent* e)
{
	cancelTriggered();
	e->ignore();
}

void MvdMovieEditor::storeTriggered()
{
	if (mCollection == 0)
		return;

	bool modified = false;

	if (mMovieId == 0)
		modified = true;
	else
	{
		for (int i = 0; !modified && i < mPages.size(); ++i)
			modified = mPages.at(i)->isModified();
	}
	
	if (modified)
		if (!storeMovie())
			return;

	accept();
}

//! \internal Simply stores the changes made to the movie or adds a new movie to the collection.
bool MvdMovieEditor::storeMovie()
{
	MvdMovie movie;

	for (int i = 0; i < mPages.size(); ++i)
	{
		MvdMovieEditorPage* p = mPages.at(i);
		if (!p->store(movie))
		{
			//! \todo either show the alert box AFTER changing page or replace it with a status message bar in the widget.
			showPage(p);
			return false;
		}
	}

	if (mMovieId == 0)
		mMovieId = mCollection->addMovie(movie);
	else mCollection->updateMovie(mMovieId, movie);

	return true;
}

//! \internal Returns true if the movie needs to be stored.
bool MvdMovieEditor::isModified() const
{
	bool modified = false;

	for (int i = 0; !modified && i < mPages.size(); ++i)
		modified = mPages.at(i)->isModified();

	return modified;
}

//! Enables or disables the "previous movie" button.
void MvdMovieEditor::setPreviousEnabled(bool enabled)
{
	previousButton->setEnabled(enabled);
}

//! Enables or disables the "next movie" button.
void MvdMovieEditor::setNextEnabled(bool enabled)
{
	nextButton->setEnabled(enabled);
}
