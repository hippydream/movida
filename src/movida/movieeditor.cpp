/**************************************************************************
** Filename: movieeditor.cpp
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
#include <QMessageBox>

/*!
	\class MvdMovieEditor movieeditor.h
	\ingroup Movida

	\brief Movie editor dialog.
*/


/*!
	Creates a new dialog for editing of movie descriptions.
*/
MvdMovieEditor::MvdMovieEditor(MvdMovieCollection* c, QWidget* parent)
: MvdMultiPageDialog(parent), mCollection(c), mMovieId(0)
{
	//! \todo set a better title (and update on setMovie() or title editing)
	setWindowTitle(tr("movida movie editor"));

	QDialogButtonBox* box = MvdMultiPageDialog::buttonBox();
	mHelpButton = box->addButton(QDialogButtonBox::Help);
	mHelpButton->setEnabled(false);
	mPreviousButton = box->addButton(tr("&Previous"), QDialogButtonBox::ActionRole);
	mPreviousButton->setEnabled(false);
	mNextButton = box->addButton(tr("&Next"), QDialogButtonBox::ActionRole);
	mNextButton->setEnabled(false);
	mOkButton = box->addButton(QDialogButtonBox::Ok);
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

	mResetPageId = addAdvancedControl(tr("reset this page"), false);
	mResetAllId = addAdvancedControl(tr("reset all"), false);
	mStoreChangesId = addAdvancedControl(tr("store changes"), false);

	setAdvancedControlsVisible(true);

	connect( box, SIGNAL(rejected()), this, SLOT(cancelTriggered()) );
	connect( box, SIGNAL(accepted()), this, SLOT(storeTriggered()) );

	connect( mPreviousButton, SIGNAL(clicked()), this, SIGNAL(previousRequested()) );
	connect( mNextButton, SIGNAL(clicked()), this, SIGNAL(nextRequested()) );

	connect( this, SIGNAL(currentPageChanged(MvdMPDialogPage*)), this, SLOT(currentPageChanged(MvdMPDialogPage*)) );
}

/*!
	Sets data from a new movie removing any previously set info.
	Returns false if the movie has not been set (i.e. there was another
	movie being edited and the user didn't want to lose it).
*/
bool MvdMovieEditor::setMovie(mvdid id, bool confirmIfModified)
{
	if (mCollection == 0)
		return false;

	if (confirmIfModified && isModified())
	{
		if (!confirmDiscardMovie())
			return false;
	}

	MvdMovie movie = mCollection->movie(id);

	for (int i = 0; i < mPages.size(); ++i)
	{
		MvdMovieEditorPage* p = mPages.at(i);
		p->setMovie(movie);
		p->setModified(false);
	}

	mMovieId = id;

	return true;
}

//! \todo Implement
bool MvdMovieEditor::setMovies(const QList<mvdid>& movies, bool confirmIfModified)
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
		bool valid = isValid();
		QString msg = valid ? 
			tr("The movie has been modified. Do you want to save the movie, discard the changes or cancel the operation?")
			: tr("The movie has been modified but cannot be saved because required data is missing. Do you want to discard the changes or cancel the operation?");

		int res = valid ? 
			QMessageBox::question(this, MVD_CAPTION, msg, QMessageBox::Save, QMessageBox::Discard, QMessageBox::Cancel)
			: QMessageBox::question(this, MVD_CAPTION, msg, QMessageBox::Discard, QMessageBox::Cancel);

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
		else p->setModified(false);
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

//! \internal Returns true if some page is not valid.
bool MvdMovieEditor::isValid() const
{
	bool valid = true;
	for (int i = 0; valid && i < mPages.size(); ++i)
	{
		const MvdMovieEditorPage* p = mPages.at(i);
		if (!p->isValid())
			valid = false;
	}
	return valid;
}

//! Enables or disables the "previous movie" button.
void MvdMovieEditor::setPreviousEnabled(bool enabled)
{
	mPreviousButton->setEnabled(enabled);
}

//! Enables or disables the "next movie" button.
void MvdMovieEditor::setNextEnabled(bool enabled)
{
	mNextButton->setEnabled(enabled);
}

//! Enables or disables the Ok button.
void MvdMovieEditor::validationStateChanged(MvdMPDialogPage* page)
{
	Q_ASSERT(page);
	Q_UNUSED(page);

	bool valid = isValid();
	mOkButton->setEnabled(valid);
}

//! Enables or disables the some UI elements.
void MvdMovieEditor::modifiedStateChanged(MvdMPDialogPage* page)
{
	Q_ASSERT(page);
	Q_UNUSED(page);

	bool modified = isModified();
	setAdvancedControlEnabled(mResetPageId, currentPage()->isModified());
	setAdvancedControlEnabled(mResetAllId, modified);
	setAdvancedControlEnabled(mStoreChangesId, modified);
}

//!
void MvdMovieEditor::advancedControlHandler(int control)
{
	if (control == mResetPageId)
	{
		int res = QMessageBox::question(this, MVD_CAPTION, tr("Are you sure you want to reset this page to the last stored version?"),
			QMessageBox::Yes, QMessageBox::No);
		if (res != QMessageBox::Yes)
			return;

		MvdMovieEditorPage* p = qobject_cast<MvdMovieEditorPage*>(currentPage());
		if (p)
		{
			p->setMovie(mCollection->movie(mMovieId));
			p->setModified(false);
		}
	}
	else if (control == mResetAllId)
	{
		Q_ASSERT(mCollection);

		int res = QMessageBox::question(this, MVD_CAPTION, tr("Are you sure you want to reset this movie to the last stored version?"),
			QMessageBox::Yes, QMessageBox::No);
		if (res != QMessageBox::Yes)
			return;

		for (int i = 0; i < mPages.size(); ++i)
		{
			MvdMovieEditorPage* p = mPages.at(i);
			p->setMovie(mCollection->movie(mMovieId));
			p->setModified(false);
		}
	}
	else if (control == mStoreChangesId)
	{
		if (storeMovie())
		{
			Q_ASSERT(mMovieId != MvdNull);
			setMovie(mMovieId, false);
		}
	}
}

//!
void MvdMovieEditor::currentPageChanged(MvdMPDialogPage* page)
{
	Q_ASSERT(page);
	setAdvancedControlEnabled(mResetPageId, page->isModified());
}
