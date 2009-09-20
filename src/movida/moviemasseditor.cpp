/**************************************************************************
** Filename: moviemasseditor.cpp
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

#include "moviemasseditor.h"

#include "maininfopage.h"

#include "mvdcore/core.h"
#include "mvdcore/movie.h"
#include "mvdcore/moviecollection.h"

#include <QtGui/QCloseEvent>
#include <QtGui/QMessageBox>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>

/*!
    \class MvdMovieMassEditor moviemasseditor.h
    \ingroup Movida

    \brief Convenience widget for editing multiple movies at once.
*/


class MvdMovieMassEditor::Private
{
public:
    Private(MvdMovieMassEditor *p) :
        q(p)
    {

    }

private:
    MvdMovieMassEditor *q;
};


//////////////////////////////////////////////////////////////////////////


/*!
    Creates a new dialog.
*/
MvdMovieMassEditor::MvdMovieMassEditor(MvdMovieCollection *c, QWidget *parent) :
    MvdMovieEditor(c, parent),
    d(new Private(this))
{
#if 0
    setupUi(this);

    //! \todo set a better title (and update on setMovie() or title editing)
    setWindowTitle(tr("movida movie mass editor"));

    Ui::MvdMovieMassEditor::ratingLabel->setIcon(QIcon(":/images/rating.svgz"));
    Ui::MvdMovieMassEditor::markAsSeen->setIcon(QIcon(":/images/seen.svgz"));
    Ui::MvdMovieMassEditor::markAsSpecial->setIcon(QIcon(":/images/special.svgz"));
    Ui::MvdMovieMassEditor::markAsLoaned->setIcon(QIcon(":/images/loaned.svgz"));

    quint8 maxRating = Movida::core().parameter("mvdcore/max-rating").toUInt();
    Ui::MvdMovieMassEditor::ratingLabel->setMaximum(maxRating);

    ratingHovered(-1);

    connect(Ui::MvdMovieMassEditor::ratingStatus, SIGNAL(linkActivated(const QString &)), this, SLOT(linkActivated(const QString &)));
    connect(Ui::MvdMovieMassEditor::ratingLabel, SIGNAL(hovered(int)), this, SLOT(ratingHovered(int)));

    connect(Ui::MvdMovieMassEditor::buttonBox, SIGNAL(rejected()), this, SLOT(cancelTriggered()));
    connect(Ui::MvdMovieMassEditor::buttonBox, SIGNAL(accepted()), this, SLOT(storeTriggered()));

    connect(Ui::MvdMovieMassEditor::cbStorageID, SIGNAL(toggled(bool)), this, SLOT(updateUi()));

    Ui::MvdMovieMassEditor::markAsLoaned->setTristate(false);
    Ui::MvdMovieMassEditor::markAsSeen->setTristate(false);
    Ui::MvdMovieMassEditor::markAsSpecial->setTristate(false);
#endif
}

MvdMovieMassEditor::~MvdMovieMassEditor()
{
    delete d;
}

void MvdMovieMassEditor::loadPages()
{

}

bool MvdMovieMassEditor::setMovies(const QList<mvdid> &ids, bool confirmIfModified)
{
    initialize();
    
    addPage(new MvdMainInfoPage(collection(), this));

    return false;
}

QList<mvdid> MvdMovieMassEditor::movieIds() const
{
    return QList<mvdid>();
}

bool MvdMovieMassEditor::storeMovies()
{
    return false;
}

void MvdMovieMassEditor::closeEvent(QCloseEvent *e)
{
    MvdMovieEditor::closeEvent(e);
}

void MvdMovieMassEditor::validationStateChanged(MvdMPDialogPage *page)
{
    MvdMovieEditor::validationStateChanged(page);
}

void MvdMovieMassEditor::modifiedStateChanged(MvdMPDialogPage *page)
{
    MvdMovieEditor::modifiedStateChanged(page);
}

#if 0

