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
#include "settings.h"
#include "searchengine.h"
#include <QMessageBox>
#include <QRegExp>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QTemporaryFile>
#include <QProcess>
#include <QHttpRequestHeader>
#include <QDateTime>
#include <QLocale>
#include <math.h>

using namespace Movida;

MpiMovieImport::MpiMovieImport(QObject* parent)
: QObject(parent), mImportDialog(0), mHttpHandler(0), mRequestId(-1), mTempFile(0), mCurrentEngine(-1), 
mInterpreter(0), mCurrentState(NoState)
{
}

MpiMovieImport::~MpiMovieImport()
{
	if (mHttpHandler)
		mHttpHandler->abort();

	for (int i = 0; i < mTemporaryData.size(); ++i) {
		const QString& s = mTemporaryData.at(i);
		if (!QFile::remove(s)) {
			wLog() << "MpiMovieImport: failed to delete temporary file '" << s << "'.";
		}
	}
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
		iLog() << QString("MpiMovieImport: Engine %1 registered with id %2.").arg(e->name).arg(id);
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
	MpiBlue::Engine* engine = mRegisteredEngines.value(engineId);
	Q_ASSERT(engine);

	if (mCurrentState == NoState)
	{
		iLog() << QString("MpiMovieImport: Search request for query '%1' and engine '%2'").arg(query).arg(engine->name);

		mSearchResults.clear();
		mImportsQueue.clear();
		mCurrentImportJob = -1;

		mCurrentEngine = engineId;
		mCurrentQuery = query;

		if (!engine->scriptsFetched)
		{
			QString path = MpiBluePlugin::instance->dataStore(Movida::UserScope);
			if (path.isEmpty())
				path = MpiBluePlugin::instance->dataStore(Movida::SystemScope);
			QFileInfo fi(path);

			// We need to check for possible script updates and
			// replace the script file names with absolute, clean and localized paths.
			if (fi.isDir() && fi.isWritable() && MpiBlue::engineRequiresUpdate(*engine))
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

				// Prepare import script URL
				replacement = engine->importUrl.isEmpty() ? engine->importScript : engine->importUrl;
				mNextUrl = engineUpdateUrl;

				if (rep <= 0)
					mNextUrl.append(replacement);
				else mNextUrl.replace("{SCRIPT}", replacement);

				mTempFile = createTemporaryFile();
				if (!mTempFile)
				{
					mImportDialog->done();
					return;
				}

				initHttpHandler();
				Q_ASSERT(mHttpHandler);

				QUrl url(currentUrl);
				iLog() << QString("MpiMovieImport: Attempting a connection with %1:%2").arg(url.host()).arg(url.port() < 0 ? 80 : url.port());
				mHttpHandler->setHost(url.host(), url.port() < 0 ? 80 : url.port());

				mCurrentLocation = url.path();
				if (url.hasQuery())
					mCurrentLocation.append("?").append(url.encodedQuery());

				mCurrentState = FetchingResultsScriptState;

				mImportDialog->showMessage(tr("Looking for updated scripts."));

				QString date = scriptDate(engine->importScript);
				if (date.isEmpty()) {
					iLog() << "MpiMovieImport: Attempting to update results script: " << mCurrentLocation;
					mRequestId = mHttpHandler->get(mCurrentLocation, mTempFile);
				} else {
					iLog() << "MpiMovieImport: Attempting to update results script (if-mod-since " << date << "): " << mCurrentLocation;
					QHttpRequestHeader header(QLatin1String("GET"), mCurrentLocation);
					header.setValue(QLatin1String("Host"), url.host());
					header.setValue(QLatin1String("Connection"), QLatin1String("Keep-Alive"));
					header.setValue(QLatin1String("If-Modified-Since"), date);
					mRequestId = mHttpHandler->request(header, 0, mTempFile);
				}
			}
			else {
				// Cannot update scripts. Set absolute paths and go!
				engine->scriptsFetched = true;
				setScriptPaths(engine);
				performSearch(query, engine, engineId);
			}

		// Scripts fetched. We have the latest versions and absolute paths!
		} else performSearch(query, engine, engineId);
	}
	else if (mCurrentState == FetchingResultsScriptState)
	{
		QString path = MpiBluePlugin::instance->dataStore(Movida::UserScope);
		if (path.isEmpty())
			path = MpiBluePlugin::instance->dataStore(Movida::SystemScope);
		QFileInfo fi(path);

		ScriptStatus ss = isValidScriptFile();

		if (ss == InvalidScript || !fi.isDir() || !fi.isWritable()) {
			engine->scriptsFetched = true;
			setScriptPaths(engine);
			deleteTemporaryFile(&mTempFile, true);
			performSearch(query, engine, engineId);
			return;
		} else if (ss == ValidScript) {
			// copy the file to the user's script directory
			path.append(engine->resultsScript);
			QFile::remove(path);
			if (!mTempFile->rename(path)) {
				wLog() << "MpiMovieImport: Failed to save results script file:" << path;
				engine->scriptsFetched = true;
				setScriptPaths(engine);
				deleteTemporaryFile(&mTempFile, true);
				performSearch(query, engine, engineId);
				return;
			} else {
				Movida::settings().setValue(QString("plugins/blue/engines/%1/updated").arg(engine->name),
					QDateTime::currentDateTime().toString(Qt::ISODate));
				mImportDialog->showMessage(tr("Results parsing script updated."));
				iLog() << "MpiMovieImport: Results script file saved: " << path;
			}
		} else iLog() << "MpiMovieImport: No updated results script found.";

		deleteTemporaryFile(&mTempFile, true);

		mTempFile = createTemporaryFile();
		if (!mTempFile)
		{
			mImportDialog->done();
			return;
		}

		initHttpHandler();
		Q_ASSERT(mHttpHandler);

		QUrl url(mNextUrl);
		mNextUrl.clear();

		iLog() << QString("MpiMovieImport: Attempting a connection with %1:%2").arg(url.host()).arg(url.port() < 0 ? 80 : url.port());
		mHttpHandler->setHost(url.host(), url.port() < 0 ? 80 : url.port());

		mCurrentLocation = url.path();
		if (url.hasQuery())
			mCurrentLocation.append("?").append(url.encodedQuery());

		mCurrentState = FetchingImportScriptState;

		QString date = scriptDate(engine->importScript);
		if (date.isEmpty()) {
			iLog() << "MpiMovieImport: Attempting to update import script: " << mCurrentLocation;
			mRequestId = mHttpHandler->get(mCurrentLocation, mTempFile);
		} else {
			iLog() << "MpiMovieImport: Attempting to update import script (if-mod-since " << date << "): " << mCurrentLocation;
			QHttpRequestHeader header(QLatin1String("GET"), mCurrentLocation);
			header.setValue(QLatin1String("Host"), url.host());
			header.setValue(QLatin1String("Connection"), QLatin1String("Keep-Alive"));
			header.setValue(QLatin1String("If-Modified-Since"), date);
			mRequestId = mHttpHandler->request(header, 0, mTempFile);
		}
	}
	else if (mCurrentState == FetchingImportScriptState)
	{
		QString path = MpiBluePlugin::instance->dataStore(Movida::UserScope);
		if (path.isEmpty())
			path = MpiBluePlugin::instance->dataStore(Movida::SystemScope);
		QFileInfo fi(path);

		ScriptStatus ss = isValidScriptFile();

		if (ss == InvalidScript || !fi.isDir() || !fi.isWritable()) {
			engine->scriptsFetched = true;
			setScriptPaths(engine);
			deleteTemporaryFile(&mTempFile, true);
			performSearch(query, engine, engineId);
			return;
		} else if (ss == ValidScript) {
			// copy the file to the user's script directory
			path.append(engine->importScript);
			QFile::remove(path);
			if (!mTempFile->rename(path)) {
				wLog() << "MpiMovieImport: Failed to save import script file:" << path;
				engine->scriptsFetched = true;
				setScriptPaths(engine);
				deleteTemporaryFile(&mTempFile, true);
				performSearch(query, engine, engineId);
				return;
			} else {
				Movida::settings().setValue(QString("plugins/blue/engines/%1/updated").arg(engine->name),
					QDateTime::currentDateTime().toString(Qt::ISODate));
				mImportDialog->showMessage(tr("Import script updated."));
				iLog() << "MpiMovieImport: Import script file saved: " << path;
			}
		} else iLog() << "MpiMovieImport: No updated import script found.";

		deleteTemporaryFile(&mTempFile, true);

		mCurrentState = FetchingResultsState;

		if (engine->updateInterval != MpiBlue::UpdateAlways)
			engine->scriptsFetched = true;
		setScriptPaths(engine);
		performSearch(query, engine, engineId);
	}
}

