/**************************************************************************
** Filename: global.h
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

#ifndef MVD_GLOBAL_H
#define MVD_GLOBAL_H

#include <QtCore/QCoreApplication>
#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QtGlobal>

#ifndef MVD_EXPORT
# ifdef Q_OS_WIN
#  if defined (MVD_BUILD_CORE_DLL)
#   define MVD_EXPORT __declspec(dllexport)
#  else
#   define MVD_EXPORT __declspec(dllimport)
#  endif // MVD_BUILD_CORE_DLL
# else // Q_OS_WIN
#  define MVD_EXPORT
# endif
#endif // MVD_EXPORT

#define MVD_CAPTION "movida"

#define MVD_EXIT_SUCCESS 0
#define MVD_ERROR_UNKNOWN 1
#define MVD_ERROR_INIT 2

#if defined (Q_WS_WIN)
# define MVD_LINEBREAK "\r\n"
#elif defined (Q_WS_MAC)
# define MVD_LINEBREAK "\r"
#else
# define MVD_LINEBREAK "\n"
#endif

// movida version numbers
static const int MVD_VER_MAJOR = 0;
static const int MVD_VER_MINOR = 3;
// Lowest 8 bits are for minor version, next higher 8 bits for major
// version 1.2 would be 0x00000102
static const int MVD_VERSION = ((MVD_VER_MAJOR << 8) & 0xFF00) | MVD_VER_MINOR;

// Data IDs
typedef quint32 mvdid;

// Invalid Id :)
static const mvdid MvdNull = 0;

// Movida namespace
namespace Movida {
enum DataRole {
    NoRole = 0x000,

    ActorRole = 0x001,
    DirectorRole = 0x002,
    ProducerRole = 0x004,
    CrewMemberRole = 0x008,
    PersonRole = ActorRole | DirectorRole | ProducerRole | CrewMemberRole,

    GenreRole = 0x010,
    CountryRole = 0x020,
    LanguageRole = 0x040,
    TagRole = 0x080,
};

enum Scope {
    UserScope,
    SystemScope
};

enum ColorMode { Color, BlackWhite, UnknownColorMode };
enum Tag { NoTag = 0, SeenTag = 2, LoanedTag = 4, SpecialTag = 8 };
Q_DECLARE_FLAGS(Tags, Tag);

// In movie.cpp
extern QString colorModeToString(ColorMode m);
extern ColorMode colorModeFromString(QString s);
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Movida::Tags)

// libxml string conversion macros
#define MVD_QSTR(s) QString::fromUtf8((const char *)s)
#define MVD_CSTR(s) s.toUtf8().constData()

typedef QHash<QString, QString> MvdAttributeMap;
typedef QPair<QString, QString> MvdAttribute;

#define MVD_ATTR(key, value) MvdAttribute(key, value)
#define MVD_CATTR(ckey, value) MvdAttribute(QLatin1String(key), value)

#endif // MVD_GLOBAL_H
