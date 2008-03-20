/****************************************************************************
** Filename: movie.cpp
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
**********************************************************************/

#include "movie.h"
#include "core.h"
#include <QFileInfo>
#include <QDateTime>
#include <QStringList>
#include <QCoreApplication>
#include <QRegExp>

/*!
	\class MvdMovie movie.h
	\ingroup MovidaCore

	\brief Reppresents a movie in a Movida collection.
	\todo Direct support for awards, certification, soundtrack, technical data
*/


/************************************************************************
MvdMovie_P
*************************************************************************/

//! \internal
class MvdMovie_P
{
public:
	MvdMovie_P();
	MvdMovie_P(const MvdMovie_P& other);

	int isValidYear(const QString& s);
	QString cleanString(const QString& s);

	QAtomic ref;

	QString title;
	QString originalTitle;
	QString edition;
	QString imdbId;
	QString plot;
	QString notes;
	QString productionYear;
	QString releaseYear;
	QString storageId;
	QString poster;

	MvdMovie::ColorMode colorMode;

	quint8 runningTime;
	quint8 rating;

	QList<mvdid> genres;
	QList<mvdid> tags;
	QList<mvdid> countries;
	QList<mvdid> languages;

	QHash<mvdid, QStringList> actors;
	QHash<mvdid, QStringList> crewMembers;
	QList<mvdid> directors;
	QList<mvdid> producers;

	QList<MvdUrl> urls;

	QStringList specialContents;

	MvdMovie::Tags specialTags;
};

//! \internal
MvdMovie_P::MvdMovie_P()
{
	ref = 1;

	runningTime = 0;
	colorMode = MvdMovie::UnknownColorMode;
	rating = 0;
}

//! \internal
MvdMovie_P::MvdMovie_P(const MvdMovie_P& other)
{
	ref = 1;

	title = other.title;
	originalTitle = other.originalTitle;
	edition = other.edition;
	imdbId = other.imdbId;
	poster = other.poster;
	plot = other.plot;
	notes = other.notes;
	productionYear = other.productionYear;
	releaseYear = other.releaseYear;
	storageId = other.storageId;

	runningTime = other.runningTime;

	tags = other.tags;
	urls = other.urls;
	genres = other.genres;
	directors = other.directors;
	producers = other.producers;

	languages = other.languages;

	countries = other.countries;

	colorMode = other.colorMode;
	rating = other.rating;

	actors = other.actors;
	crewMembers = other.crewMembers;

	specialContents = other.specialContents;

	specialTags = other.specialTags;
}

/*!
	\internal Returns >= 0 if \p s is a valid year, 0 if the \p s is empty
	or minimum - 1 and < 0 if \p s is invalid.
*/
int MvdMovie_P::isValidYear(const QString& s)
{
	if (s.isEmpty())
		return 0;

	bool ok;
	int n = s.toInt(&ok);

	if (!ok)
		return -1;

	quint16 min = MvdCore::parameter("mvdcore/min-movie-year").toUInt();

	if (n == min - 1)
		return 0;

	return (n >= min && n <= QDate::currentDate().year()) ? n : -1;
}

QString MvdMovie_P::cleanString(const QString& s)
{
	int maxLength = MvdCore::parameter("mvdcore/max-edit-length").toInt();

	if (s.length() > maxLength) {
		QString _s(s);
		_s.truncate(maxLength);
		return _s;
	}
	return s;
}

/************************************************************************
MvdMovie
*************************************************************************/

/*!
	Builds an empty movie object.
 */
MvdMovie::MvdMovie()
: d(new MvdMovie_P)
{
}

/*!
	Builds a new movie as a copy of an existing one.
 */
MvdMovie::MvdMovie(const MvdMovie& m)
: d(m.d)
{
	d->ref.ref();
}

/*!
	Deletes this movie and releases used resources.
 */
MvdMovie::~MvdMovie()
{
	if (!d->ref.deref())
		delete d;
}

/*!
	Assignment operator.
 */
MvdMovie& MvdMovie::operator=(const MvdMovie& other)
{
	qAtomicAssign(d, other.d);
	return *this;
}

//! \internal Forces a detach.
void MvdMovie::detach()
{
	qAtomicDetach(d);
}

