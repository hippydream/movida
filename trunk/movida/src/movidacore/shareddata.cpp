/**************************************************************************
** Filename: shareddata.cpp
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

#include "shareddata.h"
#include "logger.h"
#include "global.h"

#include <QImage>

#define MVD_SMD_CHECK_ID(id) \
	if (id == 0x7FFFFFFF) \
	{\
		eLog() << "SD: available IDs exhausted.";\
		return 0;\
	}

#define MVD_SET_HARDCODED(hardcoded, id) \
	if (hardcoded)\
		id |= 0x80000000;

using namespace Movida;

/*!
	\class MvdSharedData shareddata.h
	\ingroup movidacore

	\brief Stores common data such as person names or genres.
	Every movie collection has its own shared movie data object.
	A global, application wide, SD object contains some default values like
	famous actor names or common genres.
	Items added to the global SD have a special "hardcoded" ID.
	The SD records associations between shared items and movies (or other
	shared objects) that "use" them.
*/


/************************************************************************
MvdSharedData_P
*************************************************************************/

//! \internal
class MvdSharedData_P
{
public:
	MvdSharedData_P();
	MvdSharedData_P(const MvdSharedData_P& s);
	~MvdSharedData_P();

	void linkMovie(QList<mvdid>& v, mvdid id, bool add);

	//! \todo merge "*Data" data structures and move them to some more global class
	QHash<smdid,MvdSharedData::PersonData>* persons;
	QHash<smdid,MvdSharedData::StringData>* genres;
	QHash<smdid,MvdSharedData::StringData>* tags;
	QHash<smdid,MvdSharedData::StringData>* countries;
	QHash<smdid,MvdSharedData::StringData>* languages;
	QHash<smdid,MvdSharedData::UrlData>* urls;

	smdid personID;
	smdid genreID;
	smdid tagID;
	smdid countryID;
	smdid languageID;
	smdid urlID;

	//! true if not linked and not hard coded items should be removed
	bool autoPurge;

	//! true if there is some data in the SDB and if purgeData has not been called since last insert.
	bool canPurge;
};

//! \internal
MvdSharedData_P::MvdSharedData_P()
{
	// Data structures will be initialized at first use
	persons = 0;
	genres = 0;
	tags = 0;
	countries = 0;
	languages = 0;
	urls = 0;

	personID = genreID = tagID = countryID = languageID = urlID = 1;

	// default: automatically delete items with no links
	autoPurge = true;
	canPurge = false;
}

//! \internal
MvdSharedData_P::MvdSharedData_P(const MvdSharedData_P& s)
{
	persons = s.persons == 0 ? 0 : new QHash<smdid,MvdSharedData::PersonData>(*(s.persons));
	genres = s.genres == 0 ? 0 : new QHash<smdid,MvdSharedData::StringData>(*(s.genres));
	tags = s.tags == 0 ? 0 : new QHash<smdid,MvdSharedData::StringData>(*(s.tags));
	countries = s.countries == 0 ? 0 : new QHash<smdid,MvdSharedData::StringData>(*(s.countries));
	languages = s.languages == 0 ? 0 : new QHash<smdid,MvdSharedData::StringData>(*(s.languages));
	urls = s.urls == 0 ? 0 : new QHash<smdid,MvdSharedData::UrlData>(*(s.urls));

	personID = s.personID;
	genreID = s.genreID;
	tagID = s.tagID;
	countryID = s.countryID;
	languageID = s.languageID;
	urlID = s.urlID;

	autoPurge = s.autoPurge;
	canPurge = s.canPurge;
}

//! \internal
MvdSharedData_P::~MvdSharedData_P()
{
	delete persons;
	delete genres;
	delete tags;
	delete countries;
	delete languages;
	delete urls;
}

//! \internal Links or unlinks a movie/SD item to/from a SD item.
void MvdSharedData_P::linkMovie(QList<mvdid>& v, mvdid id, bool add)
{
	for (QList<mvdid>::Iterator it = v.begin(); it != v.end(); ++it)
	{
		if (*it > id)
		{
			if (add)
				v.insert(it, id);
			else return; // id's are ordered
		}

		if (*it == id)
		{
			if (add)
				return;
			v.erase(it);
		}
	}

	if (add)
		v.append(id);
}


/************************************************************************
Data types
*************************************************************************/

//! Constructor.
MvdSharedData::StringData::StringData()
{
}

//! Copy constructor. Only user data is copied, no movie IDs!
MvdSharedData::StringData::StringData(const StringData& o)
{
	*this = o;
}

//! Lexicographically compares two string items (case insensitive compare).
bool MvdSharedData::StringData::operator <(const MvdSharedData::StringData& p) const
{
	return (name.toLower() < p.name.toLower());
}

//! Returns true if two items have the same string value (case insensitive compare).
bool MvdSharedData::StringData::operator ==(const MvdSharedData::StringData& p) const
{
	return (name.toLower() == p.name.toLower());
}

//! Copy operator. Only user data is copied, no movie IDs!
MvdSharedData::StringData& MvdSharedData::StringData::operator =(const MvdSharedData::StringData& p)
{
	name = p.name;
	return *this;
}

//! Constructor.
MvdSharedData::PersonData::PersonData()
: defaultUrl(0)
{
}

//! Copy constructor. Only user data is copied, no movie IDs!
MvdSharedData::PersonData::PersonData(const PersonData& o)
{
	*this = o;
}

