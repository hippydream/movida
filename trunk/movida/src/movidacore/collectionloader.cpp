/**************************************************************************
** Filename: collectionloader.cpp
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

#include "collectionloader.h"
#include "shareddata.h"
#include "logger.h"
#include "global.h"
#include "moviecollection.h"
#include "unzip.h"
#include "md5.h"
#include "settings.h"
#include "pathresolver.h"
#include "movie.h"
#include "core.h"

#include <QString>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QFileInfo>
#include <QTextStream>
#include <QCoreApplication>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

using namespace Movida;


/*!
	\class MvdCollectionLoader collectionloader.h
	\ingroup movidacore

	\brief Loads a movie collection from a Movida mmc file.
	\todo No message box is shown in case of error now! Check calling code!
*/


/************************************************************************
MvdCollectionLoader_P
*************************************************************************/

//! \internal
namespace MvdCollectionLoader_P
{
	//! \internal
	struct IdMapper
	{
		void clear()
		{
			c_persons.clear();
			c_genres.clear();
			c_tags.clear();
			c_languages.clear();
			c_countries.clear();
			c_urls.clear();
		}

		// User-defined items only!
		QHash<quint32,smdid> c_persons;
		QHash<quint32,smdid> c_genres;
		QHash<quint32,smdid> c_tags;
		QHash<quint32,smdid> c_languages;
		QHash<quint32,smdid> c_countries;
		QHash<quint32,smdid> c_urls;
	};

	static inline bool checkArchiveVersion(const QString& attribute);
	static inline bool loadXmlDocument(const QString& path, xmlDocPtr* doc, xmlNodePtr* cur, 
		int* itemCount);

	static inline void parseUrlDescriptions(const IdMapper& idMapper, 
		xmlDocPtr doc, xmlNodePtr node, smdid* defaultValue, 
		QList<smdid>* urls);
	static inline QStringList parseStringDescriptions(xmlDocPtr doc, xmlNodePtr node, 
		const QString& tag = "item");
	static inline void parseCollection(const IdMapper& idMapper, 
		MvdMovieCollection* collection, 
		xmlDocPtr doc, xmlNodePtr cur, int itemCount);
	static inline void parsePersonDescriptions(IdMapper* idMapper, 
		MvdMovieCollection* collection, xmlDocPtr doc, 
		xmlNodePtr cur, int itemCount);
	static inline void parseSimpleItemDescriptions(IdMapper* idMapper, 
		MvdMovieCollection* collection, xmlDocPtr doc,
		xmlNodePtr cur, int itemCount, Movida::SmdDataRole role);
	static inline void parsePersonIdList(const IdMapper& idMapper, 
		MvdMovie* movie, xmlDocPtr doc, xmlNodePtr cur, 
		Movida::SmdDataRole role);
	static inline void parseSimpleIdList(const IdMapper& idMapper, 
		MvdMovie* movie, xmlDocPtr doc, xmlNodePtr cur, 
		Movida::SmdDataRole role);
};

/*! 
	\internal Parses person descriptions from an xml file and adds 
	them to the SD database.
	
	\warning The urls section should have been parsed before calling this 
	method or user defined link IDs will not be resolved.

	\warning xmlFreeDoc is not called within this method; you will have to 
	free resources after calling it.
*/
void MvdCollectionLoader_P::parsePersonDescriptions(IdMapper* idMapper,
	MvdMovieCollection* collection,
	xmlDocPtr doc, xmlNodePtr cur, int itemCount)
{
	Q_UNUSED(itemCount)

	xmlChar* attr = 0;
	cur = cur->xmlChildrenNode;

	xmlNodePtr personNode = 0;
	QString nodeName;

	while (cur)
	{
		if (cur->type != XML_ELEMENT_NODE || xmlStrcmp(cur->name, (const xmlChar*) "person"))
		{
			cur = cur->next;
			continue;
		}

		attr = xmlGetProp(cur, (const xmlChar*) "id");
		if (!attr)
		{
			cur = cur->next;
			continue;
		}

		quint32 id = MvdCore::atoui32((const char*)attr);
		xmlFree(attr);
		if (id == 0)
		{
			cur = cur->next;
			continue;
		}

		QString fname;
		QString lname;
		QList<smdid> urls;
		smdid defaultUrl = 0;

		personNode = cur->xmlChildrenNode;
		while (personNode)
		{
			nodeName = (const char*) personNode->name;

			if (nodeName == "first-name")
				fname = QString((const char*)xmlNodeListGetString(doc, personNode->xmlChildrenNode, 1));
			else if (nodeName == "last-name")
				lname = QString((const char*)xmlNodeListGetString(doc, personNode->xmlChildrenNode, 1));
			else if (nodeName == "urls")
				parseUrlDescriptions(*idMapper, doc, personNode, &defaultUrl, &urls);

			personNode = personNode->next;

		} // while (personNode)


		if (!(lname.isEmpty() && fname.isEmpty()))
		{
			smdid smd_id = collection->smd().addPerson(fname, lname, urls);
			if (smd_id > 0)
				idMapper->c_persons.insert(id, smd_id);
		}

		cur = cur->next;

	} // while (cur != 0)
}

