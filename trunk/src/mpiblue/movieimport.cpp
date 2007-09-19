/**************************************************************************
** Filename: movieimport.cpp
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

#include "movieimport.h"
#include "core.h"
#include "logger.h"
#include "searchengine.h"
#include <QMessageBox>
#include <QRegExp>
#include <QDir>
#include <QTemporaryFile>
#include <QProcess>
#include <math.h>

using namespace Movida;

MpiMovieImport::MpiMovieImport(QObject* parent)
: QObject(parent), mImportDialog(0), mHttpHandler(0), mRequestId(-1), mTempFile(0), mCurrentEngine(-1), 
mInterpreter(0)
{
}

MpiMovieImport::~MpiMovieImport()
{

}

//! Entry point for the "imdb-import" action.
void MpiMovieImport::runImdbImport(const QList<MpiBlue::Engine*>& engines)
{
	mImportDialog = new MvdImportDialog(qobject_cast<QWidget*>(parent()));
	
	for (int i = 0; i < engines.size(); ++i)
	{
		MpiBlue::Engine* e = engines.at(i);
		MvdSearchEngine mvdEngine;
		mvdEngine.name = e->displayName;
		int id = mImportDialog->registerEngine(mvdEngine);
		mRegisteredEngines.insert(id, e);
		iLog() << QString("MpiBlue: Engine %1 registered with id %2.").arg(e->name).arg(id);
	}

	connect( mImportDialog, SIGNAL(engineConfigurationRequest(int)),
		this, SLOT(configureEngine(int)) );
	connect( mImportDialog, SIGNAL(searchRequest(const QString&, int)),
		this, SLOT(search(const QString&, int)) );
	connect( mImportDialog, SIGNAL(importRequest(const QList<int>&)),
		this, SLOT(import(const QList<int>&)) );

	mImportDialog->setWindowModality(Qt::WindowModal);
	mImportDialog->exec();
}

//! \todo Implement MpiMovieImport::configureEngine
void MpiMovieImport::configureEngine(int engine)
{
	Q_UNUSED(engine);
	QWidget* p = qobject_cast<QWidget*>(this->parent());
	QMessageBox::warning(p, "Movie engine configuration", "Not implemented");
}

/*!
	Initializes internal state and prepares the search engine, then performs a query 
	and adds found results to the import dialog.
*/
void MpiMovieImport::search(const QString& query, int engineId)
{
	iLog() << QString("MpiBlue: search request for query '%1' and engine %2").arg(query).arg(engineId);

	mSearchResults.clear();
	mImportsQueue.clear();

	MpiBlue::Engine* engine = mRegisteredEngines.value(engineId);
	if (!engine->scriptsFetched)
	{
		// We need to check for possible script updates and
		// replace the script file names with absolute, clean and localized paths.
		if (engine->updateUrl.startsWith("http://"))
		{
			// Check for updates.
			int rep = engine->updateUrl.indexOf("{SCRIPT}");
			QString engineUpdateUrl = engine->updateUrl;
			if (rep <= 0)
			{
				if (!engineUpdateUrl.endsWith("/") && !engineUpdateUrl.endsWith("="))
					engineUpdateUrl.append("/");
			}
			
			// Search script
			QString replacement = engine->resultsUrl.isEmpty() ? engine->resultsScript : engine->resultsUrl;
			QString currentUrl = engineUpdateUrl;

			if (rep <= 0)
				currentUrl.append(replacement);
			else currentUrl.replace("{SCRIPT}", replacement);

			iLog() << "MpiBlue: Attempting to update results script: " << currentUrl;

			// Import script
			replacement = engine->importUrl.isEmpty() ? engine->importScript : engine->importUrl;
			currentUrl = engineUpdateUrl;

			if (rep <= 0)
				currentUrl.append(replacement);
			else currentUrl.replace("{SCRIPT}", replacement);

			iLog() << "MpiBlue: Attempting to update import script: " << currentUrl;
		}
		else
		{
			// Locate script and set absolute paths.
		}

	} else performSearch(query, engine, engineId);
}

