/**************************************************************************
** Filename: collectionsaver.cpp
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
	\ingroup MvdCore

	\brief Parses a movie collection and writes its contents to a Movida 
	archive file.
*/


/************************************************************************
MvdCollectionSaver_P
*************************************************************************/

//! \internal
namespace MvdCollectionSaver_P
{
	static inline QFile* createFile(const QString& name);

	static inline void writeDocumentRoot(MvdXmlWriter* xml, int itemCount);

	static inline void writePersonList(MvdXmlWriter* xml, const QList<MvdRoleItem>& data);
	static inline void writePersonList(MvdXmlWriter* xml, const QList<mvdid>& data);

	static inline void writeUrlList(MvdXmlWriter* xml, const QList<MvdUrl>& data);

	static inline void writeIdList(MvdXmlWriter* xml, const QList<mvdid>& data, const QString& tag);
	static inline void writeStringList(MvdXmlWriter* xml, const QStringList& data, const QString& tag);
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
		eLog() << QString("MvdCollectionSaver_P: Unable to create file: %1").arg(name);
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
	\internal Writes out a list of persons with a possible list of roles.

	\verbatim
	<person id="13">
		<roles>
			<role>Mr Brown</role>
			<role>Mr Yellow</role>
		</roles>
	</person>
	\endverbatim
*/
void MvdCollectionSaver_P::writePersonList(MvdXmlWriter* xml, 
	const QList<MvdRoleItem>& idRoleList)
{
	QHash<QString,QString> attrs;

	for (int i = 0; i < idRoleList.size(); ++i)
	{
		const MvdRoleItem& item = idRoleList.at(i);
		mvdid id = item.first;
		QStringList roles = item.second;
		attrs.insert("id", QString::number(id));

		if (roles.isEmpty())
		{
			xml->writeAtomTag("person", attrs);
			attrs.clear();
		}
		else
		{
			xml->writeOpenTag("person", attrs);
			attrs.clear();
			if (!roles.isEmpty())
			{
				xml->writeOpenTag("roles", attrs);
				for (int i = 0; i < roles.size(); ++i)
					xml->writeTaggedString("role", roles.at(i));
				xml->writeCloseTag("roles");
			}
			xml->writeCloseTag("person");
		}

		attrs.clear();
	}
}

/*!
	\internal Writes out a list of persons (and no role).

	\verbatim
	<person id="23"/>
	\endverbatim
*/
void MvdCollectionSaver_P::writePersonList(MvdXmlWriter* xml,
	const QList<mvdid>& list)
{
	QHash<QString,QString> attrs;

	for (int i = 0; i < list.size(); ++i)
	{
		mvdid id = list.at(i);
		attrs.insert("id", QString::number(id));
		xml->writeAtomTag("person", attrs);
		attrs.clear();
	}
}

/*!
	\internal Writes out a list of IDs.

	\verbatim
	<$TAG id="34"/>
	\endverbatim
*/
void MvdCollectionSaver_P::writeIdList(MvdXmlWriter* xml, 
	const QList<mvdid>& list, const QString& tag)
{
	QHash<QString,QString> attrs;

	for (int i = 0; i < list.size(); ++i)
	{
		mvdid id = list.at(i);
		attrs.insert("id", QString::number(id));
		xml->writeAtomTag(tag, attrs);
		attrs.clear();
	}
}

/*!
	\internal Writes out a list of MvdUrls.

	\verbatim
	<url description="lallalala" default="true">http://abc.com</url>
	\endverbatim
*/
void MvdCollectionSaver_P::writeUrlList(MvdXmlWriter* xml,
	const QList<MvdUrl>& list)
{
	QHash<QString,QString> attrs;

	for (int i = 0; i < list.size(); ++i)
	{
		MvdUrl url = list.at(i);
		attrs.insert("description", url.description);
		if (url.isDefault)
			attrs.insert("default", "true");
		xml->writeTaggedString("url", url.url, attrs);
		attrs.clear();
	}
}

/*!
	\internal Writes out a list of strings.

	\verbatim
	<$TAG>blah blah blah</$TAG>
	\endverbatim
*/
void MvdCollectionSaver_P::writeStringList(MvdXmlWriter* xml,
	const QStringList& data, const QString& tag)
{
	for (int i = 0; i < data.size(); ++i)
		xml->writeTaggedString(tag, data.at(i));
}


/************************************************************************
MvdCollectionSaver
*************************************************************************/

/*!
	Attempts to write the collection to file using the given filename or 
	the collection's filename (if \p mmcFilename is empty).
*/
MvdCollectionSaver::ErrorCode MvdCollectionSaver::save(MvdMovieCollection* collection, QString mmcFilename)
{
	if (!collection)
		return InvalidCollectionError;

	bool storeFilename = true;

	if (mmcFilename.isEmpty())
	{
		storeFilename = false;
		mmcFilename = collection->path();
		if (mmcFilename.isEmpty())
			return InvalidFileError;
	}

	MvdZip zipper;
	MvdZip::ErrorCode zerr = zipper.createArchive(mmcFilename);
	if (zerr != MvdZip::NoError)
	{
		eLog() << QString("MvdCollectionSaver: Unable to create zip archive (%1): %2")
			.arg(zipper.formatError(zerr)).arg(mmcFilename);
		
		return ZipError;
	}

	// data path is SOME_TEMP_DIR/persistent
	QString dataPath = collection->metaData(MvdMovieCollection::DataPathInfo);
	iLog() << QString("MvdCollectionSaver: Data path: %1").arg(dataPath);

	// cdup to exit the 'persistent' directory
	QDir dataPathDir(dataPath);
	dataPathDir.cdUp();

	QString mmcBase = dataPathDir.absolutePath().append("/");
	iLog() << QString("MvdCollectionSaver: MMC base path: %1").arg(mmcBase);

	QString currFile;
	QHash<QString,QString> attrs;

	QFile* file = 0;
	MvdXmlWriter* xml = 0;
	
	
	// **************** metadata.xml ****************

	currFile = mmcBase + "metadata.xml";

	file = MvdCollectionSaver_P::createFile(currFile);
	if (!file)
	{
		paths().removeDirectoryTree(mmcBase, "persistent");
		return FileOpenError;
	}
	
	xml = new MvdXmlWriter(file);
	xml->setSkipEmptyAttributes(true);
	xml->setSkipEmptyTags(true);
	
	MvdCollectionSaver_P::writeDocumentRoot(xml, 1);

	xml->writeOpenTag("info");

	xml->writeTaggedString("movies", QString::number(collection->count()));
	xml->writeTaggedString("name", collection->metaData(MvdMovieCollection::NameInfo));
	xml->writeTaggedString("owner", collection->metaData(MvdMovieCollection::OwnerInfo));
	xml->writeTaggedString("notes", collection->metaData(MvdMovieCollection::NotesInfo));
	xml->writeTaggedString("email", collection->metaData(MvdMovieCollection::EMailInfo));
	xml->writeTaggedString("website", collection->metaData(MvdMovieCollection::WebsiteInfo));

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
	
	MvdCollectionSaver_P::writeDocumentRoot(xml, collection->sharedData().countItems());

	MvdSharedData::ItemList sharedData = collection->sharedData().items(Movida::NoRole);
	if (!sharedData.isEmpty())
	{
		xml->writeOpenTag("shared-data");
		for (MvdSharedData::ItemList::ConstIterator it = sharedData.constBegin(); 
			it != sharedData.constEnd(); ++it)
		{
			mvdid id = it.key();
			const MvdSdItem& item = it.value();

			attrs.insert("id", QString::number(id));
			attrs.insert("type", MvdSharedData::roleToString(item.role));
			xml->writeOpenTag("shared-item", attrs);
			attrs.clear();

			xml->writeTaggedString("value", item.value);
			xml->writeTaggedString("description", item.description);
			xml->writeTaggedString("identifier", item.id);
			if (!item.urls.isEmpty())
			{
				xml->writeOpenTag("urls");
				for (int i = 0; i < item.urls.size(); ++i)
				{
					const MvdUrl& url = item.urls.at(i);
					attrs.insert("description", url.description);
					if (url.isDefault)
						attrs.insert("default", "true");
					xml->writeTaggedString("url", url.url);
					attrs.clear();
				}
				xml->writeCloseTag("urls");
			}

			xml->writeCloseTag("shared-item");
		}
		xml->writeCloseTag("shared-data");
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
	
	MvdCollectionSaver_P::writeDocumentRoot(xml, collection->count());

	MvdMovieCollection::MovieList movies = collection->movies();

	QStringList persistentData;

	QDir imgDir(dataPath + "/images");
	QStringList storedImages = imgDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
	for (int i = 0; i < storedImages.size(); ++i)
		persistentData.append( QString("images/").append(storedImages.at(i)) );

	if (!movies.isEmpty())
	{
		xml->writeOpenTag("movies");

		for (MvdMovieCollection::MovieList::ConstIterator it = movies.constBegin();
			it != movies.constEnd(); ++it)
		{
			const MvdMovie& movie = it.value();

			xml->writeOpenTag("movie");

			xml->writeTaggedString("title", movie.title());
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
			if (movie.hasSpecialTagEnabled(MvdMovie::SeenTag))
				xml->writeTaggedString("seen", "true");
			if (movie.hasSpecialTagEnabled(MvdMovie::LoanedTag))
				xml->writeTaggedString("loaned", "true");
			if (movie.hasSpecialTagEnabled(MvdMovie::SpecialTag))
				xml->writeTaggedString("special", "true");

			xml->writeOpenTag("cast");
			QList<MvdRoleItem> persons = movie.actors();
			if (!persons.isEmpty())
				MvdCollectionSaver_P::writePersonList(xml, persons);
			xml->writeCloseTag("cast");

			xml->writeOpenTag("crew");
			persons = movie.crewMembers();
			if (!persons.isEmpty())
				MvdCollectionSaver_P::writePersonList(xml, persons);
			xml->writeCloseTag("crew");

			xml->writeOpenTag("directors");
			QList<mvdid> ids = movie.directors();
			if (!ids.isEmpty())
				MvdCollectionSaver_P::writePersonList(xml, ids);
			xml->writeCloseTag("directors");

			xml->writeOpenTag("producers");
			ids = movie.producers();
			if (!ids.isEmpty())
				MvdCollectionSaver_P::writePersonList(xml, ids);
			xml->writeCloseTag("producers");

			xml->writeOpenTag("genres");
			ids = movie.genres();
			if (!ids.isEmpty())
				MvdCollectionSaver_P::writeIdList(xml, ids, "genre");
			xml->writeCloseTag("genres");

			xml->writeOpenTag("countries");
			ids = movie.countries();
			if (!ids.isEmpty())
				MvdCollectionSaver_P::writeIdList(xml, ids, "country");
			xml->writeCloseTag("countries");

			xml->writeOpenTag("languages");
			ids = movie.languages();
			if (!ids.isEmpty())
				MvdCollectionSaver_P::writeIdList(xml, ids, "language");
			xml->writeCloseTag("languages");

			xml->writeOpenTag("tags");
			ids = movie.tags();
			if (!ids.isEmpty())
				MvdCollectionSaver_P::writeIdList(xml, ids, "tag");
			xml->writeCloseTag("tags");

			MvdMovie::ColorMode cmode = movie.colorMode();
			if (cmode != MvdMovie::UnknownColorMode)
				xml->writeTaggedString("color-mode", 
				cmode == MvdMovie::Color ? "color" : "bw");

			QList<MvdUrl> urls = movie.urls();
			if (!urls.isEmpty())
			{
				xml->writeOpenTag("urls");
				MvdCollectionSaver_P::writeUrlList(xml, urls);
				xml->writeCloseTag("urls");	
			}

			QStringList list = movie.specialContents();
			xml->writeOpenTag("special-contents");
			if (!list.isEmpty())
				MvdCollectionSaver_P::writeStringList(xml, list, "item");
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

		xml->writeCloseTag("movies");

	} // if (!movies.isEmpty())

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
		eLog() << QString("MvdCollectionSaver: Unable to add files to zip archive (%1): %2")
			.arg(zipper.formatError(zerr)).arg(mmcFilename);
		return ZipError;
	}

	zerr = zipper.closeArchive();
	if (!zerr == MvdZip::NoError)
	{
		eLog() << QString("MvdCollectionSaver: Unable to close zip archive (%1): %2")
			.arg(zipper.formatError(zerr)).arg(mmcFilename);
		return ZipError;
	}

	if (storeFilename)
	{
		QFileInfo fi(mmcFilename);
		collection->setFileName(fi.completeBaseName());
		collection->setPath(fi.absoluteFilePath());
	}

	return NoError;
}
