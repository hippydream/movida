/**************************************************************************
** Filename: templatemanager.cpp
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

#include "core.h"
#include "templatemanager.h"
#include "shareddata.h"
#include "xsltproc.h"
#include "pathresolver.h"
#include "xmlwriter.h"
#include <QDir>
#include <QMutex>
#include <QUrl>
#include <stdexcept>

using namespace Movida;

Q_GLOBAL_STATIC(QMutex, MvdTemplateManagerLock)

/*!
        \class MvdTemplateManager templatemanager.h
        \ingroup MvdCore Singletons

        <b>Movida::tmanager()</b> can be used as a convenience method to access the singleton.

        \brief Handles XSL templates and related transformations.
*/


/************************************************************************
MvdTemplateManager::Private
*************************************************************************/

//! \internal
class MvdTemplateManager::Private
{
public:
        Private();
};

//! \internal
MvdTemplateManager::Private::Private()
{
}


/************************************************************************
MvdTemplateManager
*************************************************************************/

//! \internal
volatile MvdTemplateManager* MvdTemplateManager::mInstance = 0;
bool MvdTemplateManager::mDestroyed = false;

//! \internal Private constructor.
MvdTemplateManager::MvdTemplateManager()
: d(new Private)
{
}

/*!
        Returns the unique application instance.
*/
MvdTemplateManager& MvdTemplateManager::instance()
{
        if (!mInstance) {
                QMutexLocker locker(MvdTemplateManagerLock());
                if (!mInstance) {
                        if (mDestroyed)
                                throw std::runtime_error("Template Manager: access to dead reference");
                        create();
                }
        }

        return (MvdTemplateManager&) *mInstance;
}

//! Destructor.
MvdTemplateManager::~MvdTemplateManager()
{
        delete d;
        mInstance = 0;
        mDestroyed = true;
}

void MvdTemplateManager::create()
{
        // Local static members are instantiated as soon
        // as this function is entered for the first time
        // (Scott Meyers singleton)
        static MvdTemplateManager instance;
        mInstance = &instance;
}

/*!
        Returns the name of the available movie templates.
        \todo Templates will have an xml descriptor with name, author, etc and a thumbnail!
*/
QStringList MvdTemplateManager::templates(const QString& category) const
{
        QDir dir(QString(":/Templates/%1/").arg(category));
        QFileInfoList files = dir.entryInfoList();

        QStringList names;

        for (int i = 0; i < files.size(); ++i)
        {
                if (!files.at(i).isDir())
                        continue;
                names << files.at(i).fileName();
        }

        dir = QDir(paths().resourcesDir().append("Templates/").append(category));
        files = dir.entryInfoList();

        for (int i = 0; i < files.size(); ++i)
        {
                if (!files.at(i).isDir())
                        continue;
                names << files.at(i).fileName();
        }

        return names;
}