//! Do not call this directly. Use search() as it will perform some initialization.
void MpiMovieImport::performSearch(const QString& query, MpiBlue::Engine* engine, int engineId)
{
	Q_ASSERT(engine && !mTempFile);

	mImportDialog->showMessage(tr("All scripts updated. Sending query."));

	iLog() << QString("MpiMovieImport: performing search for query '%1' and engine %2").arg(query).arg(engine->name);
	Q_ASSERT(engine->scriptsFetched);

	QString searchUrlString = engine->searchUrl;
	searchUrlString.replace("{QUERY}", QString::fromLatin1(MvdCore::toLatin1PercentEncoding(query)));

	QUrl searchUrl(searchUrlString);

	if (searchUrl.host().isEmpty())
	{
		eLog() << "MpiMovieImport: Invalid search engine host";
		mImportDialog->done();
		return;
	}

	mTempFile = createTemporaryFile();
	if (!mTempFile)
	{
		mImportDialog->done();
		return;
	}
	
	mCurrentEngine = engineId;

	initHttpHandler();
	Q_ASSERT(mHttpHandler);

	iLog() << QString("MpiMovieImport: Attempting a connection with %1:%2").arg(searchUrl.host()).arg(searchUrl.port() < 0 ? 80 : searchUrl.port());
	mHttpHandler->setHost(searchUrl.host(), searchUrl.port() < 0 ? 80 : searchUrl.port());
 	
	mCurrentLocation = searchUrl.path();
	if (searchUrl.hasQuery())
		mCurrentLocation.append("?").append(searchUrl.encodedQuery());

	mCurrentState = FetchingResultsState;

	iLog() << QString("MpiMovieImport: Sending http request for '%1'").arg(mCurrentLocation);
	mRequestId = mHttpHandler->get(mCurrentLocation, mTempFile);
}

