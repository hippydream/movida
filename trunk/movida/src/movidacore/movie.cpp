/****************************************************************************
** Filename: movie.cpp
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
	\ingroup movidacore

	\brief Reppresents a movie in a Movida collection.
*/


/************************************************************************
MvdMovie_P
*************************************************************************/

//! \internal
class MvdMovie_P
{
public:
	MvdMovie_P();
	MvdMovie_P(const MvdMovie& m);

	int isValidYear(const QString& s);

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

	QList<smdid> genres;
	QList<smdid> tags;
	QList<smdid> countries;
	QList<smdid> languages;

	QHash<smdid, QStringList> cast;
	QHash<smdid, QStringList> crew;
	QList<smdid> directors;
	QList<smdid> producers;

	QList<smdid> links;
	smdid defaultLink;

	QStringList specialContents;
};

//! \internal
MvdMovie_P::MvdMovie_P()
{
	ref = 1;

	runningTime = 0;
	colorMode = MvdMovie::UnknownColorMode;
	rating = 0;
	defaultLink = 0;
}

//! \internal
MvdMovie_P::MvdMovie_P(const MvdMovie& m)
{
	ref = 1;

	title = m.title();
	originalTitle = m.originalTitle();
	edition = m.edition();
	imdbId = m.imdbId();
	poster = m.poster();
	plot = m.plot();
	notes = m.notes();
	productionYear = m.productionYear();
	releaseYear = m.releaseYear();
	storageId = m.storageId();

	runningTime = m.runningTime();

	defaultLink = m.defaultUrl();

	tags = m.tags();
	links = m.urls();
	genres = m.genres();
	directors = m.directors();
	producers = m.producers();

	languages = m.languages();

	countries = m.countries();

	colorMode = m.colorMode();
	rating = m.rating();

	cast = m.actors();
	crew = m.crewMembers();

	specialContents = m.specialContents();
}