//! Returns an empty string if \p movie is not valid. \todo Use XmlWriter or encode special chars (i.e. "&").
QString MvdTemplateManager::movieToXml(const MvdMovie& movie,
        const MvdMovieCollection& collection)
{
        //! \todo movie to MvdMovie class!
        //! \todo sanitize xml (maybe in mvdmovie or moviedata class?)

        if (!movie.isValid())
                return QString();

        QString xml = "<movie>\n";

        QString s = movie.validTitle();
        xml.append(QString("\t<title>%1</title>\n").arg(s));

        s = movie.title();
        if (!s.isEmpty()) {
                s = movie.originalTitle();
                if (!s.isEmpty())
                        xml.append(QString("\t<original-title>%1</original-title>\n").arg(s));
        }

        s = movie.year();
        if (!s.isEmpty())
                xml.append(QString("\t<year>%1</year>\n").arg(s));

        s = movie.imdbId();
        if (!s.isEmpty()) {
                s = MvdCore::parameter("mvdcore/imdb-movie-url").toString().arg(s);
                xml.append(QString("\t<imdb-url>%1</imdb-url>\n").arg(s));
        }

        s = movie.plot();
        if (!s.isEmpty())
                xml.append(QString("\t<plot>\n\t\t<![CDATA[%1]]>\n\t</plot>\n").arg(s));

        s = movie.notes();
        if (!s.isEmpty())
                xml.append(QString("\t<notes>\n\t\t<![CDATA[%1]]>\n\t</notes>\n").arg(s));

        s = movie.storageId();
        if (!s.isEmpty())
                xml.append(QString("\t<storage-id>%1</storage-id>\n").arg(s));

        quint8 sh = movie.rating();
        if (sh != 0)
                xml.append(QString("\t<rating>%1</rating>\n").arg(sh));

        sh = movie.runningTime();
        if (sh != 0) {
                xml.append(QString("\t<running-time>%1</running-time>\n").arg(sh));
                s = movie.runningTimeString();
                xml.append(QString("\t<running-time-string>%1</running-time-string>\n").arg(s));
        }

        s = movie.colorModeString();
        if (!s.isEmpty())
                xml.append(QString("\t<color-mode>%1</color-mode>\n").arg(s));

        QList<mvdid> idList = movie.languages();
        if (!idList.isEmpty())
        {
                xml.append("\t<languages>\n");
                for (int i = 0; i < idList.size(); ++i)
                {
                        mvdid id = idList.at(i);
                        const MvdSdItem& sd = collection.sharedData().item(id);
                        if (!sd.value.isEmpty())
                                xml.append(QString("\t\t<language>%1</language>\n").arg(sd.value));
                }
                xml.append("\t</languages>\n");
        }

        idList = movie.countries();
        if (!idList.isEmpty())
        {
                xml.append("\t<countries>\n");
                for (int i = 0; i < idList.size(); ++i)
                {
                        mvdid id = idList.at(i);
                        const MvdSdItem& sd = collection.sharedData().item(id);
                        if (!sd.value.isEmpty())
                                xml.append(QString("\t\t<country>%1</country>\n").arg(sd.value));
                }
                xml.append("\t</countries>\n");
        }

        idList = movie.tags();
        if (!idList.isEmpty())
        {
                xml.append("\t<tags>\n");
                for (int i = 0; i < idList.size(); ++i)
                {
                        mvdid id = idList.at(i);
                        const MvdSdItem& sd = collection.sharedData().item(id);
                        if (!sd.value.isEmpty())
                                xml.append(QString("\t\t<tag>%1</tag>\n").arg(sd.value));
                }
                xml.append("\t</tags>\n");
        }

        idList = movie.genres();
        if (!idList.isEmpty())
        {
                xml.append("\t<genres>\n");
                for (int i = 0; i < idList.size(); ++i)
                {
                        mvdid id = idList.at(i);
                        const MvdSdItem& sd = collection.sharedData().item(id);
                        if (!sd.value.isEmpty())
                                xml.append(QString("\t\t<genre>%1</genre>\n").arg(sd.value));
                }
                xml.append("\t</genres>\n");
        }

        idList = movie.directors();
        if (!idList.isEmpty())
        {
                xml.append("\t<directors>\n");
                for (int i = 0; i < idList.size(); ++i)
                {
                        mvdid id = idList.at(i);
                        const MvdSdItem& sd = collection.sharedData().item(id);
                        if (!sd.value.isEmpty())
                        {
                                xml.append("\t\t<person>\n");
                                xml.append(QString("\t\t\t<name>%1</name>\n").arg(sd.value));
                                xml.append(QString("\t\t\t<imdb-id>%1</imdb-id>\n").arg(sd.id));
                                xml.append("\t\t</person>\n");
                        }
                }
                xml.append("\t</directors>\n");
        }

        idList = movie.producers();
        if (!idList.isEmpty())
        {
                xml.append("\t<producers>\n");
                for (int i = 0; i < idList.size(); ++i)
                {
                        mvdid id = idList.at(i);
                        const MvdSdItem& sd = collection.sharedData().item(id);
                        if (!sd.value.isEmpty())
                        {
                                xml.append("\t\t<person>\n");
                                xml.append(QString("\t\t\t<name>%1</name>\n").arg(sd.value));
                                xml.append(QString("\t\t\t<imdb-id>%1</imdb-id>\n").arg(sd.id));
                                xml.append("\t\t</person>\n");
                        }
                }
                xml.append("\t</producers>\n");
        }

        QList<MvdRoleItem> idRoleList = movie.actors();
        if (!idRoleList.isEmpty())
        {
                xml.append("\t<cast>\n");
                for (int i = 0; i < idRoleList.size(); ++i)
                {
                        const MvdRoleItem& item = idRoleList.at(i);
                        mvdid id = item.first;
                        QStringList roles = item.second;
                        const MvdSdItem& sd = collection.sharedData().item(id);
                        if (!sd.value.isEmpty())
                        {
                                xml.append("\t\t<person>\n");
                                xml.append(QString("\t\t\t<name>%1</name>\n").arg(sd.value));
                                xml.append(QString("\t\t\t<imdb-id>%1</imdb-id>\n").arg(sd.id));
                                if (!roles.isEmpty())
                                {
                                        xml.append("\t\t\t<roles>\n");
                                        for (int i = 0; i < roles.size(); ++i)
                                                if (!roles.at(i).isEmpty())
                                                        xml.append(QString("\t\t\t\t<role>%1</role>\n").arg(roles.at(i)));
                                        xml.append("\t\t\t</roles>\n");
                                }
                                xml.append("\t\t</person>\n");
                        }
                }
                xml.append("\t</cast>\n");
        }

        idRoleList = movie.crewMembers();
        if (!idRoleList.isEmpty())
        {
                xml.append("\t<crew>\n");
                for (int i = 0; i < idRoleList.size(); ++i)
                {
                        const MvdRoleItem& item = idRoleList.at(i);
                        mvdid id = item.first;
                        QStringList roles = item.second;
                        const MvdSdItem& sd = collection.sharedData().item(id);
                        if (!sd.value.isEmpty())
                        {
                                xml.append("\t\t<person>\n");
                                xml.append(QString("\t\t\t<name>%1</name>\n").arg(sd.value));
                                xml.append(QString("\t\t\t<imdb-id>%1</imdb-id>\n").arg(sd.id));
                                if (!roles.isEmpty())
                                {
                                        xml.append("\t\t\t<roles>\n");
                                        for (int i = 0; i < roles.size(); ++i)
                                                if (!roles.at(i).isEmpty())
                                                        xml.append(QString("\t\t\t\t<role>%1</role>\n").arg(roles.at(i)));
                                        xml.append("\t\t\t</roles>\n");
                                }
                                xml.append("\t\t</person>\n");
                        }
                }
                xml.append("\t</crew>\n");
        }

        QList<MvdUrl> urlList = movie.urls();
        if (!urlList.isEmpty())
        {
                xml.append("\t<urls>\n");
                for (int i = 0; i < urlList.size(); ++i)
                {
                        MvdUrl url = urlList.at(i);

                        xml.append("\t\t<url>\n");
                        if (url.description.isEmpty())
                                xml.append(QString("\t\t\t<url>%1</url>\n").arg(url.url));
                        else xml.append(QString("\t\t\t<url description=\"%1\">%2</url>\n").arg(url.description).arg(url.url));
                        xml.append("\t\t</url>\n");
                }
                xml.append("\t</urls>\n");
        }

        QStringList sl = movie.specialContents();
        if (!sl.isEmpty())
        {
                xml.append("\t<special-contents>\n");
                for (int i = 0; i < sl.size(); ++i)
                {
                        if (!sl.at(i).isEmpty())
                                xml.append(QString("\t\t<item>%1</item>\n").arg(sl.at(i)));
                }
                xml.append("\t</special-contents>\n");
        }

        Movida::Tags tags = movie.specialTags();
        if (tags & Movida::SeenTag) {
                xml.append("\t<seen>true</seen>\n");
        }
        if (tags & Movida::LoanedTag) {
                xml.append("\t<loaned>true</loaned>\n");
        }
        if (tags & Movida::SpecialTag) {
                xml.append("\t<special>true</special>\n");
        }

        s = movie.poster();
        if (!s.isEmpty()) {
                QString p = collection.metaData(MvdMovieCollection::DataPathInfo);
                p.append("/images/").append(s);
                QUrl url = QUrl::fromLocalFile(p);
                xml.append(QString("\t<poster>%1</poster>\n").arg(url.toString()));
        }

        xml.append("</movie>");
        return xml;
}

