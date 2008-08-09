/**************************************************************************
** Filename: movieexport.cpp
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

#include "movieexport.h"
#include "mvdcore/core.h"
#include "mvdcore/logger.h"
#include "mvdcore/settings.h"
#include "mvdshared/exportengine.h"
#include <QRegExp>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QTemporaryFile>
#include <QDateTime>
#include <QLocale>

#include "ui_csvexportconfig.h"

using namespace Movida;

MpiMovieExport::MpiMovieExport(QObject* parent)
: QObject(parent)
{
	mCsvSeparator = QChar(',');
	mIgnoreHeader = false;
}

MpiMovieExport::~MpiMovieExport()
{
}

void MpiMovieExport::run()
{
	mExportDialog = new MvdExportDialog(qobject_cast<QWidget*>(parent()));
	
	connect( mExportDialog, SIGNAL(exportRequest(int,MvdExportDialog::ExportOptions)),
		this, SLOT(exportRequest(int,MvdExportDialog::ExportOptions)) );
	connect( mExportDialog, SIGNAL(engineConfigurationRequest(int)),
		this, SLOT(engineConfigurationRequest(int)) );
	/*connect( mExportDialog, SIGNAL(resetRequest()),
		this, SLOT(reset()) );*/

	MvdExportEngine csvEngine(tr("CSV/TSV file"));
	csvEngine.options = MvdExportEngine::CustomizableAttributesOption;
	csvEngine.canConfigure = true;
	mCsvEngineId = mExportDialog->registerEngine(csvEngine);

	MvdExportEngine movidaXmlEngine(tr("Movida XML file"));
	mMovidaXmlEngineId = mExportDialog->registerEngine(movidaXmlEngine);

	mExportDialog->setWindowModality(Qt::WindowModal);
	mExportDialog->exec();
}

void MpiMovieExport::exportRequest(int engine, const MvdExportDialog::ExportOptions& opt)
{
	qDebug("Export %d", engine);
}

void MpiMovieExport::engineConfigurationRequest(int engine)
{
	if (engine == mCsvEngineId) {
		showCsvConfigurationDlg();
	}
}

void MpiMovieExport::customCsvSeparatorTriggered()
{
	QRadioButton* radio = dynamic_cast<QRadioButton*>(sender());
	if (!radio)
		return;

	QWidget* w = radio->parentWidget();
	if (!w)
		return;

	QLineEdit* le = w->findChild<QLineEdit*>();
	if (!le)
		return;

	bool checked = radio->isChecked();
	le->setEnabled(checked);
	if (checked) {
		le->setFocus(Qt::OtherFocusReason);
		le->setSelection(0, 1);
	}
}

void MpiMovieExport::showCsvConfigurationDlg()
{
	QDialog dialog(mExportDialog);
	Ui::MpiCsvExportConfig ui;
	ui.setupUi(&dialog);
	connect(ui.otherSeparator, SIGNAL(toggled(bool)), this, SLOT(customCsvSeparatorTriggered()));
	if (mCsvSeparator == QLatin1Char(','))
		ui.commaSeparator->setChecked(true);
	else if (mCsvSeparator == QLatin1Char('\t'))
		ui.tabSeparator->setChecked(true);
	else {
		ui.otherSeparator->setChecked(true);
		ui.otherSeparatorInput->setText(mCsvSeparator);
	}
	ui.ignoreHeader->setChecked(mIgnoreHeader);
	int res = dialog.exec();
	if (res != QDialog::Accepted) 
		return;
	if (ui.commaSeparator->isChecked())
		mCsvSeparator = QChar(',');
	else if (ui.tabSeparator->isChecked())
		mCsvSeparator = QChar('\t');
	else if (ui.otherSeparator->isChecked()) {
		QString s = ui.otherSeparatorInput->text();
		mCsvSeparator = s.isEmpty() ? QChar(',') : s.at(0);
	}
	mIgnoreHeader = ui.ignoreHeader->isChecked();
}
