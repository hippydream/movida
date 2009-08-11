/**************************************************************************
** Filename: crash_main_other.cpp
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
#ifdef Q_WS_WIN
#error "This file does not compile on the Windows platform."
#endif

#include <sys/types.h>
#include <unistd.h>
#include <QMessageBox>
#include <QApplication>
#include <QFile>
#include <QFileInfo>

namespace MovidaCrash {
bool isParentOfMovida();
}

bool MovidaCrash::isParentOfMovida()
{
    //! \todo MAC OSX does not (always) use the /proc filesystem!

    QString procFileName = QString("/proc/%1/cmdline").arg(getppid());
    QFile procFile(procFileName);

    if (procFile.open(QIODevice::ReadOnly)) {
        QString line = procFile.readLine();
        QFileInfo info(line);
        return info.fileName().toLower() == "movida";
    }
    return true;
}
