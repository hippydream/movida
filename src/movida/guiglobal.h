/**************************************************************************
** Filename: guiglobal.h
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

#ifndef MVD_GUIGLOBAL_H
#define MVD_GUIGLOBAL_H

#include "mvdcore/global.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QtGlobal>
#include <QtGui/QIcon>

#define MVD_WINDOW_ICON setWindowIcon(QIcon(":/images/logo.svgz"));

class QKeySequence;
class QShortcut;

namespace Movida {
//! Additional item view roles
enum ViewRole {
    IdRole = Qt::UserRole + 1,
    NewItemRole,
    PlaceholderRole,
    TextColorBackupRole,
    FontBackupRole,
    ValidationRole,
    MovieAttributeRole,
    SmartViewDisplayRole,
    MoviePosterRole,
    UniqueDisplayRole,
    SortRole,
    FilterRole
};

enum ItemValidator {
    NoValidator = 0,
    UndoEmptyValitator,
    RegExpValidator
};

//! Item view movie attributes (i.e. used as column identifiers)
enum MovieAttribute {
    TitleAttribute,
    OriginalTitleAttribute,
    YearAttribute,
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
    DateImportedAttribute,

    InvalidMovieAttribute     // Reserved
};
typedef QList<MovieAttribute> MovieAttributeList;

//! Item view shared data attributes (i.e. used as column identifiers)
enum SharedDataAttribute {
    NameSDA, RolesSDA, GenreSDA, CountrySDA, LanguageSDA, TagSDA, ImdbIdSDA,
    InvalidSDA     // Reserved
};
typedef QList<SharedDataAttribute> SharedDataAttributeList;

enum AttributeFilter {
    NoAttributeFilter,
    MainAttributeFilter,
    SmartViewAttributeFilter,
    SDEditorAttributeFilter
};

enum AttributeContext {
    NoAttributeContext,
    SmartViewContext,
    SharedDataEditorContext
};

QList<MovieAttribute> movieAttributes(AttributeFilter filter = NoAttributeFilter);
QString movieAttributeString(MovieAttribute attribute, AttributeContext context = NoAttributeContext);

QList<SharedDataAttribute> sharedDataAttributes(Movida::DataRole role, AttributeFilter filter = NoAttributeFilter);
QString sharedDataAttributeString(SharedDataAttribute attribute);

enum FilterFunction {
    InvalidFilterFunction = 0,

    // For internal use (IDs are not accessible to users):
    MovieIdFilter, SharedDataIdFilter,

    // Also suited for users:
    MarkAsSeenFilter, MarkAsLoanedFilter, MarkAsSpecialFilter,
    RatingFilter, RunningTimeFilter
};

QString filterFunctionName(FilterFunction ff);
QRegExp filterFunctionRegExp(FilterFunction ff);
FilterFunction filterFunction(const QString &name);

class ShortcutMonitor : public QObject
{
    Q_OBJECT

public slots:
    void ambiguousActivation();
};

QShortcut *createShortcut(const QKeySequence &ks, QWidget *parent, const char *member, QObject *receiver = 0);

} // Movida namespace


#endif // MVD_GUIGLOBAL_H
