/**************************************************************************
** Filename: exportstartpage.cpp
**
** Copyright (C) 2007-2008 Angius Fabrizio. All rights reserved.
**
** This file is part of the Movida project (http://movida.42cows.org/).
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

#include "exportstartpage.h"
#include "actionlabel.h"
#include "mvdcore/core.h"
#include "mvdcore/plugininterface.h"
#include "mvdcore/settings.h"
#include <QLabel>
#include <QComboBox>
#include <QGridLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QFormLayout>
#include <QMessageBox>

/*!
	\class MvdExportStartPage exportstartpage.h
	\ingroup MovidaShared

	\brief First page of the export wizard, showing the available engines and 
	the main export options.
*/

/*!
	This Export Wizard page allows to select an export engine
	(if more than one are available) and some options.
*/
MvdExportStartPage::MvdExportStartPage(QWidget* parent)
: MvdImportExportPage(parent)
{
	MvdPluginContext* ctx = MvdCore::pluginContext();
	Q_ASSERT(ctx);

	bool hasSelectedMovies = !ctx->selectedMovies.isEmpty();

	setTitle(tr("Movida export wizard"));
	setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/import-wizard/watermark.png"));

	mInfoLabel = new QLabel;
	mInfoLabel->setWordWrap(true);
	mInfoLabel->setText(tr("Please select an output format. You might be prompted for further options on the following page."));
	
	mEngineCombo = new QComboBox;
	connect( mEngineCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(engineChanged()) );

	QGridLayout* gridLayout = new QGridLayout(this);
	gridLayout->addWidget(mInfoLabel, 0, 0, 1, 2);
	gridLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed), 1, 1, 1, 1);

	QFormLayout* engineLayout = new QFormLayout;

	QLabel* engineLabel = new QLabel(this);
	engineLabel->setText(tr("Output format:"));
	engineLayout->addRow(engineLabel, mEngineCombo);
	
	mExportSelectedButton = new QRadioButton(tr("Export selected movies"), this);
	mExportAllButton = new QRadioButton(tr("Export entire collection"), this);

	if (hasSelectedMovies) {
		mExportSelectedButton->setChecked(true);
	} else {
		mExportSelectedButton->setVisible(false);
		mExportAllButton->setVisible(false);
		mExportSelectedButton->setEnabled(false);
		mExportAllButton->setChecked(true);
	}

	engineLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Expanding));
	engineLayout->addRow(mExportSelectedButton);
	engineLayout->addRow(mExportAllButton);

	gridLayout->addLayout(engineLayout, 2, 0, 1, 2);

	gridLayout->addItem(new QSpacerItem(20, 60, QSizePolicy::Minimum, QSizePolicy::Expanding), 3, 0, 1, 1);

	mControls = new MvdActionLabel(this);
	mConfigureEngineId = mControls->addControl(tr("Configure format"), false);
	mConfigurePluginId = mControls->addControl(tr("Configure plugin"), true);
	connect( mControls, SIGNAL(controlTriggered(int)), this, SLOT(controlTriggered(int)) );

	gridLayout->addItem(new QSpacerItem(60, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 4, 0, 1, 1);
	gridLayout->addWidget(mControls, 4, 1, 1, 1);
}

//! \internal
void MvdExportStartPage::engineChanged()
{
	if (mEngines.isEmpty())
		return;

	const MvdExportEngine& e = mEngines.at(mEngineCombo->currentIndex());
	mControls->setControlEnabled(mConfigureEngineId, e.canConfigure);
}

//! \internal
void MvdExportStartPage::controlTriggered(int id)
{
	if (id == mConfigureEngineId) {
		emit engineConfigurationRequest(mEngineCombo->currentIndex());
	} else if (id == mConfigurePluginId) {
		QMessageBox::information(this, "Movida blue plugin", "Sorry, this feature has not been implemented yet.");
	}
}

//! Returns the ID of the currently selected search engine.
int MvdExportStartPage::engine() const
{
	return mEngineCombo->currentIndex();
}

//! Resets anything before the page is shown.
void MvdExportStartPage::initializePage()
{
}

//!
void MvdExportStartPage::reset()
{
	setBusyStatus(false);
}

int MvdExportStartPage::registerEngine(const MvdExportEngine& engine)
{
	if (mEngines.contains(engine))
		return -1;
	mEngines.append(engine);
	mEngineCombo->addItem(engine.name);
	return mEngineCombo->count() - 1;
}
