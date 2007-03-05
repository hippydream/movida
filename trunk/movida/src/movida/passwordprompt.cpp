/**************************************************************************
** Filename: passwordprompt.cpp
** Revision: 1
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

#include "passwordprompt.h"
#include "guiglobal.h"
#include "logger.h"

#include <QMessageBox>


/*!
	\class MvdPasswordPrompt passwordprompt.h
	\ingroup movida

	\brief Simple dialog to enter a password or skip encrypted files.
*/

/*!
	Creates a new dialog for given file.
*/
MvdPasswordPrompt::MvdPasswordPrompt(const QString& file, QWidget* parent)
: QDialog(parent), mStatus(ExitOk)
{
	setupUi(this);
	
	MVD_WINDOW_ICON
	
	connect(mPB_Ok, SIGNAL(clicked()), this, SLOT(ok()));
	connect(mPB_Skip, SIGNAL(clicked()), this, SLOT(skip()));
	connect(mPB_SkipAll, SIGNAL(clicked()), this, SLOT(skipAll()));
	
	mL_File->setText(file);
}

/*!
	Returns the password entered by the user.
*/
QString MvdPasswordPrompt::password() const
{
	return mLE_Pwd->text();
}

/*!
	Returns the dialog exit status.
*/
MvdPasswordPrompt::ExitStatus MvdPasswordPrompt::exitStatus() const
{
	return mStatus;
}

/*!
	Sets the exit status to ES_OK.
*/
void MvdPasswordPrompt::ok()
{
	if (mLE_Pwd->text().isEmpty())
	{
		QMessageBox::warning(this, _CAPTION_, tr("Please enter a password or skip the file."));
		return;
	}

	mStatus = ExitOk;
	accept();
}

/*!
	Sets the exit status to ExitSkip.
*/
void MvdPasswordPrompt::skip()
{
	mStatus = ExitSkip;
	accept();
}

/*!
	Sets the exit status to ExitSkipAll.
*/
void MvdPasswordPrompt::skipAll()
{
	mStatus = ExitSkipAll;
	accept();
}
