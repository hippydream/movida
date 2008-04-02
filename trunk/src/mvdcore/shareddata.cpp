/**************************************************************************
** Filename: shareddata.cpp
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

#include "shareddata.h"
#include "logger.h"
#include "global.h"
#include "sditem.h"
#include "core.h"
#include <QImage>
#include <QStringList>

#define MVD_SD_CHECK_ID\
	if (d->nextId == 0xFFFFFFFF) \
	{\
		eLog() << "MvdSharedData: available IDs exhausted.";\
		return MvdNull;\
	}

using namespace Movida;

/*!
	\class MvdSharedData shareddata.h
	\ingroup MvdCore

	\brief Stores common data such as person names or genres.

	Every movie collection has its own shared data (SD) object.
	The SD records associations between shared items and movies (or other
	shared objects) that reference them.
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

	inline void logNewItem(const MvdSdItem& item);

	QHash<mvdid,MvdSdItem> data;

	mvdid nextId;

	//! true if items with nobody referencing them should be removed.
	bool autoPurge;
	//! true if there is some data in the SDB and if purgeData has not been called since last insert.
	bool canPurge;
};

//! \internal
MvdSharedData_P::MvdSharedData_P()
{
	nextId = 1;
	autoPurge = true;
	canPurge = false;
}

//! \internal
MvdSharedData_P::MvdSharedData_P(const MvdSharedData_P& s)
{
	data = s.data;
	nextId = s.nextId;
	autoPurge = s.autoPurge;
	canPurge = s.canPurge;
}

//! \internal
MvdSharedData_P::~MvdSharedData_P()
{
}

//! \internal
void MvdSharedData_P::logNewItem(const MvdSdItem& item)
{
	iLog() << "MvdSharedData: Item added: " << item.value << "("
		<< (item.id.isEmpty() ? QString("no id; ") : QString("id: %1; ").arg(item.id))
		<< (item.description.isEmpty() ? QString("no descr.") : QString("descr.: %1").arg(item.description))
		<< ")";
}

/************************************************************************
MvdSharedData
*************************************************************************/

QString MvdSharedData::roleToString(Movida::DataRole role)
{
	if (role & Movida::ActorRole || role & Movida::DirectorRole 
		|| role & Movida::ProducerRole || role & CrewMemberRole)
		role = Movida::PersonRole;

	switch (role)
	{
	case Movida::PersonRole: return "PersonItem";
	case Movida::GenreRole: return "GenreItem";
	case Movida::TagRole: return "TagItem";
	case Movida::LanguageRole: return "LanguageItem";
	case Movida::CountryRole: return "CountryItem";
	default: Q_ASSERT_X(false, "MvdSharedData::roleToString()", "Internal error");
	}

	return QString();
}

Movida::DataRole MvdSharedData::roleFromString(QString role)
{
	role = role.trimmed().toLower();

	if (role == "personitem")
		return Movida::PersonRole;
	if (role == "genreitem")
		return Movida::GenreRole;
	if (role == "tagitem")
		return Movida::TagRole;
	if (role == "languageitem")
		return Movida::LanguageRole;
	if (role == "countryitem")
		return Movida::CountryRole;

	return Movida::NoRole;
}

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
MvdSharedData::MvdSharedData(const MvdSharedData& m)
: QObject(), d(new MvdSharedData_P(*(m.d)))
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
	emit destroyed();
	delete d;
}

/************************************************************************
Retrieve item(s)
*************************************************************************/

/*!
	Returns the item with given ID or a null item.
 */
MvdSdItem MvdSharedData::item(mvdid id) const
{
	if (d->data.isEmpty())
		return MvdSdItem();
	
	QHash<mvdid,MvdSdItem>::ConstIterator it = d->data.find(id);
	if (it != d->data.end())
		return it.value();

	return MvdSdItem();
}

/*!
*/
MvdSharedData::ItemList MvdSharedData::items(Movida::DataRole role) const
{
	if (role == NoRole)
		return d->data;
	
	QHash<mvdid, MvdSdItem> results;
	for (QHash<mvdid,MvdSdItem>::ConstIterator it = d->data.constBegin();
		it != d->data.constEnd(); ++it)
	{
		if (it.value().role & role)
			results.insert(it.key(), it.value());
	}

	return results;
}

