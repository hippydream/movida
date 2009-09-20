/**************************************************************************
** Filename: moviedata.cpp
**
** Copyright (C) 2007-2009 Angius Fabrizio. All rights reserved.
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

#include "moviedata.h"

#include "core.h"
#include "logger.h"
#include "utils.h"
#include "xmlwriter.h"

#include <QtCore/QFile>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

using namespace Movida;

/*!
    \class MvdMovieData moviedata.h
    \ingroup MvdCore

    \brief Stores "raw" movie data - i.e. no references to shared items.
    \todo Make shared.
*/

//! \internal
namespace {
QList<MvdMovieData::PersonData> extractPersonData(xmlNodePtr parent, xmlDocPtr doc);
QList<MvdMovieData::UrlData> extractUrlData(xmlNodePtr parent, xmlDocPtr doc);
QStringList extractStringList(xmlNodePtr parent, xmlDocPtr doc, const char *tag);
QHash<QString, QVariant> extractKeyValueList(xmlNodePtr parent, xmlDocPtr doc, const char *tag);
void writeToXml(MvdXmlWriter *writer, const MvdMovieData &movie, MvdMovieData::Options o);
}

/*!	Extracts a list of person descriptions from an xml node.

    \verbatim
    <person>
    <name>Some Name</name>
    <imdb-id>0123456</imdb-id>
    <roles>
    <role>Some Role</role>
    <role>Wow, another role!</role>
    </roles>
    </person>
    \endverbatim
*/
QList<MvdMovieData::PersonData> extractPersonData(xmlNodePtr parent, xmlDocPtr doc)
{
    QList<MvdMovieData::PersonData> persons;

    xmlNodePtr node = parent->xmlChildrenNode;
    while (node) {
        if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (const xmlChar *)"person")) {
            node = node->next;
            continue;
        }

        MvdMovieData::PersonData pd;
        xmlNodePtr pNode = node->xmlChildrenNode;
        while (pNode) {
            if (pNode->type != XML_ELEMENT_NODE) {
                pNode = pNode->next;
                continue;
            }

            if (!xmlStrcmp(pNode->name, (const xmlChar *)"name")) {
                xmlChar *c = xmlNodeListGetString(doc, pNode->xmlChildrenNode, 1);
                pd.name = MvdCore::decodeXmlEntities((const char *)c).trimmed();
                xmlFree(c);
            } else if (!xmlStrcmp(pNode->name, (const xmlChar *)"imdb-id")) {
                xmlChar *c = xmlNodeListGetString(doc, pNode->xmlChildrenNode, 1);
                QString s = MvdCore::decodeXmlEntities((const char *)c).trimmed();
                xmlFree(c);
                QRegExp rx(Movida::core().parameter("mvdcore/imdb-id-regexp").toString());
                if (rx.exactMatch(s))
                    pd.imdbId = s;
            } else if (!xmlStrcmp(pNode->name, (const xmlChar *)"roles")) {
                QStringList roles;
                xmlNodePtr rNode = pNode->xmlChildrenNode;
                while (rNode) {
                    if ((rNode->type != XML_ELEMENT_NODE) || xmlStrcmp(rNode->name, (const xmlChar *)"role")) {
                        rNode = rNode->next;
                        continue;
                    }

                    xmlChar *c = xmlNodeListGetString(doc, rNode->xmlChildrenNode, 1);
                    QString s = MvdCore::decodeXmlEntities((const char *)c).trimmed();
                    xmlFree(c);
                    if (!s.isEmpty())
                        roles.append(s);
                    rNode = rNode->next;
                }
                pd.roles = roles;
            }

            pNode = pNode->next;
        }

        int idx = persons.indexOf(pd);
        if (idx >= 0)
            persons[idx].merge(pd);
        else persons.append(pd);

        node = node->next;
    }

    return persons;
}

