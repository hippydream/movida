/**************************************************************************
** Filename: importdialog.cpp
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

#include "importdialog.h"
#include "labelanimator.h"
#include "importstartpage.h"
#include "importresultspage.h"
#include "importfinalpage.h"
#include <QPushButton>

/*!
	\class MvdImportDialog importdialog.h
	\ingroup MovidaShared

	\brief Wizard dialog to import movies from a local or remote source.
*/

/*!
	Creates a new Movida import wizard for the search engines listed in \p engines.
	The dialog is a QWizard subclass so you can use the superclass methods to customize
	the dialog's look:

	\verbatim
	myWizard.setPixmap(QWizard::LogoPixmap, QPixmap(":/myLogo.png"));
	// ...
	myWizard.setWindowTitle("MyPlugin import wizard");
	// ...
	\endverbatim

	Please refer to the QWizard documentation for further details.
*/
MvdImportDialog::MvdImportDialog(QWidget* parent)
: QWizard(parent)
{
	//! \todo Check for supported engine URLs and handle file:// protocol with a QFileBrowser and a query-like filter instead of a standard query input widget

	startPage = new MvdImportStartPage;
	startPageId = addPage(startPage);
	connect( startPage, SIGNAL(engineConfigurationRequest(int)), 
		this, SIGNAL(engineConfigurationRequest(int)) );
	connect( this, SIGNAL(currentIdChanged(int)),
		this, SLOT(pageChanged(int)) );

	resultsPage = new MvdImportResultsPage;
	resultsPageId = addPage(resultsPage);
	finalPage = new MvdImportFinalPage;
	finalPageId = addPage(finalPage);

	setPixmap(QWizard::LogoPixmap, QPixmap(":/images/import/logo.png"));
	setPixmap(QWizard::BannerPixmap, QPixmap(":/images/import/banner.png"));
	setWindowTitle(tr("Movida import wizard"));
}

/*!
	This will signal the current page that the plugin handler has finished handling
	the last request.
	This should be called only after the handler has finished handling a request.
	I.e. after a searchRequest() signal, handler is supposed to call addMatch()
	for any found result and then call done(), so that the user can be allowed to
	select and import the required items.

	The use of multiple handlers and thus multiple calls to the done() method
	for the same request (i.e. after the searchRequest() signal and before the
	importRequest() signal) leads to undefined behavior!
*/
void MvdImportDialog::done()
{
	qDebug("MvdImportDialog::done()");

	MvdImportPage* p = dynamic_cast<MvdImportPage*>(currentPage());
	if (p)
		p->setBusyStatus(false);
}

/*!
	Registers a new search engine. Returns an identifier that will be used
	to refer to this engine (i.e. in the searchRequest() signal).
	This method should be called before calling show() or exec().
*/
int MvdImportDialog::registerEngine(const MvdSearchEngine& engine)
{
	return startPage->registerEngine(engine);
}

//! Shows a status message if the current page supports it.
void MvdImportDialog::showMessage(const QString& msg, MessageType type)
{
	MvdImportPage* p = dynamic_cast<MvdImportPage*>(currentPage());
	if (p)
		p->showMessage(msg, type);
}

/*! Adds a search result to the results list. \p notes can contain additional data used
	to help the user distinguish between similar results.
	Returns a unique identifier used to refer to this search result later on (i.e. in the
	importRequest() signal).
*/
int MvdImportDialog::addMatch(const QString& title, const QString& year, const QString& notes)
{
	return resultsPage->addMatch(title, year, notes);
}

/*!
	Starts a new top level section to group search results. \p notes is an optional tooltip text.
	The section will be marked as <i>active</i> and subsequent calls to addMatch() will cause
	the matches to be placed in this section.
*/
void MvdImportDialog::addSection(const QString& title, const QString& notes)
{
	resultsPage->addSection(title, notes);
}

/*!
	Starts a new second level section to group search results. \p notes is an optional tooltip text.
	The section will be marked as <i>active</i> and subsequent calls to addMatch() will cause
	the matches to be placed in this section.
*/
void MvdImportDialog::addSubSection(const QString& title, const QString& notes)
{
	resultsPage->addSubSection(title, notes);
}

//!
void MvdImportDialog::accept()
{	
	QDialog::accept();
}

void MvdImportDialog::pageChanged(int id)
{
	if (id == resultsPageId && startPage)
	{
		qDebug("emitting queryRequest");
		emit searchRequest(startPage->query(), startPage->engine());
	}
	else if (id == finalPageId && resultsPage)
	{
		qDebug("emitting importRequest");
		emit importRequest(resultsPage->jobs());
	}
}