//! \internal
bool MvdMovie::isDetached() const
{
	return d->ref == 1;
}

//! Returns false if no title has been set.
bool MvdMovie::isValid() const
{
	return !(d->title.isEmpty() && d->originalTitle.isEmpty());
}

//! Returns the localized title.
QString MvdMovie::title() const
{
	return d->title;
}

//! Sets the localized title.
void MvdMovie::setTitle(const QString& s)
{
	detach();
	d->title = d->cleanString(s);
}

//! Returns the original title.
QString MvdMovie::originalTitle() const
{
	return d->originalTitle;
}

//! Sets the original title.
void MvdMovie::setOriginalTitle(const QString& s)
{
	detach();
	d->originalTitle = d->cleanString(s);
}

//! Returns the localized title or the original title if the localized is empty.
QString MvdMovie::validTitle() const
{
	return d->title.isEmpty() ? d->originalTitle : d->title;
}

//! Returns the release year.
QString MvdMovie::releaseYear() const
{
	return d->releaseYear;
}

/*!
	Sets the release year and returns true iif s is a valid 4-digit year.
	Clears the year if \p s is empty or
	MvdCore::parameter("mvdcore/min-movie-year")-1.
	'Valid' means a year >= MvdCore::parameter("mvdcore/min-movie-year")
	and <= currentYear().
*/
bool MvdMovie::setReleaseYear(const QString& s)
{
	int y = d->isValidYear(s);
	if (y < 0)
		return false;

	detach();
	d->releaseYear = y == 0 ? QString() : QString::number(y);
	return true;
}

//! Returns the production year.
QString MvdMovie::productionYear() const
{
	return d->productionYear;
}

/*!
	Sets the production year and returns true iif s is a valid 4-digit year.
	Clears the year if \p s is empty or
	MvdCore::parameter("mvdcore/min-movie-year")-1.
	'Valid' means a year >= MvdCore::parameter("mvdcore/min-movie-year")
	and <= currentYear().
*/
bool MvdMovie::setProductionYear(const QString& s)
{
	int y = d->isValidYear(s);
	if (y < 0)
		return false;

	detach();
	d->productionYear = y == 0 ? QString() : QString::number(y);
	return true;
}

//! Returns the edition.
QString MvdMovie::edition() const
{
	return d->edition;
}

//! Sets the edition.
void MvdMovie::setEdition(const QString& s)
{
	detach();
	d->edition = d->cleanString(s);
}

//! Returns the IMDb id.
QString MvdMovie::imdbId() const
{
	return d->imdbId;
}

//! Sets the IMDb id if s is a valid IMDb id.
void MvdMovie::setImdbId(const QString& s)
{
	QString pattern = MvdCore::parameter("mvdcore/imdb-id-regexp").toString();
	QRegExp rx(pattern);
	if (rx.exactMatch(s))
	{
		detach();
		d->imdbId = s;
	}
}

//! Returns the plot.
QString MvdMovie::plot() const
{
	return d->plot;
}

//! Sets the plot.
void MvdMovie::setPlot(const QString& s)
{
	detach();
	d->plot = s;
}

//! Returns the notes.
QString MvdMovie::notes() const
{
	return d->notes;
}

//! Sets the notes.
void MvdMovie::setNotes(const QString& s)
{
	detach();
	d->notes = s;
}

//! Returns the storage identifier.
QString MvdMovie::storageId() const
{
	return d->storageId;
}

//! Sets the storage identifier.
void MvdMovie::setStorageId(const QString& s)
{
	detach();
	d->storageId = d->cleanString(s);
}

//! Returns the rating.
quint8 MvdMovie::rating() const
{
	return d->rating;
}

/*!
	Sets the rating for this movie. Returns false if rating is greater than
	MvdCore::parameter("mvdcore/max-rating").
	A null (0) rating value means that the movie has no rating.
 */
bool MvdMovie::setRating(quint8 rating)
{
	quint8 max = MvdCore::parameter("mvdcore/max-rating").toUInt();

	if (rating > max)
		return false;

	detach();
	d->rating = rating;
	return true;
}

//! Sets the color mode for this movie.
void MvdMovie::setColorMode(ColorMode mode)
{
	detach();
	d->colorMode = mode;
}