/*!
	\internal Parses simple string descriptions from an XML file and adds 
	them to the SD database. \p role determines what to do with the data.
	
	\warning xmlFreeDoc is not called within this method; you will have to 
	free resources after calling it.
*/
void MvdCollectionLoader_P::parseSimpleItemDescriptions(IdMapper* idMapper,
	MvdMovieCollection* collection,
	xmlDocPtr doc, xmlNodePtr cur, int itemCount, Movida::SmdDataRole role)
{
	Q_UNUSED(itemCount)

	xmlChar* attr = 0;
	cur = cur->xmlChildrenNode;

	while (cur)
	{
		if (cur->type != XML_ELEMENT_NODE)
		{
			cur = cur->next;
			continue;
		}

		if (xmlStrcmp(cur->name, (const xmlChar*) "item"))
		{
			cur = cur->next;
			continue;
		}

		attr = xmlGetProp(cur, (const xmlChar*) "id");
		if (attr == 0)
		{
			cur = cur->next;
			continue;
		}

		quint32 id = MvdCore::atoui32((const char*)attr);
		xmlFree(attr);
		if (id == 0)
		{
			cur = cur->next;
			continue;
		}

		QString val = QString((const char*)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));

		if (!val.isEmpty())
		{
			smdid smd_id;
			switch(role)
			{
			case Movida::TagRole:
				{
					smd_id = collection->smd().addTag(val);
					if (smd_id > 0)
						idMapper->c_tags.insert(id, smd_id);
					break;
				}
			case Movida::CountryRole:
				{
					smd_id = collection->smd().addCountry(val);
					if (smd_id > 0)
						idMapper->c_countries.insert(id, smd_id);
					break;
				}
			case Movida::LanguageRole:
				{
					smd_id = collection->smd().addLanguage(val);
					if (smd_id > 0)
						idMapper->c_languages.insert(id, smd_id);
					break;
				}
			case Movida::UrlRole:
				{
					attr = xmlGetProp(cur, (const xmlChar*) "description");
					if (attr != 0)
					{
						smd_id = collection->smd().addUrl(val, QString( (const char*)attr ));
						xmlFree(attr);
					}
					smd_id = collection->smd().addUrl(val);
					if (smd_id > 0)
						idMapper->c_urls.insert(id, smd_id);
					break;
				}
			case Movida::GenreRole:
				{
					smd_id = collection->smd().addGenre(val);
					if (smd_id > 0)
						idMapper->c_genres.insert(id, smd_id);
					break;
				}
			default: ;
			}

		} // if (!val.isempty)

		cur = cur->next;
	}
}