//! Lexicographically compares two persons (last name is compared first - case insensitive compare).
bool MvdSharedData::PersonData::operator <(const MvdSharedData::PersonData& p) const
{
	if (lastName.toLower() != p.lastName.toLower())
		return lastName.toLower() < p.lastName.toLower();
	else return (firstName.toLower() < p.firstName.toLower());
}

//! Returns true if two items refer to the same person (case insensitive compare).
bool MvdSharedData::PersonData::operator ==(const MvdSharedData::PersonData& p) const
{
	return ((firstName.toLower() == p.firstName.toLower()) && (lastName.toLower() == p.lastName.toLower()));
}

//! Copy operator. Only user data is copied, no movie IDs!
MvdSharedData::PersonData& MvdSharedData::PersonData::operator =(const MvdSharedData::PersonData& p)
{
	lastName = p.lastName;
	firstName = p.firstName;
	defaultUrl = p.defaultUrl;
	urls = p.urls;
	return *this;
}

//! Constructor.
MvdSharedData::UrlData::UrlData()
{
}

//! Copy constructor. Only user data is copied, no movie IDs!
MvdSharedData::UrlData::UrlData(const UrlData& o)
{
	*this = o;
}

//! Lexicographically compares two urls (case insensitive compare).
bool MvdSharedData::UrlData::operator <(const MvdSharedData::UrlData& c) const
{
	if (url.toLower() == c.url.toLower())
		return (description.toLower() < c.description.toLower());
	return url.toLower() < c.url.toLower();
}

//! Returns true if two UrlData have the same URL (case insensitive compare).
bool MvdSharedData::UrlData::operator ==(const MvdSharedData::UrlData& c) const
{
	return url.toLower() == c.url.toLower();
}

//! Copy operator. Only user data is copied, no movie IDs!
MvdSharedData::UrlData& MvdSharedData::UrlData::operator =(const MvdSharedData::UrlData& c)
{
	url = c.url;
	description = c.description;
	return *this;
}


/************************************************************************
MvdSharedData
*************************************************************************/

//! This is the application wide global SD.
static MvdSharedData* mvdGlobalSD = 0;

/*!
	Creates a new empty SD.
 */
MvdSharedData::MvdSharedData()
: QObject(), d(new MvdSharedData_P)
{
}

/*!
	Creates a copy of another SD.
 */
MvdSharedData::MvdSharedData(const MvdSharedData& s)
: QObject(), d(s.d)
{
}

/*!
	Assignment operator. Creates a copy of the SD.
*/
MvdSharedData& MvdSharedData::operator=(const MvdSharedData& m)
{
	delete d;
	d = new MvdSharedData_P(*(m.d));
	return *this;
}

/*!
	Deletes this SD instance.
 */
MvdSharedData::~MvdSharedData()
{
	delete d;
}

/************************************************************************
Retrieve item by ID
*************************************************************************/

/*!
	Returns a pointer to the person with given ID or 0 if no such actor exists.
 */
const MvdSharedData::PersonData* MvdSharedData::person(smdid id) const
{
	if (d->persons == 0)
		return 0;
	
	QHash<smdid,PersonData>::ConstIterator itr = d->persons->find(id);
	if (itr != d->persons->end())
		return &itr.value();
		
	return 0;
}

/*!
	Returns a pointer to the genre with given ID or 0 if no such genre exists.
 */
const MvdSharedData::StringData* MvdSharedData::genre(smdid id) const
{
	if (d->genres == 0)
		return 0;
	
	QHash<smdid,StringData>::ConstIterator itr = d->genres->find(id);
	if (itr != d->genres->end())
		return &itr.value();
		
	return 0;
}

/*!
	Returns a pointer to the tag with given ID or 0 if no such tag exists.
 */
const MvdSharedData::StringData* MvdSharedData::tag(smdid id) const
{
	if (d->tags == 0)
		return 0;
	
	QHash<smdid,StringData>::ConstIterator itr = d->tags->find(id);
	if (itr != d->tags->end())
		return &itr.value();
		
	return 0;
}


/*!
	Returns a pointer to the country with given ID or 0 if no such country exists.
*/
const MvdSharedData::StringData* MvdSharedData::country(smdid id) const
{
	if (d->countries == 0)
		return 0;

	QHash<smdid,StringData>::ConstIterator itr = d->countries->find(id);
	if (itr != d->countries->end())
		return &itr.value();

	return 0;
}


/*!
	Returns a pointer to the language with given ID or 0 if no such language exists.
*/
const MvdSharedData::StringData* MvdSharedData::language(smdid id) const
{
	if (d->languages == 0)
		return 0;

	QHash<smdid,StringData>::ConstIterator itr = d->languages->find(id);
	if (itr != d->languages->end())
		return &itr.value();

	return 0;
}

/*!
	Returns a pointer to the URL with given ID or 0 if no such URL exists.
 */
const MvdSharedData::UrlData* MvdSharedData::url(smdid id) const
{
	if (d->urls == 0)
		return 0;
	
	QHash<smdid,UrlData>::ConstIterator itr = d->urls->find(id);
	if (itr != d->urls->end())
		return &itr.value();
		
	return 0;
}


/************************************************************************
Retrieve all the SD data items
*************************************************************************/

/*!
	Returns a pointer to the persons list or 0 if no person has been added yet.
 */
const QHash<smdid,MvdSharedData::PersonData>* MvdSharedData::persons() const
{
	return d->persons;
}

/*!
	Returns a pointer to the genres list or 0 if no genre has been added yet.
 */
const QHash<smdid,MvdSharedData::StringData>* MvdSharedData::genres() const
{
	return d->genres;
}

/*!
	Returns a pointer to the tags list or 0 if no tag has been added yet.
 */
