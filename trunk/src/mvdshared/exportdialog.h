/**************************************************************************
** Filename: exportdialog.h
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

#ifndef MVD_EXPORTDIALOG_H
#define MVD_EXPORTDIALOG_H

#include "sharedglobal.h"
#include "exportengine.h"
#include <QDialog>
#include <QHash>
#include <QVariant>
#include <QPushButton>
#include <QWizard>
#include <QStringList>

class MVD_EXPORT_SHARED MvdExportDialog : public QWizard
{
	Q_OBJECT

public:
	//! What happened?
	enum Result {
		/*! No error occurred. */
		Success = 0,
		/* Export failed for some movie */
		Failed,
		/* No export could be performed or some other critical error occurred. */
		CriticalError
	};

	//! Why did it happen?
	enum ErrorType {
		UnknownError = 0,
		NetworkError,
		FileError
	};

	enum ExportType {
		ExportSelectedMovies = 0,
		ExportCollection
	};

	struct ExportOptions {
		ExportType type;
	};

	MvdExportDialog(QWidget* parent = 0);

	int registerEngine(const MvdExportEngine& engine);

	virtual int nextId() const;

	void showMessage(const QString& msg, MvdShared::MessageType type = MvdShared::InfoMessage);

	void setErrorType(ErrorType type);
	ErrorType errorType() const;

	void done(Result res = Success);
	Result result() const;

	void accept();

	bool preventCloseWhenBusy() const;
	bool isBusy() const;

	bool confirmCloseWizard();
	
public slots:
	virtual void reject();

protected:
	void closeEvent(QCloseEvent* e);
	void keyPressEvent(QKeyEvent* e);

signals:
	void engineConfigurationRequest(int engine);
	void exportRequest(int engine, const MvdExportDialog::ExportOptions& options);
	void resetRequest();

private slots:
	void pageChanged(int newPage);

private:
	class Private;
	Private* d;
};

#endif // MVD_EXPORTDIALOG_H
