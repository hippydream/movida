/**************************************************************************
** Filename: movie.h
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

#ifndef MVD_MOVIE_H
#define MVD_MOVIE_H

#include "global.h"

#include <QList>
#include <QHash>
#include <QStringList>

class MvdMovie_P;

class MVD_EXPORT MvdMovie
{
public:
	MvdMovie();
	MvdMovie(const MvdMovie& m);
	MvdMovie& operator=(const MvdMovie& m);
	virtual ~MvdMovie();
	
	bool isValid() const;
	
	enum ColorMode { Color, BlackWhite, UnknownColorMode };
	/*enum SubtitleContents { StandardSubtitle, HearingImpairedSubtitle, 
		CommentarySubtitle, UnknownSubtitle };*/

	QString title() const;
	void setTitle(const QString& s);

	QString originalTitle() const;
	void setOriginalTitle(const QString& s);

	QString releaseYear() const;
	bool setReleaseYear(const QString& s);

	QString productionYear() const;
	bool setProductionYear(const QString& s);

	QString edition() const;
	void setEdition(const QString& s);

	QString imdbId() const;
	void setImdbId(const QString& s);

	QString plot() const;
	void setPlot(const QString& s);

	QString notes() const;
	void setNotes(const QString& s);

	QString storageId() const;
	void setStorageId(const QString& s);
	
	quint16 runningTime() const;
	QString runningTimeString(const QString& format = QString()) const;
	void setRunningTime(quint16 minutes);

	quint8 rating() const;
	bool setRating(quint8 rating);
	
	ColorMode colorMode() const;
	QString colorModeString() const;
	void setColorMode(ColorMode mode);

	QList<smdid> languages() const;
	void addLanguage(smdid countryID);
	void setLanguages(const QList<smdid>& countryIDs);
	void clearLanguages();

	QList<smdid> countries() const;
	void addCountry(smdid id);
	void setCountries(const QList<smdid>& countryIDs);
	void clearCountries();

	QList<smdid> tags() const;
	void addTag(smdid tag);
	void setTags(const QList<smdid>& tagIDs);
	void clearTags();

	QList<smdid> genres() const;
	void addGenre(smdid genre);
	void setGenres(const QList<smdid>& genreIDs);
	void clearGenres();

	QList<smdid> directors() const;
	void addDirector(smdid id);
	void setDirectors(const QList<smdid>& ids);
	void clearDirectors();
	
	QList<smdid> producers() const;
	void addProducer(smdid id);
	void setProducers(const QList<smdid>& ids);
	void clearProducers();
	
	QHash<smdid,QStringList> crewMembers() const;
	QStringList crewMemberRoles(smdid memberID) const;
	QList<smdid> crewMemberIDs() const;
	QList<smdid> crewMemberIDs(const QString& role) const;
	void addCrewMember(smdid id, const QStringList& roles = QStringList());
	void setCrewMembers(const QHash<smdid,QStringList>& members);
	void clearCrewMembers();

	QHash<smdid,QStringList> actors() const;
	QStringList actorRoles(smdid actorID) const;
	QList<smdid> actorIDs() const;
	void addActor(smdid actorID, const QStringList& roles = QStringList());
	void setActors(const QHash<smdid,QStringList>& actors);
	void clearActors();
	
	QList<smdid> urls() const;
	smdid defaultUrl() const;
	void addUrl(smdid id, bool setDefault = false);
	void setUrls(const QList<smdid>& urls, smdid def = 0);
	void clearUrls();
	
	QStringList specialContents() const;
	void setSpecialContents(const QStringList& list);
	void clearSpecialContents();

	QString poster() const;
	void setPoster(const QString& path);

	///// internal
	void detach();
	bool isDetached() const;
	
private:
	MvdMovie_P* d;
};
Q_DECLARE_SHARED(MvdMovie)

#endif // MVD_MOVIE_H
