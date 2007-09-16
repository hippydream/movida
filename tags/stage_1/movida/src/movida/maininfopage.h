/**************************************************************************
** Filename: maininfopage.h
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

#ifndef MVD_MAININFOPAGE_H
#define MVD_MAININFOPAGE_H

#include <QList>

#include "ui_maininfopage.h"
#include "movieeditorpage.h"

class QIcon;

class MvdMainInfoPage : public MvdMovieEditorPage, private Ui::MvdMainInfoPage
{
	Q_OBJECT

public:
	MvdMainInfoPage(MvdMovieCollection* c, QWidget* parent = 0);
	
	void reset();
	QString label();
	QIcon icon();

	bool isModified() const;

protected:
	void setMovieImpl(const MvdMovie& movie);
	void setMoviesImpl(const QList<MvdMovie>& movies);

	bool store(MvdMovie& movie);

private slots:
	void linkActivated(const QString& url);
	void ratingHovered(int);
	void selectMoviePoster();
	void setMoviePoster(const QString& path = QString());
	bool posterDragEntered(const QMimeData& mimeData) const;
	bool posterDropped(const QMimeData& mimeData);
	void resetPosterStatus();

private:
	quint16 mDefaultPYear;
	quint16 mDefaultRYear;
	quint16 mDefaultRunningTime;
	quint8 mDefaultRating;
	bool mDefaultIsFavorite;
	QString mDefaultPoster;

	QString mPosterPath;
};

#endif // MVD_MAININFOPAGE_H
