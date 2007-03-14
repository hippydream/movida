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

#include "ui_imdbimportstart.h"

#include <movidacore/plugininterface.h>
#include <QtGlobal>
#include <QHttp>

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

class MvdwImportDialog;
class Ui::MvdImdbImportStart;
class QHttp;
class QPushButton;

class MvdBasicMpi : public MvdPluginInterface
{
	Q_OBJECT

public:
	MvdBasicMpi(QObject* parent = 0);

	bool init();
	void unload();

	QString lastError() const;
	PluginInfo info() const;
	QList<PluginAction*> actions() const;
	void actionTriggeredImplementation(const QString& name);

private slots:
	void readResponseHeader(const QHttpResponseHeader& responseHeader);
	void httpRequestFinished(int id, bool error);
	void validateQuery(const QString& query);
	void showImportPage();
	void showStartPage();
	void import();
	void abortRequest();

private:
	enum HttpRequest { NoRequest = 0, SearchMovieRequest };

	void imdbMovieImport();
	void retrieveImdbMovie(const QString& id);
	void searchImdbMovie(const QString& name);
	void initHttp();

	QHttp* http;
	QPushButton* togglePageButton;
	QPushButton* importButton;
	QPushButton* cancelButton;
	MvdwImportDialog* importDialog;
	Ui::MvdImdbImportStart* startPageUi;
	HttpRequest currentRequest;
	int httpGetId;
};

extern "C" MVD_BASICMPI_EXPORT 
MvdPluginInterface* pluginInterface(QObject* parent);

#endif // MVDMPI_BASIC_H
