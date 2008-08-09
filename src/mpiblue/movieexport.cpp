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
#include <QRegExp>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QTemporaryFile>
#include <QDateTime>
#include <QLocale>

using namespace Movida;

MpiMovieExport::MpiMovieExport(QObject* parent)
: QObject(parent)
{
}

MpiMovieExport::~MpiMovieExport()
{
}

void MpiMovieExport::run()
{
	mExportDialog = new MvdExportDialog(qobject_cast<QWidget*>(parent()));
	
	connect( mExportDialog, SIGNAL(exportRequest(int,MvdExportDialog::ExportOptions)),
		this, SLOT(exportRequest(int,MvdExportDialog::ExportOptions)) );
	/*connect( mExportDialog, SIGNAL(resetRequest()),
		this, SLOT(reset()) );*/

	int csvEngine = mExportDialog->registerEngine(tr("CSV/TSV file"));
	int movidaXmlEngine = mExportDialog->registerEngine(tr("Movida XML file"));

	mExportDialog->setWindowModality(Qt::WindowModal);
	mExportDialog->exec();
}

void MpiMovieExport::exportRequest(int engine, const MvdExportDialog::ExportOptions& opt)
{
	qDebug("Export");
}
