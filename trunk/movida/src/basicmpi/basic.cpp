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
#include <movidacore/logger.h>
#include <movidawidgets/searchengine.h>

#include <QMessageBox>
#include <QRegExp>
#include <QDir>
#include <QTemporaryFile>

#include <math.h>

using namespace Movida;

//! Public interface for this plugin.
MvdPluginInterface* pluginInterface(QObject* parent)
{
	return new MvdBasicMpi(parent);
}

MvdBasicMpi::MvdBasicMpi(QObject* parent)
: MvdPluginInterface(parent), importDialog(0), httpHandler(0), requestId(-1), tempFile(0)
{
	// Register engines:
	// us.imdb.com is the basic archive
	// akas.imdb.com is an extended archive

	SearchEngine e;
	e.query = "/find?s=tt&q=%1";
	
	e.name = tr("IMDb (basic archive)");
	e.host = "us.imdb.com";	
	availableEngines.append(e);

	e.name = tr("IMDb (extended archive)");
	e.host = "akas.imdb.com";
	availableEngines.append(e);
}

MvdBasicMpi::~MvdBasicMpi()
{
}

bool MvdBasicMpi::init()
{
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
		imdbImportEntryPoint();
}

//! Entry point for the "imdb-import" action.
void MvdBasicMpi::imdbImportEntryPoint()
{
	registeredEngines.clear();

	QWidget* p = qobject_cast<QWidget*>(this->parent());

	importDialog = new MvdwImportDialog(p);
	
	for (int i = 0; i < availableEngines.size(); ++i)
	{
		const SearchEngine& e = availableEngines.at(i);
		MvdwSearchEngine imdbEngine;
		imdbEngine.name = e.name;
		int id = importDialog->registerEngine(imdbEngine);
		registeredEngines.insert(id, e);
		qDebug("Engine %s registered (%d)", e.name.toLatin1().constData(), id);
	}

	connect( importDialog, SIGNAL(engineConfigurationRequest(int)),
		this, SLOT(configureEngine(int)) );
	connect( importDialog, SIGNAL(searchRequest(const QString&, int)),
		this, SLOT(search(const QString&, int)) );
	connect( importDialog, SIGNAL(importRequest(const QList<int>&)),
		this, SLOT(import(const QList<int>&)) );

	importDialog->exec();
}

//! \todo Implement MvdBasicMpi::configureEngine
void MvdBasicMpi::configureEngine(int engine)
{
	Q_UNUSED(engine);
	QWidget* p = qobject_cast<QWidget*>(this->parent());
	QMessageBox::warning(p, "Movie engine configuration", "Not implemented");
}

//! Performs a query and adds found results to the import dialog.
void MvdBasicMpi::search(const QString& query, int engine)
{
	qDebug("MvdBasicMpi::search(%s, %d)", query.toLatin1().constData(), engine);

	searchResults.clear();
	importsQueue.clear();

	SearchEngine e = registeredEngines.value(engine);
	if (e.host.isEmpty())
	{
		qDebug("Invalid search engine host");
		importDialog->done();
		return;
	}

	if (tempFile)
		deleteTemporaryFile(&tempFile);

	tempFile = createTemporaryFile();
	if (!tempFile)
	{
		qDebug("Failed to create a temporary file");
		importDialog->done();
		return;
	}

	// Assert no jobs are running
	if (httpHandler)
		httpHandler->abort();
	else
	{
		//! \todo Handle proxy in the main application (best with Qt >= 4.3)
		httpHandler = new QHttp(this);

		connect(httpHandler, SIGNAL(requestFinished(int, bool)),
			this, SLOT(requestFinished(int, bool)));
		connect(httpHandler, SIGNAL(responseHeaderReceived(const QHttpResponseHeader&)),
			this, SLOT(httpResponseHeader(const QHttpResponseHeader&)));
	}

	qDebug("HTTP HOST %s:%d", e.host.toLatin1().constData(), e.port < 0 ? 80 : e.port);
	httpHandler->setHost(e.host, e.port < 0 ? 80 : e.port);
 	
	currentLocation = e.query.arg(QString::fromLatin1(MvdCore::toLatin1PercentEncoding(query)));

	qDebug("HTTP GET %s", currentLocation.toLatin1().constData());
	requestId = httpHandler->get(currentLocation, tempFile);
}

//! \internale Downloads and imports the specified search results.
void MvdBasicMpi::import(const QList<int>& list)
{
	qDebug("MvdBasicMpi::import()");
	if (list.isEmpty())
	{
		importDialog->done();
		return;
	}

	importsQueue = list;
	processNextImport();
}

