/**************************************************************************
** Filename: movieexport.h
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

#ifndef MPI_MOVIEEXPORT_H
#define MPI_MOVIEEXPORT_H

#include "blue.h"
#include "mvdshared/exportdialog.h"

class MpiMovieExport : QObject
{
	Q_OBJECT

public:
	MpiMovieExport(QObject* parent = 0);
	virtual ~MpiMovieExport();

	void run();

private slots:
	void exportRequest(int engine, const MvdExportDialog::ExportOptions& opt);
	void engineConfigurationRequest(int engine);
	void customCsvSeparatorTriggered();

private:
	void showCsvConfigurationDlg();

	MvdExportDialog* mExportDialog;
	int mCsvEngineId;
	int mMovidaXmlEngineId;
	QChar mCsvSeparator;
	bool mIgnoreHeader;
};

#endif // MPI_MOVIEEXPORT_H