/*!
	Returns MvdNull if \p item could not be found.
*/
mvdid MvdSharedData::findItem(const MvdSdItem& item) const
{
	for (QHash<mvdid,MvdSdItem>::ConstIterator it = d->data.constBegin();
		it != d->data.constEnd(); ++it)
	{
		if (it.value() == item)
			return it.key();
	}

	return MvdNull;
}

/*!
	Returns MvdNull if \p item could not be found.
*/
mvdid MvdSharedData::findItemByValue(QString value, Qt::CaseSensitivity cs) const
{
	value = value.trimmed();
	for (QHash<mvdid,MvdSdItem>::ConstIterator it = d->data.constBegin();
		it != d->data.constEnd(); ++it)
	{
		if (!QString::compare(it.value().value, value, cs))
			return it.key();
	}

	return MvdNull;
}

/************************************************************************
Retrieve the number of movies, persons or both referencing a specific item
*************************************************************************/

//! Returns the number of movies, persons or both referencing a specific item.
int MvdSharedData::usageCount(mvdid id, ReferenceTypes rt) const
{
	if (d->data.isEmpty())
		return 0;

	int count = 0;
	
	QHash<mvdid,MvdSdItem>::ConstIterator it = d->data.find(id);
	if (it != d->data.end())
	{
		if (rt.testFlag(MovieReferences))
			count += it.value().movies.size();
		if (rt.testFlag(PersonReferences))
			count += it.value().persons.size();
	}

	return count;
}

/************************************************************************
Adds a new item to the SD (if no similar item exists)
*************************************************************************/

/*!
	Adds a new item to the data store and returns its ID. Only returns the
	ID if an equal item exists.
 */
mvdid MvdSharedData::addItem(const MvdSdItem& item)
{
	MVD_SD_CHECK_ID

	if (item.value.isEmpty())
		return MvdNull;
	
	for (QHash<mvdid,MvdSdItem>::ConstIterator it = d->data.constBegin(); 
		it != d->data.constEnd(); ++it)
	{
		if (it.value() == item)
		{
			iLog() << QString("MvdSharedData: Item %1 already registered").arg(item.value);
			return it.key();
		}
	}	
	
	d->canPurge = true;

	int maxLength = MvdCore::parameter("mvdcore/max-edit-length").toInt();

	mvdid newId = d->nextId++;
	if (item.value.length() > maxLength || item.description.length() > maxLength) {
		MvdSdItem _item(item);
		_item.value.truncate(maxLength);
		_item.description.truncate(maxLength);
		d->data.insert(newId, _item);
		d->logNewItem(_item);
	} else {
		d->data.insert(newId, item);
		d->logNewItem(item);
	}
	emit itemAdded(newId);
	return newId;
}


/************************************************************************
Updates
*************************************************************************/

/*!
	Updates the item with given id.
	If \p id is a valid ID, a itemUpdated(id) is emitted and the function returns true.
	Returns false if \p id is not a valid id.
 */
bool MvdSharedData::updateItem(mvdid id, const MvdSdItem& item)
{
	if (d->data.isEmpty())
		return false;

	QHash<mvdid,MvdSdItem>::Iterator it = d->data.find(id);
	if (it == d->data.end())
	{
		eLog() << "MvdSharedData: item update failed to resolve an ID.";
		return false;
	}
	
	iLog() << QString("MvdSharedData: Item %1 updated.").arg(it.value().value);

	d->data.insert(id, item);
	emit itemUpdated(id);
	return true;
}

/************************************************************************
Links or unlinks an item to or from a movie/person
*************************************************************************/

/*!
	Adds a reference to a movie.
*/
void MvdSharedData::addMovieLink(mvdid sd_id, mvdid movie_id)
{
	if (d->data.isEmpty())
		return;

	QHash<mvdid, MvdSdItem>::Iterator it = d->data.find(sd_id);
	if (it != d->data.end())
	{
		bool linked = false;
		QList<mvdid>& list = it.value().movies;
		for (int i = 0; i < list.size(); ++i)
		{
			mvdid m = list.at(i);
			if (m == movie_id)
				break;
			else if (m > movie_id)
			{
				list.insert(i, movie_id);
				linked = true;
				break;
			}
		}

		if (!linked)
			list.append(movie_id);
		
		emit itemReferenceChanged(sd_id);
	}
}

