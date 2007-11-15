/**************************************************************************
** Filename: crash_main.cpp
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

#ifndef Q_OS_WIN
#include "crash_main_other.cpp"
#else
#include "crash_main_win.cpp"
#endif

#define MVDCH_CAPTION "Movida crash handler"

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	if (!MovidaCrash::isParentOfMovida()) {
		QMessageBox::critical(0, MVDCH_CAPTION, QCoreApplication::translate("", "Sorry, but this application can only be executed by movida."));
		return EXIT_FAILURE;
	}

	QMessageBox::critical(0, MVDCH_CAPTION, QCoreApplication::translate("", "Sorry, but movida crashed :-(\n\nActually, you should not wonder about it, as movida is still under heavy development.\nFeel free to contact the authors or wait for another version."));
}
