/**************************************************************************
** Filename: guiglobal.cpp
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
		list << SeenAttribute;
		list << SpecialAttribute;
		list << LoanedAttribute;
		break;
	case MainAttributeFilter:
		list << TitleAttribute;
		list << OriginalTitleAttribute;
		list << ProductionYearAttribute;
		list << ReleaseYearAttribute;
		list << RunningTimeAttribute;
		list << StorageIdAttribute;
		list << RatingAttribute;
		break;
	case SmartViewAttributeFilter:
		list << TitleAttribute;
		list << RunningTimeAttribute;
		list << ReleaseYearAttribute;
		list << DirectorsAttribute;
		list << CastAttribute;
		list << ProducersAttribute;
		list << GenresAttribute;
		list << TagsAttribute;
		list << RatingAttribute;
		break;
	default: ;
	}
	return list;
}

QString Movida::movieAttributeString(MovieAttribute attribute, AttributeContext context)
{
	switch (attribute)
	{
	case TitleAttribute:
		if (context == SmartViewContext)
			return QString();
		return QCoreApplication::translate("MovieAttribute", "Title", "No special context");

	case OriginalTitleAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Original title: ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Original title", "No special context");
		
	case ProductionYearAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Produced in ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Production year", "No special context");

	case ReleaseYearAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Released in ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Release year", "No special context");

	case ProducersAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Produced by ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Producers", "No special context");

	case DirectorsAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Directed by ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Directors", "No special context");

	case CastAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Starring ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Cast", "No special context");
	
	case CrewAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Crew members: ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Crew", "No special context");
	
	case RunningTimeAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Running time: ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Running time", "No special context");
	
	case StorageIdAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Storage ID: ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Storage ID", "No special context");
	
	case GenresAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Genres: ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Genres", "No special context");
	
	case LanguagesAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Languages: ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Languages", "No special context");
	
	case CountriesAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Countries: ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Countries", "No special context");
	
	case TagsAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Tags: ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Tags", "No special context");
	
	case ColorModeAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Color mode: ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Color", "No special context");
	
	case ImdbIdAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "IMDb ID: ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "IMDb ID", "No special context");
	
	case RatingAttribute: 
		if (context == SmartViewContext)
			return QCoreApplication::translate("MovieAttribute", "Rating: ", "Smart view context");
		return QCoreApplication::translate("MovieAttribute", "Rating", "No special context");
	
	case SeenAttribute: return QCoreApplication::translate("MovieAttribute", "Seen", "No special context");
	case SpecialAttribute: return QCoreApplication::translate("MovieAttribute", "Special", "No special context");
	case LoanedAttribute: return QCoreApplication::translate("MovieAttribute", "Loaned", "No special context");
	
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
		if (filter != SDEditorAttributeFilter && (role & Movida::ActorRole || role & Movida::CrewMemberRole))
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