/*!	Extracts a list of url descriptions from an xml node.

    \verbatim
    <url description="some url" default="true">http://www.somehost.com</url>
    \endverbatim
*/
QList<MvdMovieData::UrlData> extractUrlData(xmlNodePtr parent, xmlDocPtr doc)
{
    QList<MvdMovieData::UrlData> urls;

    xmlNodePtr node = parent->xmlChildrenNode;
    while (node) {
        if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (const xmlChar *)"url")) {
            node = node->next;
            continue;
        }

        MvdMovieData::UrlData ud;
        xmlChar *c = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        ud.url = MvdCore::decodeXmlEntities((const char *)c).trimmed();
        xmlFree(c);

        xmlChar *attr = xmlGetProp(node, (const xmlChar *)"description");
        if (attr) {
            ud.description = MvdCore::decodeXmlEntities((const char *)attr);
            xmlFree(attr);
        }

        attr = xmlGetProp(node, (const xmlChar *)"default");
        if (attr) {
            ud.isDefault = !xmlStrcmp(attr, (const xmlChar *)"true");
            xmlFree(attr);
        }

        urls.append(ud);

        node = node->next;
    }

    return urls;
}

/*!	Extracts a list of simple string tags. Excludes duplicates (case insensitive comparison).

    \verbatim
    <$tag>some string</$tag>
    \endverbatim
*/
QStringList extractStringList(xmlNodePtr parent, xmlDocPtr doc, const char *tag)
{
    Q_ASSERT(tag);
    QStringList list;

    xmlNodePtr node = parent->xmlChildrenNode;
    while (node) {
        if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (const xmlChar *)tag)) {
            node = node->next;
            continue;
        }

        xmlChar *c = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        QString s = MvdCore::decodeXmlEntities((const char *)c).trimmed();
        xmlFree(c);
        if (!list.contains(s, Qt::CaseInsensitive))
            list.append(s);

        node = node->next;
    }

    return list;
}

/*!	Extracts a list of simple key=value tags.
    Movida::stringToVariant() is used to decode the values.

    \verbatim
    <$tag name="key">some value</$tag>
    \endverbatim
*/
QHash<QString, QVariant> extractKeyValueList(xmlNodePtr parent, xmlDocPtr doc, const char *tag)
{
    Q_ASSERT(tag);
    QHash<QString, QVariant> list;

    xmlNodePtr node = parent->xmlChildrenNode;
    while (node) {
        if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (const xmlChar *)tag)) {
            node = node->next;
            continue;
        }

        xmlChar *attr = xmlGetProp(node, (const xmlChar *)"name");
        if (!attr) {
            node = node->next;
            continue;
        }

        QString key = MvdCore::decodeXmlEntities((const char *)attr);
        xmlFree(attr);

        xmlChar *c = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        QString s = MvdCore::decodeXmlEntities((const char *)c).trimmed();
        xmlFree(c);

        QVariant val = Movida::stringToVariant(s);
        list.insert(key, val);

        node = node->next;
    }

    return list;
}

