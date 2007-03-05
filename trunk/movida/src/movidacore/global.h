/**************************************************************************
** Filename: global.h
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

#ifndef MVD_GLOBAL_H
#define MVD_GLOBAL_H

#include <QtGlobal>
#include <QString>

#  ifndef MOVIDA_EXPORT
#  ifdef Q_OS_WIN
#    define MOVIDA_EXPORT __declspec(dllexport)
#  elif defined(QT_VISIBILITY_AVAILABLE)
#    define MOVIDA_EXPORT __attribute__((visibility("default")))
#  endif
#  endif
#  ifndef MOVIDA_EXPORT
#    define MOVIDA_EXPORT
#  endif

#define _APP_NAME_ "Movida"
#define _COMPANY_ "BlueSoft"
#define _CAPTION_ "Movida"

#define _ERROR_INIT_ -1

#ifdef Q_WS_WIN

#define LOG_FILENAME "Movida.log"
#define PREF_FILENAME "Settings.xml"
#define APP_DIR "Movida"
#define COMPANY_DIR "BlueSoft"

#else

#define LOG_FILENAME "movida.log"
#define PREF_FILENAME "settings.xml"
#define APP_DIR "movida"
#define COMPANY_DIR ".bluesoft"

#endif

#if defined(Q_WS_WIN)
# define MVD_LINEBREAK "\r\n"
#elif defined(Q_WS_MAC)
# define MVD_LINEBREAK "\r"
#else
# define MVD_LINEBREAK "\n"
#endif

// movida version numbers
static const int VER_MAJOR = 0;
static const int VER_MINOR = 3;
// lowest 8 bits are for minor version, next higher 8 bits for major
// version 1.2 would be 0x00000102
static const int VERSION = ((VER_MAJOR << 8) & 0xFF00) | VER_MINOR;

// Data IDs
typedef quint32 mvdid;

typedef mvdid smdid;
typedef mvdid movieid;

// Movida namespace
namespace Movida
{
	// ENUMS
	enum SmdDataType
	{
		NoData = 0x0,

		PersonData,
		SimpleData,
		UrlData
	};

	enum SmdDataRole
	{
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
		UrlRole = 0x100
	};
};

#endif // MVD_GLOBAL_H