//! Returns the color mode for this movie.
MvdMovie::ColorMode MvdMovie::colorMode() const
{
	return d->colorMode;
}

//! Returns the color mode for this movie as a string.
QString MvdMovie::colorModeString() const
{
	switch (d->colorMode)
	{
	case MvdMovie::Color:
		return QCoreApplication::translate("Movie color mode", "Color");
	case MvdMovie::BlackWhite:
		return QCoreApplication::translate("Movie color mode", "B/W");
	default: ;
	}

	return QCoreApplication::translate("Movie color mode", "Unknown");
}

//! Adds a genre for this movie.
void MvdMovie::addGenre(mvdid genreID)
{
	if (genreID == 0)
		return;

	if (d->genres.contains(genreID))
		return;

	detach();
	d->genres.append(genreID);
}

/*!
	Sets the genres for this movie.
	Does not check for duplicate or invalid IDs.
 */
void MvdMovie::setGenres(const QList<mvdid>& genres)
{
	if (d->genres.isEmpty() && genres.isEmpty())
		return;

	detach();
	d->genres = genres;
}

/*!
	Clears the genres list for this movie.
 */
void MvdMovie::clearGenres()
{
	if (d->genres.isEmpty())
		return;

	detach();
	d->genres.clear();
}

/*!
	Returns the list of genres for this movie.
 */
QList<mvdid> MvdMovie::genres() const
{
	return d->genres;
}

/*!
	Adds a country for this movie.
 */
void MvdMovie::addCountry(mvdid countryID)
{
	if (countryID == 0)
		return;

	if (d->countries.contains(countryID))
		return;

	detach();
	d->countries.append(countryID);
}

/*!
	Sets the countries for this movie.
	Does not check for duplicate or invalid IDs.
 */
void MvdMovie::setCountries(const QList<mvdid>& countries)
{
	if (d->countries.isEmpty() && countries.isEmpty())
		return;

	detach();
	d->countries = countries;
}

/*!
	Clears the countries list for this movie.
 */
void MvdMovie::clearCountries()
{
	if (d->countries.isEmpty())
		return;

	detach();
	d->countries.clear();
}

/*!
	Returns the list of countries for this movie.
 */
QList<mvdid> MvdMovie::countries() const
{
	return d->countries;
}

/*!
	Adds a tag for this movie.
 */
void MvdMovie::addTag(mvdid tagID)
{
	if (tagID == 0)
		return;

	if (d->tags.contains(tagID))
		return;

	detach();
	d->tags.append(tagID);
}

/*!
	Sets the tags for this movie.
	Does not check for duplicate or invalid IDs.
 */
void MvdMovie::setTags(const QList<mvdid>& tags)
{
	if (tags.isEmpty() && d->tags.isEmpty())
		return;

	detach();
	d->tags = tags;
}

/*!
	Clears the tags list for this movie.
 */
void MvdMovie::clearTags()
{
	if (d->tags.isEmpty())
		return;

	detach();
	d->tags.clear();
}

/*!
	Returns the list of tags for this movie.
 */
QList<mvdid> MvdMovie::tags() const
{
	return d->tags;
}

/*!
	Adds a crewMembers member with given role and ID to the crewMembers list.
	Duplicates won't be added.
 */
void MvdMovie::addCrewMember(mvdid memberID, const QStringList& roles)
{
	if (memberID == 0)
		return;

	// Clean roles
	QStringList _roles;
	for (int i = 0; i < roles.size(); ++i)
		_roles.append( roles.at(i).trimmed() );

	if (d->crewMembers.isEmpty())
	{
		detach();
		d->crewMembers.insert(memberID, _roles);
		return;
	}

	QHash<mvdid,QStringList>::Iterator itr = d->crewMembers.find(memberID);
	if (itr != d->crewMembers.end())
	{
		if (roles.isEmpty())
			return;

		QStringList& dest = itr.value();
		bool detached = false;

		// append role(s)
		for (int i = 0; i < roles.size(); ++i)
		{
			QString s = _roles.at(i);
			if (dest.contains(s, Qt::CaseInsensitive))
				continue;
			if (!detached)
				detach();
			dest.append(s);
		}

		return;
	}

	detach();
	d->crewMembers.insert(memberID, _roles);
}

