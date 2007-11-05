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
#include "settings.h"
#include "logger.h"
#include <QLabel>
#include <QRadioButton>
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

	ui.setupUi(this);

	ui.filterMovies->setChecked(settings().value("movida/import-wizard/auto-create-filter", true).toBool());

	connect(ui.restartWizard, SIGNAL(toggled(bool)), this, SLOT(restartWizardToggled()));
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

	ui.messageLabel->setText(msg);
}

void MvdImportFinalPage::initializePage()
{
	int totalMatches = field("resultsCount").toInt();
	int selectedMatches = field("selectedResultsCount").toInt();
	int importedMovies =  field("importedMoviesCount").toInt();

	QString msg;
	bool hasSomeImport = importedMovies > 0;

	// wizard()->hasVisitedPage(MvdImportDialog_P::ResultsPage)
	if (totalMatches == 0) {
		msg = tr("No movie has been found matching your search criteria.\n\nPlease try to check your spelling, use a different engine or use different key words.");
	} else if (selectedMatches == 0 || importedMovies == 0) {
		msg = tr("No movie has been selected for import.\n\nYou can use the wizard for a different search or just continue having fun with movida.");
	} else {
		msg = tr("%1 movie(s) have been imported with success.", "Number of actually imported movies", importedMovies).arg(importedMovies);
	}

	showMessage(msg, MvdImportDialog::InfoMessage);
	ui.filterMovies->setEnabled(hasSomeImport);
}

void MvdImportFinalPage::cleanupPage()
{
	//setBusyStatus(false);
}

//! Toggles the finish/new_search button.
void MvdImportFinalPage::restartWizardToggled()
{
	if (finishButtonText.isEmpty())
		finishButtonText = wizard()->buttonText(QWizard::FinishButton);

	wizard()->setButtonText(QWizard::FinishButton, ui.restartWizard->isChecked() ? tr("&New search") : finishButtonText);

	int importedMovies =  field("importedMoviesCount").toInt();
	bool hasSomeImports = importedMovies > 0;

	ui.filterMovies->setEnabled(hasSomeImports && ui.closeWizard->isChecked());
}

/*! Returns false and calls QWizard::restart() if a new search is to be performed. Returns true, causing the wizard to
	close (as this is supposed to be the last page) otherwise.
*/
bool MvdImportFinalPage::validatePage()
{
	if (ui.restartWizard->isChecked()) {
		Q_ASSERT(QMetaObject::invokeMethod(wizard(), "restart", Qt::QueuedConnection));
		return false;
	}

	settings().setValue("movida/import-wizard/auto-create-filter", ui.filterMovies->isChecked());

	return true;
}