/*! \internal Takes a new import job from the queue and processes it.
	Calls MvdwImportDialog::done() when no more imports are available.
*/
void MvdBasicMpi::processNextImport()
{
	if (importsQueue.isEmpty())
	{
		importDialog->done();
		return;
	}

	int id = importsQueue.takeFirst();
	QHash<int,SearchResult>::Iterator it = searchResults.find(id);
	if (it == searchResults.end())
	{
		qDebug("Skipping invalid job");
		processNextImport();
		return;
	}

	qDebug("Processing job %d", id);
	SearchResult& job = it.value();
	
	if (job.sourceType == CachedSource)
	{
		parseImdbMoviePage(job);
	}
}

//! \internal Parses a local IMDb movie page.
void MvdBasicMpi::parseImdbMoviePage(SearchResult& job)
{
	QFile file(job.dataSource);
	if (!file.open(QIODevice::ReadOnly))
		return;

	QTextStream in(&file);
	QString line;

	// Use a not-so-stupid "algorithm" and check for a different target (or target set)
	// each time- This ain't very smart but it's still better than a hundred of comparisons.
	// IMDb pages are actualy slightly heterogeneous and we can't be too strict here.
	//! \todo Implement a text configuration file based dynamic parsing engine
	enum Target { Script1 = 0, Poster, Rating, Hr1, Info, End };
	int currentTarget = int(Script1);
	int previousTarget = currentTarget;
	bool targetHit = false;

	QRegExp infoTypeRx("^<h5>(.*):</h5>$");

	// <b>7.4/10</b>
	QRegExp ratingRx("^.*([0-9.,]*)/([0-9.,]*).*$");

	// <a href="/name/nmXXX/">NAME</a></td><td class="ddd"> ... </td><td class="char">ROLE</td>
	QRegExp castRx("<a href=\"/name/nm([0-9]*)/\"[^<>]*>([^<>]*)</a></td><td[^<>]*>[^<>]*</td><td class=\"char\">([^<>]*)</td>");

	MvdMovieData::PropertyName infoType;
	QString extra;

	while ( !(line = in.readLine()).isNull() && currentTarget != End )
	{
		if (currentTarget != previousTarget)
		{
			previousTarget = currentTarget;
			qDebug("New target: %d", (int)currentTarget);
		}

		line = line.trimmed();

		switch (Target(currentTarget))
		{
		case Poster:
			if (line == "//-->")
			{
				currentTarget++; // Target missed
				targetHit = false;
			}
			else if (targetHit) // Line contains poster URL
			{
				qDebug("Poster hit");
				currentTarget++;
				targetHit = false;
			}
			else if (line == "")
				targetHit = true; // Next line contains the poster URL
			break;

		case Script1:
			if (line == "//-->")
				currentTarget++;
			break;

		case Rating:
			if (line == "//-->")
			{
				currentTarget++; // Target missed
				targetHit = false;
			}
			else if (targetHit)
			{
				qDebug("Rating hit");

				if (ratingRx.exactMatch(line))
				{
					float n = ratingRx.cap(1).toFloat();
					float d = ratingRx.cap(2).toFloat();

					bool ok;
					int m = MvdCore::parameter("movidacore-max-rating").toInt(&ok);
					if (!ok) m = 5;
					
					// IMDb rating and movida rating have different scales
					int rating = int( (m * n) / d );
					job.data.rating = rating;

					qDebug("IMDb rating: %f/%f -> Movida rating: %d", n, d, rating);
				}

				currentTarget++;
				targetHit = false;
			}
			else if (line == "<b>User Rating:</b>")
				targetHit = true; // Next line contains the poster URL
			break;

		case Hr1:
			if (line == "<hr/>")
				currentTarget++;
			break;

		// Directors, Writers, Release dates, Genres, Tags, Plot
		// AKAs, Running time, Countries, Languages, Color mode, 
		// AR, Sound mix, Certifications, Locations, MOVIEmeter, Companies
		// Trivia, Goofs, Quotes, Soundtrack
		case Info:
			if (line.startsWith("<img src=\"http://i.imdb.com/images/tn15/header_cast.gif"))
			{
				// CAST is a huge one-liner containing <td class="char">FIRST_NAME LAST_NAME</td>
				qDebug("Cast hit");
				int pos = 0;
				while ((pos = castRx.indexIn(line, pos)) != -1)
				{
					QString id = castRx.cap(1).trimmed();
					QString name = castRx.cap(2).trimmed();
					QString role = castRx.cap(3).trimmed();

					MvdMovieData::PersonData d;
					d.name = name;
					d.imdbId = id;
					d.roles = role;

					qDebug("Name: %s ID: %s", d.name.toLatin1().data(), d.imdbId.toLatin1().data());

					pos += castRx.matchedLength();
				}
				targetHit = false;
			}
			else if (line.startsWith("<img src=\"http://i.imdb.com/images/tn15/header_faq.gif"))
			{
				qDebug("FAQ hit");
				currentTarget++; // End of info section
				targetHit = false;
			}
			else if (targetHit)
			{
				qDebug("Info type %d hit", infoType);
				targetHit = false;
				// Parse contents
			}
			else if (infoTypeRx.exactMatch(line))
			{
				QString nameString = infoTypeRx.cap(1).trimmed().toLower();
				infoType = MvdMovieData::InvalidProperty;
				
				if (line.startsWith("director"))
					infoType = MvdMovieData::Directors;
				else if (line.startsWith("writer"))
				{
					infoType = MvdMovieData::CrewMembers;
					extra = tr("Writer", "Movie writer role");
				}
				else if (line.startsWith("release"))
					infoType = MvdMovieData::ReleaseYear;
				else if (line.startsWith("genre"))
					infoType = MvdMovieData::Genres;
				else if (line.startsWith("plot outline"))
					infoType = MvdMovieData::Plot;
				else if (line.startsWith("plot keyword"))
					infoType = MvdMovieData::Tags;
				else if (line.startsWith("plot outline"))
					infoType = MvdMovieData::Plot;

				if (infoType != MvdMovieData::InvalidProperty)
					targetHit = true;
			}
			break;
		default:
			;
		}
	}
}