void writeToXml(MvdXmlWriter *writer, const MvdMovieData &movie, MvdMovieData::Options o)
{
    Q_ASSERT(writer);

    if (!movie.isValid())
        return;

    writer->setSkipEmptyTags(true);
    writer->writeOpenTag("movie");

    if (!movie.title.isEmpty()) {
        writer->writeTaggedString("title", movie.title);
        if (!movie.originalTitle.isEmpty())
            writer->writeTaggedString("original-title", movie.originalTitle);
    } else {
        writer->writeTaggedString("title", movie.originalTitle);
    }

    writer->writeTaggedString("year", movie.year);

    if (!movie.imdbId.isEmpty()) {
        writer->writeTaggedString("imdb-id", movie.imdbId);
        writer->writeTaggedString("imdb-url", Movida::core().parameter("mvdcore/imdb-movie-url").toString().arg(movie.imdbId));
    }

    writer->writeCDataString("plot", movie.plot);
    writer->writeCDataString("notes", movie.notes);
    writer->writeTaggedString("storage-id", movie.storageId);
    writer->writeTaggedString("rating", QString::number(movie.rating),
        MvdAttribute("maximum", Movida::core().parameter("mvdcore/max-rating").toString()));
    writer->writeTaggedString("running-time", QString::number(movie.runningTime));
    writer->writeTaggedString("color-mode", Movida::colorModeToString(movie.colorMode));

    if (movie.specialTags & Movida::SeenTag)
        writer->writeTaggedString("seen", "true");
    if (movie.specialTags & Movida::LoanedTag)
        writer->writeTaggedString("loaned", "true");
    if (movie.specialTags & Movida::SpecialTag)
        writer->writeTaggedString("special", "true");

    if (!movie.languages.isEmpty()) {
        writer->writeOpenTag("languages");
        for (int i = 0; i < movie.languages.size(); ++i)
            writer->writeTaggedString("language", movie.languages.at(i));
        writer->writeCloseTag("languages");
    }

    if (!movie.countries.isEmpty()) {
        writer->writeOpenTag("countries");
        for (int i = 0; i < movie.countries.size(); ++i)
            writer->writeTaggedString("country", movie.countries.at(i));
        writer->writeCloseTag("countries");
    }

    if (!movie.tags.isEmpty()) {
        writer->writeOpenTag("tags");
        for (int i = 0; i < movie.tags.size(); ++i)
            writer->writeTaggedString("tag", movie.tags.at(i));
        writer->writeCloseTag("tags");
    }

    if (!movie.genres.isEmpty()) {
        writer->writeOpenTag("genres");
        for (int i = 0; i < movie.genres.size(); ++i)
            writer->writeTaggedString("genre", movie.genres.at(i));
        writer->writeCloseTag("genres");
    }

    if (!movie.directors.isEmpty()) {
        writer->writeOpenTag("directors");
        for (int i = 0; i < movie.directors.size(); ++i) {
            const MvdMovieData::PersonData &pd = movie.directors.at(i);
            writer->writeOpenTag("person");
            writer->writeTaggedString("name", pd.name);
            writer->writeTaggedString("imdb-id", pd.imdbId);
            writer->writeCloseTag("person");
        }
        writer->writeCloseTag("directors");
    }

    if (!movie.producers.isEmpty()) {
        writer->writeOpenTag("producers");
        for (int i = 0; i < movie.producers.size(); ++i) {
            const MvdMovieData::PersonData &pd = movie.producers.at(i);
            writer->writeOpenTag("person");
            writer->writeTaggedString("name", pd.name);
            writer->writeTaggedString("imdb-id", pd.imdbId);
            writer->writeCloseTag("person");
        }
        writer->writeCloseTag("producers");
    }

    if (!movie.actors.isEmpty()) {
        writer->writeOpenTag("cast");
        for (int i = 0; i < movie.actors.size(); ++i) {
            const MvdMovieData::PersonData &pd = movie.actors.at(i);
            writer->writeOpenTag("person");
            writer->writeTaggedString("name", pd.name);
            writer->writeTaggedString("imdb-id", pd.imdbId);
            if (!pd.roles.isEmpty()) {
                writer->writeOpenTag("roles");
                for (int j = 0; j < pd.roles.size(); ++j) {
                    writer->writeTaggedString("role", pd.roles.at(j));
                }
                writer->writeCloseTag("roles");
            }
            writer->writeCloseTag("person");
        }
        writer->writeCloseTag("cast");
    }

    if (!movie.crewMembers.isEmpty()) {
        writer->writeOpenTag("crew");
        for (int i = 0; i < movie.crewMembers.size(); ++i) {
            const MvdMovieData::PersonData &pd = movie.crewMembers.at(i);
            writer->writeOpenTag("person");
            writer->writeTaggedString("name", pd.name);
            writer->writeTaggedString("imdb-id", pd.imdbId);
            if (!pd.roles.isEmpty()) {
                writer->writeOpenTag("roles");
                for (int j = 0; j < pd.roles.size(); ++j) {
                    writer->writeTaggedString("role", pd.roles.at(j));
                }
                writer->writeCloseTag("roles");
            }
            writer->writeCloseTag("person");
        }
        writer->writeCloseTag("crew");
    }

    if (!movie.urls.isEmpty()) {
        writer->writeOpenTag("urls");
        for (int i = 0; i < movie.urls.size(); ++i) {
            const MvdMovieData::UrlData &ud = movie.urls.at(i);
            if (!ud.description.isEmpty()) {
                if (ud.isDefault)
                    writer->writeTaggedString("url", ud.url, MvdAttribute("description", ud.description),
                        MvdAttribute("default", "true"));
                else writer->writeTaggedString("url", ud.url, MvdAttribute("description", ud.description));
            } else writer->writeTaggedString("url", ud.url);
        }
        writer->writeCloseTag("urls");
    }

    if (!movie.specialContents.isEmpty()) {
        writer->writeOpenTag("special-contents");
        for (int i = 0; i < movie.specialContents.size(); ++i) {
            writer->writeTaggedString("item", movie.specialContents.at(i));
        }
        writer->writeCloseTag("special-contents");
    }

    if (o & MvdMovieData::EmbedMoviePoster) {
        if (!movie.posterPath.isEmpty()) {
            static const int MaxFileSize = 2 * 1024 * 1024;
            QFile file(movie.posterPath);
            if (file.size() <= MaxFileSize && file.open(QFile::ReadOnly)) {
                QByteArray b = file.readAll();
                QString s = b.toBase64();
                b.clear();
                writer->writeTaggedString("poster-data", s);
            }
        }
    } else {
        writer->writeTaggedString("poster", movie.posterPath);
    }

    if (!movie.extendedAttributes.isEmpty()) {
        writer->writeOpenTag("extended-attributes");
        QHash<QString, QVariant>::ConstIterator begin = movie.extendedAttributes.constBegin();
        QHash<QString, QVariant>::ConstIterator end = movie.extendedAttributes.constEnd();
        while (begin != end) {
            QString k = begin.key().trimmed();
            const QVariant &v = begin.value();
            if (!k.isEmpty()) {
                QString s = Movida::variantToString(v);
                writer->writeTaggedString("attribute", s,
                    MvdAttribute("name", k));
            }
            ++begin;
        }
        writer->writeCloseTag("extended-attributes");
    }

    writer->writeCloseTag("movie");
}

