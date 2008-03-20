/**************************************************************************
** Filename: crewpage.cpp
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

#include "crewpage.h"
#include "guiglobal.h"
#include "mvdcore/core.h"
#include <QIcon>
#include <QTreeWidgetItem>

/*!
	\class MvdCrewPage crewpage.h
	\ingroup Movida

	\brief Crew editing widget for movie editing dialog.
*/

/*!
	Creates a new page.
*/
MvdCrewPage::MvdCrewPage(MvdMovieCollection* c, MvdMovieEditor* parent)
: MvdMovieEditorPage(c, parent)
{
	setupUi(this);

	cast->setMovieCollection(mCollection);
	cast->setDataSource(Movida::ActorRole);
	connect(cast, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));

	directors->setMovieCollection(mCollection);
	directors->setDataSource(Movida::DirectorRole);
	connect(directors, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));

	producers->setMovieCollection(mCollection);
	producers->setDataSource(Movida::ProducerRole);
	connect(producers, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));

	crew->setMovieCollection(mCollection);
	crew->setDataSource(Movida::CrewMemberRole);
	connect(crew, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));

	linkActivated("movida://toggle-view/cast");
	connect(toggleLabel, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)));
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
	return QIcon();
}

/*!
	Sets data from a single movie.
*/
void MvdCrewPage::setMovieImpl(const MvdMovie& movie)
{
	cast->setMovie(movie);
	cast->setDataSource(Movida::ActorRole);

	directors->setMovie(movie);
	directors->setDataSource(Movida::DirectorRole);

	producers->setMovie(movie);
	producers->setDataSource(Movida::ProducerRole);

	crew->setMovie(movie);
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

//! \internal
void MvdCrewPage::linkActivated(const QString& url)
{
	//! \bug Temporary bug fix. Trolltech task tracker #172324, scheduled for Qt 4.3.2 (http://trolltech.com/developer/task-tracker/index_html?method=entry&id=172324)
	Q_ASSERT(QMetaObject::invokeMethod(this, "doLinkActivated", Qt::QueuedConnection, Q_ARG(QString, url)));
}

void MvdCrewPage::doLinkActivated(const QString& url)
{
	MvdCore::LabelAction a = MvdCore::parseLabelAction(url);
	if (!a.isValid())
		return;

	int index = -1;

	if (a.action == "toggle-view")
	{
		if (a.parameter == "cast")
		{
			toggleLabel->setText(tr("Currently showing movie <b>cast</b>. Click to show <a href=\"movida://toggle-view/directors\">directors</a>, <a href=\"movida://toggle-view/producers\">producers</a> or other <a href=\"movida://toggle-view/crew\">crew members</a>."));
			index = 0;
		}
		else if (a.parameter == "directors")
		{
			toggleLabel->setText(tr("Currently showing movie <b>directors</b>. Click to show <a href=\"movida://toggle-view/cast\">cast</a>, <a href=\"movida://toggle-view/producers\">producers</a> or other <a href=\"movida://toggle-view/crew\">crew members</a>."));
			index = 1;
		}
		else if (a.parameter == "producers")
		{
			toggleLabel->setText(tr("Currently showing movie <b>producers</b>. Click to show <a href=\"movida://toggle-view/cast\">cast</a>, <a href=\"movida://toggle-view/directors\">directors</a> or other <a href=\"movida://toggle-view/crew\">crew members</a>."));
			index = 2;
		}
		else if (a.parameter == "crew")
		{
			toggleLabel->setText(tr("Currently showing movie <b>crew</b>. Click to show <a href=\"movida://toggle-view/cast\">cast</a>, <a href=\"movida://toggle-view/directors\">directors</a> or <a href=\"movida://toggle-view/producers\">producers</a>."));
			index = 3;
		}

		stack->setCurrentIndex(index);
	}
}

//!
void MvdCrewPage::updateModifiedStatus()
{
	if (cast->isModified() || directors->isModified() || 
		producers->isModified() || crew->isModified())
		setModified(true);
	else setModified(false);
}