//! Do not call this directly. Use search() as it will perform some initialization.
void MpiMovieImport::performSearch(const QString& query, MpiBlue::Engine* engine, int engineId)
{
	Q_ASSERT(engine);

	iLog() << QString("MpiBlue: performing search for query '%1' and engine %2").arg(query).arg(engine->name);
	Q_ASSERT(engine->scriptsFetched);

	QString searchUrlString = engine->searchUrl;
	searchUrlString.replace("{QUERY}", QString::fromLatin1(MvdCore::toLatin1PercentEncoding(query)));

	QUrl searchUrl(searchUrlString);

	if (searchUrl.host().isEmpty())
	{
		eLog() << "MpiBlue: Invalid search engine host";
		mImportDialog->done();
		return;
	}

	if (mTempFile)
		deleteTemporaryFile(&mTempFile);

	mTempFile = createTemporaryFile();
	if (!mTempFile)
	{
		eLog() << "MpiBlue: Failed to create a temporary file";
		mImportDialog->done();
		return;
	}
	
	mCurrentEngine = engineId;

	// Assert no jobs are running
	if (mHttpHandler)
		mHttpHandler->abort();
	else
	{
		//! \todo Handle proxy in the main application (best with Qt >= 4.3)
		mHttpHandler = new QHttp(this);

		connect(mHttpHandler, SIGNAL(requestFinished(int, bool)),
			this, SLOT(requestFinished(int, bool)));
		connect(mHttpHandler, SIGNAL(responseHeaderReceived(const QHttpResponseHeader&)),
			this, SLOT(httpResponseHeader(const QHttpResponseHeader&)));
	}

	iLog() << QString("MpiBlue: Connecting to %1:%2").arg(searchUrl.host()).arg(searchUrl.port() < 0 ? 80 : searchUrl.port());
	mHttpHandler->setHost(searchUrl.host(),searchUrl.port() < 0 ? 80 : searchUrl.port());
 	
	mCurrentLocation = searchUrl.path();
	if (searchUrl.hasQuery())
		mCurrentLocation.append("?").append(searchUrl.encodedQuery());

	iLog() << QString("MpiBlue: Sending http request for '%1'").arg(mCurrentLocation);
	mRequestId = mHttpHandler->get(mCurrentLocation, mTempFile);
}

//! \internale Downloads and imports the specified search results.
void MpiMovieImport::import(const QList<int>& list)
{
	if (list.isEmpty())
	{
		mImportDialog->done();
		return;
	}

	mImportsQueue = list;
	processNextImport();
}

/*! \internal Takes a new import job from the queue and processes it.
	Calls MvdwImportDialog::done() when no more imports are available.
*/
void MpiMovieImport::processNextImport()
{
	if (mImportsQueue.isEmpty())
	{
		mImportDialog->done();
		return;
	}

	int id = mImportsQueue.takeFirst();
	QHash<int,SearchResult>::Iterator it = mSearchResults.find(id);
	if (it == mSearchResults.end())
	{
		wLog() << "MpiBlue: Skipping invalid job";
		processNextImport();
		return;
	}

	iLog() << "MpiBlue: Processing job " << id;
	SearchResult& job = it.value();
	
	if (job.sourceType == CachedSource)
	{
		parseImdbMoviePage(job);
	}
}

//! \internal Parses a local IMDb movie page.
void MpiMovieImport::parseImdbMoviePage(SearchResult& job)
{
	QFile file(job.dataSource);
	if (!file.open(QIODevice::ReadOnly))
		return;
}

//! \internal Handles a QHttp::requestFinished() signal.
void MpiMovieImport::requestFinished(int id, bool error)
{
	Q_ASSERT(mHttpHandler);

	if (error)
	{
		if (mHttpHandler->error() == QHttp::Aborted)
			wLog() << "MpiBlue: Request aborted.";
		else eLog() << "MpiBlue: Request failed: " << mHttpHandler->errorString();

		if (mTempFile)
			deleteTemporaryFile(&mTempFile);

		mImportDialog->done();
		return;
	}

	if (id != mRequestId)
		return;

	if (!mTempFile)
	{
		eLog() << "MpiBlue: No temporary file!";
		return;
	}

	mTempFile->seek(0);
	parseQueryResponse();
	mImportDialog->done();
}