/*!
	Returns the roles associated to the given crewMembers member.
	Returns 0 if memberID is negative or if no such member is in the list.
 */
QStringList MvdMovie::crewMemberRoles(mvdid memberID) const
{
	if (memberID == 0)
		return QStringList();

	QHash<mvdid,QStringList>::Iterator itr = d->crewMembers.find(memberID);
	return itr == d->crewMembers.end() ? QStringList() : itr.value();
}

/*!
	Returns a list of crewMembers member IDs.
 */
QList<mvdid> MvdMovie::crewMemberIDs() const
{
	return d->crewMembers.keys();
}

/*!
	Returns a list of crewMembers member IDs with given role.
 */
QList<mvdid> MvdMovie::crewMemberIDs(const QString& role) const
{
	if (role.isEmpty())
		return QList<mvdid>();

	QString lRole = role.toLower();

	QList<mvdid> fCrew;

	for (QHash<mvdid,QStringList>::ConstIterator itr = d->crewMembers.constBegin(); itr != d->crewMembers.constEnd(); ++itr)
	{
		for (QStringList::ConstIterator itr2 = itr.value().constBegin(); itr2 != itr.value().constEnd(); ++itr2)
		{
			if ((*itr2).toLower() == lRole)
			{
				fCrew.append(itr.key());
				break;
			}
		}
	}

	return fCrew;
}

/*!
	Clears the crewMembers list.
 */
void MvdMovie::clearCrewMembers()
{
	if (d->crewMembers.isEmpty())
		return;

	detach();
	d->crewMembers.clear();
}

/*!
	Sets the crewMembers members for this movie.
	Does not check for duplicate or invalid IDs.
	Please ensure no empty strings are set as roles!
 */
void MvdMovie::setCrewMembers(const QHash<mvdid,QStringList>& members)
{
	if (d->crewMembers.isEmpty() && members.isEmpty())
		return;

	detach();
	d->crewMembers = members;
}

/*!
	Returns the crewMembers members list for this movie.
 */
QHash<mvdid,QStringList> MvdMovie::crewMembers() const
{
	return d->crewMembers;
}

/*!
	Adds a director for this movie.
 */
void MvdMovie::addDirector(mvdid id)
{
	if (id == 0)
		return;

	if (d->directors.isEmpty())
	{
		detach();
		d->directors.append(id);
		return;
	}

	if (d->directors.contains(id))
		return;

	detach();
	d->directors.append(id);
}


/*!
	Returns the list of directors for this movie.
 */
QList<mvdid> MvdMovie::directors() const
{
	return d->directors;
}

/*!
	Clears the list of directors for this movie.
	Returns false if no director has been set.
 */
void MvdMovie::clearDirectors()
{
	if (d->directors.isEmpty())
		return;

	detach();
	d->directors.clear();
}

/*!
	Sets the directors for this movie.
	Does not check for duplicate or invalid IDs.
 */
void MvdMovie::setDirectors(const QList<mvdid>& directors)
{
	if (directors.isEmpty() && d->directors.isEmpty())
		return;

	detach();
	d->directors = directors;
}

/*!
	Adds a producer for this movie.
 */
void MvdMovie::addProducer(mvdid id)
{
	if (id == 0)
		return;

	if (d->producers.isEmpty())
	{
		detach();
		d->producers.append(id);
		return;
	}

	if (d->producers.contains(id))
		return;

	detach();
	d->producers.append(id);
}

/*!
	Returns the list of producers for this movie.
 */
QList<mvdid> MvdMovie::producers() const
{
	return d->producers;
}

/*!
	Clears the list of producers for this movie.
 */
void MvdMovie::clearProducers()
{
	if (d->producers.isEmpty())
		return;

	detach();
	d->producers.clear();
}

/*!
	Sets the producers for this movie.
	Does not check for duplicate or invalid IDs.
 */
void MvdMovie::setProducers(const QList<mvdid>& prod)
{
	if (prod.isEmpty() && d->producers.isEmpty())
		return;

	detach();
	d->producers = prod;
}

/*!
	Adds an actor to the actors for this movie.
	No duplicates are added. Roles are merged if the actor already exists.
 */
