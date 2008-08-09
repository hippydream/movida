/**************************************************************************
** Filename: importdialog.cpp
**
** Copyright (C) 2007-2008 Angius Fabrizio. All rights reserved.
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

#include "importdialog.h"
#include "importdialog_p.h"
#include "mvdcore/core.h"
#include "mvdcore/plugininterface.h"
#include "mvdcore/settings.h"
#include <QPushButton>
#include <QKeyEvent>
#include <QMessageBox>

/*!
	\class MvdImportDialog importdialog.h
	\ingroup MovidaShared

	\brief Wizard dialog to import movies from a local or remote source.
*/

Q_DECLARE_METATYPE(MvdMovieDataList)


QStringList MvdImportDialog_P::buildQueryList(QString s) const
{
	s = s.toLower();
	QStringList l = s.split(QRegExp("[;,]"), QString::SkipEmptyParts);
	l.sort();

	// Remove duplicates
	for (int i = 0; i < l.size() - 1; ++i) {
		if (l.at(i).trimmed() == l.at(i + 1).trimmed()) {
			l.removeAt(i--);
		}
	}

	return l;
}

//////////////////////////////////////////////////////////////////////////


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

	d->closing = false;
	d->importSteps = 1;
	d->importCount = 0;
	d->searchSteps = 1;
	d->importResult = Success;
	d->errorType = UnknownError;

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

	setPixmap(QWizard::LogoPixmap, QPixmap(":/images/import-wizard/logo.png"));
	setPixmap(QWizard::BannerPixmap, QPixmap(":/images/import-wizard/banner.png"));
	setWindowTitle(tr("Movida import wizard"));

	Q_UNUSED(qRegisterMetaType<MvdMovieDataList>());
}

