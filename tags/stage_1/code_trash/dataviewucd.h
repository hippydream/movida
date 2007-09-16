/****************************************************************************
** Filename: dataviewucd.h
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** Common interface for unique application-wide column descriptors.
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

#ifndef MOVIDA_DATAVIEWUCD__H
#define MOVIDA_DATAVIEWUCD__H

#include <QObject>

#define DATAVIEW_ROLE_TYPE Qt::UserRole + 0
#define DATAVIEW_ROLE_ID Qt::UserRole + 1
#define DATAVIEW_ROLE_PRIORITY Qt::UserRole + 2
#define DATAVIEW_ROLE_TIME Qt::UserRole + 3

class DataViewUCD : public QObject
{
	Q_OBJECT

public:
	DataViewUCD() : QObject() {}

	enum ColumnType { CT_TEXT, CT_INT, CT_FLOAT, CT_TIME, CT_RESOLUTION, CT_FRACTION };

	virtual QString resolveLabel(int columnID) const = 0;
	virtual ColumnType resolveType(int columnID) const = 0;
};

#endif // MOVIDA_DATAVIEWUCD__H
