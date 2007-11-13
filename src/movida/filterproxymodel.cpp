/**************************************************************************
** Filename: filterproxymodel.cpp
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

#include "filterproxymodel.h"

MvdFilterProxyModel::MvdFilterProxyModel(QObject* parent)
: QSortFilterProxyModel(parent)
{
}

bool MvdFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
	QModelIndex index1 = sourceModel()->index(sourceRow, 1, sourceParent);
	QModelIndex index2 = sourceModel()->index(sourceRow, 2, sourceParent);
	
	return (sourceModel()->data(index0).toString().contains(filterRegExp())
			|| sourceModel()->data(index1).toString().contains(filterRegExp()));
}
