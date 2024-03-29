/**************************************************************************
** Filename: blueglobal.h
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

#ifndef MPI_BLUEGLOBAL_H
#define MPI_BLUEGLOBAL_H

#include "mvdcore/global.h"

#include <QtCore/QtGlobal>

#ifndef MPI_EXPORT_BLUE
# ifdef Q_OS_WIN
#  if defined (MPI_BUILD_BLUE_DLL)
#   define MPI_EXPORT_BLUE __declspec(dllexport)
#  else
#   define MPI_EXPORT_BLUE __declspec(dllimport)
#  endif // MPI_BUILD_BLUE_DLL
# else // Q_OS_WIN
#  define MPI_EXPORT_BLUE
# endif
#endif // MPI_EXPORT_BLUE

#endif // MPI_BLUEGLOBAL_H