/*!
	\internal Parses movie descriptions from an XML file.
	
	\warning xmlFreeDoc is not called within this method; you will have to 
	free resources after calling it.
*/
void MvdCollectionLoader_P::parseCollection(const IdMapper& idMapper, 
	MvdMovieCollection* collection, 
	xmlDocPtr doc, xmlNodePtr cur, int itemCount)
{
	Q_UNUSED(itemCount)

	QString posterDir = collection->info(MvdMovieCollection::DataPathInfo)
		.append("/images/");

	xmlChar* attr = 0;
	cur = cur->xmlChildrenNode;

	xmlNodePtr mNode = 0;
	QString nodeName;

	while (cur)
	{
		if (cur->type != XML_ELEMENT_NODE || xmlStrcmp(cur->name, (const xmlChar*) "movie"))
		{
			cur = cur->next;
			continue;
		}

		attr = xmlGetProp(cur, (const xmlChar*) "title");
		if (!attr)
		{
			cur = cur->next;
			continue;
		}

		MvdMovie movie;
		if (attr)
		{
			movie.setTitle(QString((const char*)attr));
			xmlFree(attr);
		}

		// Parse <movie> node
		mNode = cur->children;

		while (mNode)
		{
			if (mNode->type != XML_ELEMENT_NODE)
			{
				mNode = mNode->next;
				continue;
			}

			nodeName = (const char*) mNode->name;

			if (nodeName == "cast")
				parsePersonIdList(idMapper, &movie, doc, mNode, Movida::ActorRole);
			else if (nodeName == "tags")
				parseSimpleIdList(idMapper, &movie, doc, mNode, Movida::TagRole);
			else if (nodeName == "color-mode")
			{
				attr = xmlGetProp(mNode, (const xmlChar*) "value");
				if (attr != 0)
				{
					nodeName = (const char*) attr;
					xmlFree(attr);

					if (nodeName == "bw")
						movie.setColorMode(MvdMovie::BlackWhite);
					else if (nodeName == "color")
						movie.setColorMode(MvdMovie::Color);					
				}
			}
			else if (nodeName == "countries")
				parseSimpleIdList(idMapper, &movie, doc, mNode, Movida::CountryRole);
			else if (nodeName == "languages")
				parseSimpleIdList(idMapper, &movie, doc, mNode, Movida::LanguageRole);
			else if (nodeName == "crew")
				parsePersonIdList(idMapper, &movie, doc, mNode, Movida::CrewMemberRole);
			else if (nodeName == "directors")
				parsePersonIdList(idMapper, &movie, doc, mNode, Movida::DirectorRole);
			else if (nodeName == "genres")
				parseSimpleIdList(idMapper, &movie, doc, mNode, Movida::GenreRole);
			else if (nodeName == "imdb-id")
			{
				attr = xmlNodeListGetString(doc, mNode->xmlChildrenNode, 1);
				if (attr)
				{
					quint32 idLen = MvdCore::parameter("movidacore-imdb-id-length").toInt();
					if (strlen((const char*)attr) == idLen)
					{
						quint32 id = MvdCore::atoui32((const char*)attr);
						if (id != 0)
							movie.setImdbId(QString::number(id));
					}
					xmlFree(attr);
				}
			}
			else if (nodeName == "running-time")
			{
				attr = xmlNodeListGetString(doc, mNode->xmlChildrenNode, 1);
				if (attr)
				{
					quint32 minutes = MvdCore::atoui32((const char*)attr);
					movie.setRunningTime(minutes);
					xmlFree(attr);
				}
			}
			else if (nodeName == "urls")
			{
				smdid def = 0;
				QList<smdid> urls;
				parseUrlDescriptions(idMapper, doc, mNode, &def, &urls);
				if (!urls.isEmpty())
					movie.setUrls(urls, def);
			}
			else if (nodeName == "notes")
			{
				/*! \todo multiple notes handling with optional automatic 
					title generation like in opera web browser
				*/
				attr = xmlNodeListGetString(doc, mNode->xmlChildrenNode, 1);
				if (attr)
				{
					movie.setNotes(QString((const char*)attr));
					xmlFree(attr);
				}
			}
			else if (nodeName == "original-title")
			{
				attr = xmlNodeListGetString(doc, mNode->xmlChildrenNode, 1);
				if (attr)
				{
					movie.setOriginalTitle(QString((const char*)attr));
					xmlFree(attr);
				}
			}
			else if (nodeName == "plot")
			{
				attr = xmlNodeListGetString(doc, mNode->xmlChildrenNode, 1);
				if (attr)
				{
					movie.setPlot(QString((const char*)attr));
					xmlFree(attr);
				}
			}
			else if (nodeName == "producers")
				parsePersonIdList(idMapper, &movie, doc, mNode, Movida::ProducerRole);
			else if (nodeName == "production-year")
			{
				attr = xmlNodeListGetString(doc, mNode->xmlChildrenNode, 1);
				if (attr)
				{
					movie.setProductionYear(QString((const char*)attr));
					xmlFree(attr);
				}
			}
			else if (nodeName == "rating")
			{
				attr = xmlNodeListGetString(doc, mNode->xmlChildrenNode, 1);
				if (attr)
				{
					movie.setRating(MvdCore::atoui32((const char*) attr));
					xmlFree(attr);
				}
			}
			else if (nodeName == "release-year")
			{
				attr = xmlNodeListGetString(doc, mNode->xmlChildrenNode, 1);
				if (attr)
				{
					movie.setReleaseYear(QString((const char*)attr));
					xmlFree(attr);
				}
			}
			else if (nodeName == "storage-id")
			{
				attr = xmlNodeListGetString(doc, mNode->xmlChildrenNode, 1);
				if (attr)
				{
					movie.setStorageId(QString((const char*)attr));
					xmlFree(attr);
				}
			}
			else if (nodeName == "special-contents")
			{
				QStringList list = parseStringDescriptions(doc, mNode);
				movie.setSpecialContents(list);
			}
			else if (nodeName == "edition")
			{
				attr = xmlNodeListGetString(doc, mNode->xmlChildrenNode, 1);
				if (attr)
				{
					movie.setEdition(QString((const char*)attr));
					xmlFree(attr);
				}
			}
			else if (nodeName == "poster")
			{
				attr = xmlNodeListGetString(doc, mNode->xmlChildrenNode, 1);
				if (attr)
				{
					QString poster = QString((const char*)attr);
					xmlFree(attr);
					if (!QFile::exists(posterDir + poster))
					{
						Movida::wLog() << QString("Missing movie poster: %1")
							.arg(posterDir + poster);
					}
					else
						movie.setPoster(poster);
				}
			}

			mNode = mNode->next;
		}

		cur = cur->next;

		collection->addMovie(movie);

	} // loop over <movie> nodes
}

