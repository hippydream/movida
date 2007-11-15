/**************************************************************************
** Filename: main_other.cpp
** Revision: 3
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

#include <QtGlobal>
#ifdef Q_WS_WIN
#error "This file does not compile on the Windows platform."
#endif

#include "global.h"
#include "application.h"
#include "mainwindow.h"
#include "logger.h"
#include "pathresolver.h"
#include <QProcess>
#include <iostream>
#include <signal.h>

namespace Movida {
	int runApp(int argc, char** argv);

	void initCrashHandler();
	static void defaultCrashHandler(int sig);
};

using namespace std;
using namespace Movida;

int Movida::runApp(int argc, char** argv)
{
	MvdApplication app(argc, argv);
	initCrashHandler();
	app.parseCommandLine();
	if (app.usingGui()) {
		int appRetVal = app.init();
		if (appRetVal != MVD_EXIT_SUCCESS)
			return(appRetVal);
		return app.exec();
	}
	return MVD_EXIT_SUCCESS;
}

void Movida::initCrashHandler()
{
	typedef void (*HandlerType)(int);
	HandlerType handler = 0;
	handler = defaultCrashHandler;
	if (!handler)
		handler = SIG_DFL;
	sigset_t mask;
	sigemptyset(&mask);
#ifdef SIGSEGV
	signal (SIGSEGV, handler);
	sigaddset(&mask, SIGSEGV);
#endif
#ifdef SIGFPE
	signal (SIGFPE, handler);
	sigaddset(&mask, SIGFPE);
#endif
#ifdef SIGILL
	signal (SIGILL, handler);
	sigaddset(&mask, SIGILL);
#endif
#ifdef SIGABRT
	signal (SIGABRT, handler);
	sigaddset(&mask, SIGABRT);
#endif
	sigprocmask(SIG_UNBLOCK, &mask, 0);
}

void Movida::defaultCrashHandler(int sig)
{
	static int crashRecursionCounter = 0;
	crashRecursionCounter++;
	signal(SIGALRM, SIG_DFL);
	if (crashRecursionCounter < 2) {
		crashRecursionCounter++;
		eLog() << "movida crashed due to signal " << sig;
		if (MvdApplication::UseGui) {
			Movida::MainWindow->cleanUp();
		}
		alarm(300);
		
		QProcess::execute(QCoreApplication::applicationDirPath().append("/mvdcrash"));
	}
		
	exit(255);
}
