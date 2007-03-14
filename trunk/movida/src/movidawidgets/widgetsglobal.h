/**************************************************************************
** Filename: widgetsglobal.h
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

#ifndef MVDW_WIDGETSGLOBAL_H
#define MVDW_WIDGETSGLOBAL_H

#include <QtGlobal>

#ifndef MVDW_EXPORT
# ifdef Q_OS_WIN
#  if defined(MVD_BUILD_MOVIDAWIDGETS)
#   define MVDW_EXPORT __declspec(dllexport)
#  else
#   define MVDW_EXPORT __declspec(dllimport)
#  endif // MVD_BUILD_MOVIDAWIDGETS
# else // Q_OS_WIN
#  define MVDW_EXPORT
# endif
#endif // MVDW_EXPORT

#endif // MVDW_WIDGETSGLOBAL_H
