/****************************************************************************
** Filename: movspecial.h
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


#ifndef MOVIDA_MOVSPECIAL__H
#define MOVIDA_MOVSPECIAL__H

#include "ui_movspecial.h"
#include "complexpage.h"

class QIcon;

class MovieInfoSpecialContents : public ComplexPage, private Ui::MovieInfoSpecialContents
{
	Q_OBJECT
		
public:
	MovieInfoSpecialContents();
	~MovieInfoSpecialContents();
	
	void store();
	void reset();
	QString label();
	QIcon icon();
};

#endif // MOVIDA_MOVSPECIAL__H