/*!
	\internal Parses a list of url-IDs.
*/
void MvdCollectionLoader_P::parseUrlDescriptions(const IdMapper& idMapper, 
	xmlDocPtr doc, xmlNodePtr node, 
	smdid* defaultValue, QList<smdid>* list)
{
	xmlNodePtr linkItem = node->children;
	xmlChar* attr = 0;

	while (linkItem)
	{
		if (linkItem->type == XML_ELEMENT_NODE && 
			!xmlStrcmp(linkItem->name, (const xmlChar*)"id" ))
		{
			attr = xmlNodeListGetString(doc, linkItem->xmlChildrenNode, 1);
			if (attr)
			{
				quint32 id = MvdCore::atoui32((const char*) attr);
				xmlFree(attr);

				if (id != 0)
				{
					bool isDefault = false;
					bool isHardcoded = false;

					attr = xmlGetProp(linkItem, (const xmlChar*) "default");
					if (attr)
					{
						isDefault = !xmlStrcmp(attr, (const xmlChar*) "true");
						xmlFree(attr);
					}

					attr = xmlGetProp(linkItem, (const xmlChar*) "hardcoded");
					if (attr)
					{
						isHardcoded = !xmlStrcmp(attr, (const xmlChar*)"true");
						xmlFree(attr);
					}

					// Hard coded items have the SD id.
					if (!isHardcoded)
					{
						QHash<quint32,smdid>::ConstIterator it = idMapper.c_urls.find(id);
						if (it != idMapper.c_urls.constEnd())
							id = it.value();
						else id = 0;
					}

					if (id != 0)
					{
						list->append(id);
						if (isDefault)
							*defaultValue = id;
					}
				}
			}
		}		

		linkItem = linkItem->next;
	}
}

