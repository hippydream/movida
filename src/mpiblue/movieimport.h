/**************************************************************************
** Filename: movieimport.h
**
** Copyright (C) 2007-2009 Angius Fabrizio. All rights reserved.
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

#ifndef MPI_MOVIEIMPORT_H
#define MPI_MOVIEIMPORT_H

#include "blue.h"

#include "mvdcore/moviedata.h"

#include "mvdshared/importdialog.h"

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QProcess>
#include <QtNetwork/QHttp>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

class MvdImportDialog;
class MvdMovieData;

class QTemporaryFile;
class QTextStream;

class MpiMovieImport : QObject
{
    Q_OBJECT

public:
    MpiMovieImport(QObject *parent = 0);
    virtual ~MpiMovieImport();

    void run(const QList<MpiBlue::Engine *> &engines);

private slots:
    void reset();

    void configureEngine(int engine);
    void search(const QString &query, int engineId);
    void performSearch(const QString &query, MpiBlue::Engine *engine, int engineId);
    void import(const QList<int> &list);

    void httpRequestFinished(int id, bool error);
    void httpResponseHeader(const QHttpResponseHeader &responseHeader);
    void httpStateChanged(int state);

    void interpreterFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void interpreterStateChanged(QProcess::ProcessState state);

    void processNextImport();
    void processResponseFile();
    void processMoviePoster();

    void done();

private:
    enum HttpStatusClass {
        NoStatusClass = 0,
        InformationalClass = 1,
        SuccessClass = 2,
        RedirectionClass = 3,
        ClientErrorClass = 4,
        ServerErrorClass = 5
    };

    struct SearchEngine {
        SearchEngine() :
            port(-1) { }

        QString name;
        QString host;
        QString query;
        int port;
    };

    enum DataSourceType {
        CachedSource,
        RemoteSource
    };

    struct SearchResult {
        SearchResult() :
            sourceType(CachedSource) { }

        QString dataSource;
        DataSourceType sourceType;
        MvdMovieData data;
    };

    enum State {
        NoState = 0,
        FetchingResultsScriptState,
        FetchingImportScriptState,
        FetchingResultsState,
        FetchingMovieDataState,
        FetchingMoviePosterState
    };

    enum {
        HttpNotModified = 304
    };

    MvdImportDialog *mImportDialog;
    QHttp *mHttpHandler;
    int mRequestId;
    QTemporaryFile *mTempFile;
    QString mCurrentLocation;
    int mCurrentEngine;
    bool mHttpNotModified;
    QString mCurrentQuery;
    QStringList mQueryQueue;
    QProcess *mInterpreter;
    QString mInterpreterName;
    State mCurrentState;
    QString mNextUrl;
    int mCurrentImportJob;
    MvdImportDialog::Result mImportResult;

    QHash<int, MpiBlue::Engine *> mRegisteredEngines;
    QHash<int, SearchResult> mSearchResults;
    QList<int> mImportsQueue;

    // Files to be removed before we finish
    QStringList mTemporaryData;

    void completeJob();
    void parseCachedMoviePage(SearchResult &job);
    void deleteTemporaryFile(QTemporaryFile **file, bool removeFile = true);
    QTemporaryFile *createTemporaryFile();
    void initHttpHandler();
    QString scriptDate(const QString &name) const;
    void processResultsFile(const QString &path);
    void processMovieDataFile(const QString &path);
    bool isValidResult(SearchResult &result, const QString &path);
    bool parseSearchResults(xmlDocPtr doc, xmlNodePtr node, const QString &path, const QString &group = QString());
};

#endif // MPI_MOVIEIMPORT_H
