/****************************************************************************
** Filename: defaultucdresolver.h
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** Provides default application-wide column identifiers.
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

#ifndef MOVIDA_DEFAULTUCDRESOLVER__H
#define MOVIDA_DEFAULTUCDRESOLVER__H

#include "dataviewucd.h"

class DefaultUCDResolver : public DataViewUCD
{
	Q_OBJECT
	
public:
	DefaultUCDResolver() : DataViewUCD() { };
	~DefaultUCDResolver() { };
	
	QString resolveLabel(int /*columnID*/) const
	{
		return QString();
	}
	
	ColumnType resolveType(int /*columnID*/) const
	{
		return CT_TEXT;
	}
};

#endif // MOVIDA_DEFAULTUCDRESOLVER__H
