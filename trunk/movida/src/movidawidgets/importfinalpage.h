/**************************************************************************
** Filename: importfinalpage.h
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

#ifndef MVDW_IMPORTFINAL_H
#define MVDW_IMPORTFINAL_H

#include "ui_importfinalpage.h"
#include "importpage.h"

class QTemporaryFile;

class MvdwImportFinalPage : public MvdwImportPage
{
	Q_OBJECT

public:
	MvdwImportFinalPage(QWidget* parent = 0);

	void showMessage(const QString& msg, MvdwImportPage::MessageType t = MvdwImportPage::InfoMessage);

	void initializePage();
	void cleanupPage();
	void setBusyStatus(bool busy);

signals:
	void importRequest(const QList<int>&);

private:
	Ui::MvdwImportFinalPage ui;
	bool locked;

	void setLock(bool lock);
};

#endif // MVDW_IMPORTFINAL_H