//! \internal
bool MvdMovieData::PersonData::operator==(const PersonData &pd) const
{
    if (!imdbId.isEmpty() && !pd.imdbId.isEmpty())
        return imdbId.toLower() == pd.imdbId.toLower();
    return name.toLower() == pd.name.toLower();
}

//! \internal
bool MvdMovieData::PersonData::operator<(const PersonData &pd) const
{
    return name < pd.name;
}

//! \internal
void MvdMovieData::PersonData::merge(const PersonData &pd)
{
    if (imdbId.isEmpty())
        imdbId = pd.imdbId;
    mergeRoles(pd.roles);
}

//! \internal
void MvdMovieData::PersonData::mergeRoles(const QStringList &roles)
{
    for (int i = 0; i < roles.size(); ++i) {
        const QString &o = roles.at(i);
        if (this->roles.contains(o, Qt::CaseInsensitive))
            continue;
        this->roles.append(o);
    }
}

//! Returns true if the movie has at least a localized or an original title.
bool MvdMovieData::isValid() const
{
    return !title.isEmpty() || !originalTitle.isEmpty();
}

//! Loads a movie data description from a movida-movie-data XML file.
bool MvdMovieData::loadFromXml(const QString &path, Options options)
{
    xmlDocPtr doc = xmlParseFile(path.toLatin1().constData());

    if (!doc) {
        eLog() << "MvdMovieData: Invalid movie data file.";
        return false;
    }

    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (xmlStrcmp(node->name, (const xmlChar *)"movida-movie-data")) {
        eLog() << "MvdMovieData: Invalid movie data file.";
        return false;
    }

    node = node->xmlChildrenNode;
    while (node) {
        if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (const xmlChar *)"movie")) {
            node = node->next;
            continue;
        }

        xmlNodePtr resultNode = node->xmlChildrenNode;
        while (resultNode) {
            if (resultNode->type != XML_ELEMENT_NODE) {
                resultNode = resultNode->next;
                continue;
            }

            // alphabetically ordered accepted tags :-)

            if (!xmlStrcmp(resultNode->name, (const xmlChar *)"cast")) {
                QList<PersonData> pd = ::extractPersonData(resultNode, doc);
                actors = pd;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"color-mode")) {
                xmlChar *c = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
                QString s = MvdCore::decodeXmlEntities((const char *)c).trimmed().toLower();
                xmlFree(c);
                if (s == "color")
                    colorMode = Movida::Color;
                else if (s == "bw")
                    colorMode = Movida::BlackWhite;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"countries")) {
                QStringList sl = ::extractStringList(resultNode, doc, "country");
                countries = sl;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"crew")) {
                QList<PersonData> pd = ::extractPersonData(resultNode, doc);
                crewMembers = pd;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"directors")) {
                QList<PersonData> pd = ::extractPersonData(resultNode, doc);
                directors = pd;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"extended-attributes")) {
                QHash<QString, QVariant> kvl = ::extractKeyValueList(resultNode, doc, "attribute");
                extendedAttributes = kvl;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"genres")) {
                QStringList sl = ::extractStringList(resultNode, doc, "genre");
                genres = sl;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"imdb-id")) {
                xmlChar *c = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
                QString s = MvdCore::decodeXmlEntities((const char *)c).trimmed();
                xmlFree(c);
                QRegExp rx(Movida::core().parameter("mvdcore/imdb-id-regexp").toString());
                if (rx.exactMatch(s))
                    imdbId = s;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"languages")) {
                QStringList sl = ::extractStringList(resultNode, doc, "language");
                languages = sl;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"loaned")) {
                xmlChar *c = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
                if (xmlStrcmp(c, (const xmlChar *)"true"))
                    specialTags |= Movida::LoanedTag;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"notes")) {
                xmlChar *c = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
                notes = MvdCore::decodeXmlEntities((const char *)c).trimmed();
                xmlFree(c);
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"original-title")) {
                xmlChar *c = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
                originalTitle = MvdCore::decodeXmlEntities((const char *)c).trimmed();
                xmlFree(c);
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"plot")) {
                xmlChar *c = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
                plot = MvdCore::decodeXmlEntities((const char *)c).trimmed();
                xmlFree(c);
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"poster")) {
                xmlChar *c = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
                posterPath = MvdCore::decodeXmlEntities((const char *)c).trimmed();
                xmlFree(c);
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"producers")) {
                QList<PersonData> pd = ::extractPersonData(resultNode, doc);
                producers = pd;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"year")) {
                xmlChar *c = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
                QString s = MvdCore::decodeXmlEntities((const char *)c).trimmed();
                xmlFree(c);
                if (MvdCore::isValidYear(s))
                    year = s;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"rating")) {
                xmlChar *c = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
                QString s = MvdCore::decodeXmlEntities((const char *)c).trimmed();
                xmlFree(c);
                xmlChar *attr = xmlGetProp(resultNode, (const xmlChar *)"maximum");
                int mvdMaximum = Movida::core().parameter("mvdcore/max-rating").toInt();
                int xmlMaximum = mvdMaximum;
                bool ok;
                if (attr) {
                    int n = QString::fromLatin1((const char *)attr).toInt(&ok);
                    xmlFree(attr);
                    if (ok && n > 0)
                        xmlMaximum = n;
                }
                float n = s.toFloat(&ok);
                if (ok) {
                    n = (mvdMaximum * n) / xmlMaximum;
                    if (n > mvdMaximum)
                        n = mvdMaximum;
                    rating = (int)n;
                }
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"running-time")) {
                xmlChar *c = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
                QString s = MvdCore::decodeXmlEntities((const char *)c).trimmed();
                xmlFree(c);
                bool ok;
                int n = s.toInt(&ok);
                if (ok && n <= Movida::core().parameter("mvdcore/max-running-time").toInt())
                    runningTime = n;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"seen")) {
                xmlChar *c = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
                if (xmlStrcmp(c, (const xmlChar *)"true"))
                    specialTags |= Movida::SeenTag;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"special")) {
                xmlChar *c = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
                if (xmlStrcmp(c, (const xmlChar *)"true"))
                    specialTags |= Movida::SpecialTag;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"special-contents")) {
                QStringList sl = ::extractStringList(resultNode, doc, "item");
                specialContents = sl;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"storage-id")) {
                xmlChar *c = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
                storageId = MvdCore::decodeXmlEntities((const char *)c).trimmed();
                xmlFree(c);
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"tags")) {
                QStringList sl = ::extractStringList(resultNode, doc, "tag");
                tags = sl;
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"title")) {
                xmlChar *c = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
                title = MvdCore::decodeXmlEntities((const char *)c).trimmed();
                xmlFree(c);
            } else if (!xmlStrcmp(resultNode->name, (const xmlChar *)"urls")) {
                QList<UrlData> pd = ::extractUrlData(resultNode, doc);
                urls = pd;
            }

            resultNode = resultNode->next;
        }

        if (options.testFlag(StopAtFirstMovie))
            break;
    }

    return true;
}

