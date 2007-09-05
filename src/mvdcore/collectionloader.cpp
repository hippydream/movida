/**************************************************************************
** Filename: collectionloader.cpp
** Revision: 3
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
#include "sditem.h"
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
	\ingroup MvdCore

	\brief Loads a movie collection from a Movida mmc file.
*/


/************************************************************************
MvdCollectionLoader_P
*************************************************************************/

//! \internal
namespace MvdCollectionLoader_P
{
	//! \internal
	typedef QHash<mvdid, mvdid> IdMapper;

	static inline bool checkArchiveVersion(const QString& attribute);
	static inline bool loadXmlDocument(const QString& path, xmlDocPtr* doc, xmlNodePtr* cur, int* itemCount);

	
	// Shared data parser
	static inline void parseSharedItem(xmlDocPtr doc, xmlNodePtr node, 
		IdMapper* idMapper, MvdMovieCollection* collection, int itemCount);


	// Movie parser
	static inline void parseCollection(xmlDocPtr doc, xmlNodePtr cur, 
		const IdMapper& idMapper, MvdMovieCollection* collection, int itemCount);


	// Shared data ID parsers
	static inline void parsePersonIdList(xmlDocPtr doc, xmlNodePtr cur,
		const IdMapper& idMapper, MvdMovie* movie, Movida::DataRole role);

	static inline void parseSimpleIdList(xmlDocPtr doc, xmlNodePtr cur, 
		const IdMapper& idMapper, MvdMovie* movie, Movida::DataRole role);


	// Non-shared data parsers
	static inline void parseUrlDescriptions(xmlDocPtr doc, xmlNodePtr node,
		QList<MvdUrl>* urls);

	static inline QStringList parseStringDescriptions(xmlDocPtr doc, xmlNodePtr node, 
		const QString& tag);
};

/*! 
	\internal 

	\verbatim
	<shared-item id="1" type="PersonItem">
		<value>Sharon Stoned</value>
		<description></description>
		<identifier>1234565</identifier>
		<urls>
			<url description="SharonRocks website" default="true">http://sharonrocks.net</url>
			<url description="Wikipedia">http://en.wikipedia.org/wiki/Sharon_Stoned</url>
		</urls>
	</shared-item>
	\endverbatim

	\warning xmlFreeDoc is not called within this method; you will have to 
	free resources after calling it.
*/
void MvdCollectionLoader_P::parseSharedItem(xmlDocPtr doc, xmlNodePtr node,
	IdMapper* idMapper, MvdMovieCollection* collection, int itemCount)
{
	Q_ASSERT(doc && node && idMapper && collection);
	Q_UNUSED(itemCount);

	xmlChar* attr = 0;

	attr = xmlGetProp(node, (const xmlChar*) "id");
	if (!attr)
		return;

	mvdid id = MvdCore::atoid((const char*) attr);
	xmlFree(attr);

	attr = xmlGetProp(node, (const xmlChar*) "type");
	if (!attr)
		return;

	Movida::DataRole type = MvdSharedData::roleFromString(QString((const char*)attr));
	xmlFree(attr);

	if (type == Movida::NoRole)
		return;

	MvdSdItem item;
	xmlNodePtr n = node->children;

	while (n)
	{
		if (n->type != XML_ELEMENT_NODE)
		{
			n = n->next;
			continue;
		}

		QString data = QString((const char*)xmlNodeListGetString(doc, n->xmlChildrenNode, 1)).trimmed();

		if (!xmlStrcmp(n->name, (const xmlChar*) "value"))
			item.value = data;
		else if (!xmlStrcmp(n->name, (const xmlChar*) "description"))
			item.description = data;
		else if (!xmlStrcmp(n->name, (const xmlChar*) "identifier"))
			item.id = data;
		else if (!xmlStrcmp(n->name, (const xmlChar*) "urls"))
		{
			QList<MvdUrl> urls;
			parseUrlDescriptions(doc, n, &urls);
			if (!urls.isEmpty())
				item.urls = urls;
		}
		n = n->next;
	}

	if (!item.value.isEmpty())
	{
		item.role = type;
		mvdid newId = collection->smd().addItem(item);
		if (newId != MvdNull)
			idMapper->insert(id, newId);
	}
}

