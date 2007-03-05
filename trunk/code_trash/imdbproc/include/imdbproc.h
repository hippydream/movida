/****************************************************************************
** Filename: imdbproc.h
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** imdbproc main window.
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

#include "ui_imdbproc.h"

#include <QMainWindow>
#include <QTextStream>

class ImdbProc : public QMainWindow, private Ui::ImdbProcMain
{
	Q_OBJECT

public:
	ImdbProc();	

private slots:
	void updateUi();
	void browseInput();
	void browseOutput();
	void process();

private:
	QString mLastIn;
	QString mLastOut;
	int mItemCount;

	void writeItem(QTextStream& out, const QString& item);
};