const QHash<smdid,MvdSharedData::StringData>* MvdSharedData::tags() const
{
	return d->tags;
}

/*!
	Returns a pointer to the country list or 0 if no country has been added yet.
*/
const QHash<smdid,MvdSharedData::StringData>* MvdSharedData::countries() const
{
	return d->countries;
}

/*!
	Returns a pointer to the languages list or 0 if no language has been added yet.
*/
const QHash<smdid,MvdSharedData::StringData>* MvdSharedData::languages() const
{
	return d->languages;
}

/*!
	Returns a pointer to the URL list or 0 if no URL has been added yet.
 */
const QHash<smdid,MvdSharedData::UrlData>* MvdSharedData::urls() const
{
	return d->urls;
}


/************************************************************************
Retrieve the number of movies, persons or both referencing a specific item
*************************************************************************/

//! Returns the number of items referencing a specific person.
int MvdSharedData::personUsageCount(smdid id, ReferenceTypes rt) const
{
	if (d->persons != 0)
		return 0;

	int count = 0;

	QHash<smdid,MvdSharedData::PersonData>::ConstIterator itr = d->persons->find(id);
	if (itr != d->persons->end())
	{
		if (rt.testFlag(MovieReferences))
			count += itr.value().movies.size();
		if (rt.testFlag(PersonReferences))
			count += itr.value().persons.size();
	}
	
	return count;
}

//! Returns the number of items referencing a specific genre.
int MvdSharedData::genreUsageCount(smdid id, ReferenceTypes rt) const
{
	if (d->genres == 0)
		return 0;
	
	int count = 0;

	QHash<smdid,MvdSharedData::StringData>::ConstIterator itr = d->genres->find(id);
	if (itr != d->genres->end())
	{
		if (rt.testFlag(MovieReferences))
			count += itr.value().movies.size();
		if (rt.testFlag(PersonReferences))
			count += itr.value().persons.size();
	}
	
	return count;
}

//! Returns the number of items referencing a specific tag.
int MvdSharedData::tagUsageCount(smdid id, ReferenceTypes rt) const
{
	if (d->tags == 0)
		return 0;
	
	int count = 0;

	QHash<smdid,MvdSharedData::StringData>::ConstIterator itr = d->tags->find(id);
	if (itr != d->tags->end())
	{
		if (rt.testFlag(MovieReferences))
			count += itr.value().movies.size();
		if (rt.testFlag(PersonReferences))
			count += itr.value().persons.size();
	}
	
	return count;
}

//! Returns the number of items referencing a specific URL.
int MvdSharedData::urlUsageCount(smdid id, ReferenceTypes rt) const
{
	if (d->urls == 0)
		return 0;
	
	int count = 0;

	QHash<smdid,MvdSharedData::UrlData>::ConstIterator itr = d->urls->find(id);
	if (itr != d->urls->end())
	{
		if (rt.testFlag(MovieReferences))
			count += itr.value().movies.size();
		if (rt.testFlag(PersonReferences))
			count += itr.value().persons.size();
	}
	
	return count;
}

//! Returns the number of items referencing a specific country.
int MvdSharedData::countryUsageCount(smdid id, ReferenceTypes rt) const
{	
	if (d->countries == 0)
		return 0;

	int count = 0;

	QHash<smdid,MvdSharedData::StringData>::ConstIterator itr = d->countries->find(id);
	if (itr != d->countries->end())
	{
		if (rt.testFlag(MovieReferences))
			count += itr.value().movies.size();
		if (rt.testFlag(PersonReferences))
			count += itr.value().persons.size();
	}
		
	return count;
}

//! Returns the number of items referencing a specific language.
int MvdSharedData::languageUsageCount(smdid id, ReferenceTypes rt) const
{
	if (d->languages == 0)
		return 0;

	int count = 0;

	QHash<smdid,MvdSharedData::StringData>::ConstIterator itr = d->languages->find(id);
	if (itr != d->languages->end())
	{
		if (rt.testFlag(MovieReferences))
			count += itr.value().movies.size();
		if (rt.testFlag(PersonReferences))
			count += itr.value().persons.size();
	}

	return count;
}


/************************************************************************
Retrieve an item ID by it's value
*************************************************************************/

/*!
	Returns the id of a person or 0 if there is no such person in the database.
 */
smdid MvdSharedData::findPerson(const QString& _fname, const QString& _lname) const
{
	if (d->persons == 0)
		return 0;
	
	QString fname = _fname.trimmed();
	QString lname = _lname.trimmed();

	if (lname.isEmpty() && fname.isEmpty())
		return 0;
		
	PersonData p;
	p.firstName = fname;
	p.lastName = lname;
	
	for (QHash<smdid,PersonData>::ConstIterator itr = d->persons->constBegin(); itr != d->persons->constEnd(); ++itr)
	{
		if (p == itr.value())
			return itr.key();
	}

	return 0;
}

/*!
	Returns the id of a genre or 0 if there is no such genre in the database.
 */
smdid MvdSharedData::findGenre(const QString& name) const
{
	if (d->genres == 0)
		return 0;

	if (name.isEmpty())
		return 0;
	
	StringData _d;
	_d.name = name;
	
	for (QHash<smdid,StringData>::ConstIterator itr = d->genres->constBegin(); itr != d->genres->constEnd(); ++itr)
	{
		if (itr.value() == _d)
			return itr.key();
	}

	return 0;
}

/*!
	Returns the id of a tag or 0 if there is no such tag in the database.
 */