//! Uses the default template if \p templateName is empty.
QString MvdTemplateManager::movieToHtml(const MvdMovie& movie, const MvdMovieCollection& collection,
        const QString& templateCategory, const QString& templateName)
{
        if (!movie.isValid())
                return QString();

        QString path = paths().resourcesDir().append("Templates/").append(templateCategory).append("/").append(templateName).append("/Movie.xsl");
        if (templateName.isEmpty() || !QFile::exists(path))
                path = QString(":/Templates/%1/Default/Movie.xsl").arg(templateCategory);

        MvdXsltProc xsl(path);
        return xsl.processText(movieToXml(movie, collection));
}

//! Uses the default template if \p templateName is empty.
bool MvdTemplateManager::movieToHtmlFile(const MvdMovie& movie, const MvdMovieCollection& collection,
        const QString& filename, const QString& templateCategory, const QString& templateName)
{
        if (!movie.isValid())
                return false;

        QString path = paths().resourcesDir().append("Templates/").append(templateCategory).append("/").append(templateName).append("/Movie.xsl");
        if (templateName.isEmpty() || !QFile::exists(path))
                path = QString(":/Templates/%1/Default/Movie.xsl").arg(templateCategory);

        QFile file(filename);
        if (!file.open(QIODevice::ReadWrite))
                return false;

        MvdXsltProc xsl(path);
        return xsl.processTextToDevice(movieToXml(movie, collection), &file);
}

