/**************************************************************************
** Filename: collectionsaver.cpp
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

#include "collectionsaver.h"
#include "logger.h"
#include "global.h"
#include "movie.h"
#include "zip.h"
#include "md5.h"
#include "settings.h"
#include "pathresolver.h"
#include "movie.h"
#include "xmlwriter.h"

#include <QString>
#include <QStringList>
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
	\class MvdCollectionSaver collectionsaver.h
	\ingroup movidacore

	\brief Parses a movie collection and writes its contents to a Movida 
	archive file.
	\todo No message box is shown in case of error now! Check calling code!
*/


/************************************************************************
MvdCollectionSaver_P
*************************************************************************/

//! \internal
namespace MvdCollectionSaver_P
{
	static inline QFile* createFile(const QString& name);
	static inline void writeDocumentRoot(MvdXmlWriter* xml, int itemCount);

	static inline void writePersonData(MvdXmlWriter* xml, 
		const QHash<smdid,MvdSharedData::PersonData>& map);
	static inline void writeStringData(MvdXmlWriter* xml, 
		const QHash<smdid,MvdSharedData::StringData>& map);
	static inline void writeUrlData(MvdXmlWriter* xml, 
		const QHash<smdid,MvdSharedData::UrlData>& map);
	static inline void writeMovieData(MvdXmlWriter* xml, 
		const MvdMovieCollection::MovieTable& map, QStringList& persistentData);
	static inline void writePersonList(MvdXmlWriter* xml, 
		const QHash<smdid, QStringList>& map);
	static inline void writePersonList(MvdXmlWriter* xml, const QList<smdid>& list);
	static inline void writeIDList(MvdXmlWriter* xml, 
		const QList<smdid>& map, smdid defaultID = -1);
	static inline void writeStringList(MvdXmlWriter* xml, 
		const QStringList& map);
};

/*!
	\internal Creates and opens a new file for writing.
	Returns 0 in case of error.
*/
QFile* MvdCollectionSaver_P::createFile(const QString& name)
{
	QFile* file = new QFile(name);
	if (!file->open( QIODevice::WriteOnly ))
	{
		eLog() << QString("CSaver: Unable to create file: %1").arg(name);
		delete file;
		return 0;
	}

	return file;
}

//! \internal
void MvdCollectionSaver_P::writeDocumentRoot(MvdXmlWriter* xml, int itemCount)
{
	QHash<QString,QString> attrs;
	attrs.insert("items", QString::number(itemCount));
	attrs.insert("version", QString::number(MvdCollectionSaver::version));
	attrs.insert("update", QDateTime::currentDateTime().toString(Qt::ISODate));
	xml->writeOpenTag("movida-xml-doc", attrs);
	attrs.clear();
}

/*!
	\internal Writes out SD person descriptions to an XML file.
*/
void MvdCollectionSaver_P::writePersonData(MvdXmlWriter* xml, 
	const QHash<smdid,MvdSharedData::PersonData>& map)
{
	QHash<QString,QString> attrs;

	for (QHash<smdid,MvdSharedData::PersonData>::ConstIterator it = 
		map.constBegin(); it != map.constEnd(); ++it)
	{
		smdid id = it.key();
		MvdSharedData::PersonData pd = it.value();

		attrs.insert("id", QString::number(id));

		xml->writeOpenTag("person", attrs);
		xml->writeTaggedString("name", pd.name);
		if (!pd.imdbId.isEmpty())
			xml->writeTaggedString("imdb-id", pd.imdbId);
		if (!pd.urls.isEmpty())
		{
			xml->writeOpenTag("links");
			writeIDList(xml, pd.urls, pd.defaultUrl);
			xml->writeCloseTag("links");
		}
		xml->writeCloseTag("person");
	}
}

/*!
	\internal Writes out SD string data descriptions to an XML file.
*/
void MvdCollectionSaver_P::writeStringData(MvdXmlWriter* xml, 
	const QHash<smdid,MvdSharedData::StringData>& map)
{
	QHash<QString,QString> attrs;

	for (QHash<smdid,MvdSharedData::StringData>::ConstIterator it = map.constBegin();
		it != map.constEnd(); ++it)
	{
		smdid id = it.key();
		QString val = it.value().name;

		attrs.insert("id", QString::number(id));

		xml->writeTaggedString("item", val, attrs);

		attrs.clear();
	}
}

