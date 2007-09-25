/**************************************************************************
** Filename: templatemanager.cpp
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

#include "templatemanager.h"
#include "shareddata.h"
#include "xsltproc.h"
#include "pathresolver.h"
#include <QDir>

using namespace Movida;


/*!
	\class MvdTemplateManager templatemanager.h
	\ingroup MvdCore Singletons

	<b>Movida::tmanager()</b> can be used as a convenience method to access the singleton.

	\brief Handles XSL templates and related transformations.
*/


/************************************************************************
MvdTemplateManager_P
*************************************************************************/

//! \internal
class MvdTemplateManager_P
{
public:
	MvdTemplateManager_P();
};

//! \internal
MvdTemplateManager_P::MvdTemplateManager_P()
{
}


/************************************************************************
MvdTemplateManager
*************************************************************************/

//! \internal
MvdTemplateManager* MvdTemplateManager::mInstance = 0;

//! \internal Private constructor.
MvdTemplateManager::MvdTemplateManager()
: d(new MvdTemplateManager_P)
{
}

/*!
	Returns the unique application instance.
*/
MvdTemplateManager& MvdTemplateManager::instance()
{
	if (mInstance == 0)
		mInstance = new MvdTemplateManager();

	return *mInstance;
}

//! Destructor.
MvdTemplateManager::~MvdTemplateManager()
{
	if (this == mInstance)
		delete d;
	else delete mInstance;
}