/*!
	\internal Parses an XML node containing a list of strings.
*/
QStringList MvdCollectionLoader_P::parseStringDescriptions(xmlDocPtr doc, 
	xmlNodePtr node, const QString& tag)
{
	xmlNodePtr item = node->children;
	xmlChar* attr = 0;

	QString nodeName;
	QStringList list;

	while (item)
	{
		nodeName = (const char*) item->name;
		if (nodeName == tag)
		{
			attr = xmlNodeListGetString(doc, item->xmlChildrenNode, 1);
			if (attr != 0)
			{
				list.append(QString((const char*) attr));
				xmlFree(attr);
			}
		}

		item = item->next;
	}

	return list;
}

/*!
	\internal Attempts to parse the contents of a node containing person IDs.
	\p role determines if additional info (i.e. role in case of crew member 
	or cast) should be and what should be done with the persons.
*/
void MvdCollectionLoader_P::parsePersonIdList(const IdMapper& idMapper, 
	MvdMovie* movie, xmlDocPtr doc, 
	xmlNodePtr cur, Movida::SmdDataRole role)
{
	xmlNodePtr person = cur->children;
	xmlChar* attr = 0;

	while (person)
	{
		if (xmlStrcmp(person->name, (const xmlChar*) "person"))
		{
			person = person->next;
			continue;
		}

		attr = xmlGetProp(person, (const xmlChar*) "id");
		if (!attr)
		{
			person = person->next;
			continue;
		}

		quint32 id = MvdCore::atoui32((const char*) attr);

		xmlFree(attr);
		attr = 0;

		if (id == 0)
		{
			person = person->next;
			continue;
		}

		bool hardcoded = false;

		attr = xmlGetProp(person, (const xmlChar*) "hardcoded");
		if (attr)
		{
			hardcoded = !xmlStrcmp(attr, (const xmlChar*)"true");
			xmlFree(attr);
		}

		if (!hardcoded)
		{
			QHash<quint32,smdid>::ConstIterator it = idMapper.c_persons.find(id);
			if (it == idMapper.c_persons.constEnd())
			{
				wLog() << QString("CLoader: Invalid person ID: %1").arg(id);
				person = person->next;
				continue;
			}
			else id = it.value();
		}

		// Role info is optional!
		if (role == Movida::ActorRole || role == Movida::CrewMemberRole)
		{
			QStringList roles = parseStringDescriptions(doc, person, "role");
			if (role == Movida::ActorRole)
				movie->addActor(id, roles);
			else movie->addCrewMember(id, roles);
		}
		else if (role == Movida::DirectorRole)
			movie->addDirector(id);
		else if (role == Movida::ProducerRole)
			movie->addProducer(id);

		person = person->next;
	}
}

