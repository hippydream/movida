/**************************************************************************
** Filename: crewpage.h
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

#ifndef MVD_CREWPAGE_H
#define MVD_CREWPAGE_H

#include "ui_crewpage.h"
#include "movieeditorpage.h"

class QIcon;

class MvdCrewPage : public MvdMovieEditorPage, private Ui::MvdCrewPage
{
	Q_OBJECT
		
public:
	MvdCrewPage(MvdMovieCollection* c, MvdMovieEditor* parent = 0);
	
	QString label();
	QIcon icon();

	void setMovieImpl(const MvdMovie& movie);

	bool store(MvdMovie& movie);

private slots:
	void doLinkActivated(const QString& url);
	void linkActivated(const QString& url);
	void updateModifiedStatus();
	void moveUp();
	void moveDown();
	void itemSelectionChanged();

private:
	MvdSDTreeWidget* currentView() const;
};

#endif // MVD_CREWPAGE_H
