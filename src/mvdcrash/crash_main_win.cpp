/**************************************************************************
** Filename: crash_main_win.cpp
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

#include <QtGlobal>
#ifndef Q_WS_WIN
#error "This file compiles on the Windows platform only."
#endif

#include <QMessageBox>
#include <QApplication>
#include <QFileInfo>
#include <qt_windows.h>
#include <qlibrary.h>
#include <Psapi.h>
#include <tlhelp32.h>

#pragma comment(lib, "Psapi.lib")

namespace MovidaCrash {
bool isParentOfMovida();
}

bool MovidaCrash::isParentOfMovida()
{
    /* Unfortunately, Win32 does not have an API that gives you the PID of the parent
       process, and you have to create it yourself. We will use functions found in
       Psapi.dll and in the Kernel32.DLL, that will be loaded programmatically. */

    QLibrary library(QLatin1String("Kernel32"));

    typedef HANDLE (WINAPI * F_GetSnapshot)(DWORD, DWORD);
    F_GetSnapshot GetSnapshot = (F_GetSnapshot)library.resolve("CreateToolhelp32Snapshot");
    if (!GetSnapshot)
        return false;

    typedef BOOL (WINAPI * F_FirstProcess)(HANDLE, LPPROCESSENTRY32);
    F_FirstProcess FirstProcess = (F_FirstProcess)library.resolve("Process32First");
    if (!FirstProcess)
        return false;

    typedef BOOL (WINAPI * F_NextProcess)(HANDLE, LPPROCESSENTRY32);
    F_NextProcess NextProcess = (F_NextProcess)library.resolve("Process32Next");
    if (!NextProcess)
        return false;

    HANDLE snapShot;
    if ((snapShot = GetSnapshot(TH32CS_SNAPPROCESS, 0)) == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32 procEntry;

    // Get the first process' information.
    memset((LPVOID)&procEntry, 0, sizeof(PROCESSENTRY32));
    procEntry.dwSize = sizeof(PROCESSENTRY32);
    BOOL cont = FirstProcess(snapShot, &procEntry);
    DWORD parentPid = 0;

    // While there are processes, keep looping.
    DWORD currentPid = GetCurrentProcessId();
    while (cont) {
        if (currentPid == procEntry.th32ProcessID)
            parentPid = procEntry.th32ParentProcessID;

        procEntry.dwSize = sizeof(PROCESSENTRY32);
        cont = !parentPid && NextProcess(snapShot, &procEntry);
    }

    if (!parentPid)
        return false;

    HANDLE processHandle = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE, procEntry.th32ParentProcessID);

    if (!processHandle)
        return false;

    HMODULE module;
    char fileName[MAX_PATH];
    DWORD size = 0;

    if (!EnumProcessModules(processHandle, &module, sizeof(module), &size))
        return false;

    if (!GetModuleFileNameEx(processHandle, module, (LPWSTR)fileName, sizeof(fileName)))
        return false;

    QString parentExePath = QString::fromUtf16((ushort *)fileName);
    QFileInfo info(parentExePath);

    return info.fileName().toLower() == "movida.exe";
}
