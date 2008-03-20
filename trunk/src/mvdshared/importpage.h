/**************************************************************************
** Filename: importpage.h
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

#ifndef MVD_IMPORTPAGE_H
#define MVD_IMPORTPAGE_H

#include "importdialog.h"
#include <QWizardPage>

/*!
	\class MvdImportPage importpage.h
	\ingroup MovidaShared

	\brief Base class for a movie import wizard page.
*/

class MvdImportPage : public QWizardPage
{
	Q_OBJECT

public:
	MvdImportPage(QWidget* parent = 0)
	: QWizardPage(parent), busy(false), preventCloseOnBusy(false) { }
	
	virtual void setBusyStatus(bool busy)
	{ this->busy = busy; emit completeChanged(); }

	bool busyStatus() const { return busy; }

	virtual void showMessage(const QString& msg, MvdImportDialog::MessageType t)
	{ Q_UNUSED(msg); Q_UNUSED(t); }

	//! Re-implements the superclass method to ensure that the status is not busy.
	virtual bool isComplete() const
	{ return !busy && QWizardPage::isComplete(); }

	//! Set this property to prevent the wizard being closed on a busy page. Default is false.
	bool preventCloseWhenBusy() const
	{ return preventCloseOnBusy; }

	void setPreventCloseWhenBusy(bool prevent)
	{ preventCloseOnBusy = prevent; }

	/*! Re-implement this method to force enabling or disabling of the wizard buttons.
		Some of the wizard buttons (e.g. "Finish" and "Next"/"Back") cannot be disabled
		with a simple QWidget::setEnabled() because the wizard will change their state, unless
		it is done in this method.
	*/
	virtual void updateButtons()
	{ }
	
	//! This method is supposed to do what it tells.. reset the page and prepare for a new search.
	virtual void reset()
	{ }

	MvdImportDialog* importDialog() const
	{
		MvdImportDialog* w = qobject_cast<MvdImportDialog*>(wizard());
		Q_ASSERT_X(w, "MvdImportPage::importDialog()", "Internal error.");
		return w;
	}

private:
	bool busy;
	bool preventCloseOnBusy;
};

#endif // MVD_IMPORTPAGE_H
