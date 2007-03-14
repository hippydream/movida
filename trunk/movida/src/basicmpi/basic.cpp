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

MvdBasicMpi::MvdBasicMpi(QObject* parent)
: MvdPluginInterface(parent), http(0), togglePageButton(0), importButton(0),
cancelButton(0), importDialog(0), startPageUi(0), currentRequest(NoRequest),
httpGetId(0)
{
}

bool MvdBasicMpi::init()
{
	QHash<QString, QVariant> parameters;
	parameters.insert("basicmpi-imdb-host", "imdb.com");
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

	QDialogButtonBox* buttonBox = id.buttonBox();
	togglePageButton = buttonBox->addButton(tr("Next"), QDialogButtonBox::ActionRole);
	togglePageButton->setDisabled(true);
	cancelButton = buttonBox->addButton(QDialogButtonBox::Cancel);
	
	ui.input->setFocus(Qt::OtherFocusReason);

	connect( ui.input, SIGNAL(textChanged(const QString&)), 
		this, SLOT(validateQuery(const QString&)) );
	connect( togglePageButton, SIGNAL(clicked()), 
		this, SLOT(showImportPage()) );
	connect( cancelButton, SIGNAL(clicked()), 
		importDialog, SLOT(close()) );

	id.exec();

	// Reset convenience pointers
	togglePageButton = 0;
	importButton = 0;
	cancelButton = 0;
	importDialog = 0;
	startPageUi = 0;
}

//! Sends a movie query to the IMDb website.
void MvdBasicMpi::searchImdbMovie(const QString& name)
{
	if (!importDialog)
		return;

	initHttp();

	currentRequest = SearchMovieRequest;
	importDialog->setBusyStatus(true);
	importDialog->setStatus(tr("Searching for '%1'...").arg(name));

	httpGetId = http->get(MvdCore::parameter("basicmpi-imdb-find-movie")
		.toString().arg(name));
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

		//! \todo Handle proxy (best with Qt >= 4.3)
		http->setHost(MvdCore::parameter("basicmpi-imdb-host").toString());
	}
}

void MvdBasicMpi::readResponseHeader(const QHttpResponseHeader& responseHeader)
{
	if (responseHeader.statusCode() != 200)
	{
		importDialog->setStatus(tr("Sorry, an error occurred (%1).\nPlease <a href='movida://httpget/search'>click here</a> to retry.").arg(responseHeader.reasonPhrase()));
		http->abort();
		resetImportPage(false);
	}
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
		//QByteArray response = http->readAll();
		importDialog->setStatus(tr("Please select one or more results and press the import button."));
		resetImportPage(true);
	}
}

void MvdBasicMpi::resetImportPage(bool success)
{
	importDialog->setBusyStatus(false);

	importDialog->buttonBox()->removeButton(cancelButton);
	cancelButton = importDialog->buttonBox()->addButton(QDialogButtonBox::Cancel);
	connect( cancelButton, SIGNAL(clicked()), importDialog, SLOT(close()) );

	importButton->setEnabled(success);
	togglePageButton->setEnabled(true);
}

void MvdBasicMpi::validateQuery(const QString& query)
{
	if (!togglePageButton)
		return;

	togglePageButton->setDisabled(query.trimmed().isEmpty());
}

//! Shows the import page and sends a search/download request to IMDb.
void MvdBasicMpi::showImportPage()
{
	if (!importDialog)
		return;

	importDialog->showNextPage();

	importDialog->buttonBox()->removeButton(togglePageButton);
	importDialog->buttonBox()->removeButton(cancelButton);

	togglePageButton = importDialog->buttonBox()->addButton(tr("Previous"), 
		QDialogButtonBox::ActionRole);
	togglePageButton->setEnabled(false);
	importButton = importDialog->buttonBox()->addButton(tr("Import"), 
		QDialogButtonBox::ActionRole);
	importButton->setEnabled(false);
	cancelButton = importDialog->buttonBox()->addButton(QDialogButtonBox::Abort);
	
	connect( togglePageButton, SIGNAL(clicked()), this, SLOT(showStartPage()) );
	connect( importButton, SIGNAL(clicked()), this, SLOT(import()) );
	connect( cancelButton, SIGNAL(clicked()), this, SLOT(abortRequest()) );

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
	if (!importDialog || !startPageUi)
		return;

	importDialog->showPreviousPage();

	// Reset button box
	importDialog->buttonBox()->removeButton(togglePageButton);
	importDialog->buttonBox()->removeButton(importButton);

	togglePageButton = importDialog->buttonBox()->addButton(tr("Next"), 
		QDialogButtonBox::ActionRole);

	connect( togglePageButton, SIGNAL(clicked()), this, SLOT(showImportPage()) );
}

//! Downloads (if necessary) and imports the selected movies.
void MvdBasicMpi::import()
{
	if (!importDialog || !startPageUi)
		return;
	
}

//! Interrupts the current operation and closes the dialog.
void MvdBasicMpi::abortRequest()
{
	// TEMPORARY!
	showStartPage();
}

//! Public interface for this plugin.
MvdPluginInterface* pluginInterface(QObject* parent)
{
	return new MvdBasicMpi(parent);
}