//! Uses the default template if \p templateName is empty.
QString MvdTemplateManager::movieDataToHtml(const MvdMovieData& movieData,
        const QString& templateCategory, const QString& templateName)
{
        if (!movieData.isValid())
                return QString();

        QString path = paths().resourcesDir().append("Templates/").append(templateCategory).append("/").append(templateName).append("/Movie.xsl");
        if (templateName.isEmpty() || !QFile::exists(path))
                path = QString(":/Templates/%1/Default/Movie.xsl").arg(templateCategory);

        MvdXsltProc xsl(path);
        QString xml;
        movieData.writeToXmlString(&xml);
        return xsl.processText(xml);
}

//! Uses the default template if \p templateName is empty.
bool MvdTemplateManager::movieDataToHtmlFile(const MvdMovieData& movieData,
        const QString& filename, const QString& templateCategory, const QString& templateName)
{
        if (!movieData.isValid())
                return false;

        QString path = paths().resourcesDir().append("Templates/").append(templateCategory).append("/").append(templateName).append("/Movie.xsl");
        if (templateName.isEmpty() || !QFile::exists(path))
                path = QString(":/Templates/%1/Default/Movie.xsl").arg(templateCategory);

        MvdXsltProc xsl(path);
        QString xml;
        movieData.writeToXmlString(&xml);

        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly))
                return false;
        return xsl.processTextToDevice(xml, &file);
}

/*! Converts an movida-movie-data XML description contained in a file to an HTML description using the specified
        template or the default template if \p templateName is empty.
*/
QString MvdTemplateManager::movieDataFileToHtml(const QString& movieDataFile,
        const QString& templateCategory, const QString& templateName)
{
        if (!QFile::exists(movieDataFile))
                return QString();

        QString path = paths().resourcesDir().append("Templates/").append(templateCategory).append("/").append(templateName).append("/Movie.xsl");
        if (templateName.isEmpty() || !QFile::exists(path))
                path = QString(":/Templates/%1/Default/Movie.xsl").arg(templateCategory);

        MvdXsltProc xsl(path);
        return xsl.processFile(movieDataFile);
}

/*! Converts an movida-movie-data XML description to an HTML description using the specified
        template or the default template if \p templateName is empty.
*/
QString MvdTemplateManager::movieDataStringToHtml(const QString& movieDataString,
        const QString& templateCategory, const QString& templateName)
{
        if (!QFile::exists(movieDataString))
                return QString();

        QString path = paths().resourcesDir().append("Templates/").append(templateCategory).append("/").append(templateName).append("/Movie.xsl");
        if (templateName.isEmpty() || !QFile::exists(path))
                path = QString(":/Templates/%1/Default/Movie.xsl").arg(templateCategory);

        MvdXsltProc xsl(path);
        return xsl.processText(movieDataString);
}

//! Uses the default template if \p templateName is empty.
bool MvdTemplateManager::collectionToHtmlFile(MvdMovieCollection* collection,
        const QString& filename, const QString& templateCategory, const QString& templateName)
{
        QString out;
        MvdXmlWriter xml(&out);
        xml.setSkipEmptyTags(true);
        xml.setSkipEmptyAttributes(true);

        QString name = collection ?
                collection->metaData(MvdMovieCollection::NameInfo) : QString();
        xml.writeOpenTag("collection-info",
                MvdXmlWriter::Attribute("movies", QString::number(collection ? collection->count() : 0)),
                MvdXmlWriter::Attribute("name", name)
        );

        if (collection) {
                // NameInfo, OwnerInfo, EMailInfo, WebsiteInfo, NotesInfo
                xml.writeTaggedString("owner", collection->metaData(MvdMovieCollection::OwnerInfo));
                xml.writeTaggedString("email", collection->metaData(MvdMovieCollection::EMailInfo));
                xml.writeTaggedString("website", collection->metaData(MvdMovieCollection::WebsiteInfo));
                xml.writeTaggedString("notes", collection->metaData(MvdMovieCollection::NotesInfo));
        }

        xml.writeCloseTag("collection-info");

        QString path = paths().resourcesDir().append("Templates/").append(templateCategory).append("/").append(templateName).append("/Collection.xsl");
        if (templateName.isEmpty() || !QFile::exists(path))
                path = QString(":/Templates/%1/Default/Collection.xsl").arg(templateCategory);

        MvdXsltProc xsl(path);
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly))
                return false;
        return xsl.processTextToDevice(out, &file);
}

//! Convenience method to access the MvdTemplateManager singleton.
MvdTemplateManager& Movida::tmanager()
{
        return MvdTemplateManager::instance();
}
