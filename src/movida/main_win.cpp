/**************************************************************************
** Filename: main_win.cpp
** Revision: 3
**
** Copyright (C) 2009 Angius Fabrizio. All rights reserved.
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

#include <QtCore/QtGlobal>
#ifndef Q_WS_WIN
#error "This file compiles on the Windows platform only."
#endif

#include "application.h"
#include "mainwindow.h"

#include "mvdcore/global.h"
#include "mvdcore/logger.h"
#include "mvdcore/pathresolver.h"

#include <QtCore/QProcess>

#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <exception>
#include <windows.h>

namespace Movida {
int runApp(MvdApplication &app);

// Windows exception handling
static LONG exceptionFilter(DWORD exceptionCode);
static QString exceptionDescription(DWORD exceptionCode);
static void defaultCrashHandler(DWORD exceptionCode);
}

// Console option arguments declared in application.cpp
extern const char MVD_ARG_CONSOLE[];
extern const char MVD_ARG_CONSOLE_SHORT[];

using namespace std;
using namespace Movida;

int Movida::runApp(MvdApplication &app)
{
    int appRetVal;

#ifndef _DEBUG
    __try
    {
#endif
    app.parseCommandLine();
    if (app.usingGui()) {
        appRetVal = app.init();
        if (appRetVal == MVD_EXIT_SUCCESS)
            appRetVal = app.exec();
    }
#ifndef _DEBUG
}

__except(exceptionFilter(GetExceptionCode()))
{
    defaultCrashHandler(GetExceptionCode());
}
#endif

    return appRetVal;
}

LONG Movida::exceptionFilter(DWORD exceptionCode)
{
    LONG result;

    switch (exceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_DATATYPE_MISALIGNMENT:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        case EXCEPTION_FLT_DENORMAL_OPERAND:
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        case EXCEPTION_FLT_INEXACT_RESULT:
        case EXCEPTION_FLT_INVALID_OPERATION:
        case EXCEPTION_FLT_OVERFLOW:
        case EXCEPTION_FLT_STACK_CHECK:
        case EXCEPTION_FLT_UNDERFLOW:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_INT_OVERFLOW:
        case EXCEPTION_INVALID_HANDLE:
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        case EXCEPTION_STACK_OVERFLOW:
            result = EXCEPTION_EXECUTE_HANDLER;
            break;

        case EXCEPTION_BREAKPOINT:
        case EXCEPTION_SINGLE_STEP:
            result = EXCEPTION_CONTINUE_EXECUTION;
            break;

        default:
            result = EXCEPTION_EXECUTE_HANDLER;
            break;
    }
    return result;
}

static QString Movida::exceptionDescription(DWORD exceptionCode)
{
    QString description;

    if (exceptionCode == EXCEPTION_ACCESS_VIOLATION)
        description = "EXCEPTION_ACCESS_VIOLATION";
    else if (exceptionCode == EXCEPTION_DATATYPE_MISALIGNMENT)
        description = "EXCEPTION_DATATYPE_MISALIGNMENT";
    else if (exceptionCode == EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
        description = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    else if (exceptionCode == EXCEPTION_FLT_DENORMAL_OPERAND)
        description = "EXCEPTION_FLT_DENORMAL_OPERAND";
    else if (exceptionCode == EXCEPTION_FLT_DIVIDE_BY_ZERO)
        description = "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    else if (exceptionCode == EXCEPTION_FLT_INEXACT_RESULT)
        description = "EXCEPTION_FLT_INEXACT_RESULT";
    else if (exceptionCode == EXCEPTION_FLT_INVALID_OPERATION)
        description = "EXCEPTION_FLT_INVALID_OPERATION";
    else if (exceptionCode == EXCEPTION_FLT_OVERFLOW)
        description = "EXCEPTION_FLT_OVERFLOW";
    else if (exceptionCode == EXCEPTION_FLT_STACK_CHECK)
        description = "EXCEPTION_FLT_STACK_CHECK";
    else if (exceptionCode == EXCEPTION_FLT_UNDERFLOW)
        description = "EXCEPTION_FLT_UNDERFLOW";
    else if (exceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION)
        description = "EXCEPTION_ILLEGAL_INSTRUCTION";
    else if (exceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO)
        description = "EXCEPTION_INT_DIVIDE_BY_ZERO";
    else if (exceptionCode == EXCEPTION_INT_OVERFLOW)
        description = "EXCEPTION_INT_OVERFLOW";
    else if (exceptionCode == EXCEPTION_INVALID_HANDLE)
        description = "EXCEPTION_INVALID_HANDLE";
    else if (exceptionCode == EXCEPTION_NONCONTINUABLE_EXCEPTION)
        description = "EXCEPTION_NONCONTINUABLE_EXCEPTION";
    else if (exceptionCode == EXCEPTION_STACK_OVERFLOW)
        description = "EXCEPTION_STACK_OVERFLOW";
    else
        description = "UNKNOWN EXCEPTION";
    return description;
}

void Movida::defaultCrashHandler(DWORD exceptionCode)
{
    static int crashRecursionCounter = 0;

    crashRecursionCounter++;
    if (crashRecursionCounter < 2) {
        crashRecursionCounter++;
        QString expDesc = exceptionDescription(exceptionCode);
        eLog() << "movida crashed due to the following exception: " << expDesc;
        if (MvdApplication::UseGui) {
            Movida::MainWindow->cleanUp();
        }

        QProcess::execute(QCoreApplication::applicationDirPath().append("\\MvdCrash.exe"));
    }
    ExitProcess(255);
}
