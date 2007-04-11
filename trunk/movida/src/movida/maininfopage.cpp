/**************************************************************************
** Filename: maininfopage.cpp
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

#include "maininfopage.h"
#include "guiglobal.h"
#include "core.h"
#include "movie.h"

#include <QIcon>
#include <QMessageBox>
#include <QDate>
#include <QRegExp>
#include <QFileDialog>
#include <QMimeData>

/*!
	\class MvdMainInfoPage maininfopage.h

	\brief Main movie info widget for movie editing dialog.
*/

/*!
	Creates a new page.	
*/
MvdMainInfoPage::MvdMainInfoPage(MvdMovieCollection* c, QWidget* parent)
: MvdMovieEditorPage(c, parent), mDefaultRunningTime(0),
mDefaultRating(0)
{
	setupUi(this);

	ratingLabel->setPixmap( MvdRatingWidget::RatedRole, QPixmap(":/images/misc/rating-rated.png") );
	ratingLabel->setPixmap( MvdRatingWidget::UnratedRole, QPixmap(":/images/misc/rating-unrated.png") );
	ratingLabel->setPixmap( MvdRatingWidget::HoveredRole, QPixmap(":/images/misc/rating-hovered.png") );
	
	quint8 maxRating = MvdCore::parameter("movidacore-max-rating").toUInt();
	quint16 maxRuntime = MvdCore::parameter("movidacore-max-running-time").toUInt();

	ratingLabel->setMaximum( maxRating );
	lengthMinutes->setMaximum( maxRuntime );

	QDate date = QDate::currentDate();

	int minYear = MvdCore::parameter("movidacore-min-movie-year").toUInt() - 1;

	mDefaultPYear = mDefaultRYear = minYear;

	productionYear->setMinimum(minYear);
	productionYear->setMaximum(date.year());
	productionYear->setSpecialValueText("-");

	releaseYear->setMaximum(date.year());
	releaseYear->setMinimum(minYear);
	releaseYear->setSpecialValueText("-");

	ratingHovered(-1);

	setMoviePoster();

	connect (clearProductionYear, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)) );
	connect (clearReleaseYear, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)) );
	connect (clearRunningTime, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)) );
	connect (ratingStatus, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)) );
	connect (ratingLabel, SIGNAL(hovered(int)), this, SLOT(ratingHovered(int)) );
	connect (poster, SIGNAL(clicked()), this, SLOT(selectMoviePoster()) );
	connect (posterStatus, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)) );
	
	poster->setDragAndDropHandler(this, "posterDragEntered", "posterDropped", "resetPosterStatus");
}

/*!
	Reset to default values.
*/
void MvdMainInfoPage::reset()
{
}

/*!
	Returns the title to be used for this page.
*/
QString MvdMainInfoPage::label()
{
	return tr("Main info");
}

/*!
	Returns the icon to be used for this page.
*/
QIcon MvdMainInfoPage::icon()
{
	return QIcon(":/images/preferences/log.png");
}

bool MvdMainInfoPage::isModified() const
{
	bool posterChanged = false;
	if (!mPosterPath.isEmpty())
	{
		QFileInfo fi(mPosterPath);
		posterChanged = fi.fileName() != mDefaultPoster;
	}
	else
		posterChanged = mPosterPath != mDefaultPoster;

	return title->isModified() 
		|| originalTitle->isModified() 
		|| version->isModified()
		|| productionYear->value() != mDefaultPYear
		|| releaseYear->value() != mDefaultRYear
		|| lengthMinutes->value() != mDefaultRunningTime
		|| storageID->isModified()
		|| ratingLabel->value() != mDefaultRating
		|| posterChanged;
}

/*!
	Sets data from multiple movies.
*/
void MvdMainInfoPage::setMoviesImpl(const QList<MvdMovie>& movies)
{
}