/*!
	\internal Parses movie descriptions from an XML file.

	\verbatim
	<movie>
		<!-- ... -->
	</movie>
	\endverbatim
	
	\warning xmlFreeDoc is not called within this method; you will have to 
	free resources after calling it.
*/
void MvdCollectionLoader_P::parseCollection(xmlDocPtr doc, xmlNodePtr cur, 
	const IdMapper& idMapper, MvdMovieCollection* collection, int itemCount)
{
	Q_UNUSED(itemCount)

	QString posterDir = collection->metaData(MvdMovieCollection::DataPathInfo)
		.append("/images/");

	xmlChar* attr = 0;
	cur = cur->xmlChildrenNode;

	xmlNodePtr mNode = 0;
	QString nodeName;

	while (cur)
	{
		if (cur->type == XML_ELEMENT_NODE && !xmlStrcmp(cur->name, (const xmlChar*) "movies"))
		{
			cur = cur->children;
			break;
		}
		else cur = cur->next;
	}

	while (cur)
	{
		if (cur->type != XML_ELEMENT_NODE || xmlStrcmp(cur->name, (const xmlChar*) "movie"))
		{
			cur = cur->next;
			continue;
		}

		MvdMovie movie;
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
				parsePersonIdList(doc, mNode, idMapper, &movie, Movida::ActorRole);
			else if (nodeName == "tags")
				parseSimpleIdList(doc, mNode, idMapper, &movie, Movida::TagRole);
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
				parseSimpleIdList(doc, mNode, idMapper, &movie, Movida::CountryRole);
			else if (nodeName == "languages")
				parseSimpleIdList(doc, mNode, idMapper, &movie, Movida::LanguageRole);
			else if (nodeName == "crew")
				parsePersonIdList(doc, mNode, idMapper, &movie, Movida::CrewMemberRole);
			else if (nodeName == "directors")
				parsePersonIdList(doc, mNode, idMapper, &movie, Movida::DirectorRole);
			else if (nodeName == "genres")
				parseSimpleIdList(doc, mNode, idMapper, &movie, Movida::GenreRole);
			else if (nodeName == "imdb-id")
			{
				attr = xmlNodeListGetString(doc, mNode->xmlChildrenNode, 1);
				if (attr)
				{
					quint32 idLen = MvdCore::parameter("mvdp://movidacore/imdb-id-length").toInt();
					if (strlen((const char*)attr) == idLen)
					{
						mvdid id = MvdCore::atoid((const char*)attr);
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
					quint32 minutes = MvdCore::atoid((const char*)attr);
					movie.setRunningTime(minutes);
					xmlFree(attr);
				}
			}
			else if (nodeName == "urls")
			{
				QList<MvdUrl> urls;
				parseUrlDescriptions(doc, mNode, &urls);
				if (!urls.isEmpty())
					movie.setUrls(urls);
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
				parsePersonIdList(doc, mNode, idMapper, &movie, Movida::ProducerRole);
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
					movie.setRating(MvdCore::atoid((const char*) attr));
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
				QStringList list = parseStringDescriptions(doc, mNode, "item");
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
						Movida::wLog() << QString("MvdCollectionLoader: Missing movie poster: %1")
							.arg(posterDir + poster);
					}
					else
						movie.setPoster(poster);
				}
			}
			else if (nodeName == "title")
			{
				attr = xmlNodeListGetString(doc, mNode->xmlChildrenNode, 1);
				if (attr)
				{
					movie.setTitle(QString((const char*)attr));
					xmlFree(attr);
				}
			}

			mNode = mNode->next;
		}

		cur = cur->next;

		if (movie.isValid())
			collection->addMovie(movie);

	} // loop over <movie> nodes
}

/*!
	\internal Parses a list of urls.

	\verbatim
	<urls>
		<url description="SharonRocks website" default="true">http://sharonrocks.net</url>
		<url description="Wikipedia">http://en.wikipedia.org/wiki/Sharon_Stoned</url>
	</urls>
	\endverbatim
*/
void MvdCollectionLoader_P::parseUrlDescriptions(xmlDocPtr doc, xmlNodePtr node, 
	QList<MvdUrl>* list)
{
	Q_ASSERT(doc && node && list);

	xmlNodePtr linkItem = node->children;
	xmlChar* attr = 0;

	while (linkItem)
	{
		if (linkItem->type == XML_ELEMENT_NODE && 
			!xmlStrcmp(linkItem->name, (const xmlChar*)"url" ))
		{
			attr = xmlNodeListGetString(doc, linkItem->xmlChildrenNode, 1);
			if (attr)
			{
				MvdUrl url;
				url.url = QString((const char*)attr).trimmed();
				xmlFree(attr);

				attr = xmlGetProp(linkItem, (const xmlChar*) "description");
				if (attr)
				{
					url.description = QString((const char*)attr).trimmed();
					xmlFree(attr);
				}

				attr = xmlGetProp(linkItem, (const xmlChar*) "default");
				if (attr)
				{
					url.isDefault = !xmlStrcmp(attr, (const xmlChar*) "true");
					xmlFree(attr);
				}
				
				if (!url.url.isEmpty())
					list->append(url);
			}
		}

		linkItem = linkItem->next;
	}
}

