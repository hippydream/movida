/**************************************************************************
** Filename: basic.cpp
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

#include "basic.h"

#include <movidacore/core.h>
#include <movidawidgets/importdialog.h>

#include <QMessageBox>
#include <QRegExp>
#include <QPushButton>
#include <QUrl>
#include <QDir>
#include <QTemporaryFile>

#include <math.h>

MvdBasicMpi::MvdBasicMpi(QObject* parent)
: MvdPluginInterface(parent), http(0),
importDialog(0), startPageUi(0), currentRequest(NoRequest),
httpGetId(0), currentTempFile(0)
{	
}

MvdBasicMpi::~MvdBasicMpi()
{
	QList<QString> tempFiles = downloadedMovies.values();
	for (int i = 0; i < tempFiles.size(); ++i)
		QFile::remove(tempFiles.at(i));
}

bool MvdBasicMpi::init()
{
	QHash<QString, QVariant> parameters;
	parameters.insert("basicmpi-imdb-host", "akas.imdb.com");
	parameters.insert("basicmpi-imdb-find-movie", "/find?s=tt&q=%1");
	MvdCore::registerParameters(parameters);

	return true;
}

void MvdBasicMpi::unload()
{

}

QString MvdBasicMpi::lastError() const
{
	return QString();
}

MvdPluginInterface::PluginInfo MvdBasicMpi::info() const
{
	MvdPluginInterface::PluginInfo info;
	info.name = "Movida basic";
	info.description = "Basic import/export plugin";
	info.author = "Fabrizio Angius";
	info.version = "0.1";
	return info;
}

QList<MvdPluginInterface::PluginAction*> MvdBasicMpi::actions() const
{
	QList<MvdPluginInterface::PluginAction*> list;
	
	MvdPluginInterface::PluginAction* a = new MvdPluginInterface::PluginAction;
	a->text = "IMDb movie import";
	a->helpText = "Import movies from the IMDb website.";
	a->name = "imdb-import";
	a->type = MvdPluginInterface::MovieImportAction;
	list << a;

	return list;
}

void MvdBasicMpi::actionTriggeredImplementation(const QString& name)
{
	if (name == "imdb-import")
		imdbMovieImport();
}

void MvdBasicMpi::imdbMovieImport()
{
	QWidget* p = qobject_cast<QWidget*>(this->parent());
	
	MvdwImportDialog id(p);
		
	QWidget* startPage = id.startPage();
	Ui::MvdImdbImportStart ui;
	ui.setupUi(startPage);

	// Store some convenience pointers.
	// Note: this can be done because we will reset the pointers before leaving
	// this method!!
	importDialog = &id;
	startPageUi = &ui;

	startPageUi->input->setFocus(Qt::PopupFocusReason);
	startPageUi->input->setText("Direktren for det hele");
	validateQuery("Direktren for det hele");

	connect( ui.input, SIGNAL(textChanged(const QString&)), 
		this, SLOT(validateQuery(const QString&)) );
	connect( importDialog, SIGNAL(searchTriggered()), 
		this, SLOT(showImportPage()) );
	connect( importDialog, SIGNAL(showStartPageTriggered()), 
		this, SLOT(showStartPage()) );
	connect( ui.input, SIGNAL(returnPressed()), 
		this, SLOT(queryReturnPressed()) );
	connect( importDialog, SIGNAL(movieRequired(const QUuid&)), 
		this, SLOT(loadMovie(const QUuid&)) );

	id.exec();

	// Reset convenience pointers	
	importDialog = 0;
	startPageUi = 0;
}

//! Sends a movie query to the IMDb website.
void MvdBasicMpi::searchImdbMovie(const QString& name)
{
	Q_ASSERT(importDialog);
	Q_ASSERT(!currentTempFile);

	currentTempFile = new QTemporaryFile(QDir::tempPath() + "/imdb-movie-page", this);
	currentTempFile->setAutoRemove(false);
	if (!currentTempFile->open())
	{
		importDialog->setStatus(tr("Sorry, but an error occurred.\n(Failed to create a temporary file)"));
		resetImportPage(false);
		delete currentTempFile;
		currentTempFile = 0;
		return;
	}

	initHttp();

	currentRequest = SearchMovieRequest;
	importDialog->setBusyStatus(true);
	importDialog->setStatus(tr("Searching for '%1'...").arg(name));

	QByteArray encodedName = MvdCore::toLatin1PercentEncoding(name);
	QByteArray path = MvdCore::parameter("basicmpi-imdb-find-movie")
		.toString().arg(QString::fromLatin1(encodedName)).toLatin1();

	httpGetId = http->get(path, currentTempFile);
}

void MvdBasicMpi::retrieveImdbMovie(const QString& id)
{
	if (id.isEmpty())
		return;

	QMessageBox::information(0, "", "retrieveImdbMovie: " + id);
	initHttp();
}

void MvdBasicMpi::initHttp()
{
	if (!http)
	{
		http = new QHttp(this);

		connect(http, SIGNAL(requestFinished(int, bool)), 
			this, SLOT(httpRequestFinished(int, bool)));
		connect(http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader&)), 
			this, SLOT(readResponseHeader(const QHttpResponseHeader&)));
		connect(http, SIGNAL(dataReadProgress(int, int)), 
			this, SLOT(httpDataReadProgress(int, int)));

		//! \todo Handle proxy (best with Qt >= 4.3)
		http->setHost(MvdCore::parameter("basicmpi-imdb-host").toString());
	}
}

void MvdBasicMpi::readResponseHeader(const QHttpResponseHeader& responseHeader)
{
	int status = responseHeader.statusCode();
	HttpStatusClass httpClass = (HttpStatusClass) int(floor(status / 100.0));

	switch (httpClass)
	{
	case RedirectionClass:
		// Redirect! Location: http://akas.imdb.com/title/tt0469754/
		{
			QString location = responseHeader.value("Location");
			QUrl locationUrl(location);
			currentTempFile->seek(0);
			location = locationUrl.path().append("?").append(locationUrl.encodedQuery());
			httpGetId = http->get(location, currentTempFile);
			importDialog->setStatus(tr("Single match, retrieving movie info."));
			currentRequest = FetchMovieRequest;
		}
		break;
	case SuccessClass:
		break;
	default:
		importDialog->setStatus(tr("Sorry, an error occurred (Code %1: %2).\nPlease <a href='movida://httpget/search'>click here</a> to retry.")
			.arg(responseHeader.statusCode()).arg(responseHeader.reasonPhrase()));
		http->abort();

		if (currentTempFile)
		{
			QFileInfo info(*currentTempFile);
			delete currentTempFile;
			currentTempFile = 0;
			QFile::remove(info.absoluteFilePath());			
		}
		resetImportPage(false);
	}
}

//!
void MvdBasicMpi::httpDataReadProgress(int data, int total)
{

}

//! Parses the result received from IMDb.
void MvdBasicMpi::httpRequestFinished(int id, bool error)
{
	if (id != httpGetId)
		return;

	HttpRequest lastRequest = currentRequest;
	currentRequest = NoRequest;

	if (error)
	{
		importDialog->setStatus(tr("Sorry, an error occurred (%1).\nPlease <a href='movida://httpget/search'>click here</a> to retry.").arg(http->errorString()));
		resetImportPage(false);
		return;
	}

	if (lastRequest == SearchMovieRequest)
	{		
		importDialog->setStatus(
			tr("Please select one or more results and press the import button."));
		resetImportPage(true);

		QFileInfo info(*currentTempFile);
		QFile::remove(info.absoluteFilePath());
		delete currentTempFile;
		currentTempFile = 0;
	}
	else if (lastRequest == FetchMovieRequest)
	{
		importDialog->setStatus(tr("Movie downloaded."));
		QString title = imdbMovieExtractTitle();
		QUuid id;
		if (!title.isEmpty())
			id = importDialog->addSearchResult(title);
		resetImportPage(true);

		Q_ASSERT(currentTempFile);
		QFileInfo info(*currentTempFile);
		QString tempFilePath = info.absoluteFilePath();
		delete currentTempFile;
		currentTempFile = 0;

		// Store filename so we can parse the movie details as soon as necessary
		if (id.isNull())
			QFile::remove(tempFilePath);
		else downloadedMovies.insert(id, tempFilePath);
	}
}

void MvdBasicMpi::resetImportPage(bool success)
{
	Q_ASSERT(importDialog);
	importDialog->setBusyStatus(false);
	importDialog->setBackButtonEnabled(true);
}

bool MvdBasicMpi::validateQuery(const QString& query)
{
	Q_ASSERT(importDialog);
	bool valid = !query.trimmed().isEmpty();
	importDialog->setSearchButtonEnabled(valid);
	return valid;
}

void MvdBasicMpi::queryReturnPressed()
{
	Q_ASSERT(importDialog);
	Q_ASSERT(startPageUi);

	// Temporary fix. "startPageUi->input"'s enterPressed signal is being emitted even if
	// we are in the import page and the widget is thus hidden. This appears to occur
	// After we return to the start page using the "New search" button and hit ENTER
	// to start a new query in the start page.
	if (!startPageUi->input->isVisible())
		return;

	if (validateQuery(startPageUi->input->text()))
		showImportPage();
	else QApplication::beep();
}

//! Shows the import page and sends a search/download request to IMDb.
void MvdBasicMpi::showImportPage()
{
	Q_ASSERT(importDialog);

	importDialog->showImportPage();

	// Send query to IMDb
	QString query = startPageUi->input->text().trimmed();

	QRegExp rx(MvdCore::parameter("movidacore-imdb-id-regexp").toString());
	if (rx.exactMatch(query))
	{
		retrieveImdbMovie(query);
	}
	else searchImdbMovie(query);
}

void MvdBasicMpi::showStartPage()
{
	Q_ASSERT(importDialog);
	Q_ASSERT(startPageUi);

	downloadedMovies.clear();
	importDialog->showStartPage();

	startPageUi->input->setFocus(Qt::PopupFocusReason);
}

//! Downloads (if necessary) and imports the selected movies.
void MvdBasicMpi::import()
{
	Q_ASSERT(importDialog);
	Q_ASSERT(startPageUi);	
}

//! Interrupts the current operation and closes the dialog.
void MvdBasicMpi::abortRequest()
{
	// TEMPORARY!
	// showStartPage();
}

//! Public interface for this plugin.
MvdPluginInterface* pluginInterface(QObject* parent)
{
	return new MvdBasicMpi(parent);
}

//! Parses a IMDb movie page and returns the movie title.
QString MvdBasicMpi::imdbMovieExtractTitle()
{
	Q_ASSERT(currentTempFile);
	currentTempFile->seek(0);
	QByteArray data = currentTempFile->readAll();
	QRegExp titleRx("<title>(.*)</title>");
	if (titleRx.indexIn(data) != -1)
		return MvdCore::decodeXmlEntities(titleRx.cap(1));
	return QString();
}


//! \todo Handle situations where the movie details need to be retrieved asynchronously.
//! Parses an IMDb movie page and adds the movie to the import dialog.
void MvdBasicMpi::loadMovie(const QUuid& id)
{
	QString path = downloadedMovies[id];
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return;

	QByteArray data = file.readAll();
	QString title, s;

	// Title: <div id="tn15title">\n<h1>$TITLE$ <span>(<a href="/Sections/Years/$YEAR$">$YEAR$</a>)</span></h1>\n</div>
	QRegExp rx("<div id=\"tn15title\">\n<h1>(.*) <span>\\(<a href=\"/Sections/Years/([0-9]{4})\">");
	if (rx.indexIn(data) != -1)
	{
		title = MvdCore::decodeXmlEntities(rx.cap(1)).trimmed();
		s = MvdCore::decodeXmlEntities(rx.cap(2)).trimmed();
	}

	if (title.isEmpty())
		return;

	QHash<QString,QVariant> movie;
	movie.insert("original-title", title);
	movie.insert("production-year", s);

	importDialog->addMovieData(id, movie);
}
