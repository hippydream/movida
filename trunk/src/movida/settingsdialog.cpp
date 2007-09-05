/**************************************************************************
** Filename: settingsdialog.cpp
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

#include "settingsdialog.h"
#include "settings.h"
#include "mainsettingspage.h"
#include <QMessageBox>
#include <QCloseEvent>
#include <QPushButton>

/*!
	\class MvdSettingsDialog settingsdialog.h
	\ingroup Movida

	\brief Settings editor dialog.
*/


/*!
	Creates a new settings editor.
*/
MvdSettingsDialog::MvdSettingsDialog(QWidget* parent)
: MvdMultiPageDialog(parent)
{
	setWindowTitle(tr("movida settings"));

	QDialogButtonBox* box = MvdMultiPageDialog::buttonBox();
	box->addButton(QDialogButtonBox::Help);
	box->addButton(QDialogButtonBox::Reset);
	box->addButton(QDialogButtonBox::Apply);
	box->addButton(QDialogButtonBox::Ok);
	box->addButton(QDialogButtonBox::Cancel);

	MvdMPDialogPage* page;

	page = new MvdMainSettingsPage(this);
	addPage(page);

	connect( box, SIGNAL(rejected()), this, SLOT(cancelSettings()) );
	connect( box, SIGNAL(accepted()), this, SLOT(applySettings()) );
}

//! \internal 
void MvdSettingsDialog::cancelSettings()
{
	reject();
}

//! \internal \todo Handle ESC key
void MvdSettingsDialog::closeEvent(QCloseEvent* e)
{
	cancelSettings();
	e->ignore();
}

//! \internal 
void MvdSettingsDialog::applySettings()
{
}
