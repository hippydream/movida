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
#include "core.h"
#include "actionlabel.h"
#include "templatemanager.h"
#include <QLabel>
#include <QGridLayout>

/*!
	\class MvdImportFinalPage importfinalpage.h
	\ingroup MovidaShared

	\brief Last page of the import wizard, showing the results of the import.
*/

MvdImportFinalPage::MvdImportFinalPage(QWidget* parent)
: MvdImportPage(parent), locked(false), previousVisibleJob(-1), currentVisibleJob(-1)
{
	setTitle(tr("Import finished"));
	setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/import/watermark.png"));

	ui.setupUi(this);
	
	previousResultId = ui.previousResult->addControl(tr("Preview previous"), false);
	connect(ui.previousResult, SIGNAL(controlTriggered(int)), this, SIGNAL(previewPreviousJob()));
	nextResultId = ui.nextResult->addControl(tr("Preview next"), false);
	connect(ui.nextResult, SIGNAL(controlTriggered(int)), this, SIGNAL(previewNextJob()));
}


//! Override. Unlocks the UI if it was locked.
void MvdImportFinalPage::setBusyStatus(bool busy)
{
	if (!busy && locked)
		setLock(false);

	MvdImportPage::setBusyStatus(busy);
}

//! Adds a movie data object to the import list.
void MvdImportFinalPage::addMovieData(const MvdMovieData& md)
{
	jobs.append(ImportJob(md));
}

//! Locks or unlocks (part of) the GUI.
void MvdImportFinalPage::setLock(bool lock)
{
	if (lock == locked)
		return;
	locked = lock;
	if (locked)
		qDebug("Locked");
	else {
		if (jobs.isEmpty()) {
			showMessage(tr("No movies have been imported.\nPlease press the Back button to repeat the search."),
				MvdImportDialog::InfoMessage);
			return;
		}
		ui.stack->setCurrentIndex(1);
		previousVisibleJob = -1;
		currentVisibleJob = 0;
		visibleJobChanged();
	}
}

//! Override.
void MvdImportFinalPage::showMessage(const QString& msg, MvdImportDialog::MessageType t)
{
	Q_UNUSED(t);
	ui.stack->setCurrentIndex(0);
	ui.noResultsLabel->setText(msg);
}

void MvdImportFinalPage::initializePage()
{
	// Wait until the search is done.
	setBusyStatus(true);
	setLock(true);

	showMessage(tr("Downloading movie data."), 
		MvdImportDialog::InfoMessage);
}

void MvdImportFinalPage::cleanupPage()
{
	//setBusyStatus(false);
}

//! Updates the job preview area with the current job.
void MvdImportFinalPage::visibleJobChanged()
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
	
	ui.currentResultLabel->setText(tr("Showing movie %1 out of %2.").arg(currentVisibleJob - 1).arg(jobs.size())
			.prepend("<b>").append("</b>"));
}

//! Shows the previous import job in the preview area.
void MvdImportFinalPage::previewPreviousJob()
{
	previousResultId = currentVisibleJob;
	++currentVisibleJob;
}

//! Shows the next import job in the preview area.
void MvdImportFinalPage::previewNextJob()
{
	previousResultId = currentVisibleJob;
	--currentVisibleJob;
}