void MvdMovie::addActor(mvdid actorID, const QStringList& roles)
{
	if (actorID == 0)
		return;

	// Clean roles
	QStringList _roles;
	for (int i = 0; i < roles.size(); ++i)
		_roles.append( roles.at(i).trimmed() );

	if (d->actors.isEmpty())
	{
		detach();
		d->actors.insert(actorID, _roles);
		return;
	}

	QHash<mvdid,QStringList>::Iterator itr = d->actors.find(actorID);
	if (itr != d->actors.end())
	{
		if (roles.isEmpty())
			return;

		QStringList& dest = itr.value();
		bool detached = false;

		// append role(s)
		for (int i = 0; i < roles.size(); ++i)
		{
			QString s = _roles.at(i);
			if (dest.contains(s, Qt::CaseInsensitive))
				continue;
			if (!detached)
				detach();
			dest.append(s);
		}

		return;
	}

	detach();
	d->actors.insert(actorID, _roles);
}

/*!
	Sets the actors for this movie.
	Does not check for duplicate or invalid IDs.
	Please ensure no empty strings are set as roles!
 */
void MvdMovie::setActors(const QHash<mvdid,QStringList>& actors)
{
	if (actors.isEmpty() && d->actors.isEmpty())
		return;

	detach();
	d->actors = actors;
}

/*!
	Returns a list of actors without role info.
 */
QList<mvdid> MvdMovie::actorIDs() const
{
	return d->actors.keys();
}

/*!
	Returns the roles associated to the given actor.
 */
QStringList MvdMovie::actorRoles(mvdid actorID) const
{
	if (actorID == 0)
		return QStringList();

	QHash<mvdid,QStringList>::ConstIterator itr = d->actors.find(actorID);
	return itr == d->actors.end() ? QStringList() : itr.value();
}

/*!
	Clears the actors for this movie.
 */
void MvdMovie::clearActors()
{
	if (d->actors.isEmpty())
		return;

	detach();
	d->actors.clear();
}

/*!
	Returns the actors for this movie.
 */
QHash<mvdid,QStringList> MvdMovie::actors() const
{
	return d->actors;
}

/*!
	Adds a url for this movie.
 */
void MvdMovie::addUrl(const MvdUrl& url)
{
	MvdUrl u = url;
	u.url = u.url.trimmed();

	if (u.url.isEmpty())
		return;

	if (d->urls.isEmpty())
	{
		detach();
		d->urls.append(u);
		return;
	}

	if (d->urls.contains(u))
		return;

	detach();
	d->urls.append(u);
}

/*!
	Sets the urls for this movie.
	Does not check for duplicate or invalid URLs.
 */
void MvdMovie::setUrls(const QList<MvdUrl>& urls)
{
	if (urls.isEmpty() && d->urls.isEmpty())
		return;

	detach();
	d->urls = urls;
}
/*!
	Returns the list of urls for this movie.
 */
QList<MvdUrl> MvdMovie::urls() const
{
	return d->urls;
}

/*!
	Clears the urls for this movie.
 */
void MvdMovie::clearUrls()
{
	if (d->urls.isEmpty())
		return;

	detach();
	d->urls.clear();
}

/*!
	Adds a language for this movie.
 */
void MvdMovie::addLanguage(mvdid id)
{
	if (id == 0)
		return;

	if (d->languages.isEmpty())
	{
		detach();
		d->languages.append(id);
		return;
	}

	if (d->languages.contains(id))
		return;

	detach();
	d->languages.append(id);
}

/*!
	Sets the languages for this movie.
	Does not check for duplicate or invalid IDs.
 */
void MvdMovie::setLanguages(const QList<mvdid>& langs)
{
	if (langs.isEmpty() && d->languages.isEmpty())
		return;

	detach();
	d->languages = langs;
}
/*!
	Returns the list of languages for this movie.
 */
QList<mvdid> MvdMovie::languages() const
{
	return d->languages;
}

/*!
	Clears the languages for this movie.
 */
void MvdMovie::clearLanguages()
{
	if (d->languages.isEmpty())
		return;

	detach();
	d->languages.clear();
}

/*!
	Sets the special contents for this movie.
 */
