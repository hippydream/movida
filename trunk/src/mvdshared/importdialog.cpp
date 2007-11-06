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
#include "importdialog_p.h"
#include "labelanimator.h"
#include "settings.h"
#include <QPushButton>
#include <QKeyEvent>
#include <QMessageBox>

/*!
	\class MvdImportDialog importdialog.h
	\ingroup MovidaShared

	\brief Wizard dialog to import movies from a local or remote source.
*/

Q_DECLARE_METATYPE(MvdMovieDataList)

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
: QWizard(parent), d(new MvdImportDialog_P)
{
	//! \todo Check for supported engine URLs and handle file:// protocol with a QFileBrowser and a query-like filter instead of a standard query input widget

	// Add pages

	d->startPage = new MvdImportStartPage;
	setPage(MvdImportDialog_P::StartPage, d->startPage);
	d->resultsPage = new MvdImportResultsPage;
	setPage(MvdImportDialog_P::ResultsPage, d->resultsPage);
	d->summaryPage = new MvdImportSummaryPage;
	setPage(MvdImportDialog_P::SummaryPage, d->summaryPage);
	d->finalPage = new MvdImportFinalPage;
	setPage(MvdImportDialog_P::FinalPage, d->finalPage);

	setStartId(MvdImportDialog_P::StartPage);

	connect( d->startPage, SIGNAL(engineConfigurationRequest(int)), 
		this, SIGNAL(engineConfigurationRequest(int)) );
	connect( this, SIGNAL(currentIdChanged(int)),
		this, SLOT(pageChanged(int)) );

	setPixmap(QWizard::LogoPixmap, QPixmap(":/images/import/logo.png"));
	setPixmap(QWizard::BannerPixmap, QPixmap(":/images/import/banner.png"));
	setWindowTitle(tr("Movida import wizard"));

	Q_UNUSED(qRegisterMetaType<MvdMovieDataList>());
}

//! Handles the page order.
int MvdImportDialog::nextId() const
{
	switch (currentId()) {
	case MvdImportDialog_P::StartPage:
		return MvdImportDialog_P::ResultsPage;
	case MvdImportDialog_P::ResultsPage:
		if (field("selectedResultsCount").toInt() == 0)
			return MvdImportDialog_P::FinalPage;
		return MvdImportDialog_P::SummaryPage;
	case MvdImportDialog_P::SummaryPage:
		return MvdImportDialog_P::FinalPage;
	default:
		return -1;
	}

	return -1;
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
	return d->startPage->registerEngine(engine);
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
	The caller is supposed to invoke done() after the last match has been added.
*/
int MvdImportDialog::addMatch(const QString& title, const QString& year, const QString& notes)
{
	return d->resultsPage->addMatch(title, year, notes);
}

/*! Adds a movie data object to the import list.
	The caller is supposed to invoke done() after the last movie data has been added.
*/
void MvdImportDialog::addMovieData(const MvdMovieData& md)
{
	d->summaryPage->addMovieData(md);
}

/*!
	Starts a new top level section to group search results. \p notes is an optional tooltip text.
	The section will be marked as <i>active</i> and subsequent calls to addMatch() will cause
	the matches to be placed in this section.
*/
void MvdImportDialog::addSection(const QString& title, const QString& notes)
{
	d->resultsPage->addSection(title, notes);
}

/*!
	Starts a new second level section to group search results. \p notes is an optional tooltip text.
	The section will be marked as <i>active</i> and subsequent calls to addMatch() will cause
	the matches to be placed in this section.
*/
void MvdImportDialog::addSubSection(const QString& title, const QString& notes)
{
	d->resultsPage->addSubSection(title, notes);
}

//!
void MvdImportDialog::accept()
{	
	QDialog::accept();
}

//! \p id is the new page to be showed.
void MvdImportDialog::pageChanged(int id)
{
	switch (id) {
	case MvdImportDialog_P::StartPage:
	{
		// Ensure the plugins will reset its internal state at each new search request.
		emit resetRequest();
	} break;
	case MvdImportDialog_P::ResultsPage:
	{
		QString q = d->startPage->query();
		if (Movida::settings().value("movida/use-history").toBool()) {
		
			QStringList history = Movida::settings().value("movida/history/movie-import").toStringList();
			if (!history.contains(q, Qt::CaseInsensitive))
				history.append(q);
			Movida::settings().setValue("movida/history/movie-import", history);
			d->startPage->updateCompleter(history);
		}
		emit searchRequest(q, d->startPage->engine());
	} break;
	case MvdImportDialog_P::SummaryPage:
	{
		emit importRequest(d->resultsPage->jobs());
	} break;
	case MvdImportDialog_P::FinalPage:
	{
		Q_ASSERT(QMetaObject::invokeMethod(d->finalPage, "importMovies", Qt::QueuedConnection,
			Q_ARG(MvdMovieDataList, d->summaryPage->movies())));
	} break;
	default: ;
	}

	MvdImportPage* p = qobject_cast<MvdImportPage*>(page(id));
	if (p)
		p->updateButtons();
}

//! Convenience method. Returns true if the current page has the preventCloseWhenBusy property set to true.
bool MvdImportDialog::preventCloseWhenBusy() const
{
	if (MvdImportPage* p = qobject_cast<MvdImportPage*>(currentPage()))
		if (p->preventCloseWhenBusy())
			return true;
	return false;
}

//! Convenience method. Returns true if the current page is busy.
bool MvdImportDialog::isBusy() const
{
	if (MvdImportPage* p = qobject_cast<MvdImportPage*>(currentPage()))
		if (p->busyStatus())
			return true;
	return false;
}

//! Prevents the dialog to close if the current page is busy.
void MvdImportDialog::closeEvent(QCloseEvent* e)
{
	Q_UNUSED(e);
	if ((isBusy() && preventCloseWhenBusy()) || !confirmCloseWizard()) {
		e->ignore();
		return;
	}

	QWizard::closeEvent(e);
}

//! Prevents the dialog to close if the current page is busy.
void MvdImportDialog::keyPressEvent(QKeyEvent* e)
{
	switch (e->key()) {
	case Qt::Key_Escape: 
		if ((isBusy() && preventCloseWhenBusy()) || !confirmCloseWizard()) {
			return;
		}
	default: ;
	}

	QWizard::keyPressEvent(e);
}

//! Asks the user for confirmation before the wizard is closed aborting a progressing operation. Returns true if the wizard should close.
bool MvdImportDialog::confirmCloseWizard()
{
	QString msg;
	if (!isBusy()) {
		msg = tr("Are you sure you want to close the wizard?");
	} else {
		switch (currentId()) {
		case MvdImportDialog_P::ResultsPage: msg = tr("Movida is still searching for your query. Are you sure you want to close the wizard?"); break;
		case MvdImportDialog_P::SummaryPage: msg = tr("Movida is still downloading the movie details. Are you sure you want to close the wizard?"); break;
		case MvdImportDialog_P::FinalPage: msg = tr("Movida is still importing your new movies. Are you sure you want to close the wizard?"); break;
		default: msg = tr("An operation is still in progress. Are you sure you want to close the wizard?");
		}
	}

	return QMessageBox::question(this, MVD_CAPTION, msg, QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes;
}
