/**************************************************************************
** Filename: maininfopage.cpp
**
** Copyright (C) 2007-2008 Angius Fabrizio. All rights reserved.
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

#include "maininfopage.h"
#include "guiglobal.h"
#include "multipagedialog.h"
#include "mvdcore/core.h"
#include "mvdcore/movie.h"
#include <QIcon>
#include <QMessageBox>
#include <QDate>
#include <QRegExp>
#include <QFileDialog>
#include <QMimeData>
#include <QTimer>

/*!
	\class MvdMainInfoPage maininfopage.h
	\ingroup Movida

	\brief Main movie info widget for movie editing dialog.
*/

/*!
	Creates a new page.
*/
MvdMainInfoPage::MvdMainInfoPage(MvdMovieCollection* c, MvdMovieEditor* parent)
: MvdMovieEditorPage(c, parent), mDefaultRunningTime(0), mDefaultRating(0), mStatusTimer(new QTimer(this)),
mDefaultSpecialTags(0)
{
	setupUi(this);
	int w = MvdCore::parameter("movida/poster-default-width").toInt();
	qreal ar = MvdCore::parameter("movida/poster-aspect-ratio").toDouble();
	int h = int(w / ar);
	Ui::MvdMainInfoPage::poster->setFixedSize(w, h);

	Ui::MvdMainInfoPage::ratingLabel->setIcon( QIcon(":/images/rating.svgz") );
	
	quint8 maxRating = MvdCore::parameter("mvdcore/max-rating").toUInt();
	quint16 maxRuntime = MvdCore::parameter("mvdcore/max-running-time").toUInt();

	Ui::MvdMainInfoPage::ratingLabel->setMaximum(maxRating);

	Ui::MvdMainInfoPage::lengthMinutes->setMaximum(maxRuntime);
	Ui::MvdMainInfoPage::lengthMinutes->setMinimum(0);
	Ui::MvdMainInfoPage::lengthMinutes->setSpecialValueText("-");

	QDate date = QDate::currentDate();

	int minYear = MvdCore::parameter("mvdcore/min-movie-year").toUInt() - 1;

	mDefaultPYear = mDefaultRYear = minYear;

	Ui::MvdMainInfoPage::productionYear->setMinimum(minYear);
	Ui::MvdMainInfoPage::productionYear->setMaximum(date.year());
	Ui::MvdMainInfoPage::productionYear->setSpecialValueText("-");

	Ui::MvdMainInfoPage::releaseYear->setMaximum(date.year());
	Ui::MvdMainInfoPage::releaseYear->setMinimum(minYear);
	Ui::MvdMainInfoPage::releaseYear->setSpecialValueText("-");

	ratingHovered(-1);
	setMoviePoster();

	markAsSeen->setIcon(QIcon(":/images/seen.svgz"));
	markAsSpecial->setIcon(QIcon(":/images/special.svgz"));
	markAsLoaned->setIcon(QIcon(":/images/loaned.svgz"));

	// Set max length for line edits
	int maxInputLength = MvdCore::parameter("mvdcore/max-edit-length").toInt();
	Ui::MvdMainInfoPage::title->setMaxLength(maxInputLength);
	Ui::MvdMainInfoPage::originalTitle->setMaxLength(maxInputLength);
	Ui::MvdMainInfoPage::version->setMaxLength(maxInputLength);
	Ui::MvdMainInfoPage::storageID->setMaxLength(maxInputLength);

	connect (Ui::MvdMainInfoPage::clearProductionYear, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)) );
	connect (Ui::MvdMainInfoPage::clearReleaseYear, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)) );
	connect (Ui::MvdMainInfoPage::clearRunningTime, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)) );
	connect (Ui::MvdMainInfoPage::ratingStatus, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)) );
	connect (Ui::MvdMainInfoPage::ratingLabel, SIGNAL(hovered(int)), this, SLOT(ratingHovered(int)) );
	connect (Ui::MvdMainInfoPage::poster, SIGNAL(clicked()), this, SLOT(selectMoviePoster()) );
	connect (Ui::MvdMainInfoPage::posterStatus, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)) );

	connect (Ui::MvdMainInfoPage::title, SIGNAL(textEdited(const QString&)), this, SLOT(validate()) );
	connect (Ui::MvdMainInfoPage::originalTitle, SIGNAL(textEdited(const QString&)), this, SLOT(validate()) );

	connect (Ui::MvdMainInfoPage::title, SIGNAL(textEdited(QString)), this, SLOT(updateModifiedStatus()) );
	connect (Ui::MvdMainInfoPage::originalTitle, SIGNAL(textEdited(QString)), this, SLOT(updateModifiedStatus()) );
	connect (Ui::MvdMainInfoPage::version, SIGNAL(textEdited(QString)), this, SLOT(updateModifiedStatus()) );
	connect (Ui::MvdMainInfoPage::storageID, SIGNAL(textEdited(QString)), this, SLOT(updateModifiedStatus()) );
	connect (Ui::MvdMainInfoPage::productionYear, SIGNAL(valueChanged(int)), this, SLOT(updateModifiedStatus()) );
	connect (Ui::MvdMainInfoPage::releaseYear, SIGNAL(valueChanged(int)), this, SLOT(updateModifiedStatus()) );
	connect (Ui::MvdMainInfoPage::lengthMinutes, SIGNAL(valueChanged(int)), this, SLOT(updateModifiedStatus()) );
	connect (Ui::MvdMainInfoPage::ratingLabel, SIGNAL(valueChanged(int)), this, SLOT(updateModifiedStatus()) );
	connect (Ui::MvdMainInfoPage::markAsSeen, SIGNAL(toggled(bool)), this, SLOT(updateModifiedStatus()) );
	connect (Ui::MvdMainInfoPage::markAsSpecial, SIGNAL(toggled(bool)), this, SLOT(updateModifiedStatus()) );
	connect (Ui::MvdMainInfoPage::markAsLoaned, SIGNAL(toggled(bool)), this, SLOT(updateModifiedStatus()) );

	Ui::MvdMainInfoPage::poster->setDragAndDropHandler(this, "posterDragEntered", "posterDropped", "resetPosterStatus");

	mStatusTimer->setSingleShot(true);
	connect(mStatusTimer, SIGNAL(timeout()), this, SLOT(statusTimeout()));

	setValid(false);
	validate();
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
	return QIcon();
}

