/**************************************************************************
** Filename: guiglobal.h
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

#ifndef MVD_GUIGLOBAL_H
#define MVD_GUIGLOBAL_H

#include "global.h"

#include <QtGlobal>
#include <QIcon>
#include <QString>
#include <QCoreApplication>

#define MVD_WINDOW_ICON setWindowIcon(QIcon(":/images/misc/logo.png"));

namespace Movida
{
	//! Additional item view roles
	enum ViewRole
	{
		MovieIdRole = Qt::UserRole + 1
	};

	//! Item view movie attributes (i.e. used as column identifiers)
	enum MovieAttribute
	{
		TitleAttribute,
		OriginalTitleAttribute,
		ProductionYearAttribute,
		ReleaseYearAttribute,
		ProducersAttribute,
		DirectorsAttribute,
		CastAttribute,
		CrewAttribute,
		RunningTimeAttribute,
		StorageIdAttribute,
		GenresAttribute,
		CountriesAttribute,
		LanguagesAttribute,
		TagsAttribute,
		ColorModeAttribute,
		ImdbIdAttribute,
		RatingAttribute
	};

	enum MovieAttributeFilter
	{
		AllMovieAttributes,
		DetailedMovieAttributes,
		SimpleMovieAttributes
	};

	QList<MovieAttribute> movieAttributes(MovieAttributeFilter filter = AllMovieAttributes);
	QString movieAttributeString(MovieAttribute attribute);
}

#endif // MVD_GUIGLOBAL_H
