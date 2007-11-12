/**************************************************************************
** Filename: guiglobal.cpp
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

#include "guiglobal.h"

using namespace Movida;

//! Returns a list of movie attributes, ordered by relevance.
QList<MovieAttribute> Movida::movieAttributes(AttributeFilter filter)
{
	QList<MovieAttribute> list;
	switch (filter)
	{
	case NoAttributeFilter:
		list << TitleAttribute;
		list << OriginalTitleAttribute;
		list << ProductionYearAttribute;
		list << ReleaseYearAttribute;
		list << RunningTimeAttribute;
		list << RatingAttribute;
		list << StorageIdAttribute;
		list << ProducersAttribute;
		list << DirectorsAttribute;
		list << CastAttribute;
		list << CrewAttribute;
		list << GenresAttribute;
		list << CountriesAttribute;
		list << LanguagesAttribute;
		list << TagsAttribute;
		list << ColorModeAttribute;
		list << ImdbIdAttribute;
		break;
	case MainAttributeFilter:
		list << TitleAttribute;
		list << OriginalTitleAttribute;
		list << ProductionYearAttribute;
		list << ReleaseYearAttribute;
		list << RunningTimeAttribute;
		list << StorageIdAttribute;
		list << RatingAttribute;
	case SmartViewAttributeFilter:
		list << TitleAttribute;
		list << ReleaseYearAttribute;
		list << DirectorsAttribute;
		list << RunningTimeAttribute;
		list << RatingAttribute;
		break;
	default: ;
	}
	return list;
}

QString Movida::movieAttributeString(MovieAttribute attribute, AttributeContext context)
{
	const char* comment = context == NoAttributeContext ? "Default context" : "Smart view context";

	switch (attribute)
	{
	case TitleAttribute: return QCoreApplication::translate("MovieAttribute", "Title", comment);
	case OriginalTitleAttribute: return QCoreApplication::translate("MovieAttribute", "Original title", comment);
	case ProductionYearAttribute: return QCoreApplication::translate("MovieAttribute", "Production year", comment);
	case ReleaseYearAttribute: return QCoreApplication::translate("MovieAttribute", "Release year", comment);
	case ProducersAttribute: return QCoreApplication::translate("MovieAttribute", "Producers", comment);
	case DirectorsAttribute: return QCoreApplication::translate("MovieAttribute", "Directors", comment);
	case CastAttribute: return QCoreApplication::translate("MovieAttribute", "Cast", comment);
	case CrewAttribute: return QCoreApplication::translate("MovieAttribute", "Crew", comment);
	case RunningTimeAttribute: return QCoreApplication::translate("MovieAttribute", "Running time", comment);
	case StorageIdAttribute: return QCoreApplication::translate("MovieAttribute", "Storage ID", comment);
	case GenresAttribute: return QCoreApplication::translate("MovieAttribute", "Genres", comment);
	case LanguagesAttribute: return QCoreApplication::translate("MovieAttribute", "Languages", comment);
	case CountriesAttribute: return QCoreApplication::translate("MovieAttribute", "Countries", comment);
	case TagsAttribute: return QCoreApplication::translate("MovieAttribute", "Tags", comment);
	case ColorModeAttribute: return QCoreApplication::translate("MovieAttribute", "Color", comment);
	case ImdbIdAttribute: return QCoreApplication::translate("MovieAttribute", "IMDb ID", comment);
	case RatingAttribute: return QCoreApplication::translate("MovieAttribute", "Rating", comment);
	default: ;
	}
	return QString();
}

QList<SharedDataAttribute> Movida::sharedDataAttributes(Movida::DataRole role, AttributeFilter filter)
{
	QList<SharedDataAttribute> list;
	
	switch (role)
	{
	case Movida::PersonRole:
	case Movida::ActorRole:
	case Movida::DirectorRole:
	case Movida::ProducerRole:
	case Movida::CrewMemberRole:
		list << NameSDA;
		if (role & Movida::ActorRole || role & Movida::CrewMemberRole)
			list << RolesSDA;
		if (filter == NoAttributeFilter)
			list << ImdbIdSDA;
		break;
	case Movida::GenreRole:
		list << GenreSDA;
		break;
	case Movida::CountryRole:
		list << CountrySDA;
		break;
	case Movida::TagRole:
		list << TagSDA;
		break;
	case Movida::LanguageRole:
		list << LanguageSDA;
		break;
	default: ;
	}

	return list;
}

QString Movida::sharedDataAttributeString(SharedDataAttribute attribute)
{
	switch (attribute)
	{
	case NameSDA: return QCoreApplication::translate("SharedDataAttribute", "Name");
	case RolesSDA: return QCoreApplication::translate("SharedDataAttribute", "Role(s)");
	case GenreSDA: return QCoreApplication::translate("SharedDataAttribute", "Genre");
	case CountrySDA: return QCoreApplication::translate("SharedDataAttribute", "Country");
	case LanguageSDA: return QCoreApplication::translate("SharedDataAttribute", "Language");
	case TagSDA: return QCoreApplication::translate("SharedDataAttribute", "Tag");
	case ImdbIdSDA: return QCoreApplication::translate("MovieAttribute", "IMDb ID");
	default: ;
	}
	return QString();
}