//! Locates the latest version of an engine's scripts and sets the absolute path
void MpiMovieImport::setScriptPaths(MpiBlue::Engine* engine)
{
	QString path = locateScriptPath(engine->resultsScript);
	engine->resultsScript = path;
	path = locateScriptPath(engine->importScript);
	engine->importScript = path;
}

//! Returns the absolute, localized, clean path of the possibly most updated version of a script. (phew!)
QString MpiMovieImport::locateScriptPath(const QString& name) const
{
	Q_ASSERT(MpiBluePlugin::instance);

	// Search order: plugin's user data store, plugin's global data store

	QString filename;

	// plugin's user data store
	QString dataStore = MpiBluePlugin::instance->dataStore(Movida::UserScope);
	filename = QString(dataStore).append(name);
	if (QFile::exists(filename) && isValidScriptFile(filename) == ValidScript)
		return MvdCore::toLocalFilePath(filename);

	// global data store
	dataStore = MpiBluePlugin::instance->dataStore(Movida::SystemScope);
	filename = QString(dataStore).append(name);
	if (QFile::exists(filename) && isValidScriptFile(filename) == ValidScript)
		return MvdCore::toLocalFilePath(filename);

	return QString();
}

/*! Returns the file modification time of the script with the given name or an
	empty string if the script could not be found.
	The date is returned in HTTP-DATE format (see RFC 2616, section 3.3.1).
*/
QString MpiMovieImport::scriptDate(const QString& name) const
{
	QString script = locateScriptPath(name);
	if (script.isEmpty())
		return QString();

	QFileInfo fi(script);
	QDateTime dt = fi.lastModified().toTimeSpec(Qt::UTC);
	QLocale locale(QLocale::English, QLocale::UnitedStates);
	QString date = locale.toString(dt.date(), MvdCore::parameter("plugins/blue/http-date").toString());
	QString time = locale.toString(dt.time(), MvdCore::parameter("plugins/blue/http-time").toString());
	return date.append(" ").append(time);
}

