/**************************************************************************
** Filename: importsummarypage.h
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

#ifndef MVD_IMPORTSUMMARY_H
#define MVD_IMPORTSUMMARY_H

#include "ui_importsummarypage.h"
#include "sharedglobal.h"
#include "importexportpage.h"
#include "mvdcore/moviedata.h"

class QTemporaryFile;

class MvdImportSummaryPage : public MvdImportExportPage
{
	Q_OBJECT
	Q_PROPERTY(int importedMoviesCount READ importedMoviesCount)

public:
	MvdImportSummaryPage(QWidget* parent = 0);

	void showMessage(const QString& msg, MovidaShared::MessageType t);

	void initializePage();
	void cleanupPage();
	void setBusyStatus(bool busy);
	void reset();

	void addMovieData(const MvdMovieData& md);

	int importedMoviesCount() const;

	void setProgress(int v);
	void setProgressMaximum(int m);
	int progress() const;

	MvdMovieDataList movies();

signals:
	void importRequest(const QList<int>&);
	void importedMoviesCountChanged(int c = 0);

private slots:
	void visibleJobChanged();
	void previewPreviousJob();
	void previewNextJob();
	void importMovieStateChanged();
	void jumpToMovie(int index);

private:
	struct ImportJob {
		//! \todo Either use MvdMovieData pointers or make MvdMovieData implicitly shared
		inline ImportJob() : import(true), browserViewId(-1) {}
		inline ImportJob(const MvdMovieData& md) : data(md), import(true), browserViewId(-1) {}

		MvdMovieData data;
		bool import;
		int browserViewId;
	};

	Ui::MvdImportSummaryPage ui;
	bool locked;

	void setLock(bool lock);

	QList<ImportJob> jobs;
	int previousVisibleJob;
	int currentVisibleJob;
	int previousResultId;
	int nextResultId;
};

#endif // MVD_IMPORTSUMMARY_H
