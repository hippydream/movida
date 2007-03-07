/**************************************************************************
** Filename: multipagedialog.cpp
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

#include <QVariant>

#include "multipagedialog.h"
#include "mpdialogpage.h"
#include "guiglobal.h"

/*! 
	\class MvdMultiPageDialog complexdlg.h

	\brief Base class for a multi page dialog.
*/


/*!
	Creates a new empty dialog.
*/
MvdMultiPageDialog::MvdMultiPageDialog(QWidget* parent)
: QDialog(parent)
{
	setupUi(this);

	MVD_WINDOW_ICON
}

/*!
	Adds a new page to the dialog and returns an internally assigned id (or a
	negative value if the page could not be added).
*/
int MvdMultiPageDialog::addPage(MvdMPDialogPage* p)
{
	if (p == 0)
		return -1;

	p->setContentsMargins(10, 10, 10, 10);
	int id = contents->addTab(p, p->label());

	connect( p, SIGNAL(externalActionTriggered(const QString&, const QVariant&)),
		this, SLOT(dispatchActionTriggered(const QString&, const QVariant&)) );

	return id;
}

//! Shows a specific page.
void MvdMultiPageDialog::showPage(MvdMPDialogPage* p)
{
	contents->setCurrentWidget(p);
}

//! Returns a pointer to the button container.
QDialogButtonBox* MvdMultiPageDialog::buttonBox() const
{
	return Ui::MvdMultiPageDialog::buttonBox;
}
