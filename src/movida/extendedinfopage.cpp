/**************************************************************************
** Filename: extendedinfopage.cpp
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

#include "extendedinfopage.h"
#include "guiglobal.h"
#include "mvdcore/movie.h"
#include "mvdcore/moviecollection.h"
#include "mvdcore/core.h"
#include <QIcon>
#include <QList>
#include <QTreeWidgetItem>
#include <QMessageBox>

/*!
	\class MvdExtendedInfoPage extendedinfopage.h
	\ingroup Movida

	\brief Movie editor page for genre, country, language and other info.
*/

/*!
	Creates a new page.
*/
MvdExtendedInfoPage::MvdExtendedInfoPage(MvdMovieCollection* c, MvdMovieEditor* parent)
: MvdMovieEditorPage(c, parent)
{
	setupUi(this);

	genres->setDataSource(Movida::GenreRole);
	genres->setMovieCollection(c);
	connect(genres, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));

	countries->setDataSource(Movida::CountryRole);
	countries->setMovieCollection(c);
	connect(countries, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));

	languages->setDataSource(Movida::LanguageRole);
	languages->setMovieCollection(c);
	connect(languages, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));

	tags->setDataSource(Movida::TagRole);
	tags->setMovieCollection(c);
	connect(tags, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));

	linkActivated("movida://toggle-view/genres");
	connect(toggleLabel, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)));
}

/*!
	Returns the title to be used for this page.
*/
QString MvdExtendedInfoPage::label()
{
	return tr("More info");
}

/*!
	Returns the icon to be used for this page.
*/
QIcon MvdExtendedInfoPage::icon()
{
	return QIcon();
}

/*!
	Sets data from a single movie.
*/
void MvdExtendedInfoPage::setMovieImpl(const MvdMovie& movie)
{
	genres->setMovie(movie);
	genres->setDataSource(Movida::GenreRole);

	countries->setMovie(movie);
	countries->setDataSource(Movida::CountryRole);

	languages->setMovie(movie);
	languages->setDataSource(Movida::LanguageRole);

	tags->setMovie(movie);
	tags->setDataSource(Movida::TagRole);
}

bool MvdExtendedInfoPage::store(MvdMovie& movie)
{
	genres->store(movie);
	countries->store(movie);
	languages->store(movie);
	tags->store(movie);

	return true;
}

//! \internal
void MvdExtendedInfoPage::linkActivated(const QString& url)
{
	//! \bug Temporary bug fix. Trolltech task tracker #172324, scheduled for Qt 4.3.2 (http://trolltech.com/developer/task-tracker/index_html?method=entry&id=172324)
	Q_ASSERT(QMetaObject::invokeMethod(this, "doLinkActivated", Qt::QueuedConnection, Q_ARG(QString, url)));
}

void MvdExtendedInfoPage::doLinkActivated(const QString& url)
{
	MvdCore::LabelAction a = MvdCore::parseLabelAction(url);
	if (!a.isValid())
		return;

	int index = -1;

	if (a.action == "toggle-view")
	{
		if (a.parameter == "genres")
		{
			toggleLabel->setText(tr("Currently showing movie <b>genres</b>. Click to show production <a href=\"movida://toggle-view/countries\">countries</a>, <a href=\"movida://toggle-view/languages\">languages</a> or <a href=\"movida://toggle-view/tags\">tags</a>."));
			index = 0;
		}
		else if (a.parameter == "countries")
		{
			toggleLabel->setText(tr("Currently showing movie production <b>countries</b>. Click to show <a href=\"movida://toggle-view/genres\">genres</a>, <a href=\"movida://toggle-view/languages\">languages</a> or <a href=\"movida://toggle-view/tags\">tags</a>."));
			index = 1;
		}
		else if (a.parameter == "languages")
		{
			toggleLabel->setText(tr("Currently showing movie <b>languages</b>. Click to show <a href=\"movida://toggle-view/genres\">genres</a>, production <a href=\"movida://toggle-view/countries\">countries</a> or <a href=\"movida://toggle-view/tags\">tags</a>."));
			index = 2;
		}
		else if (a.parameter == "tags")
		{
			toggleLabel->setText(tr("Currently showing movie <b>tags</b>. Click to show <a href=\"movida://toggle-view/genres\">genres</a>, production <a href=\"movida://toggle-view/countries\">countries</a> or <a href=\"movida://toggle-view/languages\">languages</a>."));
			index = 3;
		}

		stack->setCurrentIndex(index);
	}
}

//!
void MvdExtendedInfoPage::updateModifiedStatus()
{
	if (genres->isModified() || countries->isModified() || 
		languages->isModified() || tags->isModified())
		setModified(true);
	else setModified(false);
}
