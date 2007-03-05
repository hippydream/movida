/**************************************************************************
** Filename: crewpage.cpp
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

#include "crewpage.h"
#include "guiglobal.h"

#include <QIcon>
#include <QTreeWidgetItem>

/*!
	\class MvdCrewPage crewpage.h

	\brief Crew editing widget for movie editing dialog.
*/

/*!
	Creates a new page.
*/
MvdCrewPage::MvdCrewPage(MvdMovieCollection* c, QWidget* parent)
: MvdMovieEditorPage(c, parent)
{
	setupUi(this);

	cast->setMovieCollection(mCollection);
	cast->setDataSource(Movida::ActorRole);

	directors->setMovieCollection(mCollection);
	directors->setDataSource(Movida::DirectorRole);

	producers->setMovieCollection(mCollection);
	producers->setDataSource(Movida::ProducerRole);

	crew->setMovieCollection(mCollection);
	crew->setDataSource(Movida::CrewMemberRole);
}

/*!
	Reset to default values.
*/
void MvdCrewPage::reset()
{
}

/*!
	Returns the title to be used for this page.
*/
QString MvdCrewPage::label()
{
	return tr("Cast and crew");
}

/*!
	Returns the icon to be used for this page.
*/
QIcon MvdCrewPage::icon()
{
	return QIcon(":/images/preferences/log.png");
}

bool MvdCrewPage::isModified() const
{
	return cast->isModified() || directors->isModified() || producers->isModified() || crew->isModified();
}

/*!
	Sets data from multiple movies.
*/
void MvdCrewPage::setMoviesImpl(const QList<MvdMovie>& movies)
{
}

/*!
	Sets data from a single movie.
*/
void MvdCrewPage::setMovieImpl(const MvdMovie& movie)
{
	cast->setMovie(movie);
	cast->setMovieCollection(mCollection);
	cast->setDataSource(Movida::ActorRole);

	directors->setMovie(movie);
	directors->setMovieCollection(mCollection);
	directors->setDataSource(Movida::DirectorRole);

	producers->setMovie(movie);
	producers->setMovieCollection(mCollection);
	producers->setDataSource(Movida::ProducerRole);

	crew->setMovie(movie);
	crew->setMovieCollection(mCollection);
	crew->setDataSource(Movida::CrewMemberRole);
}

bool MvdCrewPage::store(MvdMovie& movie)
{
	cast->store(movie);
	directors->store(movie);
	producers->store(movie);
	crew->store(movie);

	return true;
}
