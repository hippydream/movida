/**************************************************************************
** Filename: templatemanager.h
**
** Copyright (C) 2007 Angius Fabrizio. All rights reserved.
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

#ifndef MVD_TEMPLATEMANAGER_H
#define MVD_TEMPLATEMANAGER_H

class MvdTemplateManager_P;

#include "global.h"
#include "movie.h"
#include "moviedata.h"
#include "moviecollection.h"

class MVD_EXPORT MvdTemplateManager
{
public:
	static MvdTemplateManager& instance();
	virtual ~MvdTemplateManager();

	QStringList movieTemplates() const;

	QString movieToXml(const MvdMovie& movie, const MvdMovieCollection& collection);
	QString movieToHtml(const MvdMovie& movie, const MvdMovieCollection& collection, 
		const QString& templateName = QString());

	QString movieDataToHtml(const MvdMovieData& movieData, const QString& templateName = QString());
	QString movieDataFileToHtml(const QString& movieDataFile, const QString& templateName = QString());
	QString movieDataStringToHtml(const QString& movieDataString, const QString& templateName = QString());

private:
	MvdTemplateManager();
	static MvdTemplateManager* mInstance;
	MvdTemplateManager_P* d;
};

namespace Movida
{
	MVD_EXPORT extern MvdTemplateManager& tmanager();
}

#endif // MVD_TEMPLATEMANAGER_H
