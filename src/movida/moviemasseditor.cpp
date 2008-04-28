/**************************************************************************
** Filename: moviemasseditor.cpp
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

#include "moviemasseditor.h"
#include "mvdcore/core.h"
#include "mvdcore/movie.h"
#include "mvdcore/moviecollection.h"
#include <QMessageBox>
#include <QCloseEvent>
#include <QPushButton>
#include <QMessageBox>

/*!
	\class MvdMovieMassEditor moviemasseditor.h
	\ingroup Movida

	\brief Convenience widget for editing multiple movies at once.
*/


/*!
	Creates a new dialog.
*/
MvdMovieMassEditor::MvdMovieMassEditor(MvdMovieCollection* c, QWidget* parent)
: QDialog(parent), mCollection(c)
{
	setupUi(this);

	//! \todo set a better title (and update on setMovie() or title editing)
	setWindowTitle(tr("movida movie mass editor"));

	Ui::MvdMovieMassEditor::ratingLabel->setIcon( QIcon(":/images/rating.svgz") );
	Ui::MvdMovieMassEditor::markAsSeen->setIcon(QIcon(":/images/seen.svgz"));
	Ui::MvdMovieMassEditor::markAsSpecial->setIcon(QIcon(":/images/special.svgz"));
	Ui::MvdMovieMassEditor::markAsLoaned->setIcon(QIcon(":/images/loaned.svgz"));

	quint8 maxRating = MvdCore::parameter("mvdcore/max-rating").toUInt();
	Ui::MvdMovieMassEditor::ratingLabel->setMaximum(maxRating);

	ratingHovered(-1);

	connect (Ui::MvdMovieMassEditor::ratingStatus, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)) );
	connect (Ui::MvdMovieMassEditor::ratingLabel, SIGNAL(hovered(int)), this, SLOT(ratingHovered(int)) );

	connect( Ui::MvdMovieMassEditor::buttonBox, SIGNAL(rejected()), this, SLOT(cancelTriggered()) );
	connect( Ui::MvdMovieMassEditor::buttonBox, SIGNAL(accepted()), this, SLOT(storeTriggered()) );

	connect( Ui::MvdMovieMassEditor::cbStorageID, SIGNAL(toggled(bool)), this, SLOT(updateUi()) );
}

/*!
	Sets the movies to be edited.
	Only few movie attributes allow for mass editing, e.g. the storage ID or the
	"mark as" flags. This method will attempt to populate the GUI with significant
	default values for every attribute.
*/
bool MvdMovieMassEditor::setMovies(const QList<mvdid>& ids)
{
	if (mCollection == 0)
		return false;

	Ui::MvdMovieMassEditor::markAsLoaned->setCheckState(Qt::PartiallyChecked);
	Ui::MvdMovieMassEditor::markAsSeen->setCheckState(Qt::PartiallyChecked);
	Ui::MvdMovieMassEditor::markAsSpecial->setCheckState(Qt::PartiallyChecked);

	foreach (mvdid id, ids) {
		MvdMovie movie = mCollection->movie(id);
		QString sid = movie.storageId();
		if (!sid.isEmpty() && Ui::MvdMovieMassEditor::storageID->text().isEmpty())
			Ui::MvdMovieMassEditor::storageID->setText(sid);
		quint8 rat = movie.rating();
		if (rat && !Ui::MvdMovieMassEditor::ratingLabel->value())
			Ui::MvdMovieMassEditor::ratingLabel->setValue(rat);
	}

	mMovieIds= ids;
	return true;
}

void MvdMovieMassEditor::cancelTriggered()
{
	reject();
}

//! \internal \todo Handle ESC key
void MvdMovieMassEditor::closeEvent(QCloseEvent* e)
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
	foreach (mvdid id, mMovieIds) {
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
			movie.setSpecialTagEnabled(MvdMovie::LoanedTag, Ui::MvdMovieMassEditor::markAsLoaned->isChecked());
			m = true;
		}
		if (Ui::MvdMovieMassEditor::markAsSeen->checkState() != Qt::PartiallyChecked) {
			movie.setSpecialTagEnabled(MvdMovie::SeenTag, Ui::MvdMovieMassEditor::markAsSeen->isChecked());
			m = true;
		}
		if (Ui::MvdMovieMassEditor::markAsSpecial->checkState() != Qt::PartiallyChecked) {
			movie.setSpecialTagEnabled(MvdMovie::SpecialTag, Ui::MvdMovieMassEditor::markAsSpecial->isChecked());
			m = true;
		}

		if (m)
			mCollection->updateMovie(id, movie);
	}
	
	return true;
}

//! \internal
void MvdMovieMassEditor::linkActivated(const QString& url)
{
	MvdCore::LabelAction a = MvdCore::parseLabelAction(url);
	if (!a.isValid())
		return;

	if (a.action == "clear")
	{
		if (a.parameter == "rating")
		{
			ratingLabel->setValue(0);
			// Update status text
			ratingHovered(-1);
		}
	}
}

void MvdMovieMassEditor::ratingHovered(int rating)
{
	switch (rating)
	{
	case 1: ratingStatus->setText(QString("1 - %1.").arg(MvdMovie::ratingTip(rating))); break;
	case 2: ratingStatus->setText(QString("2 - %1.").arg(MvdMovie::ratingTip(rating))); break;
	case 3: ratingStatus->setText(QString("3 - %1.").arg(MvdMovie::ratingTip(rating))); break;
	case 4: ratingStatus->setText(QString("4 - %1.").arg(MvdMovie::ratingTip(rating))); break;
	case 5: ratingStatus->setText(QString("5 - %1.").arg(MvdMovie::ratingTip(rating))); break;
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
