/**************************************************************************
** Filename: movieeditor.cpp
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

#include "movieeditor.h"

#include "crewpage.h"
#include "extendedinfopage.h"
#include "linkspage.h"
#include "maininfopage.h"
#include "movieeditorpage.h"
#include "notespage.h"

#include "mvdcore/movie.h"
#include "mvdcore/moviecollection.h"

#include "mvdshared/messagebox.h"

#include <QtGui/QCloseEvent>
#include <QtGui/QPushButton>

/*!
    \class MvdMovieEditor movieeditor.h
    \ingroup Movida

    \brief Movie editor dialog.
*/


class MvdMovieEditor::Private
{
public:
    Private(MvdMovieEditor *p, MvdMovieCollection *c) :
        mCollection(c),
        mMovieId(MvdNull),
        initialized(false),
        q(p)
    {
        Q_ASSERT(c);
    }

    inline bool confirmDiscardMovie();

    MvdMovieCollection *mCollection;
    mvdid mMovieId;

    QPushButton *mPreviousButton;
    QPushButton *mNextButton;
    QPushButton *mOkButton;
    QPushButton *mCancelButton;
    QPushButton *mHelpButton;

    bool initialized;

private:
    MvdMovieEditor *q;
};


//////////////////////////////////////////////////////////////////////////


/*!
    Creates a new dialog for editing of movie descriptions.
*/
MvdMovieEditor::MvdMovieEditor(MvdMovieCollection *c, QWidget *parent) :
    MvdMultiPageDialog(parent),
    d(new Private(this, c))
{
    setWindowTitle(tr("%1 Editor", "Movie editor title").arg(QCoreApplication::applicationName()));

    resize(sizeHint());
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    QStyle *s = style();

    QDialogButtonBox *box = MvdMultiPageDialog::buttonBox();
    d->mHelpButton = box->addButton(QDialogButtonBox::Help);
    d->mHelpButton->setIcon(s->standardIcon(QStyle::SP_DialogHelpButton));
    d->mHelpButton->setText(tr("&Help"));
    d->mHelpButton->setEnabled(false);

    d->mPreviousButton = box->addButton(tr("&Previous"), QDialogButtonBox::ActionRole);
    d->mPreviousButton->setIcon(s->standardIcon(QStyle::SP_ArrowLeft));
    d->mPreviousButton->setEnabled(false);

    d->mNextButton = box->addButton(tr("&Next"), QDialogButtonBox::ActionRole);
    d->mNextButton->setIcon(s->standardIcon(QStyle::SP_ArrowRight));
    d->mNextButton->setEnabled(false);

    d->mOkButton = box->addButton(QDialogButtonBox::Ok);
    d->mOkButton->setIcon(s->standardIcon(QStyle::SP_DialogOkButton));
    d->mOkButton->setText(tr("&OK"));

    d->mCancelButton = box->addButton(QDialogButtonBox::Cancel);
    d->mCancelButton->setIcon(s->standardIcon(QStyle::SP_DialogCancelButton));
    d->mCancelButton->setText(tr("&Cancel"));

    connect(box, SIGNAL(rejected()), this, SLOT(cancelTriggered()));
    connect(box, SIGNAL(accepted()), this, SLOT(storeTriggered()));

    connect(d->mPreviousButton, SIGNAL(clicked()), this, SIGNAL(previousRequested()));
    connect(d->mNextButton, SIGNAL(clicked()), this, SIGNAL(nextRequested()));

    connect(this, SIGNAL(currentPageChanged(MvdMPDialogPage *)), this, SLOT(currentPageChanged(MvdMPDialogPage *)));
}

MvdMovieEditor::~MvdMovieEditor()
{
    delete d;
}

bool MvdMovieEditor::event(QEvent *e)
{
    return MvdMultiPageDialog::event(e);
}

void MvdMovieEditor::showEvent(QShowEvent *e)
{
    initialize();
    MvdMultiPageDialog::showEvent(e);
}

void MvdMovieEditor::initialize()
{
    if (!d->initialized) {
        loadPages();
        d->initialized = true;
    }
}

MvdMovieCollection *MvdMovieEditor::collection() const
{
    return d->mCollection;
}

void MvdMovieEditor::loadPages()
{
    addPage(new MvdMainInfoPage(d->mCollection, this));
    addPage(new MvdExtendedInfoPage(d->mCollection, this));
    addPage(new MvdCrewPage(d->mCollection, this));
    addPage(new MvdNotesPage(d->mCollection, this));
    addPage(new MvdLinksPage(d->mCollection, this));
}

int MvdMovieEditor::addPage(MvdMPDialogPage *page)
{
    if (MvdMovieEditorPage *p = dynamic_cast<MvdMovieEditorPage *>(page))
        return addPage(p);
    return -1;
}

int MvdMovieEditor::addPage(MvdMovieEditorPage *page)
{
    if (!page)
        return -1;

    page->setParent(this);
    page->setCollection(d->mCollection);

    return MvdMultiPageDialog::addPage(page);
}

