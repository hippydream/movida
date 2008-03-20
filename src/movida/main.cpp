/**************************************************************************
** Filename: main.cpp
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

#include <QtGlobal>

#ifndef Q_OS_WIN
#include "main_other.cpp"
#else
#include "main_win.cpp"
#endif

#include "mvdsvgz/iconplugin.h"

Q_IMPORT_PLUGIN(MvdSvgzIconEnginePlugin)

int main(int argc, char** argv)
{
	int result = 0;

#ifdef Q_OS_WIN
	MvdApplication a(argc, argv);
	result = runApp(a);
#else
	result = runApp(argc, argv);
#endif

	return result;
}
