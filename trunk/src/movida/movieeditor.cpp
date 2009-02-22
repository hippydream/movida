/**************************************************************************
** Filename: movieeditor.cpp
**
** Copyright (C) 2007-2008-2008 Angius Fabrizio. All rights reserved.
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

#include "movieeditor.h"
#include "movieeditorpage.h"
#include "maininfopage.h"
#include "extendedinfopage.h"
#include "crewpage.h"
#include "notespage.h"
#include "linkspage.h"
#include "mvdcore/movie.h"
#include "mvdcore/moviecollection.h"
#include "mvdshared/messagebox.h"
#include <QCloseEvent>
#include <QPushButton>

/*!
        \class MvdMovieEditor movieeditor.h
        \ingroup Movida

        \brief Movie editor dialog.
*/


/*!
        Creates a new dialog for editing of movie descriptions.
*/
MvdMovieEditor::MvdMovieEditor(MvdMovieCollection* c, QWidget* parent)
: MvdMultiPageDialog(parent), mCollection(c), mMovieId(MvdNull)
{
        setWindowTitle(tr("%1 Editor", "Movie editor title").arg(QCoreApplication::applicationName()));

        resize(sizeHint());
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        QStyle* s = style();

        QDialogButtonBox* box = MvdMultiPageDialog::buttonBox();
        mHelpButton = box->addButton(QDialogButtonBox::Help);
        mHelpButton->setIcon(s->standardIcon(QStyle::SP_DialogHelpButton));
        mHelpButton->setText(tr("&Help"));
        mHelpButton->setEnabled(false);

        mPreviousButton = box->addButton(tr("&Previous"), QDialogButtonBox::ActionRole);
        mPreviousButton->setIcon(s->standardIcon(QStyle::SP_ArrowLeft));
        mPreviousButton->setEnabled(false);

        mNextButton = box->addButton(tr("&Next"), QDialogButtonBox::ActionRole);
        mNextButton->setIcon(s->standardIcon(QStyle::SP_ArrowRight));
        mNextButton->setEnabled(false);

        mOkButton = box->addButton(QDialogButtonBox::Ok);
        mOkButton->setIcon(s->standardIcon(QStyle::SP_DialogOkButton));
        mOkButton->setText(tr("&OK"));

        mCancelButton = box->addButton(QDialogButtonBox::Cancel);
        mCancelButton->setIcon(s->standardIcon(QStyle::SP_DialogCancelButton));
        mCancelButton->setText(tr("&Cancel"));

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

//        mResetPageId = addAdvancedControl(tr("Reset Page"), false);
//        mResetAllId = addAdvancedControl(tr("Reset Movie"), false);
//        mStoreChangesId = addAdvancedControl(tr("Apply"), false);

        QDialogButtonBox* advBB = advancedButtonBox();
        advBB->addButton(QDialogButtonBox::Apply);
        setAdvancedControlsVisible(true);

        connect( box, SIGNAL(rejected()), this, SLOT(cancelTriggered()) );
        connect( box, SIGNAL(accepted()), this, SLOT(storeTriggered()) );

        connect( mPreviousButton, SIGNAL(clicked()), this, SIGNAL(previousRequested()) );
        connect( mNextButton, SIGNAL(clicked()), this, SIGNAL(nextRequested()) );

        connect( this, SIGNAL(currentPageChanged(MvdMPDialogPage*)), this, SLOT(currentPageChanged(MvdMPDialogPage*)) );
}

QSize MvdMovieEditor::sizeHint() const
{
        return QSize(600, 500);
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

/*!
        Returns the movie id set with setMovie() or after a new movie has been added to the collection.
*/
mvdid MvdMovieEditor::movieId() const
{
        return mMovieId;
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
        if (discardChanges())
                return true;

        bool modified = isModified();
        bool discard = true;

        //! \todo validate first
        if (modified)
        {
                bool valid = isValid();
                QString msg = valid ?
                        tr("Movie modified. Save or discard changes?")
                        : tr("Required data is missing. Discard changes?");

                int res = valid ?
                        MvdMessageBox::question(this, tr("Movie Editor"), msg, MvdMessageBox::Save | MvdMessageBox::Discard | MvdMessageBox::Cancel)
                        : MvdMessageBox::question(this, tr("Movie Editor"), msg, MvdMessageBox::Discard | MvdMessageBox::Cancel);

                if (res == MvdMessageBox::Save)
                        discard = storeMovie();
                else if (res == MvdMessageBox::Cancel)
                        discard = false;
        }

        return discard;
}

//! \internal \todo Handle ESC key
void MvdMovieEditor::closeEvent(QCloseEvent* e)
{
        bool discard = confirmDiscardMovie();

        if (discard) {
                reject();
                e->accept();
        } else {
                e->ignore();
        }
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

        if (mMovieId == MvdNull)
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
//        setAdvancedControlEnabled(mResetPageId, currentPage()->isModified());
//        setAdvancedControlEnabled(mResetAllId, modified);
//        setAdvancedControlEnabled(mStoreChangesId, modified);
}

//!
#if 0
void MvdMovieEditor::advancedControlHandler(int control)
{
        if (control == mResetPageId)
        {
                int res = MvdMessageBox::question(this, tr("Movie Editor"),
                        tr("Reset changes to the current page?"),
                        MvdMessageBox::Yes | MvdMessageBox::No);
                if (res != MvdMessageBox::Yes)
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

                int res = MvdMessageBox::question(this, tr("Movie Editor"),
                        tr("Reset changes to this movie?"),
                        MvdMessageBox::Yes | MvdMessageBox::No);
                if (res != MvdMessageBox::Yes)
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
#endif

//!
void MvdMovieEditor::currentPageChanged(MvdMPDialogPage* page)
{
        Q_ASSERT(page);
        //setAdvancedControlEnabled(mResetPageId, page->isModified());
}
