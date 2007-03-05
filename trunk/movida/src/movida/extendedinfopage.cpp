/**************************************************************************
** Filename: extendedinfopage.cpp
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

#include "extendedinfopage.h"
#include "guiglobal.h"
#include "movie.h"
#include "moviecollection.h"

#include <QIcon>
#include <QList>
#include <QTreeWidgetItem>
#include <QMessageBox>

/*!
	\class MvdExtendedInfoPage extendedinfopage.h

	\brief Movie editor page for genre, country, language and other info.
*/

/*!
	Creates a new page.	
*/
MvdExtendedInfoPage::MvdExtendedInfoPage(MvdMovieCollection* c, QWidget* parent)
: MvdMovieEditorPage(c, parent)
{
	setupUi(this);

	genres->setDataSource(Movida::GenreRole);
	genres->setMovieCollection(c);
	countries->setDataSource(Movida::CountryRole);
	countries->setMovieCollection(c);
	languages->setDataSource(Movida::LanguageRole);
	languages->setMovieCollection(c);	
	tags->setDataSource(Movida::TagRole);
	tags->setMovieCollection(c);
}

/*!
	Reset to default values.
*/
void MvdExtendedInfoPage::reset()
{
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
	return QIcon(":/images/preferences/log.png");
}

bool MvdExtendedInfoPage::isModified() const
{
	return genres->isModified() || countries->isModified() || 
		languages->isModified() || tags->isModified();
}

/*!
	Sets data from multiple movies.
*/
void MvdExtendedInfoPage::setMoviesImpl(const QList<MvdMovie>& movies)
{
}

/*!
	Sets data from a single movie.
*/
void MvdExtendedInfoPage::setMovieImpl(const MvdMovie& movie)
{
	genres->setMovie(movie);
	genres->setMovieCollection(mCollection);
	genres->setDataSource(Movida::GenreRole);

	countries->setMovie(movie);
	countries->setMovieCollection(mCollection);
	countries->setDataSource(Movida::CountryRole);

	languages->setMovie(movie);
	languages->setMovieCollection(mCollection);
	languages->setDataSource(Movida::LanguageRole);

	tags->setMovie(movie);
	tags->setMovieCollection(mCollection);
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