//! Checks whether \p path points to a (possibly) valid script file by verifying the signature.
MpiMovieImport::ScriptStatus MpiMovieImport::isValidScriptFile(const QString& path) const
{
	QTextStream stream;
	QFile* file = 0;

	if (path.isNull()) {
		if (mHttpNotModified)
			return NoUpdatedScript;
		stream.setDevice(mTempFile);
	}
	else {
		file = new QFile(path);
		if (!file->open(QIODevice::ReadOnly))
		{
			eLog() << "MpiMovieImport: Failed to open script file: " << path;
			delete file;
			return InvalidScript;
		}
		stream.setDevice(file);
	}

	QString signature = MvdCore::parameter("plugins/blue/script-signature").toString();
	
	QString line;
	bool valid = false;
	int maxLines = 10;
	int lineCount = 0;
	while (++lineCount <= maxLines && !valid && !(line = stream.readLine()).isNull()) {
		valid = line.contains(signature);
	}

	delete file;

	if (!valid) {
		if (path.isNull())
			eLog() << "MpiMovieImport: Downloaded file is not a valid script file.";
		else eLog() << "MpiMovieImport: Invalid script file: " << path;
	}

	return valid ? ValidScript : InvalidScript;
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
	iLog() << "MvdMovieImport: Received import request for " << list.size() << " movies.";
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

	bool ok = false;
	int id;
	QHash<int,SearchResult>::Iterator it;

	while (!ok && !mImportsQueue.isEmpty()) {
		id = mImportsQueue.takeFirst();
		it = mSearchResults.find(id);
		if (it == mSearchResults.end()) {
			wLog() << "MpiMovieImport: Skipping invalid job";
			continue;
		}
		ok = true;
	}

	if (!ok) {
		mImportDialog->done();
		return;
	}

	iLog() << "MpiMovieImport: Processing job " << id;
	mCurrentImportJob = id;
	SearchResult& job = it.value();
	
	if (job.sourceType == CachedSource) {
		parseCachedMoviePage(job);
	} else {
		// download movie page
		QUrl url(job.dataSource);

		mTempFile = createTemporaryFile();
		if (!mTempFile) {
			mImportDialog->done();
			return;
		}

		initHttpHandler();
		Q_ASSERT(mHttpHandler);

		iLog() << QString("MpiMovieImport: Attempting a connection with %1:%2").arg(url.host()).arg(url.port() < 0 ? 80 : url.port());
		mHttpHandler->setHost(url.host(), url.port() < 0 ? 80 : url.port());
	 	
		mCurrentLocation = url.path();
		if (url.hasQuery())
			mCurrentLocation.append("?").append(url.encodedQuery());

		mCurrentState = FetchingMovieDataState;

		iLog() << QString("MpiMovieImport: Sending http request for '%1'").arg(mCurrentLocation);
		mRequestId = mHttpHandler->get(mCurrentLocation, mTempFile);
	}
}