/*!
	Removes a reference to a movie.
*/
void MvdSharedData::removeMovieLink(mvdid sd_id, mvdid movie_id)
{
	if (d->data.isEmpty())
		return;

	QHash<mvdid, MvdSdItem>::Iterator it = d->data.find(sd_id);
	if (it != d->data.end())
	{
		QList<mvdid>& list = it.value().movies;
		for (int i = 0; i < list.size(); ++i)
		{
			mvdid m = list.at(i);
			if (m == movie_id)
			{
				list.removeAt(i);
				emit itemReferenceChanged(sd_id);
				break;
			}
			else if (m > movie_id)
				break;
		}
	}
}

/*!
	Adds a reference to a person.
*/
void MvdSharedData::addPersonLink(mvdid sd_id, mvdid person_sd_id)
{
	if (d->data.isEmpty())
		return;

	QHash<mvdid, MvdSdItem>::Iterator it = d->data.find(sd_id);
	if (it != d->data.end())
	{
		bool linked = false;
		QList<mvdid>& list = it.value().persons;
		for (int i = 0; i < list.size(); ++i)
		{
			mvdid m = list.at(i);
			if (m == person_sd_id)
				break;
			else if (m > person_sd_id)
			{
				list.insert(i, person_sd_id);
				linked = true;
				break;
			}
		}

		if (!linked)
			list.append(person_sd_id);
		emit itemReferenceChanged(sd_id);
	}
}

/*!
	Removes a reference to a movie.
*/
void MvdSharedData::removePersonLink(mvdid sd_id, mvdid person_sd_id)
{
	if (d->data.isEmpty())
		return;

	QHash<mvdid, MvdSdItem>::Iterator it = d->data.find(sd_id);
	if (it != d->data.end())
	{
		QList<mvdid>& list = it.value().persons;
		for (int i = 0; i < list.size(); ++i)
		{
			mvdid m = list.at(i);
			if (m == person_sd_id)
			{
				list.removeAt(i);
				emit itemReferenceChanged(sd_id);
				break;
			}
			else if (m > person_sd_id)
				break;
		}
	}
}


/************************************************************************
Removes an item from the SD
*************************************************************************/

/*!
	Removes an item. Returns false if no such item exists.
 */
bool MvdSharedData::removeItem(mvdid id)
{	
	bool removed = d->data.remove(id) > 0;
	if (removed)
		emit itemRemoved(id);
	return removed;
}

/************************************************************************
Convenience method, returns the number of items in the SD
*************************************************************************/

/*!
	Returns the number of items contained in the SD.
	More types can be OR-combined in \p type.

	\todo USE A ROLE*s* QFLAG
*/
int MvdSharedData::countItems(Movida::DataRole role) const
{
	if (role == Movida::NoRole)
		return d->data.size();
	
	int count = 0;
	for (QHash<mvdid, MvdSdItem>::ConstIterator it = d->data.constBegin();
		it != d->data.constEnd(); ++it)
		if (it.value().role & role)
			count++;
	return count;
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
	Returns true if there is some data in the SD and if purgeData has not been called 
	since last insert.
 */
bool MvdSharedData::canPurge() const
{
	return d->canPurge;
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
	d->data.clear();
	d->nextId = 1;
	d->canPurge = false;
	
	iLog() << "MvdSharedData: Shared data cleared.";
	emit cleared();
}

/*!
	Removes non hard coded data which is not linked to any movie or item.
	Returns true if some item has been removed.
 */
bool MvdSharedData::purgeData()
{
	QHash<mvdid,MvdSdItem>::Iterator pdIt;
	QHash<mvdid,MvdSdItem>::Iterator pdIt2;
	
	bool itemRemoved = false;
	
	if (d->data.isEmpty())
	{
		pdIt = d->data.begin();
		while (pdIt != d->data.end())
		{
			if (pdIt.value().movies.isEmpty() && pdIt.value().persons.isEmpty())
			{
				pdIt2 = pdIt;
				++pdIt;
				d->data.erase(pdIt2);
				itemRemoved = true;
				if (pdIt == d->data.end())
					break;
				else continue;
			}
			++pdIt;
		}
	}
	
	d->canPurge = false;
	return itemRemoved;
}