/*!
	Sets data from a single movie.
*/
void MvdMainInfoPage::setMovieImpl(const MvdMovie& movie)
{	
	//! \todo handle change line edit bgcolor if o_title is set from loc_title

	mDefaultTitle = movie.title();
	title->setText(mDefaultTitle);
	mDefaultOriginalTitle = movie.originalTitle();
	originalTitle->setText(mDefaultOriginalTitle);
	mDefaultEdition = movie.edition();
	version->setText(mDefaultEdition);
	mDefaultStorageId = movie.storageId();
	storageID->setText(mDefaultStorageId);

	markAsSeen->setChecked(movie.hasSpecialTagEnabled(MvdMovie::SeenTag));
	markAsSpecial->setChecked(movie.hasSpecialTagEnabled(MvdMovie::SpecialTag));
	markAsLoaned->setChecked(movie.hasSpecialTagEnabled(MvdMovie::LoanedTag));
	mDefaultSpecialTags = movie.specialTags();
	
	int minYear = MvdCore::parameter("mvdcore/min-movie-year").toUInt() - 1;

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

	if (mDefaultPoster.isEmpty())
		setMoviePoster();
	else setMoviePoster(mCollection->metaData(MvdMovieCollection::DataPathInfo).append("/images/").append(mDefaultPoster));

	validate();
	setModified(false);
}