//! \internal Handles a QHttp::httpRequestFinished() signal.
void MpiMovieImport::httpRequestFinished(int id, bool error)
{
	Q_ASSERT(mHttpHandler);
	bool hasPending = mHttpHandler->hasPendingRequests();
	Q_UNUSED(hasPending);

	if (error)
	{
		if (mHttpHandler->error() == QHttp::Aborted)
			wLog() << "MpiMovieImport: Http request aborted.";
		else eLog() << "MpiMovieImport: Http request failed: " << mHttpHandler->errorString();

		if (mTempFile)
			deleteTemporaryFile(&mTempFile);

		if (mHttpHandler->error() == QHttp::Aborted)
			return;

		// some states can be considered optionals. e.g. we don't care if a script or
		// poster download failed.
		if (mCurrentState == FetchingMoviePosterState) {
			Q_ASSERT(QMetaObject::invokeMethod(this, "processMoviePoster", Qt::QueuedConnection));
		} else if (mCurrentState == FetchingResultsScriptState || mCurrentState == FetchingImportScriptState) {
			MpiBlue::Engine* engine = mRegisteredEngines.value(mCurrentEngine);
			engine->scriptsFetched = true;
			engine->updateUrl.clear(); // Avoid to perform another update attempt.
			mCurrentState = NoState;
			Q_ASSERT(QMetaObject::invokeMethod(this, "search", Qt::QueuedConnection,
				Q_ARG(QString, mCurrentQuery), 
				Q_ARG(int, mCurrentEngine)));
		} else {
			Q_ASSERT(QMetaObject::invokeMethod(this, "done", Qt::QueuedConnection));
		}
		return;
	}

	int status = mHttpHandler->lastResponse().statusCode();
	mHttpNotModified = status == HttpNotModified;

	// Host lookup or some side request.
	if (id != mRequestId) {
		iLog() << "MpiMovieImport: Http host lookup finished.";
		return;
	} else {
		iLog() << "MpiMovieImport: Http request finished.";
	}

	if (!mTempFile)
	{
		eLog() << "MpiMovieImport: No temporary file found!";
		mImportDialog->showMessage(tr("Sorry, but some internal error occurred."), MvdImportDialog::ErrorMessage);
		mImportDialog->done();
		return;
	}

	mTempFile->seek(0);

	switch (mCurrentState) {
		case FetchingResultsScriptState:
		case FetchingImportScriptState:
			Q_ASSERT(QMetaObject::invokeMethod(this, "search", Qt::QueuedConnection,
				Q_ARG(QString, mCurrentQuery), 
				Q_ARG(int, mCurrentEngine)));
			break;
		case FetchingResultsState:
			Q_ASSERT(QMetaObject::invokeMethod(this, "processResponseFile", Qt::QueuedConnection));
			break;
		case FetchingMovieDataState:
			Q_ASSERT(QMetaObject::invokeMethod(this, "processResponseFile", Qt::QueuedConnection));
			break;
		case FetchingMoviePosterState:
			Q_ASSERT(QMetaObject::invokeMethod(this, "processMoviePoster", Qt::QueuedConnection));
			break;
		default: ; 
	}
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
		if (status == HttpNotModified) {
			iLog() << "MpiMovieImport: File not modified.";
		} else {
			QString location = responseHeader.value("Location");
			QUrl locationUrl(location);

			// Reset temp file
			if (mTempFile)
				deleteTemporaryFile(&mTempFile);
			mTempFile = createTemporaryFile();
			if (!mTempFile)
			{
				mImportDialog->done();
				mHttpHandler->abort();
				return;
			}

			QString query = locationUrl.encodedQuery();
			mCurrentLocation = locationUrl.path();
			if (!query.isEmpty())
				mCurrentLocation.append("?").append(query);
			iLog() << "MpiMovieImport: Redirecting to " << mCurrentLocation;
			mRequestId = mHttpHandler->get(mCurrentLocation, mTempFile);
		}
		break;
	case SuccessClass:
		iLog() << "MpiMovieImport: Successful request header received.";
		break;
	default:
		wLog() << "MpiMovieImport: Unsuccessful request header received. Http status code " << status;
		; // Errors will be handled in the httpRequestFinished() handler
	}
}

//! Logs http status changes.
void MpiMovieImport::httpStateChanged(int state)
{
	switch (state)
	{
	case QHttp::HostLookup:
		iLog() << "MpiMovieImport: Performing host name lookup.";
		break;
	case QHttp::Connecting:
		iLog() << "MpiMovieImport: Started establishing a connection.";
		break;
	case QHttp::Connected:
		iLog() << "MpiMovieImport: Connection established.";
		break;
	case QHttp::Closing:
		iLog() << "MpiMovieImport: Socket is about to close.";
		break;
	default: ;
	}
}