//! Writes this movie data object to a movida-movie-data XML file.
bool MvdMovieData::writeToXmlFile(const QString &path, Options options) const
{
    QFile file(path);

    if (!file.open(QIODevice::WriteOnly)) {
        eLog() << "MvdMovieData: Failed to open " << path << " for writing (" << file.errorString() << ").";
        return false;
    }

    MvdXmlWriter::Options w_opt = MvdXmlWriter::DefaultOptions;
    if (options & NoXmlEncoding)
        w_opt &= ~MvdXmlWriter::WriteEncodingOption;
    MvdXmlWriter writer(&file, 0, w_opt);
    ::writeToXml(&writer, *this, options);
    return true;
}

//! Writes this movie data object to device using movida-movie-data XML format.
bool MvdMovieData::writeToXmlDevice(QIODevice *dev, Options options) const
{
    if (!dev)
        return false;

    if (!(dev->openMode() & QIODevice::WriteOnly)) {
        eLog() << "MvdMovieData::writeToXmlDevice() cannot write to device.";
        return false;
    }

    MvdXmlWriter::Options w_opt = MvdXmlWriter::DefaultOptions;
    if (options & NoXmlEncoding)
        w_opt &= ~MvdXmlWriter::WriteEncodingOption;
    MvdXmlWriter writer(dev, 0, w_opt);
    ::writeToXml(&writer, *this, options);
    return true;
}

//! Writes this movie data object as a movida-movie-data XML structure to a file.
void MvdMovieData::writeToXmlString(QString *string, Options options) const
{
    if (!string)
        return;

    MvdXmlWriter::Options w_opt = MvdXmlWriter::DefaultOptions;
    if (options & NoXmlEncoding)
        w_opt &= ~MvdXmlWriter::WriteEncodingOption;
    MvdXmlWriter writer(string, 0, w_opt);
    ::writeToXml(&writer, *this, options);
}
