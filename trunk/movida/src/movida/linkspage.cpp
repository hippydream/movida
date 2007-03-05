/**************************************************************************
** Filename: linkspage.cpp
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

#include "linkspage.h"
#include "guiglobal.h"
#include "movie.h"
#include "core.h"

#include <QIcon>
#include <QRegExp>
#include <QDesktopServices>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QUrl>
#include <QMessageBox>

/*!
	\class MvdLinksPage linkspage.h

	\brief Link editing widget for the movie editor dialog.
*/

/*!
	Creates a new page.	
*/
MvdLinksPage::MvdLinksPage(MvdMovieCollection* c, QWidget* parent)
: MvdMovieEditorPage(c, parent), mModified(false)
{
	setupUi(this);

	linksWidget->setDataSource(Movida::UrlRole);
	linksWidget->setMovieCollection(c);

	connect(imdbInput, SIGNAL(textChanged(const QString&)), this, SLOT(imdbIdChanged(const QString&)));
	connect(imdbButton, SIGNAL(clicked()), this, SLOT(openImdbPage()));
}

/*!
	Reset to default values.
*/
void MvdLinksPage::reset()
{
}

/*!
	Returns the title to be used for this page.
*/
QString MvdLinksPage::label()
{
	return tr("Links");
}

/*!
	Returns the icon to be used for this page.
*/
QIcon MvdLinksPage::icon()
{
	return QIcon(":/images/preferences/log.png");
}

bool MvdLinksPage::isModified() const
{
	return mModified || linksWidget->isModified();
}

void MvdLinksPage::setMovieImpl(const MvdMovie& movie)
{
	linksWidget->setMovie(movie);
	linksWidget->setMovieCollection(mCollection);
	linksWidget->setDataSource(Movida::UrlRole);

	imdbInput->setText(movie.imdbId());

	mModified = false;
}

void MvdLinksPage::setMoviesImpl(const QList<MvdMovie>& movies)
{
}

/*!
	Enables or disables the "Visit IMDb page" button when the IMDb id changes.
*/
void MvdLinksPage::imdbIdChanged(const QString& text)
{
	QString pattern = MvdCore::parameter("movidacore-imdb-id-regexp").toString();
	QRegExp rx(pattern);

	imdbButton->setEnabled(rx.exactMatch(text));

	mModified = imdbInput->isModified();
}

/*!
	Attempts to open the movies IMDb page in the system's default browser.
*/
void MvdLinksPage::openImdbPage()
{
	QString url = MvdCore::parameter("movida-imdb-movie-url").toString();
	QDesktopServices::openUrl(url.arg(imdbInput->text()));
}

bool MvdLinksPage::store(MvdMovie& movie)
{
	linksWidget->store(movie);

	QString pattern = MvdCore::parameter("movidacore-imdb-id-regexp").toString();
	QRegExp rx(pattern);
	QString imdb = imdbInput->text();

	if (imdb.isEmpty())
		return true;

	if (rx.exactMatch(imdb))
		movie.setImdbId(imdb);
	else
	{
		int res = QMessageBox::question(this, _CAPTION_, tr("The IMDb id is invalid. Do you want to ignore it?"),
			QMessageBox::Yes, QMessageBox::No);
		if (res != QMessageBox::Yes)
			return false;
	}

	return true;
}