/*!
	Parses a query response. The response is stored in "mTempFile", the associated URL
	in "mCurrentLocation".
*/
void MpiMovieImport::processResponseFile()
{
	Q_ASSERT(mTempFile);
	if (mCurrentState == FetchingResultsState)
		mImportDialog->showMessage(tr("Attempting to parse search results."));
	else mImportDialog->showMessage(tr("Attempting to parse movie data."));

	MpiBlue::Engine* engine = mRegisteredEngines[mCurrentEngine];
	QString interpreter = MvdCore::locateApplication(engine->interpreter);
	if (interpreter.isEmpty())
	{
		eLog () << "MpiMovieImport: Failed to locate interpreter: " << engine->interpreter;
		mImportDialog->showMessage(tr("Failed to locate '%1' interpreter.").arg(engine->interpreter),
			MvdImportDialog::ErrorMessage);
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

	QString script = mCurrentState == FetchingResultsState ? engine->resultsScript : engine->importScript;

	iLog() << QString("Starting the '%1' interpreter with the '%2' script on: ")
		.arg(interpreter).arg(script).append(MvdCore::toLocalFilePath(mTempFile->fileName()));

	QFileInfo intFi(interpreter);
	mInterpreterName = intFi.baseName();
	mInterpreter->start(interpreter, QStringList() << script << MvdCore::toLocalFilePath(mTempFile->fileName()));
}

/*!
	Sets the movie poster path in the current job and completes the job.
*/
void MpiMovieImport::processMoviePoster()
{
	if (!mTempFile) {
		mImportDialog->showMessage(tr("Failed to download movie poster."), 
			MvdImportDialog::ErrorMessage);
		completeJob();
		return;
	}

	Q_ASSERT(mSearchResults.contains(mCurrentImportJob));
	SearchResult& result = mSearchResults[mCurrentImportJob];

	result.data.posterPath = mTempFile->fileName();
	mTemporaryData.append(mTempFile->fileName());
	deleteTemporaryFile(&mTempFile, false);

	mImportDialog->showMessage(tr("Movie poster downloaded."));
	completeJob();
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
	
	if (!file)
		eLog() << "MpiMovieImport: Failed to create a temporary file";

	return file;
}

void MpiMovieImport::interpreterFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	Q_ASSERT(mInterpreter);
	Q_UNUSED(exitCode);

	// Log interpreter output
	QString line;

	QByteArray buffer = mInterpreter->readAllStandardError();
	{
		QTextStream bufferIn(buffer);
		while ( !(line = bufferIn.readLine()).isNull() ) {
			eLog() << mInterpreterName << ": " << line;
		}
	}

	buffer = mInterpreter->readAllStandardOutput();
	{
		QTextStream bufferIn(buffer);
		while ( !(line = bufferIn.readLine()).isNull() ) {
			iLog() << mInterpreterName << ": " << line;
		}
	}

	if (exitStatus != QProcess::NormalExit) {
		if (mTempFile)
			deleteTemporaryFile(&mTempFile);

		mImportDialog->showMessage(tr("Script interpreter crashed."), 
			MvdImportDialog::ErrorMessage);
		eLog() << "MpiMovieImport: Interpreter crashed.";
		mImportDialog->done();
		return;
	}

	// Cached sources do not use the temporary file for the import process.
	QString filePath = mTempFile ? mTempFile->fileName() : mSearchResults[mCurrentImportJob].dataSource;

	QFileInfo info(filePath);
	QString xmlPath = info.absolutePath().append(mCurrentState == FetchingResultsState ? "/mvdmres.xml" : "/mvdmdata.xml");

	if (!QFile::exists(xmlPath)) {
		if (mTempFile)
			deleteTemporaryFile(&mTempFile);

		mImportDialog->showMessage(tr("Failed to parse search results."), 
			MvdImportDialog::ErrorMessage);
		eLog() << "MpiMovieImport: No search results file found (" << xmlPath << ").";
		mImportDialog->done();
		return;
	}

	if (mCurrentState == FetchingResultsState) {
		// Control returns to the wizard by calling done()
		processResultsFile(xmlPath);
	} else {
		// Control returns to this plugin by asynchronously calling processNextImport()
		processMovieDataFile(xmlPath);
	}
}

void MpiMovieImport::interpreterStateChanged(QProcess::ProcessState state)
{
	Q_ASSERT(mInterpreter);

	QString s = "unknown state";
	switch (state) {
		case QProcess::NotRunning: s = "not running"; break;
		case QProcess::Running: s = "running"; break;
		case QProcess::Starting: s = "starting"; break;
		default: ;
	}

	iLog() << "MpiMovieImport: Interpreter state changed to " << s;
}

//! \internal Creates a new http handler if necessary and clears any pending requests.
void MpiMovieImport::initHttpHandler()
{
	if (!mHttpHandler)
	{
		//! \todo Handle proxy in the main application (best with Qt >= 4.3)
		mHttpHandler = new QHttp(this);

		connect(mHttpHandler, SIGNAL(requestFinished(int, bool)),
			this, SLOT(httpRequestFinished(int, bool)));
		connect(mHttpHandler, SIGNAL(responseHeaderReceived(const QHttpResponseHeader&)),
			this, SLOT(httpResponseHeader(const QHttpResponseHeader&)));
		connect(mHttpHandler, SIGNAL(stateChanged(int)),
			this, SLOT(httpStateChanged(int)));

	} else mHttpHandler->clearPendingRequests();

	mHttpNotModified = false;
}