smdid MvdSharedData::findTag(const QString& name) const
{
	if (d->tags == 0)
		return 0;
	
	if (name.isEmpty())
		return 0;

	StringData _d;
	_d.name = name;
	
	for (QHash<smdid,StringData>::ConstIterator itr = d->tags->constBegin(); itr != d->tags->constEnd(); ++itr)
	{
		if (itr.value() == _d)
			return itr.key();
	}

	return 0;
}

/*!
	Returns the id of a country or 0 if there is no such country in the database.
*/
smdid MvdSharedData::findCountry(const QString& name) const
{
	if (d->countries == 0)
		return 0;

	if (name.isEmpty())
		return 0;

	StringData _d;
	_d.name = name;

	for (QHash<smdid,StringData>::ConstIterator itr = d->countries->constBegin(); itr != d->countries->constEnd(); ++itr)
	{
		if (itr.value() == _d)
			return itr.key();
	}

	return 0;
}

/*!
	Returns the id of a language or 0 if there is no such language in the database.
*/
smdid MvdSharedData::findLanguage(const QString& name) const
{
	if (d->languages == 0)
		return 0;

	if (name.isEmpty())
		return 0;

	StringData _d;
	_d.name = name;

	for (QHash<smdid,StringData>::ConstIterator itr = d->languages->constBegin(); itr != d->languages->constEnd(); ++itr)
	{
		if (itr.value() == _d)
			return itr.key();
	}

	return 0;
}

/*!
	Returns the id of a URL or 0 if there is no such URL in the database.
 */
smdid MvdSharedData::findUrl(const QString& _url) const
{
	if (d->urls == 0)
		return 0;
	
	QString url = _url.trimmed();

	if (url.isEmpty())
		return 0;
		
	UrlData p;
	p.url = url;
	
	for (QHash<smdid,UrlData>::ConstIterator itr = d->urls->constBegin(); itr != d->urls->constEnd(); ++itr)
	{
		if (p == itr.value())
			return itr.key();
	}

	return 0;
}


/************************************************************************
Adds a new item to the SD (if no similar item exists)
*************************************************************************/

/*!
	Returns the ID associated to given person or adds a new one to the persons list.
	Returns 0 if both \p fname and \p lname are empty.
 */
smdid MvdSharedData::addPerson(const QString& _fname, const QString& _lname, const QList<smdid>& urls)
{
	MVD_SMD_CHECK_ID(d->personID)

	QString fname = _fname.trimmed();
	QString lname = _lname.trimmed();

	if (lname.isEmpty() && fname.isEmpty())
		return 0;

	PersonData _p;
	_p.firstName = fname;
	_p.lastName = lname;
	
	if (d->persons == 0)
	{
		d->persons = new QHash<smdid,PersonData>();
	}
	else
	{
		for (QHash<smdid,PersonData>::ConstIterator itr = d->persons->constBegin(); itr != d->persons->constEnd(); ++itr)
		{
			if (itr.value() == _p)
			{
				iLog() << tr("SD: Person already in list: %1 %2").arg(fname).arg(lname);
				return itr.key();
			}
		}
	}
	
	// add new person
	_p.urls = urls;
	
	d->canPurge = true;

	smdid newId = d->personID++;
	MVD_SET_HARDCODED((this == mvdGlobalSD), newId)

	d->persons->insert(newId, _p);
	iLog() << tr("SD: Person added: %1 %2 (id=%3)").arg(fname).arg(lname).arg(newId);
	emit personAdded(newId);
	return newId;
}

/*!
	Returns the index associated to given genre or adds a new one to the genres list.
	Returns 0 if \p genre is empty.
 */
smdid MvdSharedData::addGenre(const QString& _genre)
{
	MVD_SMD_CHECK_ID(d->genreID)

	QString genre = _genre.trimmed();

	if (genre.isEmpty())
		return 0;

	StringData _w;
	_w.name = genre;
	
	if (d->genres == 0)
	{
		d->genres = new QHash<smdid,StringData>();
	}
	else
	{
		for (QHash<smdid,StringData>::ConstIterator itr = d->genres->constBegin(); itr != d->genres->constEnd(); ++itr)
		{
			if (itr.value() == _w)
			{
				iLog() << tr("SD: Genre already in list: %1").arg(genre);
				return itr.key();
			}
		}
	}
	
	d->canPurge = true;

	smdid newId = d->genreID++;
	MVD_SET_HARDCODED((this == mvdGlobalSD), newId)

	d->genres->insert(newId, _w);
	iLog() << tr("SD: Genre added: %1 (id=%2)").arg(genre).arg(newId);
	emit genreAdded(newId);
	return newId;
}

/*!
	Adds a new tag to the database. Returns its id or 0 if \p tag is empty.
 */
smdid MvdSharedData::addTag(const QString& _tag)
{
	MVD_SMD_CHECK_ID(d->tagID)

	QString tag = _tag.trimmed();

	if (tag.isEmpty())
		return 0;

	StringData _w;
	_w.name = tag;
	
	if (d->tags == 0)
	{
		d->tags = new QHash<smdid,StringData>();
	}
	else
	{
		for (QHash<smdid,StringData>::ConstIterator itr = d->tags->constBegin(); itr != d->tags->constEnd(); ++itr)
		{
			if (itr.value() == _w)
			{
				iLog() << tr("SD: Tag already in list: %1").arg(tag);
				return itr.key();
			}
		}
	}

	
	d->canPurge = true;

	smdid newId = d->tagID++;
	MVD_SET_HARDCODED((this == mvdGlobalSD), newId)

	d->tags->insert(newId, _w);
	iLog() << tr("SD: Tag added: %1 (id=%2)").arg(tag).arg(newId);
	emit tagAdded(newId);
	return newId;
}

