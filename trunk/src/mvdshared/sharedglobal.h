/**************************************************************************
** Filename: sharedglobal.h
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

#ifndef MVD_SHAREDGLOBAL_H
#define MVD_SHAREDGLOBAL_H

#include "mvdcore/global.h"

#include <QtCore/QtGlobal>

#ifndef MVD_EXPORT_SHARED
# ifdef Q_OS_WIN
#  if defined (MVD_BUILD_SHARED_DLL)
#   define MVD_EXPORT_SHARED __declspec(dllexport)
#  else
#   define MVD_EXPORT_SHARED __declspec(dllimport)
#  endif // MVD_BUILD_SHARED_DLL
# else // Q_OS_WIN
#  define MVD_EXPORT_SHARED
# endif
#endif // MVD_EXPORT_SHARED

namespace MovidaShared {
enum MessageType {
    InfoMessage = 0,
    WarningMessage,
    ErrorMessage
};
}

#endif // MVD_SHAREDGLOBAL_H