/*!
    Sets data from a new movie removing any previously set info.
    Returns false if the movie has not been set (i.e. there was another
    movie being edited and the user didn't want to lose it).
*/
bool MvdMovieEditor::setMovie(mvdid id, bool confirmIfModified)
{
    initialize();

    if (confirmIfModified && isModified()) {
        if (!d->confirmDiscardMovie())
            return false;
    }

    MvdMovie movie = d->mCollection->movie(id);
    if (!movie.isValid())
        return false;

    const int pc = pageCount();
    for (int i = 0; i < pc; ++i) {
        MvdMovieEditorPage *p = dynamic_cast<MvdMovieEditorPage *>(pageAt(i));
        if (p) {
            p->setMovie(movie);
            p->setModified(false);
        }
    }

    d->mMovieId = id;

    return true;
}

/*!
    Returns the movie id set with setMovie() or after a new movie has been added to the collection.
*/
mvdid MvdMovieEditor::movieId() const
{
    return d->mMovieId;
}

void MvdMovieEditor::cancelTriggered()
{
    bool discard = d->confirmDiscardMovie();

    if (!discard)
        return;

    reject();
}

/*!
    \internal Returns true if the movie is not modified or asks the user
    if the changes should be discarded or stored otherwise.
*/
bool MvdMovieEditor::Private::confirmDiscardMovie()
{
    if (q->discardChanges())
        return true;

    bool modified = q->isModified();
    bool discard = true;

    //! \todo validate first
    if (modified) {
        bool valid = q->isValid();
        QString msg = valid
            ? MvdMovieEditor::tr("Movie modified. Save or discard changes?")
            : MvdMovieEditor::tr("Required data is missing. Discard changes?");

        int res = valid ?
          MvdMessageBox::question(q, MvdMovieEditor::tr("Movie Editor"), msg, MvdMessageBox::Save | MvdMessageBox::Discard | MvdMessageBox::Cancel)
          : MvdMessageBox::question(q, MvdMovieEditor::tr("Movie Editor"), msg, MvdMessageBox::Discard | MvdMessageBox::Cancel);

        if (res == MvdMessageBox::Save)
            discard = q->storeMovie();
        else if (res == MvdMessageBox::Cancel)
            discard = false;
    }

    return discard;
}

//! \internal \todo Handle ESC key
void MvdMovieEditor::closeEvent(QCloseEvent *e)
{
    bool discard = d->confirmDiscardMovie();

    if (discard) {
        reject();
        e->accept();
    } else {
        e->ignore();
    }
}

void MvdMovieEditor::storeTriggered()
{
    bool modified = false;

    if (d->mMovieId == MvdNull)
        modified = true;
    else {
        const int pc = pageCount();
        for (int i = 0; !modified && i < pc; ++i)
            modified = pageAt(i)->isModified();
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
    if (d->mMovieId != MvdNull)
        movie = d->mCollection->movie(d->mMovieId);

    const int pc = pageCount();
    for (int i = 0; i < pc; ++i) {
        MvdMovieEditorPage *p = dynamic_cast<MvdMovieEditorPage *>(pageAt(i));
        if (p) {
            if (!p->store(movie)) {
                //! \todo either show the alert box AFTER changing page or replace it with a status message bar in the widget.
                showPage(p);
                return false;
            } else p->setModified(false);
        }
    }

    if (d->mMovieId == MvdNull)
        d->mMovieId = d->mCollection->addMovie(movie);
    else d->mCollection->updateMovie(d->mMovieId, movie);

    return true;
}

//! \internal Returns true if the movie needs to be stored.
bool MvdMovieEditor::isModified() const
{
    bool modified = false;

    const int pc = pageCount();
    for (int i = 0; !modified && i < pc; ++i)
        modified = pageAt(i)->isModified();

    return modified;
}

//! \internal Returns true if some page is not valid.
bool MvdMovieEditor::isValid() const
{
    bool valid = true;

    const int pc = pageCount();
    for (int i = 0; valid && i < pc; ++i) {
        if (!pageAt(i)->isValid())
            valid = false;
    }
    return valid;
}

//! Enables or disables the "previous movie" button.
void MvdMovieEditor::setPreviousEnabled(bool enabled)
{
    d->mPreviousButton->setEnabled(enabled);
}

//! Enables or disables the "next movie" button.
void MvdMovieEditor::setNextEnabled(bool enabled)
{
    d->mNextButton->setEnabled(enabled);
}

//! Enables or disables the Ok button.
void MvdMovieEditor::validationStateChanged(MvdMPDialogPage *page)
{
    Q_ASSERT(page);
    Q_UNUSED(page);

    bool valid = isValid();
    d->mOkButton->setEnabled(valid);
}

//! Enables or disables the some UI elements.
void MvdMovieEditor::modifiedStateChanged(MvdMPDialogPage *page)
{
    Q_ASSERT(page);
    Q_UNUSED(page);

    //bool modified = isModified();
}

//!
void MvdMovieEditor::currentPageChanged(MvdMPDialogPage *page)
{
    Q_ASSERT(page);
}
