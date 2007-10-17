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
	static QStringList extractStringList(xmlNodePtr parent, xmlDocPtr doc, const char* tag);
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
				QStringList sl = MvdMovieData_P::extractStringList(resultNode, doc, "countries");
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
				QStringList sl = MvdMovieData_P::extractStringList(resultNode, doc, "genres");
				genres = sl;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "imdb-id")) {
				QString s = QString::fromUtf8((const char*)xmlNodeListGetString(
					doc, resultNode->xmlChildrenNode, 1)).trimmed();
				QRegExp rx(MvdCore::parameter("mvdcore/imdb-id-regexp").toString());
				if (rx.exactMatch(s))
					imdbId = s;
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "languages")) {
				QStringList sl = MvdMovieData_P::extractStringList(resultNode, doc, "languages");
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
				int n = s.toInt(&ok);
				if (ok) {
					n = (mvdMaximum * n) / xmlMaximum;
					if (n > mvdMaximum)
						n = mvdMaximum;
					rating = n;
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
			}

			resultNode = resultNode->next;
		}

		if (options.testFlag(StopAtFirstMovie))
			break;
	}

	return true;
}
