/**************************************************************************
** Filename: moviemasseditor.h
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

#ifndef MVD_MOVIEMASSEDITOR_H
#define MVD_MOVIEMASSEDITOR_H

#include "mvdcore/movie.h"
#include "ui_moviemasseditor.h"
#include <QDialog>
#include <QList>

class MvdMovieCollection;
class QCloseEvent;

class MvdMovieMassEditor : public QDialog, protected Ui::MvdMovieMassEditor
{
	Q_OBJECT
		
public:
	MvdMovieMassEditor(MvdMovieCollection* c, QWidget* parent = 0);
	
	bool setMovies(const QList<mvdid>& ids);

public slots:
	bool storeMovies();

protected:
	virtual void closeEvent(QCloseEvent* e);

private slots:
	void cancelTriggered();
	void storeTriggered();
	void linkActivated(const QString& url);
	void ratingHovered(int);
	void updateUi();

private:
	MvdMovieCollection* mCollection;
	QList<mvdid> mMovieIds;
};

#endif // MVD_MOVIEMASSEDITOR_H
