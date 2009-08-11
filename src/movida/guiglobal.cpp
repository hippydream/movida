/**************************************************************************
** Filename: guiglobal.cpp
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

#include "guiglobal.h"

using namespace Movida;

//! Returns a list of movie attributes, ordered by relevance.
QList<MovieAttribute> Movida::movieAttributes(AttributeFilter filter)
{
    QList<MovieAttribute> list;
    switch (filter) {
        case NoAttributeFilter:
            list << TitleAttribute;
            list << OriginalTitleAttribute;
            list << YearAttribute;
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
            list << DateImportedAttribute;
            break;

        case MainAttributeFilter:
            list << TitleAttribute;
            list << OriginalTitleAttribute;
            list << YearAttribute;
            list << RunningTimeAttribute;
            list << StorageIdAttribute;
            list << RatingAttribute;
            list << DateImportedAttribute;
            break;

        case SmartViewAttributeFilter:
            list << TitleAttribute;
            list << RunningTimeAttribute;
            list << YearAttribute;
            list << DirectorsAttribute;
            list << CastAttribute;
            list << ProducersAttribute;
            list << GenresAttribute;
            list << TagsAttribute;
            list << RatingAttribute;
            break;

        default:
            ;
    }
    return list;
}

QString Movida::movieAttributeString(MovieAttribute attribute, AttributeContext context)
{
    switch (attribute) {
        case TitleAttribute:
            if (context == SmartViewContext)
                return QString();
            return QCoreApplication::translate("MovieAttribute", "Title", "No special context");

        case OriginalTitleAttribute:
            if (context == SmartViewContext)
                return QCoreApplication::translate("MovieAttribute", "Original title: ", "Smart view context");
            return QCoreApplication::translate("MovieAttribute", "Original title", "No special context");

        case YearAttribute:
            if (context == SmartViewContext)
                return QCoreApplication::translate("MovieAttribute", "Produced in ", "Smart view context");
            return QCoreApplication::translate("MovieAttribute", "Production year", "No special context");

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

        case SeenAttribute:
            return QCoreApplication::translate("MovieAttribute", "Seen", "No special context");

        case SpecialAttribute:
            return QCoreApplication::translate("MovieAttribute", "Special", "No special context");

        case LoanedAttribute:
            return QCoreApplication::translate("MovieAttribute", "Loaned", "No special context");

        case DateImportedAttribute:
            return QCoreApplication::translate("MovieAttribute", "Date added", "Date Imported");

        default:
            ;
    }
    return QString();
}

QList<SharedDataAttribute> Movida::sharedDataAttributes(Movida::DataRole role, AttributeFilter filter)
{
    QList<SharedDataAttribute> list;

    switch (role) {
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

        default:
            ;
    }

    return list;
}

QString Movida::sharedDataAttributeString(SharedDataAttribute attribute)
{
    switch (attribute) {
        case NameSDA:
            return QCoreApplication::translate("SharedDataAttribute", "Name");

        case RolesSDA:
            return QCoreApplication::translate("SharedDataAttribute", "Role(s)");

        case GenreSDA:
            return QCoreApplication::translate("SharedDataAttribute", "Genre");

        case CountrySDA:
            return QCoreApplication::translate("SharedDataAttribute", "Country");

        case LanguageSDA:
            return QCoreApplication::translate("SharedDataAttribute", "Language");

        case TagSDA:
            return QCoreApplication::translate("SharedDataAttribute", "Tag");

        case ImdbIdSDA:
            return QCoreApplication::translate("MovieAttribute", "IMDb ID");

        default:
            ;
    }
    return QString();
}

//////////////////////////////////////////////////////////////////////////
// MOVIE FILTER FUNCTIONS
//////////////////////////////////////////////////////////////////////////

/*
        Adding new movie filters involves the following steps:
         - Add a value to the FilterFunction enum.
         - Extend filterFunctionName() to return the string to be used in the filter
           widget (only the function name - no parentheses or operators - e.g. "rating").
           This method is basically used when the filter function is added programmatically
           to the filter widget's line edit.
         - Consider extending filterFunctionRegExp() to return a regular expression for
           a quick and basic validation when the user enters the function in the filter widget.
         - Extend filterFunction() to return the FilterFunction matching a function name (sort of
           the inverse of filterFunctionName() -- no hash table is used to ease localization
           of function names).
         - Extend MvdFilterProxyModel::testFunction() or your filter will be - obviously - of no use.

        As mentioned above, function names are all localized. This should apply even for internally
        used functions (such as those involving IDs) as the user will be more likely to understand
        what is going on by looking at the filter widget (at least until another approach will be
        taken to display automatically generated filters - e.g. using D&D or plugins).
 */