/*!
	\internal Attempts to retrieve simple value elements from a movie
	description. \p role determines if additional info should be retrieved 
	and what should be done with them.
*/
void MvdCollectionLoader_P::parseSimpleIdList(const IdMapper& idMapper, 
	MvdMovie* movie, xmlDocPtr doc,
	xmlNodePtr cur, Movida::SmdDataRole role)
{
	xmlNodePtr dataNode = cur->children;
	xmlChar* attr = 0;

	while (dataNode)
	{
		if (dataNode->type != XML_ELEMENT_NODE || 
			xmlStrcmp(dataNode->name, (const xmlChar*) "id"))
		{
			dataNode = dataNode->next;
			continue;
		}

		attr = xmlNodeListGetString(doc, dataNode->xmlChildrenNode, 1);
		if (!attr)
		{
			dataNode = dataNode->next;
			continue;
		}

		quint32 id = MvdCore::atoui32((const char*) attr);
		xmlFree(attr);

		if (id == 0)
		{
			dataNode = dataNode->next;
			continue;
		}

		bool defaultValue = false;
		if (role == Movida::UrlRole)
		{
			attr = xmlGetProp(dataNode, (const xmlChar*) "default");
			if (attr)
			{
				defaultValue = !xmlStrcmp(attr, (const xmlChar*) "true");
				xmlFree(attr);
			}
		}

		bool hardcoded = false;
		attr = xmlGetProp(dataNode, (const xmlChar*) "hardcoded");
		if (attr)
		{
			hardcoded = !xmlStrcmp(attr, (const xmlChar*)"true");
			xmlFree(attr);
		}

		switch (role)
		{
		case Movida::TagRole:
			{
				if (hardcoded)
					movie->addTag(id);
				else
				{
					QHash<quint32,smdid>::ConstIterator it = idMapper.c_tags.find(id);
					if (it != idMapper.c_tags.constEnd())
						movie->addTag(it.value());
				}
			}
			break;
		case Movida::CountryRole:
			{
				if (hardcoded)
					movie->addCountry(id);
				else
				{
					QHash<quint32,smdid>::ConstIterator it = idMapper.c_countries.find(id);
					if (it != idMapper.c_countries.constEnd())
						movie->addCountry(it.value());
				}
			}
			break;
		case Movida::LanguageRole:
			{
				if (hardcoded)
					movie->addLanguage(id);
				else
				{
					QHash<quint32,smdid>::ConstIterator it = idMapper.c_languages.find(id);
					if (it != idMapper.c_languages.constEnd())
						movie->addLanguage(it.value());
				}
			}
			break;
		case Movida::UrlRole:
			{
				if (hardcoded)
					movie->addUrl(id, defaultValue);
				else
				{
					QHash<quint32,smdid>::ConstIterator it = idMapper.c_urls.find(id);
					if (it != idMapper.c_urls.constEnd())
						movie->addUrl(it.value(), defaultValue);
				}
			}
			break;
		case Movida::GenreRole:
			{
				if (hardcoded)
					movie->addGenre(id);
				else
				{
					QHash<quint32,smdid>::ConstIterator it = idMapper.c_genres.find(id);
					if (it != idMapper.c_genres.constEnd())
						movie->addGenre(it.value());
				}
			}
			break;
		default:
			;
		}

		dataNode = dataNode->next;
	}
}

/*!
\internal Checks if the archive version is compatible with this parser.
*/
bool MvdCollectionLoader_P::checkArchiveVersion(const QString& attribute)
{
	if (attribute.isEmpty())
	{
		wLog() << "CLoader: Version info is missing. Assuming compatible archive format.";
		return true;
	}

	bool ok;
	int version = attribute.toInt(&ok);
	if (!ok)
	{
		wLog() << "CLoader: Incorrect version info. Assuming compatible archive format.";
		return true;
	}

	if (version > MvdCollectionLoader::version)
	{
		eLog() << QString("CLoader: Archive version %1 is not supported. Maximum supported: %2")
			.arg(attribute).arg(MvdCollectionLoader::version);
		return false;
	}

	iLog() << QString("CLoader: Archive version: %1").arg(version);
	return true;
}

/*!
	\internal Attempts to call xmlParseFile for the file in \p path.
	Returns true if the file has been parsed by libxml2 and if it is a valid 
	Movida XML file.
	\p itemCount will be set to the contents of the document root's \p count 
	attribute or to -1 if no such attribute is present. If no error occurs, 
	\p cur will point to the document root element.

	\warning Remember to release resources by calling xmlFreeDoc(*doc) after 
	use if the method did not return false. xmlFreeDoc is called automatically 
	in case of error before returning false!
*/
bool MvdCollectionLoader_P::loadXmlDocument(const QString& path, xmlDocPtr* doc, 
	xmlNodePtr* cur, int* itemCount)
{
	xmlChar* attr = 0;

	QByteArray cleanPath = QDir::cleanPath(path).toAscii();
	if (cleanPath.isEmpty())
	{
		eLog() << "CLoader: Invalid path.";
		return false;
	}

	*doc = xmlParseFile(cleanPath.data());

	if (!*doc)
	{
		QFileInfo fi(path);
		eLog() << QString("CLoader: %1 is not a valid XML file").arg(fi.fileName());
		return false;
	}

	*cur = xmlDocGetRootElement(*doc);
	if (!*cur)
	{
		eLog() << "CLoader: Empty XML document root";
		xmlFreeDoc(*doc);
		return false;
	}

	if (xmlStrcmp((*cur)->name, (const xmlChar*) "movida-xml-doc"))
	{
		eLog() << "CLoader: Invalid XML document root name";
		xmlFreeDoc(*doc);
		return false;
	}

	attr = xmlGetProp(*cur, (const xmlChar*) "count");
	if (attr)
	{
		*itemCount = MvdCore::atoui32((const char*)attr);
		xmlFree(attr);
	}

	return true;
}