//! Parses a mvdresults.xml file and queues import jobs (if any).
void MpiMovieImport::processResultsFile(const QString& path)
{
	mImportDialog->showMessage(tr("Processing search results."));

	xmlDocPtr doc = xmlParseFile(path.toLatin1().constData());
	if (!doc) {
		QFile::remove(path);
		eLog() << "MpiMovieImport: Invalid search results file.";
		mImportDialog->showMessage(tr("Invalid search results file."),
			MvdImportDialog::ErrorMessage);
		mImportDialog->done();
		return;
	}

	xmlNodePtr node = xmlDocGetRootElement(doc);
	if (xmlStrcmp(node->name, (const xmlChar*) "movida-movie-results")) {
		QFile::remove(path);
		eLog() << "MpiMovieImport: Invalid search results file.";
		mImportDialog->showMessage(tr("Invalid search results file."),
			MvdImportDialog::ErrorMessage);
		mImportDialog->done();
		return;
	}

	bool hasCachedResults = parseSearchResults(doc, node->xmlChildrenNode, path);
	QFile::remove(path);

	if (!hasCachedResults && mTempFile)
		deleteTemporaryFile(&mTempFile);

	mImportDialog->showMessage(tr("Done."));
	mImportDialog->done();
}

//! Parses search result nodes, possibly using recursion on <group> nodes.
bool MpiMovieImport::parseSearchResults(xmlDocPtr doc, xmlNodePtr node, const QString& path, const QString& group)
{
	bool hasCachedResults = false;
	bool groupAdded = false;

	while (node) {
		if (node->type != XML_ELEMENT_NODE) {
			node = node->next;
			continue;
		}

		//! \todo Add a "expanded" attribute to force a group to be expanded in the tree view?
		if (!xmlStrcmp(node->name, (const xmlChar*) "group")) {
			QString name;
			xmlChar* attr = xmlGetProp(node, (const xmlChar*)"name");
			if (attr) {
				name = QString::fromUtf8((const char*)attr).trimmed();
				xmlFree(attr);
			}
			if (parseSearchResults(doc, node->xmlChildrenNode, path, name))
				hasCachedResults = true;
			node = node->next;
			continue;
		} else if (xmlStrcmp(node->name, (const xmlChar*) "result")) {
			node = node->next;
			continue;
		}

		SearchResult result;
		QString notes;

		xmlNodePtr resultNode = node->xmlChildrenNode;
		while (resultNode) {
			if (resultNode->type != XML_ELEMENT_NODE) {
				resultNode = resultNode->next;
				continue;
			}

			if (!xmlStrcmp(resultNode->name, (const xmlChar*) "source")) {
				xmlChar* attr = xmlGetProp(resultNode, (const xmlChar*) "type");
				if (attr) {
					if (!xmlStrcmp(attr, (const xmlChar*) "remote"))
						result.sourceType = RemoteSource;
					else if (!xmlStrcmp(attr, (const xmlChar*) "cached"))
						result.sourceType = CachedSource;
					xmlFree(attr);
				}
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "title")) {
				xmlChar* text = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
				if (text)
					result.data.title = MvdCore::decodeXmlEntities(QString::fromUtf8((const char*)text).trimmed());
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "year")) {
				xmlChar* text = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
				if (text)
					result.data.productionYear = QString::fromLatin1((const char*)text).trimmed();
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "url")) {
				xmlChar* text = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
				if (text)
					result.dataSource = QString::fromLatin1((const char*)text).trimmed();
			} else if (!xmlStrcmp(resultNode->name, (const xmlChar*) "notes")) {
				xmlChar* text = xmlNodeListGetString(doc, resultNode->xmlChildrenNode, 1);
				if (text)
					notes = MvdCore::decodeXmlEntities(QString::fromUtf8((const char*)text).trimmed());
			}

			resultNode = resultNode->next;
		}

		if (isValidResult(result, path)) {
			if (result.sourceType == CachedSource)
				hasCachedResults = true;
			if (!groupAdded && !group.isEmpty()) {
				mImportDialog->addSection(group);
				groupAdded = true;
			}
			int id = mImportDialog->addMatch(result.data.title, result.data.productionYear, notes);
			mSearchResults.insert(id, result);
		}

		node = node->next;
	}

	return hasCachedResults;
}

