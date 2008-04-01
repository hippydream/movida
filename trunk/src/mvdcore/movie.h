/**************************************************************************
** Filename: movie.h
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

#ifndef MVD_MOVIE_H
#define MVD_MOVIE_H

#include "global.h"
#include "sditem.h"
#include <QList>
#include <QHash>
#include <QStringList>
#include <QTime>

class MvdMovie_P;

class MVD_EXPORT MvdMovie
{
public:
	MvdMovie();
	MvdMovie(const MvdMovie& m);
	virtual ~MvdMovie();
	MvdMovie& operator=(const MvdMovie& m);

	bool isValid() const;
	
	enum ColorMode { Color, BlackWhite, UnknownColorMode };
	enum Tag { NoTag = 0, SeenTag = 2, LoanedTag = 4, SpecialTag = 8 };
	Q_DECLARE_FLAGS(Tags, Tag);

	QString title() const;
	void setTitle(const QString& s);

	QString originalTitle() const;
	void setOriginalTitle(const QString& s);

	QString validTitle() const;

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
	QTime runningTimeQt() const;
	QString runningTimeString(QString format = QString()) const;
	void setRunningTime(quint16 minutes);

	quint8 rating() const;
	bool setRating(quint8 rating);
	
	ColorMode colorMode() const;
	QString colorModeString() const;
	void setColorMode(ColorMode mode);

	QList<mvdid> languages() const;
	void addLanguage(mvdid languageID);
	void setLanguages(const QList<mvdid>& languageIDs);
	void clearLanguages();

	QList<mvdid> countries() const;
	void addCountry(mvdid id);
	void setCountries(const QList<mvdid>& countryIDs);
	void clearCountries();

	QList<mvdid> tags() const;
	void addTag(mvdid tag);
	void setTags(const QList<mvdid>& tagIDs);
	void clearTags();

	QList<mvdid> genres() const;
	void addGenre(mvdid genre);
	void setGenres(const QList<mvdid>& genreIDs);
	void clearGenres();

	QList<mvdid> directors() const;
	void addDirector(mvdid id);
	void setDirectors(const QList<mvdid>& ids);
	void clearDirectors();
	
	QList<mvdid> producers() const;
	void addProducer(mvdid id);
	void setProducers(const QList<mvdid>& ids);
	void clearProducers();
	
	QHash<mvdid,QStringList> crewMembers() const;
	QStringList crewMemberRoles(mvdid memberID) const;
	QList<mvdid> crewMemberIDs() const;
	QList<mvdid> crewMemberIDs(const QString& role) const;
	void addCrewMember(mvdid id, const QStringList& roles = QStringList());
	void setCrewMembers(const QHash<mvdid,QStringList>& members);
	void clearCrewMembers();

	QHash<mvdid,QStringList> actors() const;
	QStringList actorRoles(mvdid actorID) const;
	QList<mvdid> actorIDs() const;
	void addActor(mvdid actorID, const QStringList& roles = QStringList());
	void setActors(const QHash<mvdid,QStringList>& actors);
	void clearActors();
	
	QList<MvdUrl> urls() const;
	void addUrl(const MvdUrl& url);
	void setUrls(const QList<MvdUrl>& urls);
	void clearUrls();
	
	QStringList specialContents() const;
	void setSpecialContents(const QStringList& list);
	void clearSpecialContents();

	QString poster() const;
	void setPoster(const QString& path);

	void setSpecialTagEnabled(Tag tag, bool enabled);
	bool hasSpecialTagEnabled(Tag tag) const;

	void setSpecialTags(Tags tags);
	Tags specialTags() const;

	QList<mvdid> sharedItemIds() const;

	static QString colorModeToString(ColorMode m);
	static ColorMode colorModeFromString(QString s);

	static QString ratingTip(quint8 rating);

private:
	MvdMovie_P* d;
public:
	typedef MvdMovie_P* DataPtr;
	inline DataPtr& data_ptr() { return d; }
	void detach();
	bool isDetached() const;
};
Q_DECLARE_SHARED(MvdMovie)
Q_DECLARE_OPERATORS_FOR_FLAGS(MvdMovie::Tags)

#endif // MVD_MOVIE_H