void MpiMovieImport::httpResponseHeader(const QHttpResponseHeader& responseHeader)
{
	Q_ASSERT(mHttpHandler);

	int status = responseHeader.statusCode();
	HttpStatusClass httpClass = (HttpStatusClass) int(floor(status / 100.0));

	switch (httpClass)
	{
	case RedirectionClass:
		// Redirect example: "Location: http://akas.imdb.com/title/tt0469754/"
		{
			QString location = responseHeader.value("Location");
			QUrl locationUrl(location);

			// Reset temp file
			if (mTempFile)
				deleteTemporaryFile(&mTempFile);
			mTempFile = createTemporaryFile();
			if (!mTempFile)
			{
				eLog() << "MpiBlue: Unable to create a temporary file.";
				mImportDialog->done();
				mHttpHandler->abort();
				return;
			}

			QString query = locationUrl.encodedQuery();
			mCurrentLocation = locationUrl.path();
			if (!query.isEmpty())
				mCurrentLocation.append("?").append(query);
			iLog() << "MpiBlue: Redirecting to " << mCurrentLocation;
			mRequestId = mHttpHandler->get(mCurrentLocation, mTempFile);
		}
		break;
	case SuccessClass:
		iLog() << "MpiBlue: Download finished.";
		break;
	default:
		; // Errors will be handled in the requestFinished() handler
	}
}

/*!
	Parses a query response. The response is stored in "mTempFile", the associated URL
	in "mCurrentLocation".
*/
void MpiMovieImport::parseQueryResponse()
{
	Q_ASSERT(mTempFile);
	iLog() << "MpiBlue: MpiMovieImport::parseQueryResponse()";

	MpiBlue::Engine* engine = mRegisteredEngines[mCurrentEngine];
	QString interpreter = MvdCore::locateApplication(engine->interpreter);
	if (interpreter.isEmpty())
	{
		eLog () << "MpiBlue: Failed to locate interpreter: " << engine->interpreter;
		mImportDialog->done();
		return;
	}

	if (!mInterpreter)
	{
		mInterpreter = new QProcess(this);
		connect(mInterpreter, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(interpreterFinished(int,QProcess::ExitStatus)));
		connect(mInterpreter, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(interpreterStateChanged(QProcess::ProcessState)));
	}
	Q_ASSERT(mInterpreter && mInterpreter->state() == QProcess::NotRunning);

	iLog() << QString("Starting the '%1' interpreter with the '%2' script on: ")
		.arg(interpreter).arg(engine->resultsScript).append(MvdCore::toLocalFilePath(mTempFile->fileName()));

	// mInterpreter->start(interpreter, QStringList() << engine->resultsScript << mTempFile->fileName());

	// Move to interpreterFinished()
	// deleteTemporaryFile(&mTempFile);

	mImportDialog->done();
}

//! Deletes a file and sets its pointer to 0. Asserts that file is not null.
void MpiMovieImport::deleteTemporaryFile(QTemporaryFile** file, bool removeFile)
{
	Q_ASSERT(file && *file);

	QString path = (*file)->fileName();
	delete *file;
	*file = 0;
	if (removeFile)
		QFile::remove(path);
}

//! Creates a new temporary file with AutoRemove = false. Returns 0 if an error occurs.
QTemporaryFile* MpiMovieImport::createTemporaryFile()
{
	QTemporaryFile* file = new QTemporaryFile(QDir::tempPath().append("/movida-import"));
	file->setAutoRemove(false);
	if (!file->open())
	{
		delete file;
		file = 0;
	}
	return file;
}

void MpiMovieImport::interpreterFinished(int exitCode, QProcess::ExitStatus exitStatus)
{

}

void MpiMovieImport::interpreterStateChanged(QProcess::ProcessState state)
{

}
