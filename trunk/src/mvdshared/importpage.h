/**************************************************************************
** Filename: importpage.h
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

#ifndef MVD_IMPORTPAGE_H
#define MVD_IMPORTPAGE_H

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
	enum MessageType { InfoMessage, ErrorMessage };

	MvdImportPage(QWidget* parent = 0)
	: QWizardPage(parent), busy(false) { }
	
	virtual void setBusyStatus(bool busy)
	{ this->busy = busy; emit completeChanged(); }

	bool busyStatus() const { return busy; }

	virtual void showMessage(const QString& msg, MessageType t = InfoMessage)
	{ Q_UNUSED(msg); Q_UNUSED(t); }

	//! Re-implements the superclass method to ensure that the status is not busy.
	virtual bool isComplete() const
	{ return !busy && QWizardPage::isComplete(); }

private:
	bool busy;
};

#endif // MVD_IMPORTPAGE_H