/*!
	\internal Writes out SD URLs to an XML file.
*/
void MvdCollectionSaver_P::writeUrlData(MvdXmlWriter* xml, 
	const QHash<smdid,MvdSharedData::UrlData>& map)
{
	QHash<QString,QString> attrs;

	for (QHash<smdid,MvdSharedData::UrlData>::ConstIterator it = map.constBegin();
		it != map.constEnd(); ++it)
	{
		attrs.insert("id", QString::number(it.key()));

		QString d = it.value().description;
		if (!d.isEmpty())
			attrs.insert("description", d);

		xml->writeTaggedString("item", it.value().url, attrs);

		attrs.clear();
	}
}

/*!
	\internal Writes out movie descriptions to an XML file.
*/
void MvdCollectionSaver_P::writeMovieData(MvdXmlWriter* xml, 
	const MvdMovieCollection::MovieTable& map, QStringList& persistentData)
{
	QHash<QString,QString> attrs;

	for (MvdMovieCollection::MovieTable::ConstIterator it = map.constBegin();
		it != map.constEnd(); ++it)
	{
		MvdMovie movie = it.value();

		attrs.insert("title", movie.title());
		xml->writeOpenTag("movie", attrs);
		attrs.clear();

		QHash<smdid,QStringList> persons;
		QList<smdid> ids;

		xml->writeTaggedString("original-title", movie.originalTitle());
		xml->writeTaggedString("running-time", QString::number(movie.runningTime()));
		xml->writeTaggedString("production-year", movie.productionYear());
		xml->writeTaggedString("release-year", movie.releaseYear());
		xml->writeTaggedString("rating", QString::number(movie.rating()));
		xml->writeTaggedString("imdb-id", movie.imdbId());
		xml->writeTaggedString("notes", movie.notes());
		xml->writeTaggedString("plot", movie.plot());
		xml->writeTaggedString("edition", movie.edition());
		xml->writeTaggedString("storage-id", movie.storageId());

		xml->writeOpenTag("cast");
		persons = movie.actors();
		if (!persons.isEmpty())
			writePersonList(xml, persons);
		xml->writeCloseTag("cast");

		xml->writeOpenTag("crew");
		persons = movie.crewMembers();
		if (!persons.isEmpty())
			writePersonList(xml, persons);
		xml->writeCloseTag("crew");

		xml->writeOpenTag("directors");
		ids = movie.directors();
		if (!ids.isEmpty())
			writePersonList(xml, ids);
		xml->writeCloseTag("directors");

		xml->writeOpenTag("producers");
		ids = movie.producers();
		if (!ids.isEmpty())
			writePersonList(xml, ids);
		xml->writeCloseTag("producers");

		xml->writeOpenTag("genres");
		ids = movie.genres();
		if (!ids.isEmpty())
			writeIDList(xml, ids);
		xml->writeCloseTag("genres");

		xml->writeOpenTag("countries");
		ids = movie.countries();
		if (!ids.isEmpty())
			writeIDList(xml, ids);
		xml->writeCloseTag("countries");

		xml->writeOpenTag("languages");
		ids = movie.languages();
		if (!ids.isEmpty())
			writeIDList(xml, ids);
		xml->writeCloseTag("languages");

		xml->writeOpenTag("tags");
		ids = movie.tags();
		if (!ids.isEmpty())
			writeIDList(xml, ids);
		xml->writeCloseTag("tags");

		MvdMovie::ColorMode cmode = movie.colorMode();
		if (cmode != MvdMovie::UnknownColorMode)
			xml->writeTaggedString("color-mode", 
				cmode == MvdMovie::Color ? "color" : "bw");

		ids = movie.urls();
		smdid defaultId = movie.defaultUrl();

		if (!ids.isEmpty())
		{
			xml->writeOpenTag("urls");
			writeIDList(xml, ids, defaultId);
			xml->writeCloseTag("urls");	
		}

		QStringList list = movie.specialContents();
		xml->writeOpenTag("special-contents");
		if (!list.isEmpty())
			writeStringList(xml, list);
		xml->writeCloseTag("special-contents");

		//! \todo regenerate name if there are possible name clashes (iow. poster filename = HASH.PROG_ID but no other filed named HASH exists any more)
		QString poster = movie.poster();
		if (!poster.isEmpty())
		{
			xml->writeTaggedString("poster", poster);
			persistentData.removeAll(poster.prepend("images/"));
		}

		xml->writeCloseTag("movie");
	}
}

