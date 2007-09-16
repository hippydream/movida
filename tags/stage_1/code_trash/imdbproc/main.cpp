/****************************************************************************
** Filename: main.cpp
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** Just a simple main routine.
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

#include "imdbproc.h"

#include <QtGlobal>
#include <QString>
#include <QApplication>
#include <QIcon>
#include <QMessageBox>

/*!
	Main routine.
*/
int main( int argc, char ** argv )
{
    QApplication a(argc, argv);

	ImdbProc w;
	w.show();

	return a.exec();
}
