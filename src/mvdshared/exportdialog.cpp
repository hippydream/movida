/**************************************************************************
** Filename: exportdialog.cpp
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

#include "exportdialog.h"
#include "exportdialog_p.h"
#include "importexportpage.h"
#include "mvdcore/core.h"
#include "mvdcore/plugininterface.h"
#include "mvdcore/settings.h"
#include <QPushButton>
#include <QKeyEvent>
#include <QMessageBox>

/*!
	\class MvdExportDialog exportdialog.h
	\ingroup MovidaShared

	\brief Wizard dialog to export movies to a local or remote destination.
*/


//////////////////////////////////////////////////////////////////////////


/*!
	Creates a new Movida export wizard for the search engines listed in \p engines.
	The dialog is a QWizard subclass so you can use the superclass methods to customize
	the dialog's look:

	\verbatim
	myWizard.setPixmap(QWizard::LogoPixmap, QPixmap(":/myLogo.png"));
	// ...
	myWizard.setWindowTitle("MyPlugin export wizard");
	// ...
	\endverbatim

	Please refer to the QWizard documentation for further details.
*/
MvdExportDialog::MvdExportDialog(QWidget* parent)
: QWizard(parent), d(new Private)
{
	d->closing = false;
	d->errorType = UnknownError;

	d->startPage = new MvdExportStartPage;
	setPage(Private::StartPage, d->startPage);

	d->configPage = new MvdExportConfigPage;
	setPage(Private::ConfigPage, d->configPage);

	d->finalPage = new MvdExportFinalPage;
	setPage(Private::FinalPage, d->finalPage);

	setStartId(Private::StartPage);

	setOption(QWizard::DisabledBackButtonOnLastPage);

	connect( d->startPage, SIGNAL(engineConfigurationRequest(int)), 
		this, SIGNAL(engineConfigurationRequest(int)) );
	connect( this, SIGNAL(currentIdChanged(int)),
		this, SLOT(pageChanged(int)) );

	//! \todo Using import wizard pixmaps in export wizard.
	setPixmap(QWizard::LogoPixmap, QPixmap(":/images/import-wizard/logo.png"));
	setPixmap(QWizard::BannerPixmap, QPixmap(":/images/import-wizard/banner.png"));
	setWindowTitle(tr("Movida export wizard"));
}

//! Handles the page order.
int MvdExportDialog::nextId() const
{
	if (currentId() == Private::FinalPage)
		return -1;

	if (d->exportResult != Success) {
		return Private::FinalPage;
	}

	switch (currentId()) {
	case Private::StartPage:
	{
		bool config = d->startPage->configStepRequired();
		return config ? Private::ConfigPage : Private::FinalPage;
	}
	case Private::ConfigPage:
		return Private::FinalPage;
	default:
		return -1;
	}

	return -1;
}

/*!
	This will signal the current page that the plugin handler has finished handling
	the last request.
	This should be called only after the handler has finished handling a request.
	
	The use of multiple handlers and thus multiple calls to the done() method
	for the same request (i.e. after the searchRequest() signal and before the
	importRequest() signal) leads to undefined behavior!
*/
void MvdExportDialog::done(Result res)
{
	d->exportResult = res;

	MvdImportExportPage* p = dynamic_cast<MvdImportExportPage*>(currentPage());
	if (p)
		p->setBusyStatus(false);
}

//! Returns the result of the latest export request as the client specified when calling done().
MvdExportDialog::Result MvdExportDialog::result() const
{
	return d->exportResult;
}

/*!
	Plugins can call this before calling done() for the last time,
	to show a possible error to the user.
*/
void MvdExportDialog::setErrorType(ErrorType type)
{
	d->errorType = type;
}

MvdExportDialog::ErrorType MvdExportDialog::errorType() const
{
	return d->errorType;
}

//! Shows a status message if the current page supports it.
void MvdExportDialog::showMessage(const QString& msg, MovidaShared::MessageType type)
{
	MvdImportExportPage* p = dynamic_cast<MvdImportExportPage*>(currentPage());
	if (p)
		p->showMessage(msg, type);
}

//!
void MvdExportDialog::accept()
{
	/*QList<mvdid> importedMovies = d->finalPage->importedMovies();
	if (!importedMovies.isEmpty() && d->finalPage->filterImportedMovies()) {
		QString s;
		foreach (mvdid id, importedMovies)
			s.append(QString::number(id).append(","));
		s.truncate(s.length() - 1);
		MvdCore::pluginContext()->properties.insert("movida/movies/filter", s);
	}*/
	QDialog::accept();
}

//! \p id is the new page to be showed.
void MvdExportDialog::pageChanged(int id)
{
	switch (id) {
	case Private::StartPage:
	{
		// Reset all pages
		d->startPage->reset();
		d->exportResult = Success;
		emit resetRequest();
	} break;
	case Private::ConfigPage:
	{

	}
	break;
	case Private::FinalPage:
	{
		
	}
	break;
	default: ;
	}

	MvdImportExportPage* p = qobject_cast<MvdImportExportPage*>(page(id));
	if (p)
		p->updateButtons();
}

//! Convenience method. Returns true if the current page has the preventCloseWhenBusy property set to true.
bool MvdExportDialog::preventCloseWhenBusy() const
{
	if (MvdImportExportPage* p = qobject_cast<MvdImportExportPage*>(currentPage()))
		if (p->preventCloseWhenBusy())
			return true;
	return false;
}

//! Convenience method. Returns true if the current page is busy.
bool MvdExportDialog::isBusy() const
{
	if (MvdImportExportPage* p = qobject_cast<MvdImportExportPage*>(currentPage()))
		if (p->busyStatus())
			return true;
	return false;
}

//! Prevents the dialog to close if the current page is busy.
void MvdExportDialog::closeEvent(QCloseEvent* e)
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
void MvdExportDialog::reject()
{
	if (!d->closing) {
		if ((isBusy() && preventCloseWhenBusy()) || !confirmCloseWizard()) {
			return;
		}
	}

	QWizard::reject();
}

//! Prevents the dialog to close if the current page is busy.
void MvdExportDialog::keyPressEvent(QKeyEvent* e)
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
bool MvdExportDialog::confirmCloseWizard()
{
	QString msg;
	bool isImportantPage = currentId() != Private::FinalPage && currentId() != Private::StartPage;

	if (!isBusy()) {
		if (!isImportantPage)
			return true;
		msg = tr("Are you sure you want to close the wizard?");
	} else {
		/*switch (currentId()) {
		case Private::ResultsPage: msg = tr("Movida is still searching for your query. Are you sure you want to close the wizard?"); break;
		case Private::SummaryPage: msg = tr("Movida is still downloading the movie details. Are you sure you want to close the wizard?"); break;
		case Private::FinalPage: msg = tr("Movida is still importing your new movies. Are you sure you want to close the wizard?"); break;
		default: msg = tr("An operation is still in progress. Are you sure you want to close the wizard?");
		}*/
	}

	return QMessageBox::question(this, MVD_CAPTION, msg, QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes;
}

/*!
	Adds an export engine to the wizard.
*/
int MvdExportDialog::registerEngine(const MvdExportEngine& engine)
{
	QString s = engine.name.trimmed();
	if (!d->startPage || s.isEmpty())
		return -1;

	return d->startPage->registerEngine(engine);
}