/*!	\internal Returns >= 0 if \p s is a valid year, 0 if the \p s is empty 
	or minimum-1 and < 0 if \p s is invalid.
*/
int MvdMovie_P::isValidYear(const QString& s)
{
	if (s.isEmpty())
		return 0;

	bool ok;
	int n = s.toInt(&ok);

	if (!ok)
		return -1;

	quint16 min = MvdCore::parameter("movidacore-min-movie-year").toUInt();

	if (n == min - 1)
		return 0;

	return (n >= min && n <= QDate::currentDate().year()) ? n : -1;
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
	Assignment operator.
 */
MvdMovie& MvdMovie::operator=(const MvdMovie& m)
{
	qAtomicAssign(d, m.d);
	return *this;
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
	d->title = s;
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
	d->originalTitle = s;
}

//! Returns the release year.
QString MvdMovie::releaseYear() const
{
	return d->releaseYear;
}

/*!
	Sets the release year and returns true iif s is a valid 4-digit year.
	Clears the year if \p s is empty or 
	MvdCore::parameter("movidacore-min-movie-year")-1.
	'Valid' means a year >= MvdCore::parameter("movidacore-min-movie-year") 
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
	MvdCore::parameter("movidacore-min-movie-year")-1.
	'Valid' means a year >= MvdCore::parameter("movidacore-min-movie-year") 
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
	d->edition = s;
}

//! Returns the IMDb id.
QString MvdMovie::imdbId() const
{
	return d->imdbId;
}

//! Sets the IMDb id if s is a valid IMDb id.
void MvdMovie::setImdbId(const QString& s)
{
	QString pattern = MvdCore::parameter("movidacore-imdb-id-regexp").toString();
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
	d->storageId = s;
}

//! Returns the rating.
quint8 MvdMovie::rating() const
{
	return d->rating;
}

/*!
	Sets the rating for this movie. Returns false if rating is greater than
	MvdCore::parameter("movidacore-max-rating").
	A null (0) rating value means that the movie has no rating.
 */
bool MvdMovie::setRating(quint8 rating)
{
	quint8 max = MvdCore::parameter("movidacore-max-rating").toUInt();

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
void MvdMovie::addGenre(smdid genreID)
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
void MvdMovie::setGenres(const QList<smdid>& genres)
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
QList<smdid> MvdMovie::genres() const
{
	return d->genres;
}

/*!
	Adds a country for this movie.
 */
void MvdMovie::addCountry(smdid countryID)
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
void MvdMovie::setCountries(const QList<smdid>& countries)
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
QList<smdid> MvdMovie::countries() const
{
	return d->countries;
}

/*!
	Adds a tag for this movie.
 */
void MvdMovie::addTag(smdid tagID)
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
void MvdMovie::setTags(const QList<smdid>& tags)
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
QList<smdid> MvdMovie::tags() const
{
	return d->tags;
}

/*!
	Adds a crew member with given role and ID to the crew list.
	Duplicates won't be added.
 */
void MvdMovie::addCrewMember(smdid memberID, const QStringList& roles)
{
	if (memberID == 0)
		return;

	// Clean roles
	QStringList _roles;
	for (int i = 0; i < roles.size(); ++i)
		_roles.append( roles.at(i).trimmed() );

	if (d->crew.isEmpty())
	{
		detach();
		d->crew.insert(memberID, _roles);
		return;
	}

	QHash<smdid,QStringList>::Iterator itr = d->crew.find(memberID);
	if (itr != d->crew.end())
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

	d->crew.insert(memberID, _roles);
}

/*!
	Returns the roles associated to the given crew member.
	Returns 0 if memberID is negative or if no such member is in the list.
 */
QStringList MvdMovie::crewMemberRoles(smdid memberID) const
{
	if (memberID == 0)
		return QStringList();

	QHash<smdid,QStringList>::Iterator itr = d->crew.find(memberID);
	return itr == d->crew.end() ? QStringList() : itr.value();
}

/*!
	Returns a list of crew member IDs.
 */
QList<smdid> MvdMovie::crewMemberIDs() const
{
	return d->crew.keys();
}

/*!
	Returns a list of crew member IDs with given role.
 */
QList<smdid> MvdMovie::crewMemberIDs(const QString& role) const
{
	if (role.isEmpty())
		return QList<smdid>();

	QString lRole = role.toLower();

	QList<smdid> fCrew;

	for (QHash<smdid,QStringList>::ConstIterator itr = d->crew.constBegin(); itr != d->crew.constEnd(); ++itr)
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
	Clears the crew list.
 */
void MvdMovie::clearCrewMembers()
{
	if (d->crew.isEmpty())
		return;

	detach();

	d->crew.clear();
}

/*!
	Sets the crew members for this movie.
	Does not check for duplicate or invalid IDs.
	Please ensure no empty strings are set as roles!
 */
void MvdMovie::setCrewMembers(const QHash<smdid,QStringList>& members)
{
	if (d->crew.isEmpty() && members.isEmpty())
		return;

	detach();

	d->crew = members;
}

/*!
	Returns the crew members list for this movie.
 */
QHash<smdid,QStringList> MvdMovie::crewMembers() const
{
	return d->crew;
}

/*!
	Adds a director for this movie.
 */
void MvdMovie::addDirector(smdid id)
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
QList<smdid> MvdMovie::directors() const
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
void MvdMovie::setDirectors(const QList<smdid>& directors)
{
	if (directors.isEmpty() && d->directors.isEmpty())
		return;

	detach();

	d->directors = directors;
}

/*!
	Adds a producer for this movie.
 */
void MvdMovie::addProducer(smdid id)
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
QList<smdid> MvdMovie::producers() const
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
void MvdMovie::setProducers(const QList<smdid>& prod)
{
	if (prod.isEmpty() && d->producers.isEmpty())
		return;

	detach();

	d->producers = prod;
}

/*!
	Adds an actor to the cast for this movie.
	No duplicates are added. Roles are merged if the actor already exists.
 */
void MvdMovie::addActor(smdid actorID, const QStringList& roles)
{
	if (actorID == 0)
		return;

	// Clean roles
	QStringList _roles;
	for (int i = 0; i < roles.size(); ++i)
		_roles.append( roles.at(i).trimmed() );

	if (d->cast.isEmpty())
	{
		detach();
		d->cast.insert(actorID, _roles);
		return;
	}

	QHash<smdid,QStringList>::Iterator itr = d->cast.find(actorID);
	if (itr != d->cast.end())
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

	d->cast.insert(actorID, _roles);
}

/*!
	Sets the cast for this movie.
	Does not check for duplicate or invalid IDs.
	Please ensure no empty strings are set as roles!
 */
void MvdMovie::setActors(const QHash<smdid,QStringList>& actors)
{
	if (actors.isEmpty() && d->cast.isEmpty())
		return;

	detach();

	d->cast = actors;
}

/*!
	Returns a list of actors without role info.
 */
QList<smdid> MvdMovie::actorIDs() const
{
	return d->cast.keys();
}

/*!
	Returns the roles associated to the given actor.
 */
QStringList MvdMovie::actorRoles(smdid actorID) const
{
	if (actorID == 0)
		return QStringList();

	QHash<smdid,QStringList>::ConstIterator itr = d->cast.find(actorID);
	return itr == d->cast.end() ? QStringList() : itr.value();
}

/*!
	Clears the cast for this movie.
 */
void MvdMovie::clearActors()
{
	if (d->cast.isEmpty())
		return;

	detach();

	d->cast.clear();
}

/*!
	Returns the cast for this movie.
 */
QHash<smdid,QStringList> MvdMovie::actors() const
{
	return d->cast;
}

/*!
	Adds a url for this movie.
 */
void MvdMovie::addUrl(smdid id, bool setDefault)
{
	if (id == 0)
		return;

	if (d->links.isEmpty())
	{
		detach();
		d->links.append(id);
		if (setDefault)
			d->defaultLink = id;
		return;
	}

	if (d->links.contains(id))
		return;

	detach();

	d->links.append(id);

	if (setDefault)
		d->defaultLink = id;
}

/*!
	Sets the urls for this movie.
	Does not check for duplicate or invalid IDs.
 */
void MvdMovie::setUrls(const QList<smdid>& links, smdid def)
{
	if (links.isEmpty() && d->links.isEmpty())
		return;

	detach();

	d->links = links;
	d->defaultLink = def;
}
/*!
	Returns the list of urls for this movie.
 */
QList<smdid> MvdMovie::urls() const
{
	return d->links;
}

/*!
	Returns the ID of the main (default) url.
*/
smdid MvdMovie::defaultUrl() const
{
	return d->defaultLink;
}

/*!
	Clears the urls for this movie.
 */
void MvdMovie::clearUrls()
{
	if (d->links.isEmpty())
		return;

	detach();

	d->links.clear();
}

/*!
	Adds a language for this movie.
 */
void MvdMovie::addLanguage(smdid id)
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
void MvdMovie::setLanguages(const QList<smdid>& langs)
{
	if (langs.isEmpty() && d->languages.isEmpty())
		return;

	detach();

	d->languages = langs;
}
/*!
	Returns the list of languages for this movie.
 */
QList<smdid> MvdMovie::languages() const
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
	\p minutes has to be <= than MvdCore::parameter("movidacore-max-running-time")
	or running time will be set to 0.
*/
void MvdMovie::setRunningTime(quint16 minutes)
{
	if (d->runningTime == minutes)
		return;

	detach();

	quint16 max = MvdCore::parameter("movidacore-max-running-time").toUInt();
	d->runningTime = minutes <= max ? minutes : 0;
}

/*!
	Formats the running time according to the format string in \p format.
	Allowed variables are $h$ for hours, $m$ for minutes, $mm$ for minutes with leading zero.
	Default $h$h $mm$' (i.e. 112 minutes --> 1h 52') is used if format is empty.
	\warning Whitespace is simplified using QString::simplified() removing multiple consecutive
	spaces, trailing spaces and \r, \n, \t or similar space characters.
*/
QString MvdMovie::runningTimeString(const QString& format) const
{
	int min = d->runningTime % 60;
	int hrs = d->runningTime < 60 ? 0 : d->runningTime / 60;

	QString res(format.isEmpty() ? format : "$h$h $mm$");

	res.replace("$h$", hrs == 0 ? QString() : QString::number(hrs));
	res.replace("$mm$", (hrs == 0 && min == 0) ? QString() :
		min < 10 ? QString::number(min).prepend("0") : QString::number(min));
	res.replace("$m$", QString::number(min));

	return res.simplified();
}

/*!
	Returns the path to the current movie poster.
*/
QString MvdMovie::poster() const
{
	return d->poster;
}

/*!
	Sets the path of the movie poster pixmap or clears any existing one
	if \p path is not a valid path.
*/
void MvdMovie::setPoster(const QString& path)
{
	if (!QFile::exists(path))
		d->poster.clear();
	else d->poster = path;
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