//! \internal Handles a QHttp::requestFinished() signal.
void MvdBasicMpi::requestFinished(int id, bool error)
{
	Q_ASSERT(httpHandler);

	if (error)
	{
		if (httpHandler->error() == QHttp::Aborted)
			qDebug("Request aborted.");
		else qDebug("Request failed: %s", httpHandler->errorString().toLatin1().constData());

		if (tempFile)
			deleteTemporaryFile(&tempFile);

		importDialog->done();
		return;
	}

	if (id != requestId)
		return;

	if (!tempFile)
	{
		qDebug("No temporary file!");
		return;
	}

	tempFile->seek(0);
	parseQueryResponse();
	importDialog->done();
}

void MvdBasicMpi::httpResponseHeader(const QHttpResponseHeader& responseHeader)
{
	Q_ASSERT(httpHandler);

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
			if (tempFile)
				deleteTemporaryFile(&tempFile);
			tempFile = createTemporaryFile();
			if (!tempFile)
			{
				qDebug("Unable to create a temporary file.");
				importDialog->done();
				httpHandler->abort();
				return;
			}

			QString query = locationUrl.encodedQuery();
			currentLocation = locationUrl.path();
			if (!query.isEmpty())
				currentLocation.append("?").append(query);
			qDebug("Redirecting to %s", currentLocation.toLatin1().constData());
			requestId = httpHandler->get(currentLocation, tempFile);
		}
		break;
	case SuccessClass:
		qDebug("Download finished.");
		break;
	default:
		; // Errors will be handled in the requestFinished() handler
	}
}