/*!
	Adds a new country to the database. Returns its id or 0 if \p country is empty.
*/
smdid MvdSharedData::addCountry(const QString& _country)
{
	MVD_SMD_CHECK_ID(d->countryID)

	QString country = _country.trimmed();

	if (country.isEmpty())
		return 0;

	StringData _w;
	_w.name = country;

	if (d->countries == 0)
	{
		d->countries = new QHash<smdid,StringData>();
	}
	else
	{
		for (QHash<smdid,StringData>::ConstIterator itr = d->countries->constBegin(); itr != d->countries->constEnd(); ++itr)
		{
			if (itr.value() == _w)
			{
				iLog() << tr("SD: Country already in list: %1").arg(country);
				return itr.key();
			}
		}
	}


	d->canPurge = true;

	smdid newId = d->countryID++;
	MVD_SET_HARDCODED((this == mvdGlobalSD), newId)

	d->countries->insert(newId, _w);
	iLog() << tr("SD: Country added: %1 (id=%2)").arg(country).arg(newId);
	emit countryAdded(newId);
	return newId;
}

/*!
	Adds a new language to the database. Returns its id or 0 if \p language is empty.
*/
smdid MvdSharedData::addLanguage(const QString& _language)
{
	MVD_SMD_CHECK_ID(d->languageID)

	QString language = _language.trimmed();

	if (language.isEmpty())
		return 0;

	StringData _w;
	_w.name = language;

	if (d->languages == 0)
	{
		d->languages = new QHash<smdid,StringData>();
	}
	else
	{
		for (QHash<smdid,StringData>::ConstIterator itr = d->languages->constBegin(); itr != d->languages->constEnd(); ++itr)
		{
			if (itr.value() == _w)
			{
				iLog() << tr("SD: Language already in list: %1").arg(language);
				return itr.key();
			}
		}
	}


	d->canPurge = true;

	smdid newId = d->languageID++;
	MVD_SET_HARDCODED((this == mvdGlobalSD), newId)

	d->languages->insert(newId, _w);
	iLog() << tr("SD: Language added: %1 (id=%2)").arg(language).arg(newId);
	emit languageAdded(newId);
	return newId;
}

/*!
	Adds a new URL to the database. Returns its id or 0 if \p url is empty.
 */
smdid MvdSharedData::addUrl(const QString& _url, const QString& _description)
{
	MVD_SMD_CHECK_ID(d->urlID)

	QString url = _url.trimmed();
	QString description = _description.trimmed();

	if (url.isEmpty())
		return 0;

	UrlData _w;
	_w.url = url;
	_w.description = description;

	if (d->urls == 0)
	{
		d->urls = new QHash<smdid,UrlData>();
	}
	else
	{
		for (QHash<smdid,UrlData>::ConstIterator itr = d->urls->constBegin(); itr != d->urls->constEnd(); ++itr)
		{
			if (itr.value() == _w)
			{
				iLog() << tr("SD: URL already in list: %1").arg(url);
				return itr.key();
			}
		}
	}

	
	d->canPurge = true;

	smdid newId = d->urlID++;
	MVD_SET_HARDCODED((this == mvdGlobalSD), newId)

	d->urls->insert(newId, _w);
	iLog() << tr("SD: URL added: %1 (id=%2)").arg(url).arg(newId);
	emit urlAdded(newId);
	return newId;
}


/************************************************************************
Changes an item with specific id
*************************************************************************/

/*!
	Changes the person description with given id.
	If \p id is a valid ID, a personChanged(id) is emitted and the function returns true.
	Items in the global SD cannot be changed or removed. The method always returns false if this is the global SD.
 */
bool MvdSharedData::changePerson(smdid id, const QString& _fname, const QString& _lname, const QList<smdid>& urls)
{
	if (this == mvdGlobalSD)
		return false;

	if (d->persons == 0)
		return false;

	QString lname = _lname.trimmed();
	QString fname = _fname.trimmed();

	if (lname.isEmpty() && fname.isEmpty())
		return false;
	
	QHash<smdid,PersonData>::Iterator itr = d->persons->find(id);
	if (itr == d->persons->end())
	{
		eLog() << tr("SD: changePerson() failed to resolve a SD ID.");
		return false;
	}
	
	PersonData& p = itr.value();
		
	iLog() << tr("SD: Person change. %1 %2 is now %3 %4").arg(p.firstName).arg(p.lastName).arg(fname).arg(lname);

	p.lastName = lname;
	p.firstName = fname;
	if (!urls.isEmpty())
		p.urls = urls;
	else p.urls.clear();
	
	emit personChanged(id);
	return true;
}

/*!
	Changes the genre with given id.
	If \p id is a valid ID, a genreChanged(id) is emitted and the function returns true.
	Items in the global SD cannot be changed or removed. The method always returns false if this is the global SD.
 */
bool MvdSharedData::changeGenre(smdid id, const QString& _name)
{
	if (this == mvdGlobalSD)
		return false;

	if (d->genres == 0)
		return false;

	QString name = _name.trimmed();

	if (name.isEmpty())
		return false;
	
	QHash<smdid, StringData>::Iterator itr = d->genres->find(id);
	if (itr == d->genres->end())
	{
		eLog() << tr("SD: changeGenre() failed to resolve a SD ID.");
		return false;
	}
	
	StringData& p = itr.value();

	iLog() << tr("SD: Genre change. %1 is now %2").arg(p.name).arg(name);

	p.name = name;	
	
	emit genreChanged(id);
	return true;
}

