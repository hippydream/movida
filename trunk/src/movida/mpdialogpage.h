/**************************************************************************
** Filename: mpdialogpage.h
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

#ifndef MVD_MPDIALOGPAGE_H
#define MVD_MPDIALOGPAGE_H

#include "multipagedialog.h"
#include <QWidget>
#include <QVariant>

class QIcon;

/*!
	\class MvdMPDialogPage mpdialogpage.h
	\ingroup Movida

	\brief Common interface to MvdMultiPageDialog pages.
*/

class MvdMPDialogPage : public QWidget
{
	Q_OBJECT

public:
	MvdMPDialogPage(MvdMultiPageDialog* parent = 0) : QWidget(parent), 
		mValid(true), mModified(false), mDialog(parent) {};

	MvdMultiPageDialog* dialog() const { return mDialog; }

	//! Subclasses should reset all the values to their default.
	virtual void reset() = 0;
	//! Subclasses should return a title to be displayed somewhere in the dialog.
	virtual QString label() = 0;
	//! Subclasses should return an icon that might be displayed somewhere in the dialog.
	virtual QIcon icon() = 0;

	//! Returns true if this page is valid and no required user input is missing.
	bool isValid() const { return mValid; }
	//! Sets this page to valid or not (default is true). This method will emit a validationStateChanged() signal if the status changes.
	void setValid(bool valid) { if (mValid != valid) { mValid = valid; emit validationStateChanged(); } }

	//! Returns true if this page has been modified.
	bool isModified() const { return mModified; }
	//! Sets this page to modified or not (default is false). This method will emit a modifiedStateChanged() signal if the status changes.
	void setModified(bool modified) { if (mModified != modified) { mModified = modified; emit modifiedStateChanged(); } }

	//! Subclasses can implement this to set the focus on the main widget.
	virtual void setMainWidgetFocus() {};

signals:
	void externalActionTriggered(const QString& id, const QVariant& data = QVariant());
	//! Emitted whenever the this page changes its 'valid' state.
	void validationStateChanged();
	//! Emitted whenever the this page changes its 'modified' state.
	void modifiedStateChanged();

private:
	bool mValid;
	bool mModified;
	MvdMultiPageDialog* mDialog;
};

#endif // MVD_MPDIALOGPAGE_H