/************************************************************************
MvdCollectionLoader interface
*************************************************************************/

/*!
	Loads a collection from a file. Returns true on success.
	\todo Add an error code class member.
*/
MvdCollectionLoader::ErrorCode MvdCollectionLoader::load(
	MvdMovieCollection* collection, const QString& file)
{
	QFile mmcFile(file);
	iLog() << QString("CLoader: Attempting to load collection from %1").arg(file);
	
	if (!mmcFile.exists())
	{
		eLog() << QString("CLoader: File does not exist: %1").arg(file);
		return FileNotFoundError;
	}
	
	if (!mmcFile.open(QIODevice::ReadOnly))
	{
		eLog() << QString("CLoader: Could not open file for reading (%1): %2")
			.arg(mmcFile.errorString()).arg(file);
		return FileOpenError;
	}
		
	MvdUnZip uz;
	
	MvdUnZip::ErrorCode ec = uz.openArchive(file);
	if (ec != MvdUnZip::NoError)
	{
		eLog() << QString("CLoader: Unable to open compressed zip archive: %1").arg(file);
		return ZipError;
	}
	
	if (!(uz.contains("movida-collection/metadata.xml") && uz.contains("movida-collection/collection.xml")))
	{
		eLog() << QString("CLoader: Missing Movida archive files: %1").arg(file);
		return InvalidFileError;
	}
	
	QString tmpPath = paths().generateTempDir();
	QDir tmpDir(tmpPath);

	if (!tmpDir.exists())
	{
		eLog() << QString("CLoader: Unable to create temp directory %1").arg(tmpPath);
		return TemporaryDirectoryError;
	}
	
	iLog() << QString("CLoader: Created temporary directory: %1").arg(tmpPath);

	QString dataPath = tmpPath + "/movida-collection/";
	iLog() << QString("CLoader: Temporary collection data path: %1").arg(dataPath);

	//! \todo Progress bar for big files
	ec = uz.extractAll(tmpDir);
	
	xmlDocPtr doc = 0;
	xmlNodePtr cur = 0;
	xmlChar* attr = 0;
	int itemCount = -1;

	if (!MvdCollectionLoader_P::loadXmlDocument(QString("%1%2").arg(tmpPath)
		.arg("movida-collection/metadata.xml"), &doc, &cur, &itemCount))
	{
		paths().removeDirectoryTree(tmpPath, "persistent");
		eLog() << "CLoader: Unable to load metadata.xml file";
		return InvalidFileError;
	}
	
	attr = xmlGetProp(cur, (const xmlChar*) "version");
	if (attr)
	{
		xmlFree(attr);
		//! \todo version check: notify user about too old archives or too old application!
		/*
		if (!MvdCollectionLoader_P::checkArchiveVersion(attr))
		{
			mp->removeDirectoryTree(tmpPath);
			xmlFreeDoc(doc);
			eLog() << tr("CLoader: Archive version is not compatible. Found: %1, Required: %2").arg().arg();
			return false;
		}
		*/
	}

	//! \todo use the update attribute to set some collection metadata

	QString currentNode;
	int movieCount = -1;

	collection->setInfo(MvdMovieCollection::DataPathInfo, dataPath+ "/persistent");
	
	cur = cur->xmlChildrenNode;
	while (cur != 0)
	{
		if (xmlStrcmp(cur->name, (const xmlChar*) "info"))
		{
			cur = cur->next;
			continue;
		}
		
		// retrieve archive info
		xmlNodePtr infoNode = cur->xmlChildrenNode;
		
		while (infoNode)
		{
			currentNode = (const char*) infoNode->name;
			MvdMovieCollection::CollectionInfo infoType;

			if (currentNode == "movies")
			{
				const char* str = (const char*)xmlNodeListGetString(doc, infoNode->xmlChildrenNode, 1);
				if (str != 0)
				{
					movieCount = MvdCore::atoui32(str);
					xmlFree((void*)str);
				}
				infoNode = infoNode->next;
				continue;
			}
			else if (currentNode == "name")
			{
				infoType = MvdMovieCollection::NameInfo;
			}
			else if (currentNode == "notes")
			{
				infoType = MvdMovieCollection::NotesInfo;
			}
			else if (currentNode == "owner")
			{
				infoType = MvdMovieCollection::OwnerInfo;				
			}
			else if (currentNode == "email")
			{
				infoType = MvdMovieCollection::EMailInfo;
			}
			else if (currentNode == "website")
			{
				infoType = MvdMovieCollection::WebsiteInfo;
			}

			QString val((const char*)xmlNodeListGetString(doc, 
				infoNode->xmlChildrenNode, 1));
			collection->setInfo(infoType, val);

			infoNode = infoNode->next;
			
		} // while (infoNode != 0)
		
		cur = cur->next;
	}
	
	xmlFreeDoc(doc);
	
	/*! \todo show dialog with collection info and ask to proceed 
		with loading (use a "Do not show this again" dialog!) 
	 */
	
	/*! \todo use movieCount to show a progress bar */
	
	MvdCollectionLoader_P::IdMapper idMapper;

	// ******* READ SHARED DATA *******
	if (QFile::exists(QString("%1%2").arg(dataPath).arg("shared.xml")))
	{
		if (MvdCollectionLoader_P::loadXmlDocument(QString("%1%2").arg(dataPath).arg("shared.xml"), 
			&doc, &cur, &itemCount))
		{
			// First load URLs so we resolve them in person descriptions
			xmlNodePtr n = cur->xmlChildrenNode;
			while (n)
			{
				if (!xmlStrcmp(n->name, (const xmlChar*) "urls"))
					MvdCollectionLoader_P::parseSimpleItemDescriptions(
						&idMapper, collection, doc, n, itemCount, Movida::UrlRole);
				n = n->next;
			}

			// Now parse the remaining nodes
			n = cur->xmlChildrenNode;

			while (n)
			{
				if (!xmlStrcmp(n->name, (const xmlChar*) "genres"))
					MvdCollectionLoader_P::parseSimpleItemDescriptions(
						&idMapper, collection, doc, n, itemCount, Movida::GenreRole);
				else if (!xmlStrcmp(n->name, (const xmlChar*) "tags"))
					MvdCollectionLoader_P::parseSimpleItemDescriptions(
						&idMapper, collection, doc, n, itemCount, Movida::TagRole);
				else if (!xmlStrcmp(n->name, (const xmlChar*) "countries"))
					MvdCollectionLoader_P::parseSimpleItemDescriptions(
						&idMapper, collection, doc, n, itemCount, Movida::CountryRole);
				else if (!xmlStrcmp(n->name, (const xmlChar*) "languages"))
					MvdCollectionLoader_P::parseSimpleItemDescriptions(
						&idMapper, collection, doc, n, itemCount, Movida::LanguageRole);
				else if (!xmlStrcmp(n->name, (const xmlChar*) "persons"))
					MvdCollectionLoader_P::parsePersonDescriptions(
						&idMapper, collection, doc, n, itemCount);

				n = n->next;
			}
			
			xmlFreeDoc(doc);
		}
		else
			eLog() << "CLoader: Unable to parse shared.xml file";
	}
	
	// **** COLLECTION ****
	if (!MvdCollectionLoader_P::loadXmlDocument(
		QString("%1%2").arg(dataPath).arg("collection.xml"), 
		&doc, &cur, &itemCount))
	{
		paths().removeDirectoryTree(tmpPath, "persistent");
		eLog() << "CLoader: Unable to parse collection.xml file";
		return InvalidFileError;
	}
	
	MvdCollectionLoader_P::parseCollection(idMapper, collection, doc, cur, itemCount);
	xmlFreeDoc(doc);
	
	paths().removeDirectoryTree(tmpPath, "persistent");

	QFileInfo finfo(mmcFile);
	collection->setFileName(finfo.completeBaseName());
	collection->setPath(finfo.absoluteFilePath());

	return NoError;
}