/*!
	Changes the tag with given id.
	If \p id is a valid ID, a tagChanged(id) is emitted and the function returns true.
	Items in the global SD cannot be changed or removed. The method always returns false if this is the global SD.
 */
bool MvdSharedData::changeTag(smdid id, const QString& _name)
{
	if (this == mvdGlobalSD)
		return false;

	if (d->tags == 0)
		return false;

	QString name = _name.trimmed();

	if (name.isEmpty())
		return false;

	QHash<smdid, StringData>::Iterator itr = d->tags->find(id);
	if (itr == d->tags->end())
	{
		eLog() << tr("SD: changeTag() failed to resolve a SD ID.");
		return false;
	}
	
	StringData& p = itr.value();
	
	iLog() << tr("SD: Tag change. %1 is now %2").arg(p.name).arg(name);

	p.name = name;	
		
	emit tagChanged(id);
	return id;
}

/*!
	Changes the country with given id.
	If \p id is a valid ID, a countryChanged(id) is emitted and the function returns true.
	Items in the global SD cannot be changed or removed. The method always returns false if this is the global SD.
*/
bool MvdSharedData::changeCountry(smdid id, const QString& _name)
{
	if (this == mvdGlobalSD)
		return false;

	if (d->countries == 0)
		return false;

	QString name = _name.trimmed();

	if (name.isEmpty())
		return false;

	QHash<smdid, StringData>::Iterator itr = d->countries->find(id);
	if (itr == d->countries->end())
	{
		eLog() << tr("SD: changeCountry() failed to resolve a SD ID.");
		return false;
	}

	StringData& p = itr.value();

	iLog() << tr("SD: Country change. %1 is now %2").arg(p.name).arg(name);

	p.name = name;	

	emit countryChanged(id);
	return id;
}

/*!
	Changes the language with given id.
	If \p id is a valid ID, a languageChanged(id) is emitted and the function returns true.
	Items in the global SD cannot be changed or removed. The method always returns false if this is the global SD.
*/
bool MvdSharedData::changeLanguage(smdid id, const QString& _name)
{
	if (this == mvdGlobalSD)
		return false;

	if (d->languages == 0)
		return false;

	QString name = _name.trimmed();

	if (name.isEmpty())
		return false;

	QHash<smdid, StringData>::Iterator itr = d->languages->find(id);
	if (itr == d->languages->end())
	{
		eLog() << tr("SD: languageTag() failed to resolve a SD ID.");
		return false;
	}

	StringData& p = itr.value();

	iLog() << tr("SD: Language change. %1 is now %2").arg(p.name).arg(name);

	p.name = name;	

	emit languageChanged(id);
	return id;
}

/*!
	Changes the URL with given id.
	If \p id is a valid ID, a urlChanged(id) is emitted and the function returns true.
	Items in the global SD cannot be changed or removed. The method always returns false if this is the global SD.
 */
bool MvdSharedData::changeUrl(smdid id, const QString& _name, const QString& _description)
{
	if (this == mvdGlobalSD)
		return false;

	if (d->tags == 0)
		return false;

	QString name = _name.trimmed();
	QString description = _description.trimmed();

	if (name.isEmpty())
		return false;

	QHash<smdid, UrlData>::Iterator itr = d->urls->find(id);
	if (itr == d->urls->end())
	{
		eLog() << tr("SD: changeUrl() failed to resolve a SD ID.");
		return false;
	}
	
	UrlData& p = itr.value();
	
	iLog() << tr("SD: URL change. %1 is now %2").arg(p.url).arg(name);

	p.url = name;
	p.description = description;

	emit urlChanged(id);
	return id;
}


/************************************************************************
Links or unlinks an item to or from a movie/person
*************************************************************************/

/*!
	Adds/removes a reference to a movie.
*/
void MvdSharedData::changePersonMovieReference(smdid person, ReferenceChange rc, movieid referencedBy)
{
	if (d->persons == 0)
		return;

	QHash<smdid, MvdSharedData::PersonData>::Iterator it = d->persons->find(person);
	if (it != d->persons->end())
	{
		d->linkMovie(it.value().movies, referencedBy, rc == AddReference);
	}
}

/*!
	Adds/removes a reference to a movie.
*/
void MvdSharedData::changeGenreMovieReference(smdid genre, ReferenceChange rc, movieid referencedBy)
{
	if (d->genres == 0)
		return;

	QHash<smdid, MvdSharedData::StringData>::Iterator it = d->genres->find(genre);
	if (it != d->genres->end())
	{
		d->linkMovie(it.value().movies, referencedBy, rc == AddReference);
	}
}

/*!
	Adds/removes a reference to a movie.
*/
void MvdSharedData::changeTagMovieReference(smdid tag, ReferenceChange rc, movieid referencedBy)
{
	if (d->tags == 0)
		return;

	QHash<smdid, MvdSharedData::StringData>::Iterator it = d->tags->find(tag);
	if (it != d->tags->end())
	{
		d->linkMovie(it.value().movies, referencedBy, rc == AddReference);
	}
}

/*!
	Adds/removes a reference to a movie.
*/
void MvdSharedData::changeCountryMovieReference(smdid country, ReferenceChange rc, movieid referencedBy)
{
	if (d->countries == 0)
		return;

	QHash<smdid, MvdSharedData::StringData>::Iterator it = d->countries->find(country);
	if (it != d->countries->end())
	{
		d->linkMovie(it.value().movies, referencedBy, rc == AddReference);
	}
}

