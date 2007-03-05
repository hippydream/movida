/****************************************************************************
** Filename: main.cpp
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** Just a simple main routine.
** Call to init() is made after the call to show(), so we are
** sure that all the widgets have been initialized.
**
** Copyright (C) 2006 Angius Fabrizio. All rights reserved.
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
**********************************************************************/

#include "core.h"
#include <QApplication>
#include <Windows.h>

/*!
	Main routine. Registers a crash handler and starts Movida.
*/
int main( int argc, char ** argv )
{
	QApplication a(argc, argv);


	MovidaCore::initStatus();

	return a.exec();
}
