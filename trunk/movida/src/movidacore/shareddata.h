/**************************************************************************
** Filename: shareddata.h
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

#ifndef MVD_SHAREDMOVIEDATA_H
#define MVD_SHAREDMOVIEDATA_H

#include "global.h"

#include <QObject>
#include <QHash>
#include <QList>
#include <QString>
#include <QLocale>

class MvdSharedData;
class MvdSharedData_P;

typedef QList<smdid> SmdIdList;
typedef QList<movieid> MovieIdList;

class MVD_EXPORT MvdSharedData : public QObject
{
	Q_OBJECT

public:
	enum ReferenceType
	{
		MovieReferences = 0x01, 
		PersonReferences = 0x02, 
		AllReferences = MovieReferences | PersonReferences
	};
	Q_DECLARE_FLAGS(ReferenceTypes, ReferenceType)

	enum ReferenceChange { AddReference, RemoveReference };

	struct StringData
	{
		StringData();
		StringData(const StringData&);

		bool operator<(const StringData& s) const;
		bool operator==(const StringData& s) const;
		StringData& operator=(const StringData& s);

		QString name;
		
		MovieIdList movies;
		SmdIdList persons;
	};
	
	struct PersonData
	{
		PersonData();
		PersonData(const PersonData&);

		bool operator<(const PersonData& p) const;
		bool operator==(const PersonData& p) const;
		PersonData& operator=(const PersonData& p);

		QString firstName;
		QString lastName;

		smdid defaultUrl;
		SmdIdList urls;
		
		MovieIdList movies;
		SmdIdList persons;
	};

	struct UrlData
	{
		UrlData();
		UrlData(const UrlData&);

		bool operator<(const UrlData& s) const;
		bool operator==(const UrlData& s) const;
		UrlData& operator=(const UrlData& s);

		QString url;
		QString description;
		
		MovieIdList movies;
		SmdIdList persons;
	};

	MvdSharedData();
	MvdSharedData(const MvdSharedData& s);
	~MvdSharedData();
	MvdSharedData& operator=(const MvdSharedData& m);
	
	// Retrieve item by ID
	const PersonData* person(smdid id) const;
	const StringData* genre(smdid id) const;
	const StringData* tag(smdid id) const;
	const StringData* country(smdid id) const;
	const StringData* language(smdid id) const;
	const UrlData* url(smdid id) const;

	// Retrieve all SD items
	const QHash<smdid, PersonData>* persons() const;
	const QHash<smdid, StringData>* genres() const;
	const QHash<smdid, StringData>* tags() const;
	const QHash<smdid, StringData>* countries() const;
	const QHash<smdid, StringData>* languages() const;
	const QHash<smdid, UrlData>* urls() const;

	// Retrieve the number of movies, persons or both referencing a specific item
	int personUsageCount(smdid id, ReferenceTypes rt = AllReferences) const;
	int genreUsageCount(smdid id, ReferenceTypes rt = AllReferences) const;
	int tagUsageCount(smdid id, ReferenceTypes rt = AllReferences) const;
	int countryUsageCount(smdid id, ReferenceTypes rt = AllReferences) const;
	int languageUsageCount(smdid id, ReferenceTypes rt = AllReferences) const;
	int urlUsageCount(smdid id, ReferenceTypes rt = AllReferences) const;

	// Retrieve an item ID by it's value
	//! \todo Add fuzzy item search
	smdid findPerson(const QString& fname, const QString& lname) const;
	smdid findGenre(const QString& name) const;
	smdid findTag(const QString& name) const;
	smdid findCountry(const QString& name) const;
	smdid findLanguage(const QString& name) const;
	smdid findUrl(const QString& url) const;

	// Adds a new item to the SD (no duplicates are added)
	smdid addPerson(const QString& fname, const QString& lname, const SmdIdList& urls = SmdIdList());
	smdid addGenre(const QString& genre);
	smdid addTag(const QString& name);
	smdid addCountry(const QString& name);
	smdid addLanguage(const QString& name);
	smdid addUrl(const QString& url, const QString& description = QString());
	
	// Changes an item with specific id
	bool changePerson(smdid id, const QString& fname, const QString& lname, const SmdIdList& urls = SmdIdList());
	bool changeGenre(smdid id, const QString& genre);
	bool changeTag(smdid id, const QString& name);
	bool changeCountry(smdid id, const QString& name);
	bool changeLanguage(smdid id, const QString& name);
	bool changeUrl(smdid id, const QString& url, const QString& description = QString());
	
	// Links or unlinks an item to or from a movie or person
	void changePersonMovieReference(smdid person, ReferenceChange rc, movieid referencedBy);
	void changeGenreMovieReference(smdid genre, ReferenceChange rc, movieid referencedBy);
	void changeTagMovieReference(smdid tag, ReferenceChange rc, movieid referencedBy);
	void changeCountryMovieReference(smdid country, ReferenceChange rc, movieid referencedBy);
	void changeLanguageMovieReference(smdid language, ReferenceChange rc, movieid referencedBy);
	void changeUrlMovieReference(smdid url, ReferenceChange rc, movieid referencedBy);

	void changePersonPersonReference(smdid person, ReferenceChange rc, smdid referencedBy);
	void changeUrlPersonReference(smdid url, ReferenceChange rc, smdid referencedBy);
	void changeTagPersonReference(smdid tag, ReferenceChange rc, smdid referencedBy);
	void changeCountryPersonReference(smdid country, ReferenceChange rc, smdid referencedBy);

	// Removes an item from the SD. Automatically removes the item from all referenced movies/items.
	bool removePerson(smdid id);
	bool removeGenre(smdid id);
	bool removeTag(smdid id);
	bool removeCountry(smdid id);
	bool removeLanguage(smdid id);
	bool removeUrl(smdid id);
	
	// Convenience method, returns the number of items in the SD
	int count() const;
	int countPersons() const;
	int countGenres() const;
	int countTags() const;
	int countUrls() const;
	int countCountries() const;
	int countLanguages() const;

	// Utility methods
	void setAutoPurge(bool enable);
	bool autoPurge() const;
	bool canPurge() const;

	// Static utility methods
	static bool isHardcoded(smdid id);
	
public slots:
	void clear();
	bool purgeData();
	
signals:
	void personAdded(int id);
	void personRemoved(int id);
	void personChanged(int id);
	void personReferenceChanged(int id);
	
	void genreAdded(int id);
	void genreRemoved(int id);
	void genreChanged(int id);
	void genreReferenceChanged(int id);
	
	void tagAdded(int id);
	void tagChanged(int id);
	void tagRemoved(int id);
	void tagReferenceChanged(int id);
	
	void countryAdded(int id);
	void countryChanged(int id);
	void countryRemoved(int id);
	void countryReferenceChanged(int id);

	void languageAdded(int id);
	void languageChanged(int id);
	void languageRemoved(int id);
	void languageReferenceChanged(int id);

	void urlAdded(int id);
	void urlChanged(int id);
	void urlRemoved(int id);
	void urlReferenceChanged(int id);

	void dataCleared();
	
private:
	MvdSharedData_P* d;
};
 Q_DECLARE_OPERATORS_FOR_FLAGS(MvdSharedData::ReferenceTypes)

namespace Movida
 {
	 MVD_EXPORT extern MvdSharedData& globalSD();
 }

#endif // MVD_SHAREDMOVIEDATA_H
