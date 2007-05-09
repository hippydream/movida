/**************************************************************************
** Filename: importfinalpage.cpp
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

#include "importfinalpage.h"
#include "importdialog.h"

#include <movidacore/core.h>

#include <QLabel>
#include <QGridLayout>


MvdwImportFinalPage::MvdwImportFinalPage(QWidget* parent)
: MvdwImportPage(parent), locked(false)
{
	setTitle(tr("Import finished"));
	setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/import/watermark.png"));

	ui.setupUi(this);
}


//! Override. Unlocks the UI if it was locked.
void MvdwImportFinalPage::setBusyStatus(bool busy)
{
	if (!busy && locked)
		setLock(false);

	MvdwImportPage::setBusyStatus(busy);
}

//! Locks or unlocks (part of) the GUI.
void MvdwImportFinalPage::setLock(bool lock)
{
	if (lock == locked)
		return;
	locked = lock;
	if (locked)
		qDebug("Locked");
	else qDebug("Unlocked");
}

//! Override.
void MvdwImportFinalPage::showMessage(const QString& msg, MvdwImportPage::MessageType t)
{
	Q_UNUSED(t);
	ui.stack->setCurrentIndex(0);
	ui.noResultsLabel->setText(msg);
}

void MvdwImportFinalPage::initializePage()
{
	// Wait until the search is done.
	setBusyStatus(true);
	setLock(true);

	showMessage(tr("No movies have been imported.\nPlease press the Back button to repeat the search."));
}

void MvdwImportFinalPage::cleanupPage()
{
	//setBusyStatus(false);
}