//! Handles the page order.
int MvdImportDialog::nextId() const
{
	if (currentId() == MvdImportDialog_P::FinalPage)
		return -1;

	if (d->importResult != Success) {
		return MvdImportDialog_P::FinalPage;
	}

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
void MvdImportDialog::done(Result res)
{
	d->importResult = res;

	MvdImportExportPage* p = dynamic_cast<MvdImportExportPage*>(currentPage());
	if (p)
		p->setBusyStatus(false);
}

//! Returns the result of the latest import request as the client specified when calling done().
MvdImportDialog::Result MvdImportDialog::result() const
{
	return d->importResult;
}

/*!
	Plugins can call this before calling done() for the last time,
	to show a possible error to the user.
*/
void MvdImportDialog::setErrorType(ErrorType type)
{
	d->errorType = type;
}

MvdImportDialog::ErrorType MvdImportDialog::errorType() const
{
	return d->errorType;
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
void MvdImportDialog::showMessage(const QString& msg, MvdShared::MessageType type)
{
	MvdImportExportPage* p = dynamic_cast<MvdImportExportPage*>(currentPage());
	if (p)
		p->showMessage(msg, type);
}

/*!
	Each single import job is supposed to be made up of a certain number of steps
	(e.g. download the movie data, parse the data, download the poster, etc.).
	Plugins can provide a more detailed progress using this mechanism.
	First off, the number of steps that makes up a job needs to be set using
	this method. The best is to do this before calling MvdImportDialog::exec().

	Then, after a step has been completed (e.g. after downloading the movie data),
	call setNextImportStep().

	Movida will show the current progress according to the steps and to the number
	of movies that the user has selected for import.

	Nothing happens if a step is skipped or if the steps mechanism is not used. The
	progress indicator might just appear less fluid to the user.
*/
void MvdImportDialog::setImportSteps(quint8 s)
{
	d->importSteps = s == 0 ? 1 : s;
}

//! See MvdImportDialog::setImportSteps(quint8).
void MvdImportDialog::setNextImportStep()
{
	d->summaryPage->setProgress(d->summaryPage->progress() + 1);
}

/*! This method is pretty similar to MvdImportDialog::setImportSteps(quint8), except it is
	used to measure the progress of the search process.
*/
void MvdImportDialog::setSearchSteps(quint8 s)
{
	d->searchSteps = s == 0 ? 1 : s;
}

//! See MvdImportDialog::setSearchSteps(quint8).
void MvdImportDialog::setNextSearchStep()
{
	d->resultsPage->setProgress(d->resultsPage->progress() + 1);
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
	d->summaryPage->setProgress(++(d->importCount) * d->importSteps);
}

/*!
	Starts a new top level section to group search results. \p notes is an optional tooltip text.
	The section will be marked as <i>active</i> and subsequent calls to addMatch() will cause
	the matches to be placed in this section.
	If another section with the same title exists, no new section is added and the existing one will
	be marked as active.
*/
void MvdImportDialog::addSection(const QString& title, const QString& notes)
{
	d->resultsPage->addSection(title, notes);
}

/*!
	Starts a new second level section to group search results. \p notes is an optional tooltip text.
	The section will be marked as <i>active</i> and subsequent calls to addMatch() will cause
	the matches to be placed in this section.
	If another subsection with the same title exists, no new section is added and the existing one will
	be marked as active.
*/
void MvdImportDialog::addSubSection(const QString& title, const QString& notes)
{
	d->resultsPage->addSubSection(title, notes);
}

//!
void MvdImportDialog::accept()
{
	QList<mvdid> importedMovies = d->finalPage->importedMovies();
	if (!importedMovies.isEmpty() && d->finalPage->filterImportedMovies()) {
		QString s;
		foreach (mvdid id, importedMovies)
			s.append(QString::number(id).append(","));
		s.truncate(s.length() - 1);
		MvdCore::pluginContext()->properties.insert("movida/movies/filter", s);
	}
	QDialog::accept();
}

//! \p id is the new page to be shown.
void MvdImportDialog::pageChanged(int id)
{
	switch (id) {
	case MvdImportDialog_P::StartPage:
	{
		// Ensure the plugins will reset its internal state at each new search request.
		d->startPage->reset();
		d->resultsPage->reset();
		d->summaryPage->reset();
		d->finalPage->reset();
		d->importResult = Success;
		emit resetRequest();
	} break;
	case MvdImportDialog_P::ResultsPage:
	{
		setButtonText(QWizard::CommitButton, tr("&Next"));

		QString q = d->startPage->query();
		
		int engineId = d->startPage->engine();
		const MvdSearchEngine* engineDescriptor = d->startPage->engineDescriptor(engineId);
		Q_ASSERT(engineDescriptor);
		
		QStringList queries = d->buildQueryList(q);
		QString query;
		int queryCount = 1;

		if (Movida::settings().value("movida/use-history").toBool()) {
		
			QStringList history = Movida::settings().value("movida/history/movie-import").toStringList();
			bool update = false;
			for (int i = 0; i < queries.size(); ++i) {
				QString qq = queries.at(i);
				if (!history.contains(qq, Qt::CaseInsensitive)) {
					history.append(qq);
					update = true;
				}
			}

			if (update) {
				int maxCount = Movida::settings().value("movida/max-history-items").toInt();
				while (history.count() > maxCount)
					history.takeFirst();
				Movida::settings().setValue("movida/history/movie-import", history);
				d->startPage->updateCompleter(history);
			}
		}

		if (queries.isEmpty()) {
			query = q;
		} else if (!engineDescriptor->capabilities.testFlag(MvdSearchEngine::MultipleSearchCapability)) {
			query = queries.first();
		} else {
			query = queries.join(";");
			queryCount = queries.size();
		}

		d->resultsPage->setProgress(0);
		d->resultsPage->setProgressMaximum(d->searchSteps * queryCount);

		emit searchRequest(query, engineId);

	} break;
	case MvdImportDialog_P::SummaryPage:
	{
		setButtonText(QWizard::CommitButton, tr("&Import"));

		QList<int> jobs = d->resultsPage->jobs();
		int count = jobs.size();
		d->summaryPage->setProgress(0);
		d->summaryPage->setProgressMaximum(d->importSteps * count);
		emit importRequest(jobs);
	} break;
	case MvdImportDialog_P::FinalPage:
	{
		Q_ASSERT(QMetaObject::invokeMethod(d->finalPage, "importMovies", Qt::QueuedConnection,
			Q_ARG(MvdMovieDataList, d->summaryPage->movies())));
	} break;
	default: ;
	}

	MvdImportExportPage* p = qobject_cast<MvdImportExportPage*>(page(id));
	if (p)
		p->updateButtons();
}

//! Convenience method. Returns true if the current page has the preventCloseWhenBusy property set to true.
bool MvdImportDialog::preventCloseWhenBusy() const
{
	if (MvdImportExportPage* p = qobject_cast<MvdImportExportPage*>(currentPage()))
		if (p->preventCloseWhenBusy())
			return true;
	return false;
}

//! Convenience method. Returns true if the current page is busy.
bool MvdImportDialog::isBusy() const
{
	if (MvdImportExportPage* p = qobject_cast<MvdImportExportPage*>(currentPage()))
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

	d->closing = true;
	QWizard::closeEvent(e);
}

//! Prevents the dialog to close if the current page is busy.
void MvdImportDialog::reject()
{
	if (!d->closing) {
		if ((isBusy() && preventCloseWhenBusy()) || !confirmCloseWizard()) {
			return;
		}
	}

	QWizard::reject();
}

//! Prevents the dialog to close if the current page is busy.
void MvdImportDialog::keyPressEvent(QKeyEvent* e)
{
	switch (e->key()) {
	case Qt::Key_Escape: 
		if ((isBusy() && preventCloseWhenBusy()) || !confirmCloseWizard()) {
			return;
		} else d->closing = true;
	default: ;
	}

	QWizard::keyPressEvent(e);
}

//! Asks the user for confirmation before the wizard is closed aborting a progressing operation. Returns true if the wizard should close.
bool MvdImportDialog::confirmCloseWizard()
{
	QString msg;
	bool isImportantPage = currentId() != MvdImportDialog_P::FinalPage && currentId() != MvdImportDialog_P::StartPage;

	if (!isBusy()) {
		if (!isImportantPage)
			return true;
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