/*!
	Sets data from a single movie.
*/
void MvdMainInfoPage::setMovieImpl(const MvdMovie& movie)
{	
	//! \todo handle change line edit bgcolor if o_title is set from loc_title

	title->setText(movie.title());
	originalTitle->setText(movie.originalTitle());
	version->setText(movie.edition());
	storageID->setText(movie.storageId());
	
	int minYear = MvdCore::parameter("movidacore-min-movie-year").toUInt() - 1;

	//! \todo convert year values to uint
	QString s = movie.productionYear();
	mDefaultPYear = s.isEmpty() ? minYear : s.toUShort();
	productionYear->setValue(mDefaultPYear);

	s = movie.releaseYear();
	mDefaultRYear = s.isEmpty() ? minYear : s.toUShort();
	releaseYear->setValue(mDefaultRYear);

	mDefaultRunningTime = movie.runningTime();
	lengthMinutes->setValue(mDefaultRunningTime);

	mDefaultRating = movie.rating();
	ratingLabel->setValue(mDefaultRating);

	// Set the appropriate status text.
	ratingHovered(-1);

	mDefaultPoster = movie.poster();

	setMoviePoster(mCollection->info(MvdMovieCollection::DataPathInfo)
		.append("/images/").append(mDefaultPoster));
}

