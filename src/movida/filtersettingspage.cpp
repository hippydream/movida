/**************************************************************************
** Filename: filtersettingspage.cpp
** Revision: 1
**
** Copyright (C) 2007-2008 Angius Fabrizio. All rights reserved.
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

#include "filtersettingspage.h"
#include "mvdcore/core.h"
#include "mvdcore/settings.h"
#include <QIcon>

/*!
	\class MvdFilterSettingsPage filtersettingspage.h

	\brief Filter settings widget for preferences dialog.
*/


/*!
	Creates a new page.
*/
MvdFilterSettingsPage::MvdFilterSettingsPage(MvdSettingsDialog* parent)
: MvdSettingsPage(parent)
{
	setupUi(this);
	connect(Ui::MvdFilterSettingsPage::qsResetAttributes, SIGNAL(linkActivated(QString)), this, SLOT(linkActivated(QString)));
	
	QByteArray ba = Movida::settings().value("movida/quick-filter/attributes").toByteArray();
}

/*!
	Apply and store changes.
*/
void MvdFilterSettingsPage::store()
{
}

/*!
	Reset to default values.
*/
void MvdFilterSettingsPage::reset()
{
}

/*!
	Returns the title to be used for this page.
*/
QString MvdFilterSettingsPage::label()
{
	return tr("Filter");
}

/*!
	Returns the icon to be used for this page.
*/
QIcon MvdFilterSettingsPage::icon()
{
	return QIcon(":/images/preferences/filter.png");
}

//! \internal
void MvdFilterSettingsPage::linkActivated(const QString& s)
{

}