/*!
	\internal Writes out a list of persons with a role attribute.
*/
void MvdCollectionSaver_P::writePersonList(MvdXmlWriter* xml, 
	const QHash<smdid, QStringList>& map)
{
	QHash<QString,QString> attrs;

	for (QHash<smdid, QStringList>::ConstIterator it = map.constBegin();
		it != map.constEnd(); ++it)
	{
		QStringList roles = it.value();
		attrs.insert("id", QString::number(it.key()));

		if (MvdSharedData::isHardcoded(it.key()))
			attrs.insert("hardcoded", "true");

		if (roles.isEmpty())
			xml->writeAtomTag("person", attrs);
		else
		{
			xml->writeOpenTag("person", attrs);
			for (int i = 0; i < roles.size(); ++i)
				xml->writeTaggedString("role", roles.at(i));
			xml->writeCloseTag("person");
		}

		attrs.clear();
	}
}

/*!
	\internal Writes out a list of persons.
*/
void MvdCollectionSaver_P::writePersonList(MvdXmlWriter* xml,
	const QList<smdid>& list)
{
	QHash<QString,QString> attrs;

	for (int i = 0; i < list.size(); ++i)
	{
		smdid id = list.at(i);
		attrs.insert("id", QString::number(id));
		if (MvdSharedData::isHardcoded(id))
			attrs.insert("hardcoded", "true");
		xml->writeAtomTag("person", attrs);
		attrs.clear();
	}
}

/*!
	\internal Writes out a list of IDs.
*/
void MvdCollectionSaver_P::writeIDList(MvdXmlWriter* xml, const QList<smdid>& list, 
	smdid defaultId)
{
	QHash<QString,QString> attrs;

	for (int i = 0; i < list.size(); ++i)
	{
		smdid id = list.at(i);

		if (id == defaultId)
			attrs.insert("default", "true");
		if (MvdSharedData::isHardcoded(id))
			attrs.insert("hardcoded", "true");
		xml->writeTaggedString("id", QString::number(id), attrs);
		attrs.clear();
	}
}

/*!
	\internal Writes out a list of strings.
*/
void MvdCollectionSaver_P::writeStringList(MvdXmlWriter* xml, const QStringList& map)
{
	for (QList<QString>::ConstIterator it = map.constBegin();
		it != map.constEnd(); ++it)
	{
		xml->writeTaggedString("item", *it);
	}
}


/************************************************************************
MvdCollectionSaver
*************************************************************************/