bool MvdMainInfoPage::store(MvdMovie& movie)
{
	if (title->text().isEmpty() && originalTitle->text().isEmpty())
	{
		QMessageBox::warning(this, _CAPTION_, tr("Please specify a title for this movie."));
		if (title->text().isEmpty())
			title->setFocus(Qt::OtherFocusReason);
		else originalTitle->setFocus(Qt::OtherFocusReason);
		return false;
	}

	movie.setTitle(title->text());
	movie.setOriginalTitle(originalTitle->text());
	movie.setEdition(version->text());
	movie.setStorageId(storageID->text());
	movie.setProductionYear(QString::number(productionYear->value()));
	movie.setReleaseYear(QString::number(releaseYear->value()));
	movie.setRunningTime(lengthMinutes->value());
	movie.setRating(ratingLabel->value());

	if (!mPosterPath.isEmpty())
	{
		QString posterName = mCollection->addImage(mPosterPath, 
			MvdMovieCollection::MoviePosterImage);
	
		if (posterName.isEmpty())
		{
			if (QMessageBox::question(this, _CAPTION_, tr("Sorry, the movie poster could not be imported. Continue?"), 
				QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes)
				return false;
		}

		movie.setPoster(posterName);
	}
	else movie.setPoster(QString());

	return true;
}

//! \internal
void MvdMainInfoPage::linkActivated(const QString& url)
{
	if (!url.startsWith("movida://"))
		return;

	QRegExp rx("^movida://([^/]*)/?([^/]*)?$");
	if (rx.indexIn(url) >= 0)
	{
		QString op = rx.cap(1);
		QString param = rx.cap(2);

		if (op == "clear")
		{
			if (param == "productionyear")
				productionYear->setValue(productionYear->minimum());
			else if (param == "releaseyear")
				releaseYear->setValue(releaseYear->minimum());
			else if (param == "runningtime")
				lengthMinutes->setValue(lengthMinutes->minimum());
			else if (param == "rating")
			{
				ratingLabel->setValue(0);
				// Update status text
				ratingHovered(-1);
			}
			else if (param == "poster")
				setMoviePoster();
		}
		else if (op == "select")
		{
			if (param == "poster")
				selectMoviePoster();
		}
	}
}

void MvdMainInfoPage::ratingHovered(int rating)
{
	switch (rating)
	{
	case 1: ratingStatus->setText(tr("1 - Awful movie.")); break;
	case 2: ratingStatus->setText(tr("2 - Disappointing movie.")); break;
	case 3: ratingStatus->setText(tr("3 - Mediocre movie.")); break;
	case 4: ratingStatus->setText(tr("4 - Good movie.")); break;
	case 5: ratingStatus->setText(tr("5 - Outstanding movie.")); break;
	default: 
		ratingLabel->value() ? 
			ratingStatus->setText(tr("Click to set the desired rating or <a href='movida://clear/rating'>click here</a> to clear it.")) :
			ratingStatus->setText(tr("Click to set the desired rating."));
	}
}

void MvdMainInfoPage::selectMoviePoster()
{
	//! \todo Remember last used dir
	QString filter = tr("Supported image files (%1)").arg("*.bmp *.jpg *.jpeg *.png");
	QString path = QFileDialog::getOpenFileName(this, tr("Select a movie poster"), QString(), filter);
	if (path.isEmpty())
		return;
	setMoviePoster(path);
}

void MvdMainInfoPage::setMoviePoster(const QString& path)
{
	bool posterOk = false;
	mPosterPath.clear();

	if (!path.isEmpty() && QFile::exists(path))
	{
		QPixmap pm(path);
		if (!pm.isNull())
		{
			pm = pm.scaled(poster->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			poster->setPixmap(pm);
			mPosterPath = path;
			posterOk = true;
		}
	}

	if (!posterOk)
	{
		QPixmap posterPixmap = QPixmap(":/images/misc/default-poster")
			.scaled(poster->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		poster->setPixmap(posterPixmap);
	}

	resetPosterStatus();
}

//! Returns true if the drag contains image data that can be used as a poster.
bool MvdMainInfoPage::posterDragEntered(const QMimeData& mimeData) const
{
	bool accept = false;

	if (mimeData.hasUrls())
	{
		QList<QUrl> list = mimeData.urls();
		for (int i = 0; i < list.size(); ++i)
		{
			QString path = list.at(i).toLocalFile();
			if (!path.isEmpty())
			{
				QFileInfo file(path);
				if (!file.exists())
					continue;
				QString ext = file.suffix().toLower();
				if (ext == "bmp" || ext == "jpg" || ext == "jpeg" || ext == "png")
				{
					accept = true;
					break;
				}
			}
		}
	}
	else if (mimeData.hasText()) {
		QFileInfo file(mimeData.text());
		if (file.exists())
		{
			QString ext = file.suffix().toLower();
			if (ext == "bmp" || ext == "jpg" || ext == "jpeg" || ext == "png")
				accept = true;
		}
	}

	if (accept)
		posterStatus->setText(tr("Drop to set the image as movie poster."));
	return accept;
}

//! Returns true if the drag contains image data that can be used as a poster.
bool MvdMainInfoPage::posterDropped(const QMimeData& mimeData)
{
	QString posterPath;

	if (mimeData.hasUrls())
	{
		QList<QUrl> list = mimeData.urls();
		for (int i = 0; i < list.size(); ++i)
		{
			QString path = list.at(i).toLocalFile();
			if (!path.isEmpty())
			{
				QFileInfo file(path);
				if (!file.exists())
					continue;
				QString ext = file.suffix().toLower();
				if (ext == "bmp" || ext == "jpg" || ext == "jpeg" || ext == "png")
				{
					posterPath = file.absoluteFilePath();
					break;
				}
			}
		}
	}
	else if (mimeData.hasText()) {
		QFileInfo file(mimeData.text());
		if (file.exists())
		{
			QString ext = file.suffix().toLower();
			if (ext == "bmp" || ext == "jpg" || ext == "jpeg" || ext == "png")
				posterPath = file.absoluteFilePath();
		}
	}

	if (!posterPath.isEmpty())
		setMoviePoster(posterPath);

	return !posterPath.isEmpty();
}

void MvdMainInfoPage::resetPosterStatus()
{
	QString noPosterString = tr("<a href='movida://select/poster'>Set</a> a movie poster");
	QString hasPosterString = tr("<a href='movida://clear/poster'>Remove</a> or <a href='movida://select/poster'>change</a> the poster");
	posterStatus->setText(mPosterPath.isEmpty() ? noPosterString : hasPosterString);
}