/*!
    Sets the movies to be edited.
    Only few movie attributes allow for mass editing, e.g. the storage ID or the
    "mark as" flags. This method will attempt to populate the GUI with significant
    default values for every attribute.
*/
bool MvdMovieMassEditor::setMovies(const QList<mvdid> &ids)
{
    if (mCollection == 0)
        return false;

    int loaned = 0;
    int seen = 0;
    int special = 0;
    const int count = ids.size();
    
    for (int i = 0; i < count; ++i)
    {
        mvdid id = ids.at(i);
        MvdMovie movie = mCollection->movie(id);
        QString sid = movie.storageId();

        if (!sid.isEmpty() && Ui::MvdMovieMassEditor::storageID->text().isEmpty())
            Ui::MvdMovieMassEditor::storageID->setText(sid);
        quint8 rat = movie.rating();
        if (rat && !Ui::MvdMovieMassEditor::ratingLabel->value())
            Ui::MvdMovieMassEditor::ratingLabel->setValue(rat);
        if (movie.hasSpecialTagEnabled(Movida::LoanedTag))
            ++loaned;
        if (movie.hasSpecialTagEnabled(Movida::SeenTag))
            ++seen;
        if (movie.hasSpecialTagEnabled(Movida::SpecialTag))
            ++special;
    }

    Ui::MvdMovieMassEditor::markAsLoaned->setCheckState(loaned == count 
        ? Qt::Checked 
        : loaned == 0 ? Qt::Unchecked
        : Qt::PartiallyChecked);
    Ui::MvdMovieMassEditor::markAsSeen->setCheckState(seen == count 
        ? Qt::Checked 
        : seen == 0 ? Qt::Unchecked
        : Qt::PartiallyChecked);
    Ui::MvdMovieMassEditor::markAsSpecial->setCheckState(special == count 
        ? Qt::Checked 
        : special == 0 ? Qt::Unchecked
        : Qt::PartiallyChecked);

    mMovieIds = ids;
    return true;
}

void MvdMovieMassEditor::cancelTriggered()
{
    reject();
}

//! \internal \todo Handle ESC key
void MvdMovieMassEditor::closeEvent(QCloseEvent *e)
{
    reject();
    e->accept();
}

void MvdMovieMassEditor::storeTriggered()
{
    if (mCollection == 0)
        return;

    if (!storeMovies())
        return;

    accept();
}

//! \internal Simply stores the changes made to the movies.
bool MvdMovieMassEditor::storeMovies()
{
    foreach(mvdid id, mMovieIds)
    {
        MvdMovie movie = mCollection->movie(id);

        if (!movie.isValid())
            continue;

        bool m = false;

        if (Ui::MvdMovieMassEditor::cbStorageID->isChecked()) {
            movie.setStorageId(Ui::MvdMovieMassEditor::storageID->text());
            m = true;
        }
        if (Ui::MvdMovieMassEditor::ratingBox->isChecked()) {
            movie.setRating(Ui::MvdMovieMassEditor::ratingLabel->value());
            m = true;
        }
        if (Ui::MvdMovieMassEditor::markAsLoaned->checkState() != Qt::PartiallyChecked) {
            movie.setSpecialTagEnabled(Movida::LoanedTag, Ui::MvdMovieMassEditor::markAsLoaned->isChecked());
            m = true;
        }
        if (Ui::MvdMovieMassEditor::markAsSeen->checkState() != Qt::PartiallyChecked) {
            movie.setSpecialTagEnabled(Movida::SeenTag, Ui::MvdMovieMassEditor::markAsSeen->isChecked());
            m = true;
        }
        if (Ui::MvdMovieMassEditor::markAsSpecial->checkState() != Qt::PartiallyChecked) {
            movie.setSpecialTagEnabled(Movida::SpecialTag, Ui::MvdMovieMassEditor::markAsSpecial->isChecked());
            m = true;
        }

        if (m)
            mCollection->updateMovie(id, movie);
    }

    return true;
}

//! \internal
void MvdMovieMassEditor::linkActivated(const QString &url)
{
    MvdActionUrl a = MvdCore::parseActionUrl(url);

    if (!a.isValid())
        return;

    if (a.action == "clear") {
        if (a.parameter == "rating") {
            ratingLabel->setValue(0);
            // Update status text
            ratingHovered(-1);
        }
    }
}

void MvdMovieMassEditor::ratingHovered(int rating)
{
    switch (rating) {
        case 1:
            ratingStatus->setText(QString("1 - %1.").arg(MvdMovie::ratingTip(rating))); break;

        case 2:
            ratingStatus->setText(QString("2 - %1.").arg(MvdMovie::ratingTip(rating))); break;

        case 3:
            ratingStatus->setText(QString("3 - %1.").arg(MvdMovie::ratingTip(rating))); break;

        case 4:
            ratingStatus->setText(QString("4 - %1.").arg(MvdMovie::ratingTip(rating))); break;

        case 5:
            ratingStatus->setText(QString("5 - %1.").arg(MvdMovie::ratingTip(rating))); break;

        default:
            ratingLabel->value() ?
            ratingStatus->setText(tr("Click to set the desired rating or <a href='movida://clear/rating'>click here</a> to clear it.")) :
            ratingStatus->setText(tr("Click to set the desired rating."));
    }
}

void MvdMovieMassEditor::updateUi()
{
    Ui::MvdMovieMassEditor::storageID->setEnabled(Ui::MvdMovieMassEditor::cbStorageID->isChecked());
}
#endif