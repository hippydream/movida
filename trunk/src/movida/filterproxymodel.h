/**************************************************************************
** Filename: filterproxymodel.h
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

#ifndef MVD_FILTERPROXYMODEL_H
#define MVD_FILTERPROXYMODEL_H

#include "guiglobal.h"
#include <QSortFilterProxyModel>
#include <QByteArray>

class MvdFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	MvdFilterProxyModel(QObject* parent = 0);

	void setQuickFilterAttributes(const QByteArray& alist);
	QByteArray quickFilterAttributes() const;

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;

private:
	QList<Movida::MovieAttribute> mMovieAttributes;
};

#endif // MVD_FILTERPROXYMODEL_H
