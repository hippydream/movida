/**************************************************************************
** Filename: movieexport.cpp
**
** Copyright (C) 2007-2009 Angius Fabrizio. All rights reserved.
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

#include "ui_csvexportconfig.h"

#include "mvdcore/core.h"
#include "mvdcore/logger.h"
#include "mvdcore/movie.h"
#include "mvdcore/moviecollection.h"
#include "mvdcore/moviedata.h"
#include "mvdcore/settings.h"

#include "mvdshared/exportengine.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QRegExp>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTextStream>
#include <QtGui/QDialog>

using namespace Movida;

MpiMovieExport::MpiMovieExport(QObject *parent) :
    QObject(parent)
{
    mCsvSeparator = QChar(',');
    mWriteHeader = false;
}

MpiMovieExport::~MpiMovieExport()
{ }

void MpiMovieExport::run()
{
    mExportDialog = new MvdExportDialog(qobject_cast<QWidget *>(parent()));

    connect(mExportDialog, SIGNAL(exportRequest(int, MvdExportDialog::ExportRequest)),
        this, SLOT(exportRequest(int, MvdExportDialog::ExportRequest)));
    connect(mExportDialog, SIGNAL(engineConfigurationRequest(int)),
        this, SLOT(engineConfigurationRequest(int)));
    /*connect( mExportDialog, SIGNAL(resetRequest()),
            this, SLOT(reset()) );*/

    MvdExportEngine csvEngine(tr("CSV/TSV File"));
    csvEngine.urlFilter = tr("CSV/TSV Files (*.csv; *.tsv);;All Files (*.*)");
    csvEngine.options = MvdExportEngine::CustomizableAttributesOption;
    csvEngine.canConfigure = true;
    mCsvEngineId = mExportDialog->registerEngine(csvEngine);

    MvdExportEngine movidaXmlEngine(tr("Movida XML File"));
    movidaXmlEngine.urlFilter = tr("XML Files (*.xml);;All Files (*.*)");
    mMovidaXmlEngineId = mExportDialog->registerEngine(movidaXmlEngine);

    mExportDialog->setWindowModality(Qt::ApplicationModal);
    mExportDialog->exec();
}

void MpiMovieExport::exportRequest(int engineId, const MvdExportDialog::ExportRequest &req)
{
    enum ExportEngine { CsvEngine, MovidaXmlEngine } engine;
    if (engineId == mCsvEngineId)
        engine = CsvEngine;
    else if (engineId == mMovidaXmlEngineId)
        engine = MovidaXmlEngine;
    else Q_ASSERT_X(0, "MpiMovieExport::exportRequest()", "Internal Error.");

    QString filename = MvdCore::fixedFilePath(req.url.toLocalFile());
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QString error = tr("Failed to open %1:\n%2").arg(filename).arg(file.errorString());
        mExportDialog->showMessage(error, MovidaShared::ErrorMessage);
        mExportDialog->done(MvdExportDialog::CriticalError);
        return;
    }

    if (engine == CsvEngine) {
        exportToCsv(&file, req);
    } else {
        exportToMovidaXml(&file, req);
    }
}

void MpiMovieExport::engineConfigurationRequest(int engine)
{
    if (engine == mCsvEngineId) {
        showCsvConfigurationDlg();
    }
}

void MpiMovieExport::customCsvSeparatorTriggered()
{
    QRadioButton *radio = dynamic_cast<QRadioButton *>(sender());

    if (!radio)
        return;

    QWidget *w = radio->parentWidget();
    if (!w)
        return;

    QLineEdit *le = w->findChild<QLineEdit *>();
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
    ui.writeHeader->setChecked(mWriteHeader);
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
    mWriteHeader = ui.writeHeader->isChecked();
}

void MpiMovieExport::exportToCsv(QIODevice *out, const MvdExportDialog::ExportRequest &req) const
{
    MvdPluginContext *ctx = MvdCore::pluginContext();
    MvdMovieCollection *c = Movida::core().currentCollection();
    Q_ASSERT(c);

    QList<mvdid> selected;

    if (req.type == MvdExportDialog::ExportSelectedMovies) {
        selected = ctx->selectedMovies;
        if (selected.isEmpty()) {
            wLog() << QString("MpiMovieExport: Export request for selected movies but no selection found.");
            mExportDialog->done(MvdExportDialog::Success);
            return;
        }
    } else {
        selected = c->movieIds();
        if (selected.isEmpty()) {
            wLog() << QString("MpiMovieExport: Export request for all movies but no movie found.");
            mExportDialog->done(MvdExportDialog::Success);
            return;
        }
    }

    for (int i = 0; i < selected.size(); ++i) {
        mvdid id = selected.at(i);
        MvdMovie m = c->movie(id);
        Q_ASSERT_X(m.isValid(), "MpiMovieExport::exportToCsv()", "Internal error.");
        MvdMovieData d = m.toMovieData(c);
        d.writeToXmlDevice(out, MvdMovieData::NoXmlEncoding | MvdMovieData::EmbedMoviePoster);
    }

    mExportDialog->done(MvdExportDialog::Success);
}

void MpiMovieExport::exportToMovidaXml(QIODevice *out, const MvdExportDialog::ExportRequest &req) const
{
    MvdPluginContext *ctx = MvdCore::pluginContext();
    MvdMovieCollection *c = Movida::core().currentCollection();
    Q_ASSERT(c);

    QList<mvdid> selected;

    if (req.type == MvdExportDialog::ExportSelectedMovies) {
        selected = ctx->selectedMovies;
        if (selected.isEmpty()) {
            wLog() << QString("MpiMovieExport: Export request for selected movies but no selection found.");
            mExportDialog->done(MvdExportDialog::Success);
            return;
        }
    } else {
        selected = c->movieIds();
        if (selected.isEmpty()) {
            wLog() << QString("MpiMovieExport: Export request for all movies but no movie found.");
            mExportDialog->done(MvdExportDialog::Success);
            return;
        }
    }

    //! \todo EmbedMoviePoster should be an engine option
    for (int i = 0; i < selected.size(); ++i) {
        mvdid id = selected.at(i);
        MvdMovie m = c->movie(id);
        Q_ASSERT_X(m.isValid(), "MpiMovieExport::exportToCsv()", "Internal error.");
        MvdMovieData d = m.toMovieData(c);
        d.writeToXmlDevice(out, MvdMovieData::NoXmlEncoding | MvdMovieData::EmbedMoviePoster);
    }

    mExportDialog->done(MvdExportDialog::Success);
}