//! Returns the (localized) name of a filter function as it is used in a filter query.
QString Movida::filterFunctionName(FilterFunction ff)
{
    switch (ff) {
        case MovieIdFilter:
            return QCoreApplication::translate("Filter function", "movieId", "filter movies by ID");

        case SharedDataIdFilter:
            return QCoreApplication::translate("Filter function", "dataId", "filter shared data by ID");

        case MarkAsSeenFilter:
            return QCoreApplication::translate("Filter function", "seen", "show seen movies");

        case MarkAsLoanedFilter:
            return QCoreApplication::translate("Filter function", "loaned", "show loaned movies");

        case MarkAsSpecialFilter:
            return QCoreApplication::translate("Filter function", "special", "show special movies");

        case RunningTimeFilter:
            return QCoreApplication::translate("Filter function", "length", "filter by running time");

        case RatingFilter:
            return QCoreApplication::translate("Filter function", "rating", "filter by rating");

        default:
            Q_ASSERT_X(false, "Movida::filterFunctionName", "Internal error");
    }

    return QString();
}

//! Returns a regular expression used to validate function parameters.
QRegExp Movida::filterFunctionRegExp(FilterFunction ff)
{
    switch (ff) {
        case MovieIdFilter:
        case SharedDataIdFilter:
            return QRegExp(QLatin1String("[\\s,]*\\d+[\\s,\\d]*"));

        case RunningTimeFilter:
        {
            QString hm("\\d{1,3}\\s*h(?:\\s*\\d+\\s*m?)?"); // 1h 20m OR 1h 20 OR 1h
            QString qm("\\d{1,3}\\s*'(?:\\s*\\d+\\s*(?:'')?)?"); // 1' 10'' OR 1' 10
            QString dm("\\d{1}[\\.:]\\d{1,2}"); // 1.20 or 1:20
            QString mn("\\d{1,3}\\s*m?");  // 120m or 120
            return QRegExp(
                QString("\\s*[<>=]?\\s*(?:(?:%1)|(?:%2)|(?:%3)|(?:%4))\\s*")
                    .arg(hm).arg(qm).arg(dm).arg(mn)
                );
        }

        case RatingFilter:
            return QRegExp(QLatin1String("\\s*[<>=]?\\s*\\d{1}\\s*"));

        default:
            ;
    }

    return QRegExp();
}

//! Returns the filter function associated to the given (localized) name or InvalidFilterFunction.
Movida::FilterFunction Movida::filterFunction(const QString &name)
{
    if (!name.compare(filterFunctionName(Movida::MovieIdFilter), Qt::CaseInsensitive))
        return Movida::MovieIdFilter;
    else if (!name.compare(filterFunctionName(Movida::MarkAsSeenFilter), Qt::CaseInsensitive))
        return Movida::MarkAsSeenFilter;
    else if (!name.compare(filterFunctionName(Movida::MarkAsLoanedFilter), Qt::CaseInsensitive))
        return Movida::MarkAsLoanedFilter;
    else if (!name.compare(filterFunctionName(Movida::MarkAsSpecialFilter), Qt::CaseInsensitive))
        return Movida::MarkAsSpecialFilter;
    else if (!name.compare(filterFunctionName(Movida::RunningTimeFilter), Qt::CaseInsensitive))
        return Movida::RunningTimeFilter;
    else if (!name.compare(filterFunctionName(Movida::RatingFilter), Qt::CaseInsensitive))
        return Movida::RatingFilter;
    else if (!name.compare(filterFunctionName(Movida::SharedDataIdFilter), Qt::CaseInsensitive))
        return Movida::SharedDataIdFilter;

    return Movida::InvalidFilterFunction;
}