/*!
	Attempts to write the collection to file using the given filename or 
	the collection's filename (if \p file is empty).
*/
MvdCollectionSaver::ErrorCode MvdCollectionSaver::save(
	MvdMovieCollection& collection, 
	const QString& mmcFilename)
{
	MvdZip zipper;
	MvdZip::ErrorCode zerr = zipper.createArchive(mmcFilename);
	if (zerr != MvdZip::NoError)
	{
		eLog() << QString("CSaver: Unable to create zip archive (%1): %2")
			.arg(zipper.formatError(zerr)).arg(mmcFilename);
		
		return ZipError;
	}

	// data path is SOME_TEMP_DIR/persistent
	QString dataPath = collection.info(MvdMovieCollection::DataPathInfo);
	iLog() << QString("CSaver: Data path: %1").arg(dataPath);

	// cdup to exit the persistent directory
	QDir dataPathDir(dataPath);
	dataPathDir.cdUp();

	QString mmcBase = dataPathDir.absolutePath().append("/");
	iLog() << QString("CSaver: MMC base path: %1").arg(mmcBase);

	QString currFile;
	QHash<QString,QString> attrs;

	QFile* file = 0;
	MvdXmlWriter* xml = 0;
	
	
	// **************** metadata.xml ****************

	currFile = mmcBase + "metadata.xml";

	file = MvdCollectionSaver_P::createFile(currFile);
	if (file == 0)
	{
		paths().removeDirectoryTree(mmcBase, "persistent");
		return FileOpenError;
	}
	
	xml = new MvdXmlWriter(file);
	xml->setSkipEmptyAttributes(true);
	xml->setSkipEmptyTags(true);
	
	MvdCollectionSaver_P::writeDocumentRoot(xml, 1);

	xml->writeOpenTag("info");

	xml->writeTaggedString("movies", QString::number(collection.count()));
	xml->writeTaggedString("name", collection.info(MvdMovieCollection::NameInfo));
	xml->writeTaggedString("owner", collection.info(MvdMovieCollection::OwnerInfo));
	xml->writeTaggedString("notes", collection.info(MvdMovieCollection::NotesInfo));
	xml->writeTaggedString("email", collection.info(MvdMovieCollection::EMailInfo));
	xml->writeTaggedString("website", collection.info(MvdMovieCollection::WebsiteInfo));

	//! \todo Store hard coded SD file versions and dates

	xml->writeCloseTag("info");
	xml->writeCloseTag("movida-xml-doc");

	delete xml;
	delete file;


	// **************** SHARED.XML ****************

	currFile = mmcBase + "shared.xml";

	file = MvdCollectionSaver_P::createFile(currFile);
	if (file == 0)
	{
		paths().removeDirectoryTree(mmcBase, "persistent");
		return FileOpenError;
	}
	
	xml = new MvdXmlWriter(file);
	xml->setSkipEmptyAttributes(true);
	xml->setSkipEmptyTags(true);
	
	MvdCollectionSaver_P::writeDocumentRoot(xml, collection.smd().count());

	// persons
	const QHash<smdid,MvdSharedData::PersonData>* persons = collection.smd().persons();
	if (persons != 0)
	{
		attrs.insert("itemCount", QString::number(persons->count()));
		xml->writeOpenTag("persons");
		attrs.clear();
		MvdCollectionSaver_P::writePersonData(xml, *persons);
		xml->writeCloseTag("persons");
	}

	// genres
	const QHash<smdid,MvdSharedData::StringData>* stringData = collection.smd().genres();
	if (stringData != 0)
	{
		attrs.insert("itemCount", QString::number(stringData->count()));
		xml->writeOpenTag("genres");
		attrs.clear();
		MvdCollectionSaver_P::writeStringData(xml, *stringData);
		xml->writeCloseTag("genres");
	}

	// tags
	stringData = collection.smd().tags();
	if (stringData != 0)
	{
		attrs.insert("itemCount", QString::number(stringData->count()));
		xml->writeOpenTag("tags");
		attrs.clear();
		MvdCollectionSaver_P::writeStringData(xml, *stringData);
		xml->writeCloseTag("tags");
	}

	stringData = collection.smd().countries();
	if (stringData != 0)
	{
		attrs.insert("itemCount", QString::number(stringData->count()));
		xml->writeOpenTag("countries");
		attrs.clear();
		MvdCollectionSaver_P::writeStringData(xml, *stringData);
		xml->writeCloseTag("countries");
	}

	stringData = collection.smd().languages();
	if (stringData != 0)
	{
		attrs.insert("itemCount", QString::number(stringData->count()));
		xml->writeOpenTag("languages");
		attrs.clear();
		MvdCollectionSaver_P::writeStringData(xml, *stringData);
		xml->writeCloseTag("languages");
	}

	const QHash<smdid,MvdSharedData::UrlData>* urlData = collection.smd().urls();
	if (urlData != 0)
	{
		attrs.insert("itemCount", QString::number(urlData->count()));
		xml->writeOpenTag("urls");
		attrs.clear();
		MvdCollectionSaver_P::writeUrlData(xml, *urlData);
		xml->writeCloseTag("urls");
	}

	xml->writeCloseTag("movida-xml-doc");

	delete xml;
	delete file;


	// **************** COLLECTION.XML ****************

	currFile = mmcBase + "collection.xml";

	file = MvdCollectionSaver_P::createFile(currFile);
	if (file == 0)
	{
		paths().removeDirectoryTree(mmcBase, "persistent");
		return FileOpenError;
	}
	
	xml = new MvdXmlWriter(file);
	xml->setSkipEmptyAttributes(true);
	xml->setSkipEmptyTags(true);
	
	MvdCollectionSaver_P::writeDocumentRoot(xml, collection.count());

	MvdMovieCollection::MovieTable movies = collection.movies();

	QStringList persistentData;

	QDir imgDir(dataPath + "/images");
	QStringList storedImages = imgDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
	for (int i = 0; i < storedImages.size(); ++i)
		persistentData.append( QString("images/").append(storedImages.at(i)) );

	if (!movies.isEmpty())
		MvdCollectionSaver_P::writeMovieData(xml, movies, persistentData);

	xml->writeCloseTag("movida-xml-doc");

	delete xml;
	delete file;

	// **************** remove unused persistent data ****************
	for (int i = 0; i < persistentData.size(); ++i)
		QFile::remove(dataPath + "/" + persistentData.at(i));

	// **************** ZIP IT! ****************

	zerr = zipper.addDirectory(mmcBase, "movida-collection", MvdZip::IgnoreRootOption);
	
	paths().removeDirectoryTree(mmcBase, "persistent");

	if (!zerr == MvdZip::NoError)
	{
		eLog() << QString("CSaver: Unable to add files to zip archive (%1): %2")
			.arg(zipper.formatError(zerr)).arg(mmcFilename);
		return ZipError;
	}

	zerr = zipper.closeArchive();
	if (!zerr == MvdZip::NoError)
	{
		eLog() << QString("CSaver: Unable to close zip archive (%1): %2")
			.arg(zipper.formatError(zerr)).arg(mmcFilename);
		return ZipError;
	}

	return NoError;
}