bool MvdMainInfoPage::store(MvdMovie& movie)
{
	if (title->text().isEmpty() && originalTitle->text().isEmpty())
	{
		QMessageBox::warning(this, MVD_CAPTION, tr("Please specify a title for this movie."));
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
	movie.setSpecialTagEnabled(MvdMovie::SeenTag, markAsSeen->isChecked());
	movie.setSpecialTagEnabled(MvdMovie::SpecialTag, markAsSpecial->isChecked());
	movie.setSpecialTagEnabled(MvdMovie::LoanedTag, markAsLoaned->isChecked());

	if (!mPosterPath.isEmpty())
	{
		QString posterName = mCollection->addImage(mPosterPath, 
			MvdMovieCollection::MoviePosterImage);
	
		if (posterName.isEmpty())
		{
			if (QMessageBox::question(this, MVD_CAPTION, tr("Sorry, the movie poster could not be imported. Continue?"), 
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
	//! \bug Temporary bug fix. Trolltech task tracker #172324, scheduled for Qt 4.3.2 (http://trolltech.com/developer/task-tracker/index_html?method=entry&id=172324)
	Q_ASSERT(QMetaObject::invokeMethod(this, "doLinkActivated", Qt::QueuedConnection, Q_ARG(QString, url)));
}

//! \internal
void MvdMainInfoPage::doLinkActivated(const QString& url)
{
	MvdCore::LabelAction a = MvdCore::parseLabelAction(url);
	if (!a.isValid())
		return;

	if (a.action == "clear")
	{
		if (a.parameter == "productionyear")
			productionYear->setValue(productionYear->minimum());
		else if (a.parameter == "releaseyear")
			releaseYear->setValue(releaseYear->minimum());
		else if (a.parameter == "runningtime")
			lengthMinutes->setValue(lengthMinutes->minimum());
		else if (a.parameter == "rating")
		{
			ratingLabel->setValue(0);
			// Update status text
			ratingHovered(-1);
		}
		else if (a.parameter == "poster")
			setMoviePoster();
	}
	else if (a.action == "select")
	{
		if (a.parameter == "poster")
			selectMoviePoster();
	}
}

void MvdMainInfoPage::ratingHovered(int rating)
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

void MvdMainInfoPage::selectMoviePoster()
{
	//! \todo Remember last used dir
	QString filter = tr("Supported image files (%1)").arg("*.bmp *.jpg *.jpeg *.png");
	QString path = QFileDialog::getOpenFileName(this, tr("Select a movie poster"), QString(), filter);
	if (path.isEmpty())
		return;

	setMoviePoster(path);
	updateModifiedStatus();
}

void MvdMainInfoPage::setMoviePoster(const QString& path)
{
	bool posterOk = false;
	mPosterPath.clear();

	if (!path.isEmpty() && QFile::exists(path))
	{
		if (poster->setPoster(path))
		{
			mPosterPath = path;
			posterOk = true;
		}
	}

	if (!posterOk)
	{
		poster->setPoster(":/images/default-poster.png");

		if (!path.isEmpty())
		{
			posterStatus->setText(tr("Failed to load image file."));
			mStatusTimer->start(MvdCore::parameter("movida/message-timeout-ms").toUInt());
			return;
		}
	}

	resetPosterStatus();
	updateModifiedStatus();
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

void MvdMainInfoPage::statusTimeout()
{
	resetPosterStatus();
}

void MvdMainInfoPage::validate()
{
	QString titleString = title->text().trimmed();
	QString otitleString = originalTitle->text().trimmed();

	bool invalid = titleString.isEmpty() && otitleString.isEmpty();

	QPalette p = palette();
	if (invalid)
		p.setColor(QPalette::Normal, QPalette::Base, QColor("#ede055"));
	title->setPalette(p);
	originalTitle->setPalette(p);

	setValid(!invalid);

	if (dialog())
		dialog()->setSubtitle(invalid ? tr("Invalid movie!") : titleString.isEmpty() ? otitleString : titleString);
}

//!
void MvdMainInfoPage::updateModifiedStatus()
{
	bool posterChanged = false;
	if (!mPosterPath.isEmpty())
	{
		QFileInfo fi(mPosterPath);
		posterChanged = fi.fileName() != mDefaultPoster;
	}
	else
		posterChanged = mPosterPath != mDefaultPoster;

	int currentSpecialTags = MvdMovie::NoTag;
	if (markAsSeen->isChecked()) currentSpecialTags |= MvdMovie::SeenTag;
	if (markAsSpecial->isChecked()) currentSpecialTags |= MvdMovie::SpecialTag;
	if (markAsLoaned->isChecked()) currentSpecialTags |= MvdMovie::LoanedTag;

	if (title->text().trimmed() != mDefaultTitle
		|| originalTitle->text().trimmed() != mDefaultOriginalTitle
		|| version->text().trimmed() != mDefaultEdition
		|| storageID->text().trimmed() != mDefaultStorageId
		|| productionYear->value() != mDefaultPYear
		|| releaseYear->value() != mDefaultRYear
		|| lengthMinutes->value() != mDefaultRunningTime
		|| ratingLabel->value() != mDefaultRating
		|| posterChanged 
		|| currentSpecialTags != mDefaultSpecialTags)
	{
		setModified(true);
		return;
	}

	setModified(false);
}

//!
void MvdMainInfoPage::setMainWidgetFocus()
{
	title->setFocus(Qt::ActiveWindowFocusReason);
}
