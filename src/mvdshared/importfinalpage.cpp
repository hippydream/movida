/**************************************************************************
** Filename: importfinalpage.cpp
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

#include "importfinalpage.h"
#include "importdialog.h"
#include "importdialog_p.h"
#include "core.h"
#include "actionlabel.h"
#include "templatemanager.h"
#include "logger.h"
#include <QLabel>
#include <QGridLayout>

using namespace Movida;

/*!
	\class MvdImportFinalPage importfinalpage.h
	\ingroup MovidaShared

	\brief Last page of the import wizard, showing the results of the import.
*/

MvdImportFinalPage::MvdImportFinalPage(QWidget* parent)
: MvdImportPage(parent), locked(false)
{
	setTitle(tr("We are all done!"));
	setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/import/watermark.png"));

	QGridLayout* gridLayout = new QGridLayout(this);

	messageLabel = new QLabel;
	messageLabel->setWordWrap(true);
	gridLayout->addWidget(messageLabel, 0, 0);
}


//! Override. Unlocks the UI if it was locked.
void MvdImportFinalPage::setBusyStatus(bool busy)
{
	if (!busy && locked)
		setLock(false);

	MvdImportPage::setBusyStatus(busy);
}

//! Locks or unlocks (part of) the GUI.
void MvdImportFinalPage::setLock(bool lock)
{
	if (lock == locked)
		return;
	locked = lock;
	if (!locked) {
		
	}
}

//! Override.
void MvdImportFinalPage::showMessage(const QString& msg, MvdImportDialog::MessageType t)
{
	Q_UNUSED(t);

	messageLabel->setText(msg);
}

void MvdImportFinalPage::initializePage()
{
	int totalMatches = field("resultsCount").toInt();
	int selectedMatches = field("selectedResultsCount").toInt();
	int importedMovies =  field("importedMoviesCount").toInt();
	QString msg;

	// wizard()->hasVisitedPage(MvdImportDialog_P::ResultsPage)
	if (totalMatches == 0) {
		msg = tr("No movie has been found matching your search criteria.\n\nPlease try to check your spelling, use a different engine or use different key words.");
	} else if (selectedMatches == 0 || importedMovies == 0) {
		msg = tr("No movie has been selected for import.\n\nYou can use the wizard for a different search or just continue having fun with movida.");
	} else {
		msg = tr("%1 movie(s) have been imported with success.", "Number of actually imported movies", importedMovies).arg(importedMovies);
	}

	showMessage(msg, MvdImportDialog::InfoMessage);
}

void MvdImportFinalPage::cleanupPage()
{
	//setBusyStatus(false);
}