void MvdMovie::setSpecialContents(const QStringList& list)
{
	if (list.isEmpty() && d->specialContents.isEmpty())
		return;

	detach();
	d->specialContents = list;
}

/*!
	Returns the special contents for this movie.
 */
QStringList MvdMovie::specialContents() const
{
	return d->specialContents;
}

/*!
	Clears the special contents for this movie.
 */
void MvdMovie::clearSpecialContents()
{
	if (d->specialContents.isEmpty())
		return;

	detach();
	d->specialContents.clear();
}

/*!
	Returns the movie running time in minutes (always less than 1000).
*/
quint16 MvdMovie::runningTime() const
{
	return d->runningTime;
}

/*!
	Sets the movie running time in minutes.
	\p minutes has to be <= than MvdCore::parameter("mvdcore/max-running-time")
	or running time will be set to 0.
*/
void MvdMovie::setRunningTime(quint16 minutes)
{
	if (d->runningTime == minutes)
		return;

	detach();
	quint16 max = MvdCore::parameter("mvdcore/max-running-time").toUInt();
	d->runningTime = minutes <= max ? minutes : 0;
}

//! Returns the running time as a QTime object.
QTime MvdMovie::runningTimeQt() const
{
	int min = d->runningTime % 60;
	int hrs = d->runningTime < 60 ? 0 : d->runningTime / 60;
	return QTime(hrs, min);
}

/*!
	Formats the running time according to the format string in \p format.
	The format is the same as Qt's, so please refer to the QTime::fromString()
	referece documentation for details.
	If the format string is empty, a default value registered in
	MvdCore::parameter("mvdcore/running-time-format") is used.
*/
QString MvdMovie::runningTimeString(QString format) const
{
	QTime time = runningTimeQt();

	if (format.isEmpty())
		format = MvdCore::parameter("mvdcore/running-time-format").toString();

	return time.toString(format);
}

/*!
	Returns the path to the current movie poster.
*/
QString MvdMovie::poster() const
{
	return d->poster;
}

/*!
	Sets a poster for this movie using the filename of an already existing
	movie poster.

	The poster file should exist in the collection's "persistent"
	data path directory.

	Use MvdMovieCollection::addImage() to register a new image file
	and call this method using the collection's internal filename returned
	by addImage().

	No checks are made as this movie instance has no
	knowledge of the collection (and thus of its data path) it belongs to.
	Removes any existing poster if \p filename is empty.
*/
void MvdMovie::setPoster(const QString& filename)
{
	d->poster = filename;
}

//! Returns a color mode as a string compatible with the movida XML movie descriptions.
QString MvdMovie::colorModeToString(ColorMode m)
{
	switch (m)
	{
	case Color: return "color";
	case BlackWhite: return "bw";
	default: ;
	}
	return QString();
}

//! Parses a color mode string as in a movida XML movie description.
MvdMovie::ColorMode MvdMovie::colorModeFromString(QString s)
{
	s = s.trimmed().toLower();
	if (s == "color")
		return Color;
	else if (s == "bw")
		return BlackWhite;
	return UnknownColorMode;
}

void MvdMovie::setSpecialTagEnabled(Tag tag, bool enabled)
{
	if (enabled)
		d->specialTags |= tag;
	else d->specialTags &= ~tag;
}

bool MvdMovie::hasSpecialTagEnabled(Tag tag) const
{
	return d->specialTags.testFlag(tag);
}

void MvdMovie::setSpecialTags(Tags tags)
{
	d->specialTags = tags;
}

MvdMovie::Tags MvdMovie::specialTags() const
{
	return d->specialTags;
}

//! Returns a human readable description for a rating value.
QString MvdMovie::ratingTip(quint8 rating)
{
	switch (rating) {
	case 1: return QCoreApplication::translate("Movie rating", "Awful movie");
	case 2: return QCoreApplication::translate("Movie rating", "Disappointing movie");
	case 3: return QCoreApplication::translate("Movie rating", "Mediocre movie");
	case 4: return QCoreApplication::translate("Movie rating", "Good movie");
	case 5: return QCoreApplication::translate("Movie rating", "Outstanding movie");
	}
	return QCoreApplication::translate("Movie rating", "Unrated movie");
}