/*!
	\internal Parses an XML node containing a list of strings.
*/
QStringList MvdCollectionLoader_P::parseStringDescriptions(xmlDocPtr doc, xmlNodePtr node,
	const QString& tag)
{
	xmlNodePtr item = node->children;
	xmlChar* attr = 0;
	QStringList list;

	while (item)
	{
		if (item->type == XML_ELEMENT_NODE && QString((const char*) item->name) == tag)
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
	\p role determines if additional info (i.e. roles) should be and what should 
	be done with the persons.

	\verbatim
	<person id="33">
		<roles>
			<role>The Boss</role>
		</roles>
	</person>
	\endverbatim
*/
void MvdCollectionLoader_P::parsePersonIdList(xmlDocPtr doc, xmlNodePtr cur, 
	const IdMapper& idMapper, MvdMovie* movie, Movida::DataRole role)
{
	xmlNodePtr personNode = cur->children;
	xmlChar* attr = 0;

	while (personNode)
	{
		if (xmlStrcmp(personNode->name, (const xmlChar*) "person"))
		{
			personNode = personNode->next;
			continue;
		}

		attr = xmlGetProp(personNode, (const xmlChar*) "id");
		if (!attr)
		{
			personNode = personNode->next;
			continue;
		}

		mvdid id = MvdCore::atoid((const char*) attr);

		xmlFree(attr);
		attr = 0;

		if (id == 0)
		{
			personNode = personNode->next;
			continue;
		}

		IdMapper::ConstIterator it = idMapper.find(id);
		if (it == idMapper.constEnd())
		{
			wLog() << QString("MvdCollectionLoader: Invalid person ID: %1").arg(id);
			personNode = personNode->next;
			continue;
		}
		else id = it.value();

		// Role info is optional!
		if (role == Movida::ActorRole || role == Movida::CrewMemberRole)
		{
			QStringList roles;

			xmlNodePtr rolesNode = personNode->children;
			while (rolesNode)
			{
				if (xmlStrcmp(rolesNode->name, (const xmlChar*) "roles")) {
					QStringList thisRoles = parseStringDescriptions(doc, rolesNode, "role");
					for (int i = 0; i < thisRoles.size(); ++i)
						roles.append(thisRoles.at(i));
				}

				rolesNode = rolesNode->next;
			}

			if (role == Movida::ActorRole)
				movie->addActor(id, roles);
			else movie->addCrewMember(id, roles);
		}
		else if (role == Movida::DirectorRole)
			movie->addDirector(id);
		else if (role == Movida::ProducerRole)
			movie->addProducer(id);

		personNode = personNode->next;
	}
}

/*!
	\internal Attempts to retrieve simple value elements from a movie
	description. \p role determines if additional info should be retrieved 
	and what should be done with them.
*/
void MvdCollectionLoader_P::parseSimpleIdList(xmlDocPtr doc, xmlNodePtr cur,
	const IdMapper& idMapper, MvdMovie* movie, Movida::DataRole role)
{
	xmlNodePtr dataNode = cur->children;
	xmlChar* attr = 0;

	QString tag;
	switch (role)
	{
	case Movida::GenreRole: tag = "genre"; break;
	case Movida::LanguageRole: tag = "language"; break;
	case Movida::TagRole: tag = "tag"; break;
	case Movida::CountryRole: tag = "country"; break;
	default: ;
	}

	while (dataNode)
	{
		if (dataNode->type != XML_ELEMENT_NODE || QString((const char*)dataNode->name) != tag)
		{
			dataNode = dataNode->next;
			continue;
		}

		attr = xmlGetProp(dataNode, (const xmlChar*) "id");
		if (!attr)
		{
			dataNode = dataNode->next;
			continue;
		}

		mvdid id = MvdCore::atoid((const char*) attr);
		xmlFree(attr);

		IdMapper::ConstIterator it = idMapper.find(id);
		if (it == idMapper.constEnd()) {
			wLog() << QString("MvdCollectionLoader: Invalid simple ID: %1").arg(id);
			dataNode = dataNode->next;
			continue;
		} else id = it.value();

		switch (role)
		{
		case Movida::TagRole: movie->addTag(id); break;
		case Movida::CountryRole: movie->addCountry(id); break;
		case Movida::LanguageRole: movie->addLanguage(id); break;
		case Movida::GenreRole: movie->addGenre(id); break;
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
		wLog() << "MvdCollectionLoader: Version info is missing. Assuming compatible archive format.";
		return true;
	}

	bool ok;
	int version = attribute.toInt(&ok);
	if (!ok)
	{
		wLog() << "MvdCollectionLoader: Incorrect version info. Assuming compatible archive format.";
		return true;
	}

	if (version > MvdCollectionLoader::version)
	{
		eLog() << QString("MvdCollectionLoader: Archive version %1 is not supported. Maximum supported: %2")
			.arg(attribute).arg(MvdCollectionLoader::version);
		return false;
	}

	iLog() << QString("MvdCollectionLoader: Archive version: %1").arg(version);
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
		eLog() << "MvdCollectionLoader: Invalid path.";
		return false;
	}

	*doc = xmlParseFile(cleanPath.data());

	if (!*doc)
	{
		QFileInfo fi(path);
		eLog() << QString("MvdCollectionLoader: %1 is not a valid XML file").arg(fi.fileName());
		return false;
	}

	*cur = xmlDocGetRootElement(*doc);
	if (!*cur)
	{
		eLog() << "MvdCollectionLoader: Empty XML document root";
		xmlFreeDoc(*doc);
		return false;
	}

	if (xmlStrcmp((*cur)->name, (const xmlChar*) "movida-xml-doc"))
	{
		eLog() << "MvdCollectionLoader: Invalid XML document root name";
		xmlFreeDoc(*doc);
		return false;
	}

	attr = xmlGetProp(*cur, (const xmlChar*) "count");
	if (attr)
	{
		*itemCount = MvdCore::atoid((const char*)attr);
		xmlFree(attr);
	}

	return true;
}


/************************************************************************
MvdCollectionLoader interface
*************************************************************************/

/*!
	Loads a collection from the given file or from the collection's file path if
	\p file is null.
*/
MvdCollectionLoader::StatusCode MvdCollectionLoader::load(
	MvdMovieCollection* collection, QString file)
{
	if (!collection)
		return InvalidCollectionError;

	if (file.isEmpty())
	{
		file = collection->path();
		if (file.isEmpty())
		{
			eLog() << QString("MvdCollectionLoader: No filename specified.");
			return FileNotFoundError;
		}
	}

	QFile mmcFile(file);
	iLog() << QString("MvdCollectionLoader: Attempting to load collection from %1").arg(file);
	
	if (!mmcFile.exists())
	{
		eLog() << QString("MvdCollectionLoader: File does not exist: %1").arg(file);
		return FileNotFoundError;
	}
	
	if (!mmcFile.open(QIODevice::ReadOnly))
	{
		eLog() << QString("MvdCollectionLoader: Could not open file for reading (%1): %2")
			.arg(mmcFile.errorString()).arg(file);
		return FileOpenError;
	}
		
	MvdUnZip uz;
	
	MvdUnZip::ErrorCode ec = uz.openArchive(file);
	if (ec != MvdUnZip::NoError)
	{
		eLog() << QString("MvdCollectionLoader: Unable to open compressed zip archive: %1").arg(file);
		return ZipError;
	}
	
	if (!(uz.contains("movida-collection/metadata.xml") && uz.contains("movida-collection/collection.xml")))
	{
		eLog() << QString("MvdCollectionLoader: Missing Movida archive files: %1").arg(file);
		return InvalidFileError;
	}
	
	QString tmpPath = paths().generateTempDir();
	QDir tmpDir(tmpPath);

	if (!tmpDir.exists())
	{
		eLog() << QString("MvdCollectionLoader: Unable to create temp directory %1").arg(tmpPath);
		return TemporaryDirectoryError;
	}
	
	iLog() << QString("MvdCollectionLoader: Created temporary directory: %1").arg(tmpPath);

	QString dataPath = tmpPath + "/movida-collection/";
	iLog() << QString("MvdCollectionLoader: Temporary collection data path: %1").arg(dataPath);

	//! \todo Progress bar for big files?
	ec = uz.extractAll(tmpDir);
	
	xmlDocPtr doc = 0;
	xmlNodePtr cur = 0;
	xmlChar* attr = 0;
	int itemCount = -1;

	if (!MvdCollectionLoader_P::loadXmlDocument(QString("%1%2").arg(tmpPath)
		.arg("movida-collection/metadata.xml"), &doc, &cur, &itemCount))
	{
		paths().removeDirectoryTree(tmpPath);
		eLog() << "MvdCollectionLoader: Unable to load metadata.xml file";
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
			eLog() << tr("MvdCollectionLoader: Archive version is not compatible. Found: %1, Required: %2").arg().arg();
			return false;
		}
		*/
	}

	//! \todo use the update attribute to set some collection metadata?

	QString currentNode;
	int movieCount = -1;

	collection->setMetaData(MvdMovieCollection::DataPathInfo, dataPath + "/persistent");
	
	cur = cur->xmlChildrenNode;
	while (cur != 0)
	{
		if (cur->type != XML_ELEMENT_NODE || xmlStrcmp(cur->name, (const xmlChar*) "info"))
		{
			cur = cur->next;
			continue;
		}
		
		// retrieve archive info
		xmlNodePtr infoNode = cur->xmlChildrenNode;
		
		while (infoNode)
		{
			if (cur->type != XML_ELEMENT_NODE)
			{
				infoNode = infoNode->next;
				continue;
			}

			currentNode = (const char*) infoNode->name;
			MvdMovieCollection::MetaDataType infoType = MvdMovieCollection::InvalidInfo;

			if (currentNode == "movies")
			{
				const char* str = (const char*)xmlNodeListGetString(doc, infoNode->xmlChildrenNode, 1);
				if (str != 0)
				{
					movieCount = MvdCore::atoid(str);
					xmlFree((void*)str);
				}
				infoNode = infoNode->next;
				continue;
			}
			else if (currentNode == "name")
				infoType = MvdMovieCollection::NameInfo;
			else if (currentNode == "notes")
				infoType = MvdMovieCollection::NotesInfo;
			else if (currentNode == "owner")
				infoType = MvdMovieCollection::OwnerInfo;
			else if (currentNode == "email")
				infoType = MvdMovieCollection::EMailInfo;
			else if (currentNode == "website")
				infoType = MvdMovieCollection::WebsiteInfo;

			QString val((const char*)xmlNodeListGetString(doc, infoNode->xmlChildrenNode, 1));
			collection->setMetaData(infoType, val);

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
			xmlNodePtr n = cur->xmlChildrenNode;
			while (n)
			{
				if (n->type == XML_ELEMENT_NODE && !xmlStrcmp(n->name, (const xmlChar*) "shared-data"))
				{
					xmlNodePtr nn = n->xmlChildrenNode;
					while (nn)
					{
						if (cur->type == XML_ELEMENT_NODE && !xmlStrcmp(nn->name, (const xmlChar*) "shared-item"))
							MvdCollectionLoader_P::parseSharedItem(doc, nn, &idMapper, collection, itemCount);

						nn = nn->next;
					}
				}

				n = n->next;
			}
			
			xmlFreeDoc(doc);
		}
		else
			eLog() << "MvdCollectionLoader: Unable to parse shared.xml file";
	}
	
	// **** COLLECTION ****
	if (!MvdCollectionLoader_P::loadXmlDocument(
		QString("%1%2").arg(dataPath).arg("collection.xml"), 
		&doc, &cur, &itemCount))
	{
		paths().removeDirectoryTree(tmpPath);
		eLog() << "MvdCollectionLoader: Unable to parse collection.xml file";
		return InvalidFileError;
	}
	
	MvdCollectionLoader_P::parseCollection(doc, cur, idMapper, collection, itemCount);
	xmlFreeDoc(doc);
	
	paths().removeDirectoryTree(tmpPath, "persistent");

	QFileInfo finfo(mmcFile);
	collection->setFileName(finfo.completeBaseName());
	collection->setPath(finfo.absoluteFilePath());

	return NoError;
}