/*!
	Parses a query response.
	(Only IMDb queries are supported currently).
*/
void MvdBasicMpi::parseQueryResponse()
{
	Q_ASSERT(tempFile);

	qDebug("MvdBasicMpi::parseQueryResponse()");

	// Check if we have been redirected to a movie description page
	// http://us.imdb.com/title/tt0078748/
	if (currentLocation.contains("title"))
	{
		// We can assume that the <title> tag is on one line
		// as the IMDb pages are generated automatically
		QTextStream in(tempFile);
		QString line;

		QRegExp titleRx("^<title>(.*)</title>$");
		QRegExp yearRx("^(.*) \\(([0-9]{4})\\)$");
		
	
		while ( !(line = in.readLine()).isNull() )
		{
			line = line.trimmed();qDebug(line.toLatin1().data());
			if (titleRx.exactMatch(line))
			{
				QString title = MvdCore::decodeXmlEntities(titleRx.cap(1).trimmed());
				QString year;
				
				if (yearRx.exactMatch(title))
				{
					title = yearRx.cap(1).trimmed();
					year = yearRx.cap(2); // RegExp already excludes white space here
				}

				if (!title.isEmpty())
				{
					qDebug("Match: %s (%s)", title.toLatin1().constData(), year.toLatin1().constData());
					int id = importDialog->addMatch(title, year);
					SearchResult result;
					result.sourceType = CachedSource;
					result.dataSource = tempFile->fileName();
					result.data.originalTitle = title;
					result.data.releaseYear = year;
					deleteTemporaryFile(&tempFile, false);
					searchResults.insert(id, result);
				}

				break;
			}
			else if (line == "</head>")
			{
				qDebug("Invalid or unsupported IMDb response. Header match failed.");
				break;
			}
		}
	}
	else // Multiple matches
	{
		QTextStream in(tempFile);
		QString line;
		QRegExp titleTagRx("^<title>\\s*IMDb\\s*Title\\s*Search\\s*</title>$");

		bool dataFound = false;
	
		// Parse HTML header first
		while ( !(line = in.readLine()).isNull() )
		{
			line = line.trimmed();
			if (line.startsWith("<title>"))
			{
				dataFound = titleTagRx.exactMatch(line);
				break;
			}
			else if (line == "</head>")
				break;
		}

		if (!dataFound)
		{
			qDebug("Invalid or unsupported IMDb response. Header match failed.");
			importDialog->done();
			return;
		}

		// Now loook for "//-->"
		// It's a nice and fast bookmark to look for in the currently used IMDb pages :)
		while ( !(line = in.readLine()).isNull() && !line.trimmed().startsWith("//-->") );
		
		if (line.isNull())
		{
			qDebug("Invalid or unsupported IMDb response. Script lookup failed.");
			importDialog->done();
			return;
		}
	
		QRegExp titleRx("^.*<p><b>(.*Titles.*)</b>.*$");
		QRegExp sectionRx("^<h2>(.*)</h2>$");
		QRegExp subSectionRx("^Titles \\((.*)\\)$");
	
		QString section;
		QString subSection;
	
		// Now do some serious parsing using slower regexps
		while ( !(line = in.readLine()).isNull() )
		{
			line = line.trimmed();

			if (line == "<b>No Matches.</b>")
			{
				qDebug("No matches");
				break;
			}
			
			if (sectionRx.exactMatch(line))
			{
				section = sectionRx.cap(1);
				importDialog->addSection(section);
				qDebug("Top level section %s", section.toLatin1().constData());
			}
			else if (titleRx.exactMatch(line))
			{
				// Results block starts here
				// "Titles (XXX Matches)" OR "XXX Titles"
				subSection = titleRx.cap(1).trimmed();
				if (subSectionRx.indexIn(subSection) >= 0)
					subSection = subSectionRx.cap(1).trimmed();
				else
				{
					subSection.truncate(subSection.length() - 6);
					subSection = subSection.trimmed();
				}
				if (!subSection.isEmpty())
				{
					if (section.isEmpty())
						importDialog->addSection(subSection);
					else importDialog->addSubSection(subSection);
					qDebug("2nd level section %s", subSection.toLatin1().constData());
				}
				parseResultsBlock(in, line);
			}
			else if (line.startsWith("<b>Suggestions"))
			{
				qDebug("End of results block");
				break;
			}
		}

		deleteTemporaryFile(&tempFile);
	}

	importDialog->done();
}

//! Parses a IMDb results block. \p line contains the first "valid" line of data.
void MvdBasicMpi::parseResultsBlock(QTextStream& in, QString line)
{
	qDebug("MvdBasicMpi::parseResultsBlock()");

	QRegExp movieRx("^.*<a href=\"([^\"]*)\".*>(.*)</a>.*\\(([0-9]{4}).*\\)(.*)$");
	while (!line.isNull())
	{
		int offset = line.indexOf("<li>");
		if (offset >= 0)
		{
			// <li>  <a href="URL" ...>TITLE</a> (YEAR)</li>
			// <li>  <a href="URL" ...>TITLE</a> (YEAR)<br>&nbsp;aka <em>XXX</em> - USA</li>
			line.remove(0, offset);
			offset = line.indexOf("</li>", offset);
			line.truncate(offset);
			line = line.trimmed();
			//qDebug(line.toLatin1().constData());
			
			if (movieRx.indexIn(line) >= 0)
			{
				QString url = movieRx.cap(1);
				QString title = MvdCore::decodeXmlEntities( movieRx.cap(2) );
				QString year = movieRx.cap(3);
				QString additional = MvdCore::decodeXmlEntities( movieRx.cap(4).remove("&nbsp;", Qt::CaseInsensitive) );
				while (!additional.isEmpty())
				{
					if (additional.startsWith("<br>"))
					{
						additional.remove(0, 4);
						additional = additional.trimmed();
					}
					else break;
				}
				if (! (title.isEmpty() || url.isEmpty()) )
				{
					int id = importDialog->addMatch(title, year, additional);
					SearchResult result;
					result.sourceType = RemoteSource;
					result.dataSource = url;
					searchResults.insert(id, result);
				}
			}
		}
		else break;
		line = in.readLine();
	}
}

//! Deletes a file and sets its pointer to 0. Asserts that file is not null.
void MvdBasicMpi::deleteTemporaryFile(QTemporaryFile** file, bool removeFile)
{
	Q_ASSERT(file && *file);

	QString path = (*file)->fileName();
	delete *file;
	*file = 0;
	if (removeFile)
		QFile::remove(path);
}

//! Creates a new temporary file with AutoRemove = false. Returns 0 if an error occurs.
QTemporaryFile* MvdBasicMpi::createTemporaryFile()
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
