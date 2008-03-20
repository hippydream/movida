/**************************************************************************
** Filename: guiglobal.h
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

#ifndef MVD_GUIGLOBAL_H
#define MVD_GUIGLOBAL_H

#include "mvdcore/global.h"
#include <QCoreApplication>
#include <QIcon>
#include <QString>
#include <QtGlobal>

#define MVD_WINDOW_ICON setWindowIcon(QIcon(":/images/logo.svgz"));

namespace Movida
{
	//! Additional item view roles
	enum ViewRole
	{
		IdRole = Qt::UserRole + 1,
		NewItemRole,
		PlaceholderRole,
		TextColorBackupRole,
		FontBackupRole,
		ValidationRole,
		MovieAttributeRole,
		SmartViewDisplayRole
	};

	enum ItemValidator
	{
		NoValidator = 0,
		UndoEmptyValitator,
		RegExpValidator
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
		RatingAttribute,
		SeenAttribute,
		SpecialAttribute,
		LoanedAttribute,

		InvalidMovieAttribute // Reserved
	};
	typedef QList<MovieAttribute> MovieAttributeList;

	//! Item view shared data attributes (i.e. used as column identifiers)
	enum SharedDataAttribute
	{
		NameSDA, RolesSDA, GenreSDA, CountrySDA, LanguageSDA, TagSDA, ImdbIdSDA,
		InvalidSDA // Reserved
	};
	typedef QList<SharedDataAttribute> SharedDataAttributeList;

	enum AttributeFilter
	{
		NoAttributeFilter,
		MainAttributeFilter,
		SmartViewAttributeFilter,
		SDEditorAttributeFilter
	};

	enum AttributeContext
	{
		NoAttributeContext,
		SmartViewContext,
		SharedDataEditorContext
	};

	QList<MovieAttribute> movieAttributes(AttributeFilter filter = NoAttributeFilter);
	QString movieAttributeString(MovieAttribute attribute, AttributeContext context = NoAttributeContext);

	QList<SharedDataAttribute> sharedDataAttributes(Movida::DataRole role, AttributeFilter filter = NoAttributeFilter);
	QString sharedDataAttributeString(SharedDataAttribute attribute);
}

#endif // MVD_GUIGLOBAL_H
