/****************************************************************************
** Filename: movspecial.cpp
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** DVD Special Contents editing widget for movie editing dialog.
**
** Copyright (C) 2006 Angius Fabrizio. All rights reserved.
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
**********************************************************************/

#include <QIcon>

#include "movspecial.h"

/*!
	\class MovieInfoSpecialContents movspecial.h

	\brief DVD Special Contents editing widget for movie editing dialog.
*/

/*!
	Creates a new page.	
*/
MovieInfoSpecialContents::MovieInfoSpecialContents()
{
	setupUi(this);
}

/*!
	
*/
MovieInfoSpecialContents::~MovieInfoSpecialContents()
{
}

/*!
	Apply and store changes.
*/
void MovieInfoSpecialContents::store()
{
}

/*!
	Reset to default values.
*/
void MovieInfoSpecialContents::reset()
{
}

/*!
	Returns the title to be used for this page.
*/
QString MovieInfoSpecialContents::label()
{
	return tr("DVD Special Contents");
}

/*!
	Returns the icon to be used for this page.
*/
QIcon MovieInfoSpecialContents::icon()
{
	return QIcon(":/images/preferences/log.png");
}
