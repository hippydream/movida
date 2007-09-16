/**************************************************************************
** Filename: basic.h
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

#ifndef MVDMPI_BASIC_H
#define MVDMPI_BASIC_H

#include <QtGlobal>

#ifndef MVD_BASICMPI_EXPORT
# ifdef Q_OS_WIN
#  if defined(MVD_BUILD_BASICMPI)
#   define MVD_BASICMPI_EXPORT __declspec(dllexport)
#  else
#   define MVD_BASICMPI_EXPORT __declspec(dllimport)
#  endif // MVD_BUILD_BASICMPI
# else // Q_OS_WIN
#  define MVD_BASICMPI_EXPORT
# endif
#endif // MVD_BASICMPI_EXPORT

#include <movidacore/plugininterface.h>
#include <movidacore/moviedata.h>
#include <movidawidgets/importdialog.h>

#include <QtGlobal>
#include <QHttp>
#include <QHash>
#include <QList>

class MvdMovieData;
class MvdwImportDialog;
class QTemporaryFile;
class QTextStream;

class MvdBasicMpi : public MvdPluginInterface
{
	Q_OBJECT

public:
	MvdBasicMpi(QObject* parent = 0);
	virtual ~MvdBasicMpi();

	// MvdPluginInterface overloads:
	bool init();
	void unload();
	QString lastError() const;
	PluginInfo info() const;
	QList<PluginAction*> actions() const;
	void actionTriggeredImplementation(const QString& name);

private slots:
	void configureEngine(int engine);
	void search(const QString& query, int engine);
	void import(const QList<int>& list);

	void requestFinished(int id, bool error);
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
	
	struct SearchEngine
	{
		SearchEngine() : port(-1) {}

		QString name;
		QString host;
		QString query;
		int port;
	};

	enum DataSourceType
	{
		CachedSource,
		RemoteSource
	};

	struct SearchResult
	{
		SearchResult() : sourceType(CachedSource) {}
		QString dataSource;
		DataSourceType sourceType;
		MvdMovieData data;
	};

	MvdwImportDialog* importDialog;
	QHttp* httpHandler;
	int requestId;
	QTemporaryFile* tempFile;
	QString currentLocation;
	
	QList<SearchEngine> availableEngines;
	QHash<int,SearchEngine> registeredEngines;
	QHash<int,SearchResult> searchResults;
	QList<int> importsQueue;

	void processNextImport();
	void parseImdbMoviePage(SearchResult& job);
	void parseQueryResponse();
	void parseResultsBlock(QTextStream& in, QString line);
	void deleteTemporaryFile(QTemporaryFile** file, bool removeFile = true);
	QTemporaryFile* createTemporaryFile();

	// Plugin entry point handlers
	void imdbImportEntryPoint();
};

extern "C" MVD_BASICMPI_EXPORT MvdPluginInterface* pluginInterface(QObject* parent);

#endif // MVDMPI_BASIC_H