/*!
	Adds/removes a reference to a movie.
*/
void MvdSharedData::changeLanguageMovieReference(smdid lang, ReferenceChange rc, movieid referencedBy)
{
	if (d->languages == 0)
		return;

	QHash<smdid, MvdSharedData::StringData>::Iterator it = d->languages->find(lang);
	if (it != d->languages->end())
	{
		d->linkMovie(it.value().movies, referencedBy, rc == AddReference);
	}
}

/*!
	Adds/removes a reference to a movie.
*/
void MvdSharedData::changeUrlMovieReference(smdid url, ReferenceChange rc, movieid referencedBy)
{
	if (d->urls == 0)
		return;

	QHash<smdid, MvdSharedData::UrlData>::Iterator it = d->urls->find(url);
	if (it != d->urls->end())
	{
		d->linkMovie(it.value().movies, referencedBy, rc == AddReference);
	}
}

/*!
	Adds/removes a reference to a person.
*/
void MvdSharedData::changePersonPersonReference(smdid person, ReferenceChange rc, smdid referencedBy)
{
	if (d->persons == 0)
		return;

	QHash<smdid, MvdSharedData::PersonData>::Iterator it = d->persons->find(person);
	if (it != d->persons->end())
	{
		d->linkMovie(it.value().persons, referencedBy, rc == AddReference);
	}
}

/*!
	Adds/removes a reference to a person.
*/
void MvdSharedData::changeUrlPersonReference(smdid url, ReferenceChange rc, smdid referencedBy)
{
	if (d->urls == 0)
		return;

	QHash<smdid, MvdSharedData::UrlData>::Iterator it = d->urls->find(url);
	if (it != d->urls->end())
	{
		d->linkMovie(it.value().persons, referencedBy, rc == AddReference);
	}
}

/*!
	Adds/removes a reference to a person.
*/
void MvdSharedData::changeTagPersonReference(smdid tag, ReferenceChange rc, smdid referencedBy)
{
	if (d->tags == 0)
		return;

	QHash<smdid, MvdSharedData::StringData>::Iterator it = d->tags->find(tag);
	if (it != d->tags->end())
	{
		d->linkMovie(it.value().persons, referencedBy, rc == AddReference);
	}
}

/*!
	Adds/removes a reference to a person.
*/
void MvdSharedData::changeCountryPersonReference(smdid country, ReferenceChange rc, smdid referencedBy)
{
	if (d->countries == 0)
		return;

	QHash<smdid, MvdSharedData::StringData>::Iterator it = d->countries->find(country);
	if (it != d->countries->end())
	{
		d->linkMovie(it.value().persons, referencedBy, rc == AddReference);
	}
}


/************************************************************************
Removes an item from the SD
*************************************************************************/

/*!
	Removes the crew member with given id.
	Items in the global SD cannot be changed or removed.
	The method always returns false if this is the global SD.
 */
bool MvdSharedData::removePerson(smdid id)
{
	Q_UNUSED(id);

	if (this == mvdGlobalSD)
		return false;

	//! \todo implement
	return false;
}

/*!
	Removes the genre with given id.
	Items in the global SD cannot be changed or removed.
	The method always returns false if this is the global SD.
 */
bool MvdSharedData::removeGenre(smdid id)
{
	Q_UNUSED(id);

	if (this == mvdGlobalSD)
		return false;

	//! \todo implement
	return false;
}

/*!
	Removes the tag with given id.
	Items in the global SD cannot be changed or removed.
	The method always returns false if this is the global SD.
 */
bool MvdSharedData::removeTag(smdid id)
{
	Q_UNUSED(id);

	if (this == mvdGlobalSD)
		return false;

	//! \todo implement
	return false;
}

/*!
	Removes the country with given id.
	Items in the global SD cannot be changed or removed.
	The method always returns false if this is the global SD.
*/
bool MvdSharedData::removeCountry(smdid id)
{
	Q_UNUSED(id);

	if (this == mvdGlobalSD)
		return false;

	//! \todo implement
	return false;
}

/*!
	Removes the language with given id.
	Items in the global SD cannot be changed or removed.
	The method always returns false if this is the global SD.
*/
bool MvdSharedData::removeLanguage(smdid id)
{
	Q_UNUSED(id);

	if (this == mvdGlobalSD)
		return false;

	//! \todo implement
	return false;
}

/*!
	Removes the URL with given id.
	Items in the global SD cannot be changed or removed.
	The method always returns false if this is the global SD.
 */
bool MvdSharedData::removeUrl(smdid id)
{
	Q_UNUSED(id);

	if (this == mvdGlobalSD)
		return false;

	//! \todo implement
	return false;
}


/************************************************************************
Convenience method, returns the number of items in the SD
*************************************************************************/

/*!
	Convenience method, returns the number of items contained in the SD.
*/
int MvdSharedData::count() const
{
	return countPersons() + countGenres() + countTags() + countUrls() + countCountries() + countLanguages();
}

/*!
	Returns the number of persons contained in the SD.
*/
int MvdSharedData::countPersons() const
{
	return d->persons == 0 ? 0 : d->persons->count();
}

/*!
	Returns the number of genres contained in the SD.
*/
int MvdSharedData::countGenres() const
{
	return d->genres == 0 ? 0 : d->genres->count();
}

/*!
	Returns the number of tags contained in the SD.
*/
int MvdSharedData::countTags() const
{
	return d->tags == 0 ? 0 : d->tags->count();
}

