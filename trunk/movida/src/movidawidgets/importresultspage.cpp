/**************************************************************************
** Filename: importresultspage.cpp
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

#include "importresultspage.h"

#include <movidacore/core.h>

#include <QLabel>
#include <QTreeWidget>
#include <QGridLayout>
#include <QHttp>
#include <QTemporaryFile>
#include <QDir>
#include <QAbstractButton>

#include <math.h>

MvdwImportResultsPage::MvdwImportResultsPage(const QList<MvdwSearchEngine>& _engines, QWidget* parent)
: QWizardPage(parent), engines(_engines), httpHandler(0), tempFile(0),
currentStatus(InfoStatus), initRequired(true), requestId(-1), responseHandler(0)
{
	setTitle(tr("Search results"));
	setSubTitle(tr("Please select the items you want to import."));
	setPixmap(QWizard::LogoPixmap, QPixmap(":/images/import/logo.png"));

	QGridLayout* gridLayout = new QGridLayout(this);
	
	results = new QTreeWidget(this);
	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	sizePolicy.setHorizontalStretch(5);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(results->sizePolicy().hasHeightForWidth());
	results->setSizePolicy(sizePolicy);
	
	gridLayout->addWidget(results, 0, 0, 1, 1);
	
	infoLabel = new QLabel(this);
	sizePolicy = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	sizePolicy.setHorizontalStretch(2);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(infoLabel->sizePolicy().hasHeightForWidth());
	infoLabel->setSizePolicy(sizePolicy);
	infoLabel->setMargin(10);
	
	gridLayout->addWidget(infoLabel, 0, 1, 1, 1);

	// Register fields
}

/*!
	See MvdwImportDialog::registerResponseHandler(QObject*, const char*)
*/
void MvdwImportResultsPage::registerResponseHandler(QObject* handler, const char* member)
{
	qDebug("registerResponseHandler");

	bool removeHandler = false;

	if (!handler || !member)
		removeHandler = true;
	else
	{
		responseHandler = handler;
		responseHandlerMember = QString::fromLatin1(member);
		removeHandler = responseHandlerMember.isEmpty();
	}

	if (removeHandler)
	{
		responseHandler = 0;
		responseHandlerMember.clear();
	}
}

/*!
	See MvdwImportDialog::addMatch(const QString&)
*/
void MvdwImportResultsPage::addMatch(const QString& title)
{
	QTreeWidgetItem* item = new QTreeWidgetItem(results);
	item->setText(0, title);
	item->setCheckState(0, Qt::Unchecked);
}

void MvdwImportResultsPage::initializePage()
{
// 	if (initRequired)
// 	{
// 		initRequired = false;
// 		if (wizard()->button(QWizard::BackButton))
// 			connect(wizard()->button(QWizard::BackButton), SIGNAL(clicked()), this, SLOT(backButtonPressed()));
// 	}

	QString query = field("query").toString().trimmed();
	QString engineName = field("engine").toString();

	// Set cleaned query string
	setField("query", query);

	MvdwSearchEngine engine;

	foreach (MvdwSearchEngine e, engines)
	{
		if (e.name == engineName)
		{
			engine = e;
			break;
		}
	}

	if (!engine.queryUrl.isValid())
	{	
		setStatus(tr("Invalid search engine URL."), ErrorStatus);
		return;
	}

	if (engine.queryUrl.scheme() != "http")
	{
		setStatus(tr("Unsupported search engine URL."), ErrorStatus);
		return;
	}

	setStatus(tr("Searching..."), BusyStatus);
	startHttpRequest(query, engine);
}

//! This method is called when the user hits the "back" button.
void MvdwImportResultsPage::cleanupPage()
{
	qDebug("cleanup");
	if (httpHandler)
		releaseHttpHandler();
	deleteTempFile();
	results->clear();
}

//! Re-implements the superclass method to ensure that the status is not BusyStatus.
bool MvdwImportResultsPage::isComplete() const
{
	return currentStatus != BusyStatus && QWizardPage::isComplete();
}

void MvdwImportResultsPage::setStatus(const QString& msg, StatusType t)
{
	infoLabel->setText(msg);

	if (currentStatus == t)
		return;

	currentStatus = t;

	// This will ensure that the appropriate buttons are enabled/disabled.
	emit completeChanged();
}

MvdwImportResultsPage::StatusType MvdwImportResultsPage::status() const
{
	return currentStatus;
}

// void MvdwImportResultsPage::backButtonPressed()
// {
// }

void MvdwImportResultsPage::readProgress(int data, int total)
{
}

