/**************************************************************************
** Filename: mainsettingspage.cpp
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

#include "mainsettingspage.h"
#include "core.h"
#include "settings.h"

#include <QIcon>

/*!
	\class MvdMainSettingsPage mainsettingspage.h

	\brief General settings widget for preferences dialog.
*/


/*!
	Creates a new page.
*/
MvdMainSettingsPage::MvdMainSettingsPage(QWidget* parent)
: MvdSettingsPage(parent)
{
	setupUi(this);
	connect( clearMRULabel, SIGNAL(linkActivated(const QString&)), this, SLOT(linkActivated(const QString&)) );
	int max = MvdCore::parameter("movida-maximum-recent-files").toInt();
	int current = Movida::settings().getInt("maximum-recent-files", "movida");
	maximumMRU->setMaximum(max);
	maximumMRU->setValue(current);
}

/*!
	Apply and store changes.
*/
void MvdMainSettingsPage::store()
{
}

/*!
	Reset to default values.
*/
void MvdMainSettingsPage::reset()
{
}

/*!
	Returns the title to be used for this page.
*/
QString MvdMainSettingsPage::label()
{
	return tr("General");
}

/*!
	Returns the icon to be used for this page.
*/
QIcon MvdMainSettingsPage::icon()
{
	return QIcon(":/images/preferences/general.png");
}

//! \internal
void MvdMainSettingsPage::linkActivated(const QString& url)
{
	if (!url.startsWith("movida://"))
		return;

	QRegExp rx("^movida://([^/]*)/?([^/]*)?$");
	if (rx.indexIn(url) >= 0)
	{
		QString op = rx.cap(1);
		QString param = rx.cap(2);

		if (op == "clear")
		{
			if (param == "recentfiles")
			{
				//emit externalActionTriggered();
			}
		}
	}
}
