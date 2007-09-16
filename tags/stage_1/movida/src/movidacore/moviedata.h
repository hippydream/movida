/**************************************************************************
** Filename: moviedata.h
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

#ifndef MVD_MOVIEDATA_H
#define MVD_MOVIEDATA_H

#include "global.h"
#include "movie.h"

#include <QList>

class MVD_EXPORT MvdMovieData
{
public:
	MvdMovieData()
	: runningTime(0), rating(0), colorMode(MvdMovie::UnknownColorMode)
	{}

	struct PersonData
	{
		QString name;
		QString imdbId;
		QString roles;
	};

	struct UrlData
	{
		QString url;
		QString description;
	};

	enum PropertyName
	{
		InvalidProperty = 0,
		Title, OriginalTitle, ReleaseYear, ProductionYear, Edition, ImdbId,
		Plot, Notes, StorageId, RunningTime, Rating, ColorMode, Languages,
		Countries, Tags, Genres, Directors, Producers, CrewMembers, Actors,
		Urls, SpecialContents, PosterPath
	};

	QString title;
	QString originalTitle;
	QString releaseYear;
	QString productionYear;
	QString edition;
	QString imdbId;
	QString plot;
	QString notes;
	QString storageId;
	quint16 runningTime;
	quint8 rating;
	MvdMovie::ColorMode colorMode;
	QStringList languages;
	QStringList countries;
	QStringList tags;
	QStringList genres;
	QList<PersonData> directors;
	QList<PersonData> producers;
	QList<PersonData> crewMembers;
	QList<PersonData> actors;
	QList<UrlData> urls;
	QStringList specialContents;
	QString posterPath;
};

#endif // MVD_MOVIEDATA_H
