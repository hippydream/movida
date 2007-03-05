/**************************************************************************
** Filename: main.cpp
** Revision: 1
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

#include "pathresolver.h"
#include "mainwindow.h"
#include "guiglobal.h"
#include "core.h"

#include <QtGlobal>
#include <QString>
#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QStyle>

#include <signal.h>
#include <iostream>

void movidaExit(int sigNum);


/*!
	Main routine. Registers a crash handler and starts Movida.
*/
int main( int argc, char ** argv )
{
#ifndef Q_WS_WIN
	signal (SIGSEGV, movidaExit);
	signal (SIGILL, movidaExit);
	signal (SIGTERM, movidaExit);
	signal (SIGABRT, movidaExit);
#endif

	int errcode = -1;

    QApplication a(argc, argv);

#ifdef Q_WS_WIN
	try {
#endif
		
		if (!MvdCore::initCore())
		{
			QMessageBox::warning(0, 
				QCoreApplication::translate("Main", "Application Error"), 
				QCoreApplication::translate("Main", "Failed to initialize the application.\nPlease see the log file for details."));
			exit(_ERROR_INIT_);
		}

		MvdMainWindow w;
		w.show();

		a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
		errcode = a.exec();

		Movida::paths().removeDirectoryTree(Movida::paths().tempDir());

#ifdef Q_WS_WIN
	}
	catch (...) {
		//! \todo handle exception types
		movidaExit(-1);
	}
#endif
	return errcode;
}

/*!
	Handles abnormal program termination.
*/
void movidaExit(int)
{
	//! \todo popup some message box
	QMessageBox::warning(0, QCoreApplication::translate("Crash Handler", "%1 crash report").arg(_CAPTION_), QCoreApplication::translate("Crash Handler", "Removing temporary files after application crash."));
	Movida::paths().removeDirectoryTree(Movida::paths().tempDir());
	exit(-1);
}