/*!
	Returns the name of the available movie templates.
	\todo Templates will have an xml descriptor with name, author, etc and a thumbnail!
*/
QStringList MvdTemplateManager::movieTemplates() const
{
	QDir dir(":/templates/movie");
	QFileInfoList files = dir.entryInfoList();

	QStringList names;

	for (int i = 0; i < files.size(); ++i)
	{
		if (!files.at(i).isDir())
			continue;
		names << files.at(i).fileName();
	}

	dir = QDir(paths().resourcesDir().append("Templates/Movie"));
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
	if (!movie.isValid())
		return QString();

	QString xml = "<movie>\n";

	QString s = movie.title();
	if (!s.isEmpty())
		xml.append(QString("\t<title>%1</title>\n").arg(s));

	s = movie.originalTitle();
	if (!s.isEmpty())
		xml.append(QString("\t<original-title>%1</original-title>\n").arg(s));

	s = movie.releaseYear();
	if (!s.isEmpty())
		xml.append(QString("\t<release-year>%1</release-year>\n").arg(s));

	s = movie.productionYear();
	if (!s.isEmpty())
		xml.append(QString("\t<production-year>%1</production-year>\n").arg(s));

	s = movie.edition();
	if (!s.isEmpty())
		xml.append(QString("\t<edition>%1</edition>\n").arg(s));

	s = movie.imdbId();
	if (!s.isEmpty())
		xml.append(QString("\t<imdb-id>%1</imdb-id>\n").arg(s));

	s = movie.plot();
	if (!s.isEmpty())
		xml.append(QString("\t<plot>\n\t\t<![CDATA[%1]]>\n\t</plot>\n").arg(s));

	s = movie.notes();
	if (!s.isEmpty())
		xml.append(QString("\t<notes>\n\t\t<![CDATA[%1]]>\n\t</notes>\n").arg(s));

	s = movie.storageId();
	if (!s.isEmpty())
		xml.append(QString("\t<storage-id>%1</storage-id>\n").arg(s));

	short sh = movie.rating();
	if (sh != 0)
		xml.append(QString("\t<rating>%1</rating>\n").arg(sh));

	sh = movie.runningTime();
	if (sh != 0)
		xml.append(QString("\t<running-time>%1</running-time>\n").arg(sh));

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
			const MvdSdItem& sd = collection.smd().item(id);
			if (!sd.value.isEmpty())
				xml.append(QString("\t\t<item>%1</item>\n").arg(sd.value));
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
			const MvdSdItem& sd = collection.smd().item(id);
			if (!sd.value.isEmpty())
				xml.append(QString("\t\t<item>%1</item>\n").arg(sd.value));
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
			const MvdSdItem& sd = collection.smd().item(id);
			if (!sd.value.isEmpty())
				xml.append(QString("\t\t<item>%1</item>\n").arg(sd.value));
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
			const MvdSdItem& sd = collection.smd().item(id);
			if (!sd.value.isEmpty())
				xml.append(QString("\t\t<item>%1</item>\n").arg(sd.value));
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
			const MvdSdItem& sd = collection.smd().item(id);
			if (!sd.value.isEmpty())
			{
				xml.append(QString("\t\t<name>%1</name>\n").arg(sd.value));
				xml.append(QString("\t\t<imdb-id>%1</imdb-id>\n").arg(sd.id));
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
			const MvdSdItem& sd = collection.smd().item(id);
			if (!sd.value.isEmpty())
			{
				xml.append(QString("\t\t<name>%1</name>\n").arg(sd.value));
				xml.append(QString("\t\t<imdb-id>%1</imdb-id>\n").arg(sd.id));
			}
		}
		xml.append("\t</producers>\n");
	}

	QHash<mvdid,QStringList> idRoleList = movie.actors();
	if (!idRoleList.isEmpty())
	{
		xml.append("\t<cast>\n");
		for (QHash<mvdid,QStringList>::ConstIterator it = idRoleList.constBegin();
			it != idRoleList.constEnd(); ++it)
		{
			mvdid id = it.key();
			QStringList roles = it.value();
			const MvdSdItem& sd = collection.smd().item(id);
			if (!sd.value.isEmpty())
			{
				xml.append(QString("\t\t<name>%1</name>\n").arg(sd.value));
				xml.append(QString("\t\t<imdb-id>%1</imdb-id>\n").arg(sd.id));
				if (!roles.isEmpty())
				{
					xml.append("\t\t\t<roles>\n");
					for (int i = 0; i < roles.size(); ++i)
						if (!roles.at(i).isEmpty())
							xml.append(QString("\t\t\t\t<item>%1</item>\n").arg(roles.at(i)));
					xml.append("\t\t\t</roles>\n");
				}
			}
		}
		xml.append("\t</cast>\n");
	}

	idRoleList = movie.crewMembers();
	if (!idRoleList.isEmpty())
	{
		xml.append("\t<crew>\n");
		for (QHash<mvdid,QStringList>::ConstIterator it = idRoleList.constBegin();
			it != idRoleList.constEnd(); ++it)
		{
			mvdid id = it.key();
			QStringList roles = it.value();
			const MvdSdItem& sd = collection.smd().item(id);
			if (!sd.value.isEmpty())
			{
				xml.append(QString("\t\t<name>%1</name>\n").arg(sd.value));
				xml.append(QString("\t\t<imdb-id>%1</imdb-id>\n").arg(sd.id));
				if (!roles.isEmpty())
				{
					xml.append("\t\t\t<roles>\n");
					for (int i = 0; i < roles.size(); ++i)
						if (!roles.at(i).isEmpty())
							xml.append(QString("\t\t\t\t<item>%1</item>\n").arg(roles.at(i)));
					xml.append("\t\t\t</roles>\n");
				}
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
			xml.append(QString("\t\t\t<url>%1</url>\n").arg(url.url));
				if (!url.description.isEmpty())
					xml.append(QString("\t\t\t<description>%1</description>\n").arg(url.description));
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

	xml.append("</movie>");

	return xml;
}

//! Uses the default template if \p templateName is empty.
QString MvdTemplateManager::movieToHtml(const MvdMovie& movie, 
	const MvdMovieCollection& collection, const QString& templateName)
{
	if (!movie.isValid())
		return QString();

	QString path = QString(":/Templates/Movie/Default/").append(templateName);
	if (!QFile::exists(path))
		path = paths().resourcesDir().append("Templates/Movie/").append(templateName);

	MvdXsltProc xsl(path);
	return xsl.processText(movieToXml(movie, collection));
}

//! Convenience method to access the MvdTemplateManager singleton.
MvdTemplateManager& Movida::tmanager()
{
	return MvdTemplateManager::instance();
}
