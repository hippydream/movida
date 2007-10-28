/**************************************************************************
** Filename: moviedata.cpp
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

#include "moviedata.h"
#include "logger.h"
#include "core.h"
#include "xmlwriter.h"
#include <QFile>
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
namespace MvdMovieData_P {
	static QList<MvdMovieData::PersonData> extractPersonData(xmlNodePtr parent, xmlDocPtr doc);
	static QList<MvdMovieData::UrlData> extractUrlData(xmlNodePtr parent, xmlDocPtr doc);
	static QStringList extractStringList(xmlNodePtr parent, xmlDocPtr doc, const char* tag);
	static void writeToXml(MvdXmlWriter* writer, const MvdMovieData& movie);
};

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
QList<MvdMovieData::PersonData> MvdMovieData_P::extractPersonData(xmlNodePtr parent, xmlDocPtr doc)
{
	QList<MvdMovieData::PersonData> persons;

	xmlNodePtr node = parent->xmlChildrenNode;
	while (node) {
		if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (const xmlChar*) "person")) {
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

			if (!xmlStrcmp(pNode->name, (const xmlChar*) "name")) {
				pd.name = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, pNode->xmlChildrenNode, 1)).trimmed();
			} else if (!xmlStrcmp(pNode->name, (const xmlChar*) "imdb-id")) {
				QString s = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, pNode->xmlChildrenNode, 1)).trimmed();
				QRegExp rx(MvdCore::parameter("mvdcore/imdb-id-regexp").toString());
				if (rx.exactMatch(s))
					pd.imdbId = s;
			} else if (!xmlStrcmp(pNode->name, (const xmlChar*) "roles")) {
				QStringList roles;
				xmlNodePtr rNode = pNode->xmlChildrenNode;
				while (rNode) {
					if ((rNode->type != XML_ELEMENT_NODE) || xmlStrcmp(pNode->name, (const xmlChar*) "role")) {
						rNode = rNode->next;
						continue;
					}

					QString s = QString::fromUtf8((const char*)xmlNodeListGetString(
						doc, rNode->xmlChildrenNode, 1)).trimmed();
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
QList<MvdMovieData::UrlData> MvdMovieData_P::extractUrlData(xmlNodePtr parent, xmlDocPtr doc)
{
	QList<MvdMovieData::UrlData> urls;

	xmlNodePtr node = parent->xmlChildrenNode;
	while (node) {
		if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (const xmlChar*) "url")) {
			node = node->next;
			continue;
		}

		MvdMovieData::UrlData ud;
		ud.url = QString::fromUtf8((const char*)xmlNodeListGetString(
			doc, node->xmlChildrenNode, 1)).trimmed();

		xmlChar* attr = xmlGetProp(node, (const xmlChar*) "description");
		if (attr) {
			ud.description = QString::fromUtf8((const char*)attr);
			xmlFree(attr);
		}

		attr = xmlGetProp(node, (const xmlChar*) "default");
		if (attr) {
			ud.isDefault = !xmlStrcmp(attr, (const xmlChar*) "true");
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
QStringList MvdMovieData_P::extractStringList(xmlNodePtr parent, xmlDocPtr doc, const char* tag)
{
	Q_ASSERT(tag);
	QStringList list;

	xmlNodePtr node = parent->xmlChildrenNode;
	while (node) {
		if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (const xmlChar*) tag)) {
			node = node->next;
			continue;
		}

		QString s = QString::fromUtf8((const char*)xmlNodeListGetString(
			doc, node->xmlChildrenNode, 1)).trimmed();
		if (!list.contains(s, Qt::CaseInsensitive))
			list.append(s);

		node = node->next;
	}

	return list;
}

void MvdMovieData_P::writeToXml(MvdXmlWriter* writer, const MvdMovieData& movie)
{
	Q_ASSERT(writer);

	if (!movie.isValid())
		return;

	writer->writeOpenTag("movie");

	writer->writeTaggedString("title", movie.title);
	writer->writeTaggedString("original-title", movie.originalTitle);
	writer->writeTaggedString("release-year", movie.releaseYear);
	writer->writeTaggedString("production-year", movie.productionYear);
	writer->writeTaggedString("edition", movie.edition);
	writer->writeTaggedString("imdb-id", movie.imdbId);
	writer->writeTaggedString("plot", QString(movie.plot).prepend("<![CDATA[").append("]]>"));
	writer->writeTaggedString("notes", QString(movie.notes).prepend("<![CDATA[").append("]]>"));
	writer->writeTaggedString("storage-id", movie.storageId);
	writer->writeTaggedString("rating", QString::number(movie.rating), 
		MvdXmlWriter::Attribute("maximum", MvdCore::parameter("mvdcore/max-rating").toString()));
	writer->writeTaggedString("running-time", QString::number(movie.runningTime));
	writer->writeTaggedString("color-mode", MvdMovie::colorModeToString(movie.colorMode));
	writer->writeOpenTag("languages");
	for (int i = 0; i < movie.languages.size(); ++i)
		writer->writeTaggedString("language", movie.languages.at(i));
	writer->writeCloseTag("languages");
	writer->writeOpenTag("countries");
	for (int i = 0; i < movie.countries.size(); ++i)
		writer->writeTaggedString("country", movie.countries.at(i));
	writer->writeCloseTag("countries");
	writer->writeOpenTag("tags");
	for (int i = 0; i < movie.tags.size(); ++i)
		writer->writeTaggedString("tag", movie.tags.at(i));
	writer->writeCloseTag("tags");
	writer->writeOpenTag("genres");
	for (int i = 0; i < movie.genres.size(); ++i)
		writer->writeTaggedString("genre", movie.genres.at(i));
	writer->writeCloseTag("genres");

	writer->writeOpenTag("directors");
	for (int i = 0; i < movie.directors.size(); ++i) {
		const MvdMovieData::PersonData& pd = movie.directors.at(i);
		writer->writeOpenTag("person");
		writer->writeTaggedString("name", pd.name);
		writer->writeTaggedString("imdb-id", pd.imdbId);
		writer->writeCloseTag("person");
	}
	writer->writeCloseTag("directors");

	writer->writeOpenTag("producers");
	for (int i = 0; i < movie.producers.size(); ++i) {
		const MvdMovieData::PersonData& pd = movie.producers.at(i);
		writer->writeOpenTag("person");
		writer->writeTaggedString("name", pd.name);
		writer->writeTaggedString("imdb-id", pd.imdbId);
		writer->writeCloseTag("person");
	}
	writer->writeCloseTag("producers");

	writer->writeOpenTag("cast");
	for (int i = 0; i < movie.actors.size(); ++i) {
		const MvdMovieData::PersonData& pd = movie.actors.at(i);
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

	writer->writeOpenTag("crew");
	for (int i = 0; i < movie.crewMembers.size(); ++i) {
		const MvdMovieData::PersonData& pd = movie.crewMembers.at(i);
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

	writer->writeOpenTag("urls");
	for (int i = 0; i < movie.urls.size(); ++i) {
		const MvdMovieData::UrlData& ud = movie.urls.at(i);
			if (!ud.description.isEmpty()) {
				if (ud.isDefault)
					writer->writeTaggedString("url", ud.url, MvdXmlWriter::Attribute("description", ud.description),
						MvdXmlWriter::Attribute("default", "true"));
				else writer->writeTaggedString("url", ud.url, MvdXmlWriter::Attribute("description", ud.description));
			} else writer->writeTaggedString("url", ud.url);
	}
	writer->writeCloseTag("urls");

	writer->writeOpenTag("special-contents");
	for (int i = 0; i < movie.specialContents.size(); ++i) {
			writer->writeTaggedString("item", movie.specialContents.at(i));
	}
	writer->writeCloseTag("special-contents");

	writer->writeCloseTag("movie");
}

//! \internal
bool MvdMovieData::PersonData::operator==(const PersonData& pd) const
{
	if (!imdbId.isEmpty() && !pd.imdbId.isEmpty())
		return imdbId.toLower() == pd.imdbId.toLower();
	return name.toLower() == pd.name.toLower();
}

//! \internal
bool MvdMovieData::PersonData::operator<(const PersonData& pd) const
{
	return name < pd.name;
}

//! \internal
void MvdMovieData::PersonData::merge(const PersonData& pd)
{
	if (imdbId.isEmpty())
		imdbId = pd.imdbId;
	mergeRoles(pd.roles);
}

//! \internal
void MvdMovieData::PersonData::mergeRoles(const QStringList& roles)
{
	for (int i = 0; i < roles.size(); ++i) {
		const QString& o = roles.at(i);
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
bool MvdMovieData::loadFromXml(const QString& path, Options options)
{
	xmlDocPtr doc = xmlParseFile(path.toLatin1().constData());
	if (!doc) {
		eLog() << "MvdMovieData: Invalid movie data file.";
		return false;
	}

	xmlNodePtr node = xmlDocGetRootElement(doc);
	if (xmlStrcmp(node->name, (const xmlChar*) "movida-movie-data")) {
		eLog() << "MvdMovieData: Invalid movie data file.";
		return false;
	}

	node = node->xmlChildrenNode;
	while (node) {
		if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (const xmlChar*) "movie")) {
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

			if (!xmlStrcmp(resultNode->name, (const xmlChar*) "cast")) {
				QList<PersonData> pd = MvdMovieData_P::extractPersonData(resultNode, doc);
				actors = pd;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "color-mode")) {
				QString s = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, resultNode->xmlChildrenNode, 1)).trimmed().toLower();
				if (s == "color")
					colorMode = MvdMovie::Color;
				else if (s == "bw")
					colorMode = MvdMovie::BlackWhite;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "countries")) {
				QStringList sl = MvdMovieData_P::extractStringList(resultNode, doc, "country");
				countries = sl;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "crew")) {
				QList<PersonData> pd = MvdMovieData_P::extractPersonData(resultNode, doc);
				crewMembers = pd;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "directors")) {
				QList<PersonData> pd = MvdMovieData_P::extractPersonData(resultNode, doc);
				directors = pd;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "edition")) {
				edition = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, resultNode->xmlChildrenNode, 1)).trimmed();
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "genres")) {
				QStringList sl = MvdMovieData_P::extractStringList(resultNode, doc, "genre");
				genres = sl;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "imdb-id")) {
				QString s = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, resultNode->xmlChildrenNode, 1)).trimmed();
				QRegExp rx(MvdCore::parameter("mvdcore/imdb-id-regexp").toString());
				if (rx.exactMatch(s))
					imdbId = s;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "languages")) {
				QStringList sl = MvdMovieData_P::extractStringList(resultNode, doc, "language");
				languages = sl;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "notes")) {
				notes = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, resultNode->xmlChildrenNode, 1)).trimmed();
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "original-title")) {
				originalTitle = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, resultNode->xmlChildrenNode, 1)).trimmed();
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "plot")) {
				plot = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, resultNode->xmlChildrenNode, 1)).trimmed();
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "poster")) {
				posterPath = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, resultNode->xmlChildrenNode, 1)).trimmed();
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "producers")) {
				QList<PersonData> pd = MvdMovieData_P::extractPersonData(resultNode, doc);
				producers = pd;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "production-year")) {
				QString s = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, resultNode->xmlChildrenNode, 1)).trimmed();
				if (MvdCore::isValidYear(s))
					productionYear = s;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "rating")) {
				QString s = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, resultNode->xmlChildrenNode, 1)).trimmed();
				xmlChar* attr = xmlGetProp(resultNode, (const xmlChar*) "maximum");
				int mvdMaximum = MvdCore::parameter("mvdcore/max-rating").toInt();
				int xmlMaximum = mvdMaximum;
				bool ok;
				if (attr) {
					int n = QString::fromLatin1((const char*)attr).toInt(&ok);
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
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "release-year")) {
				QString s = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, resultNode->xmlChildrenNode, 1)).trimmed();
				if (MvdCore::isValidYear(s))
					releaseYear = s;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "running-time")) {
				QString s = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, resultNode->xmlChildrenNode, 1)).trimmed();
				bool ok;
				int n = s.toInt(&ok);
				if (ok && n <= MvdCore::parameter("mvdcore/max-running-time").toInt())
					runningTime = n;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "special-contents")) {
				QStringList sl = MvdMovieData_P::extractStringList(resultNode, doc, "item");
				specialContents = sl;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "storage-id")) {
				storageId = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, resultNode->xmlChildrenNode, 1)).trimmed();
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "tags")) {
				QStringList sl = MvdMovieData_P::extractStringList(resultNode, doc, "tag");
				tags = sl;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "title")) {
				title = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, resultNode->xmlChildrenNode, 1)).trimmed();
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "urls")) {
				QList<UrlData> pd = MvdMovieData_P::extractUrlData(resultNode, doc);
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
bool MvdMovieData::writeToXmlFile(const QString& path, Options options) const
{
	Q_UNUSED(options);

	QFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		eLog() << "MvdMovieData: Failed to open " << path << " for writing (" << file.errorString() << ").";
		return false;
	}

	MvdXmlWriter writer(&file);
	MvdMovieData_P::writeToXml(&writer, *this);
	return true;
}

//! Writes this movie data object as a movida-movie-data XML structure to a file.
void MvdMovieData::writeToXmlString(QString* string, Options options) const
{
	Q_UNUSED(options);

	if (!string)
		return;

	MvdXmlWriter writer(string);
	MvdMovieData_P::writeToXml(&writer, *this);
}
