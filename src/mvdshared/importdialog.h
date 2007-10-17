/**************************************************************************
** Filename: importdialog.h
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

#ifndef MVD_IMPORTDIALOG_H
#define MVD_IMPORTDIALOG_H

#include "sharedglobal.h"
#include "searchengine.h"
#include "moviedata.h"
#include <QDialog>
#include <QHash>
#include <QVariant>
#include <QPushButton>
#include <QWizard>
#include <QStringList>

class MvdImportDialog_P;
class MvdImportStartPage;
class MvdImportResultsPage;
class MvdImportFinalPage;

class MVD_EXPORT_SHARED MvdImportDialog : public QWizard
{
	Q_OBJECT

public:
	enum MessageType {
		InfoMessage = 0,
		WarningMessage,
		ErrorMessage
	};

	MvdImportDialog(QWidget* parent = 0);

	int registerEngine(const MvdSearchEngine& engine);

	void showMessage(const QString& msg, MessageType type = InfoMessage);

	int addMatch(const QString& title, const QString& year, const QString& notes = QString());
	void addMovieData(const MvdMovieData& md);

	void addSection(const QString& title, const QString& notes = QString());
	void addSubSection(const QString& title, const QString& notes = QString());

	void done();

	void accept();

signals:
	void engineConfigurationRequest(int engine);
	void searchRequest(const QString& query, int engine);
	void importRequest(const QList<int>& matches);

private slots:
	void pageChanged(int newPage);

private:
	MvdImportDialog_P* d;
};

#endif // MVD_IMPORTDIALOG_H