void MvdwImportResultsPage::requestFinished(int id, bool error)
{
	Q_ASSERT(httpHandler);
	Q_ASSERT(tempFile);

	if (error)
	{
		setStatus(tr("Sorry, some error occurred.\n(%1)").arg(httpHandler->errorString()), ErrorStatus);
		releaseHttpHandler();
		return;
	}

	if (id != requestId)
		return;

	releaseHttpHandler();
	if (responseHandler && !responseHandlerMember.isEmpty())
	{
		tempFile->seek(0);
		int response;
		QMetaObject::invokeMethod(responseHandler, responseHandlerMember.toLatin1().constData(),
			Qt::DirectConnection, Q_RETURN_ARG(int, response),
			Q_ARG(QString, lastRequestUrl), QArgument<QIODevice>("QIODevice&", *tempFile));
	}
}

void MvdwImportResultsPage::httpResponseHeader(const QHttpResponseHeader& responseHeader)
{
	qDebug("response header");
	Q_ASSERT(httpHandler);

	int status = responseHeader.statusCode();
	HttpStatusClass httpClass = (HttpStatusClass) int(floor(status / 100.0));

	switch (httpClass)
	{
	case RedirectionClass:
		// Redirect example:
		// Location: http://akas.imdb.com/title/tt0469754/
		{
			QString location = responseHeader.value("Location");
			QUrl locationUrl(location);

			// Reset temp file
			if (!createTempFile())
			{
				setStatus(tr("Unable to create a temporary file."), ErrorStatus);
				releaseHttpHandler();
				return;
			}

			setStatus(tr("Redirecting..."), BusyStatus);
			location = locationUrl.path().append("?").append(locationUrl.encodedQuery());
			lastRequestUrl = location;
			requestId = httpHandler->get(location, tempFile);
		}
		break;
	case SuccessClass:
		setStatus(tr("Processing results..."), BusyStatus);
		break;
	default:
		setStatus(tr("Sorry, an error occurred (Code %1: %2).\nPlease <a href='movida://httpget/search'>click here</a> to retry.")
			.arg(responseHeader.statusCode()).arg(responseHeader.reasonPhrase()), ErrorStatus);
		cleanupPage();
	}
}

void MvdwImportResultsPage::startHttpRequest(const QString& query, const MvdwSearchEngine& engine)
{
	// Assert no jobs are running
	Q_ASSERT(!httpHandler);
	Q_ASSERT(!tempFile);

	if (!createTempFile())
	{
		setStatus(tr("Unable to create a temporary file."), ErrorStatus);		
		return;
	}

	// Init QHTTP
	httpHandler = new QHttp(this);

	connect(httpHandler, SIGNAL(dataReadProgress(int, int)), 
		this, SLOT(readProgress(int, int)));
	connect(httpHandler, SIGNAL(requestFinished(int, bool)), 
		this, SLOT(requestFinished(int, bool)));
	connect(httpHandler, SIGNAL(responseHeaderReceived(const QHttpResponseHeader&)), 
		this, SLOT(httpResponseHeader(const QHttpResponseHeader&)));	

	//! \todo Handle proxy in the main application (best with Qt >= 4.3)

	httpHandler->setHost(engine.queryUrl.host(), engine.queryUrl.port(80));
	
	QByteArray encodedQuery = MvdCore::toLatin1PercentEncoding(query);
	
	// Do not alter original URL
	QUrl requestUrl(engine.queryUrl);

	// All this crap is necessary because QUrl lacks of a way to set a single query item!
	requestUrl.removeQueryItem(engine.queryParameter);
	QList<QPair<QString, QString> > queryItems = requestUrl.queryItems();

	QString request = requestUrl.path();
	if (!queryItems.isEmpty())
		request.append("?");

	for (int i = 0; i < queryItems.size(); ++i)
	{
		QPair<QString, QString> item = queryItems.at(i);
		request.append(item.first).append("=").append(item.second).append("&");
	}
	request.append(engine.queryParameter).append("=").append(QString::fromLatin1(encodedQuery));

	lastRequestUrl = request;
	requestId = httpHandler->get(request.toLatin1(), tempFile);
}

//! \internal
bool MvdwImportResultsPage::createTempFile()
{
	if (tempFile)
	{
		// Delete old temp file
		deleteTempFile();
	}

	tempFile = new QTemporaryFile(QDir::tempPath() + "/movida-import", this);
	tempFile->setAutoRemove(false);
	if (!tempFile->open())
	{
		delete tempFile;
		tempFile = 0;
		return false;
	}

	return true;
}

//! \internal
void MvdwImportResultsPage::deleteTempFile()
{
	if (tempFile)
	{
		QFileInfo info(*tempFile);
		delete tempFile;
		tempFile = 0;
		QFile::remove(info.absoluteFilePath());
	}
}

//! \internal
void MvdwImportResultsPage::releaseHttpHandler()
{
	Q_ASSERT(httpHandler);
	qDebug("releaseHttp");
	httpHandler->disconnect(this);
	httpHandler->disconnect();
	// Delete after returning to the event loop.
	httpHandler->deleteLater();
	httpHandler = 0;
}
