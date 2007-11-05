/**************************************************************************
** Filename: importsummarypage.cpp
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

#include "importsummarypage.h"
#include "importdialog.h"
#include "core.h"
#include "actionlabel.h"
#include "templatemanager.h"
#include "logger.h"
#include <QLabel>
#include <QGridLayout>

using namespace Movida;

/*!
	\class MvdImportSummaryPage importsummarypage.h
	\ingroup MovidaShared

	\brief This page shows a preview of the movies selected for import, allowing the
	user to confirm the selection.
*/

MvdImportSummaryPage::MvdImportSummaryPage(QWidget* parent)
: MvdImportPage(parent), locked(false), previousVisibleJob(-1), currentVisibleJob(-1)
{
	setTitle(tr("Movie have been downloaded and ready for import."));
	setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/import/watermark.png"));

	ui.setupUi(this);
	
	previousResultId = ui.previousResult->addControl(tr("Previous movie"), false);
	connect(ui.previousResult, SIGNAL(controlTriggered(int)), this, SLOT(previewPreviousJob()));
	nextResultId = ui.nextResult->addControl(tr("Next movie"), false);
	connect(ui.nextResult, SIGNAL(controlTriggered(int)), this, SLOT(previewNextJob()));
}


//! Override. Unlocks the UI if it was locked.
void MvdImportSummaryPage::setBusyStatus(bool busy)
{
	if (!busy && locked)
		setLock(false);

	MvdImportPage::setBusyStatus(busy);
}

//! Adds a movie data object to the import list.
void MvdImportSummaryPage::addMovieData(const MvdMovieData& md)
{
	iLog() << "MvdImportSummaryPage: Movie data added: " << (md.title.isEmpty() ? md.originalTitle : md.title);
	jobs.append(ImportJob(md));
}

//! Locks or unlocks (part of) the GUI.
void MvdImportSummaryPage::setLock(bool lock)
{
	if (lock == locked)
		return;
	locked = lock;
	if (!locked) {
		if (jobs.isEmpty()) {
			showMessage(tr("No movie has been selected for import.\nPlease press the %1 button to repeat the search.")
				.arg(this->wizard()->buttonText(QWizard::BackButton)),
				MvdImportDialog::InfoMessage);
			return;
		}
		iLog() << "MvdImportSummaryPage: Movie data downloaded. Preparing import previews.";
		ui.stack->setCurrentIndex(1);
		previousVisibleJob = -1;
		currentVisibleJob = 0;
		visibleJobChanged();
	}
}

//! Override.
void MvdImportSummaryPage::showMessage(const QString& msg, MvdImportDialog::MessageType t)
{
	Q_UNUSED(t);
	ui.stack->setCurrentIndex(0);
	ui.noResultsLabel->setText(msg);
}

void MvdImportSummaryPage::initializePage()
{
	// Wait until the search is done.
	setBusyStatus(true);
	setLock(true);

	showMessage(tr("Downloading movie data."), 
		MvdImportDialog::InfoMessage);
}

void MvdImportSummaryPage::cleanupPage()
{
	//setBusyStatus(false);
}

//! Updates the job preview area with the current job.
void MvdImportSummaryPage::visibleJobChanged()
{
	Q_ASSERT(currentVisibleJob >= 0 && currentVisibleJob < jobs.size());

	// Store current status (if any)
	if (previousVisibleJob >= 0) {
		ImportJob& job = jobs[previousVisibleJob];
		job.import = ui.importMovie->isChecked();
	}

	const ImportJob& job = jobs.at(currentVisibleJob);
	ui.importMovie->setChecked(job.import);

	const MvdMovieData& d = job.data;
	ui.jobPreview->setHtml(Movida::tmanager().movieDataToHtml(d));

	ui.nextResult->setControlEnabled(nextResultId, currentVisibleJob < jobs.size() - 1);
	ui.previousResult->setControlEnabled(previousResultId, currentVisibleJob > 0);
	
	ui.currentResultLabel->setText(tr("Showing movie %1 out of %2.").arg(currentVisibleJob + 1).arg(jobs.size())
			.prepend("<b>").append("</b>"));
}

//! Shows the previous import job in the preview area.
void MvdImportSummaryPage::previewPreviousJob()
{
	previousVisibleJob = currentVisibleJob;
	--currentVisibleJob;
	visibleJobChanged();
}

//! Shows the next import job in the preview area.
void MvdImportSummaryPage::previewNextJob()
{
	previousVisibleJob = currentVisibleJob;
	++currentVisibleJob;
	visibleJobChanged();
}
