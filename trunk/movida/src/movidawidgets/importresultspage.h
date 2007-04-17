/**************************************************************************
** Filename: importresultspage.h
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

#ifndef MVDW_IMPORTRESULTSPAGE_H
#define MVDW_IMPORTRESULTSPAGE_H

#include "searchengine.h"

#include <QWizardPage>

class QTreeWidget;
class QLabel;
class QHttp;
class QHttpResponseHeader;
class QTemporaryFile;

class MvdwImportResultsPage : public QWizardPage
{
	Q_OBJECT

public:
	enum StatusType
	{
		InfoStatus, BusyStatus, ErrorStatus
	};

	MvdwImportResultsPage(const QList<MvdwSearchEngine>& engines, QWidget* parent = 0);

	void registerResponseHandler(QObject* handler, const char* member);
	void addMatch(const QString& title);
	
	void initializePage();
	void cleanupPage();
	bool isComplete() const;

	void setStatus(const QString& msg, StatusType t = InfoStatus);
	StatusType status() const;

private slots:
// 	void backButtonPressed();

	void readProgress(int data, int total);
	void requestFinished(int id, bool error);

	// HTTP specific
	void httpResponseHeader(const QHttpResponseHeader& responseHeader);

private:
	enum HttpStatusClass
	{
		NoStatusClass = 0, 
		InformationalClass = 1, 
		SuccessClass = 2, 
		RedirectionClass = 3, 
		ClientErrorClass = 4, 
		ServerErrorClass = 5
	};

	QList<MvdwSearchEngine> engines;
	QTreeWidget* results;
	QLabel* infoLabel;
	QHttp* httpHandler;
	QTemporaryFile* tempFile;
	StatusType currentStatus;
	bool initRequired;
	int requestId;
	QString lastRequestUrl;

	// Response handler
	QObject* responseHandler;
	QString responseHandlerMember;

	void startHttpRequest(const QString& query, const MvdwSearchEngine& engine);
	bool createTempFile();
	void deleteTempFile();
	void releaseHttpHandler();
};

#endif // MVDW_IMPORTRESULTSPAGE_H