//! \internal Parses a local IMDb movie page.
void MpiMovieImport::parseCachedMoviePage(SearchResult& job)
{
	mImportDialog->showMessage(tr("Attempting to parse movie data."));

	MpiBlue::Engine* engine = mRegisteredEngines[mCurrentEngine];
	QString interpreter = MvdCore::locateApplication(engine->interpreter);
	if (interpreter.isEmpty())
	{
		eLog () << "MpiMovieImport: Failed to locate interpreter: " << engine->interpreter;
		mImportDialog->showMessage(tr("Failed to locate '%1' interpreter.").arg(engine->interpreter),
			MvdImportDialog::ErrorMessage);
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

	mCurrentState = FetchingMovieDataState;
	QString script = engine->importScript;

	iLog() << QString("Starting the '%1' interpreter with the '%2' script on: ")
		.arg(interpreter).arg(script).append(MvdCore::toLocalFilePath(job.dataSource));

	QFileInfo intFi(interpreter);
	mInterpreterName = intFi.baseName();
	mInterpreter->start(interpreter, QStringList() << script << MvdCore::toLocalFilePath(job.dataSource));
}

//! Parses a mvdmovies.xml file and adds the movie to the import queue.
void MpiMovieImport::processMovieDataFile(const QString& path)
{
	mImportDialog->showMessage(tr("Processing movie data."));
	Q_ASSERT(mSearchResults.contains(mCurrentImportJob));
	SearchResult& result = mSearchResults[mCurrentImportJob];
	if (!result.data.loadFromXml(path, MvdMovieData::StopAtFirstMovie)) {
		mSearchResults.remove(mCurrentImportJob);
		mImportDialog->showMessage(tr("Discarding invalid movie data."));
		Q_ASSERT(QMetaObject::invokeMethod(this, "processNextImport", Qt::QueuedConnection));
		return;
	}

	if (!result.data.posterPath.isEmpty() && result.data.posterPath.startsWith("http://")) {
		mTempFile = createTemporaryFile();
		if (!mTempFile)
		{
			//! \todo reset everything (eg. jobs) before calling done after an error.
			mImportDialog->done();
			return;
		}

		initHttpHandler();
		Q_ASSERT(mHttpHandler);

		QUrl url(result.data.posterPath);

		iLog() << QString("MpiMovieImport: Attempting a connection with %1:%2").arg(url.host()).arg(url.port() < 0 ? 80 : url.port());
		mHttpHandler->setHost(url.host(), url.port() < 0 ? 80 : url.port());
	 	
		mCurrentLocation = url.path();
		if (url.hasQuery())
			mCurrentLocation.append("?").append(url.encodedQuery());

		mCurrentState = FetchingMoviePosterState;

		QString s = result.data.title.isEmpty() ? result.data.originalTitle : result.data.title;
		mImportDialog->showMessage(tr("Downloading movie poster for movie '%1'.").arg(s));
		iLog() << "MpiMovieImport: Downloading movie poster for movie " << s << ".";
		iLog() << QString("MpiMovieImport: Sending http request for '%1'").arg(mCurrentLocation);
		mRequestId = mHttpHandler->get(mCurrentLocation, mTempFile);
	} else {
		completeJob();
	}
}

//! Adds the movie to the import wizard and starts the next job (if any).
void MpiMovieImport::completeJob()
{
	Q_ASSERT(mSearchResults.contains(mCurrentImportJob));
	SearchResult& result = mSearchResults[mCurrentImportJob];

	QString s = result.data.title.isEmpty() ? result.data.originalTitle : result.data.title;
	mImportDialog->showMessage(tr("Movie '%1' processed.").arg(s));
	mImportDialog->addMovieData(result.data);
	mSearchResults.remove(mCurrentImportJob);
	
	Q_ASSERT(QMetaObject::invokeMethod(this, "processNextImport", Qt::QueuedConnection));
}

//! Returns true if the search result contains all required values and possibly sets the data source for cached sources.
bool MpiMovieImport::isValidResult(SearchResult& result, const QString& path)
{
	if (result.data.title.isEmpty())
		return false;
	if (result.sourceType == CachedSource) {
		if (result.dataSource.isEmpty())
			result.dataSource = path;
	} else if (result.dataSource.isEmpty())
		return false;
	else {
		QUrl url(result.dataSource);
		if (url.scheme() != "http")
			return false;
	}

	return true;
}