/*!
	Returns the number of URLs contained in the SD.
*/
int MvdSharedData::countUrls() const
{
	return d->urls == 0 ? 0 : d->urls->count();
}

/*!
	Returns the number of countries defined in this version of the application.
*/
int MvdSharedData::countCountries() const
{
	return d->countries == 0 ? 0 : d->countries->count();
}

/*!
	Returns the number of languages defined in this version of the application.
*/
int MvdSharedData::countLanguages() const
{
	return d->languages == 0 ? 0 : d->languages->count();
}


/************************************************************************
Utility methods
*************************************************************************/

/*!
	Enables or disables the auto-purge option.
 */
void MvdSharedData::setAutoPurge(bool enable)
{
	d->autoPurge = enable;
}

/*!
	Returns the status of the the auto-purge option.
 */
bool MvdSharedData::autoPurge() const
{
	return d->autoPurge;
}

/*!
	Returns true if there is some data in the SDB and if purgeData has not been called 
	since last insert.
 */
bool MvdSharedData::canPurge() const
{
	return d->canPurge;
}


/************************************************************************
Static utility methods
*************************************************************************/

/*!
	Returns true if a SD ID belongs to a hardcoded item.
*/
bool MvdSharedData::isHardcoded(smdid id)
{
	return ((id & 0x80000000) != 0);
}


/************************************************************************
Slots
*************************************************************************/

/*!
	Clears this SD object.
 */
void MvdSharedData::clear()
{
	// clear data
	if (d->persons != 0)
		d->persons->clear();
	if (d->genres != 0)
		d->genres->clear();
	if (d->tags != 0)
		d->tags->clear();
	if (d->countries != 0)
		d->countries->clear();
	if (d->languages != 0)
		d->languages->clear();
	if (d->urls != 0)
		d->urls->clear();
	
	d->personID = d->genreID = d->tagID = d->countryID = d->languageID = d->urlID = 1;
	d->canPurge = false;
	
	iLog() << tr("SD: Shared data cleared");
	emit dataCleared();
}

/*!
	Removes non hard coded data which is not linked to any movie or item.
	Returns true if some item has been removed.
 */
bool MvdSharedData::purgeData()
{
	QHash<smdid,PersonData>::Iterator pdIt;
	QHash<smdid,PersonData>::Iterator pdIt2;
	QHash<smdid,StringData>::Iterator sdIt;
	QHash<smdid,StringData>::Iterator sdIt2;
	QHash<smdid,UrlData>::Iterator udIt;
	QHash<smdid,UrlData>::Iterator udIt2;
	
	bool itemRemoved = false;
	
	if (d->persons != 0)
	{
		pdIt = d->persons->begin();
		while (pdIt != d->persons->end())
		{
			if (pdIt.value().movies.isEmpty() && pdIt.value().persons.isEmpty())
			{
				pdIt2 = pdIt;
				++pdIt;
				d->persons->erase(pdIt2);
				itemRemoved = true;
				if (pdIt == d->persons->end())
					break;
				else continue;
			}
			++pdIt;
		}
	}

	if (d->genres != 0)
	{
		sdIt = d->genres->begin();
		while (sdIt != d->genres->end())
		{
			if (sdIt.value().movies.isEmpty() && sdIt.value().persons.isEmpty())
			{
				sdIt2 = sdIt;
				++sdIt;
				d->genres->erase(sdIt2);
				itemRemoved = true;
				if (sdIt == d->genres->end())
					break;
				else continue;
			}
			++sdIt;
		}
	}

	if (d->tags != 0)
	{
		sdIt = d->tags->begin();
		while (sdIt != d->tags->end())
		{
			if (sdIt.value().movies.isEmpty() && sdIt.value().persons.isEmpty())
			{
				sdIt2 = sdIt;
				++sdIt;
				d->tags->erase(sdIt2);
				itemRemoved = true;
				if (sdIt == d->tags->end())
					break;
				else continue;
			}
			++sdIt;
		}
	}

	if (d->countries != 0)
	{
		sdIt = d->countries->begin();
		while (sdIt != d->countries->end())
		{
			if (sdIt.value().movies.isEmpty() && sdIt.value().persons.isEmpty())
			{
				sdIt2 = sdIt;
				++sdIt;
				d->countries->erase(sdIt2);
				itemRemoved = true;
				if (sdIt == d->countries->end())
					break;
				else continue;
			}
			++sdIt;
		}
	}

	if (d->languages != 0)
	{
		sdIt = d->languages->begin();
		while (sdIt != d->languages->end())
		{
			if (sdIt.value().movies.isEmpty() && sdIt.value().persons.isEmpty())
			{
				sdIt2 = sdIt;
				++sdIt;
				d->languages->erase(sdIt2);
				itemRemoved = true;
				if (sdIt == d->languages->end())
					break;
				else continue;
			}
			++sdIt;
		}
	}

	if (d->urls != 0)
	{
		udIt = d->urls->begin();
		while (udIt != d->urls->end())
		{
			if (udIt.value().movies.isEmpty() && udIt.value().persons.isEmpty())
			{
				udIt2 = udIt;
				++udIt;
				d->urls->erase(udIt2);
				itemRemoved = true;
				if (udIt == d->urls->end())
					break;
				else continue;
			}
			++udIt;
		}
	}
	
	d->canPurge = false;
	return itemRemoved;
}


/*!
	Returns the application unique global SD, used to resolve and store
	hard coded data items.
*/
MvdSharedData& Movida::globalSD()
{
	if (mvdGlobalSD == 0)
		mvdGlobalSD = new MvdSharedData;

	return *mvdGlobalSD;
}
