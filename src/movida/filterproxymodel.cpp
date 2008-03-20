/**************************************************************************
** Filename: filterproxymodel.cpp
**
** Copyright (C) 2007-2008 Angius Fabrizio. All rights reserved.
**
** This file is part of the Movida project (http://movida.42cows.org/).
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

//! Creates a new filter proxy that defaults to a quick search on the movie title attribute (localized and original).
MvdFilterProxyModel::MvdFilterProxyModel(QObject* parent)
: QSortFilterProxyModel(parent)
{
	mMovieAttributes << Movida::TitleAttribute << Movida::OriginalTitleAttribute;
	mSortColumn = (int) Movida::TitleAttribute;
	mSortOrder = Qt::AscendingOrder;
}

void MvdFilterProxyModel::setQuickFilterAttributes(const QByteArray& alist)
{
	mMovieAttributes.clear();
	for (int i = 0; i < alist.size(); ++i)
		mMovieAttributes << (Movida::MovieAttribute)alist.at(i);
}

QByteArray MvdFilterProxyModel::quickFilterAttributes() const
{
	QByteArray ba(mMovieAttributes.size(), '\0');

	for (int i = 0; i < mMovieAttributes.size(); ++i)
		ba[i] = (const char)mMovieAttributes.at(i);
	return ba;
}

bool MvdFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	for (int i = 0; i < mMovieAttributes.size(); ++i) {
		QModelIndex index = sourceModel()->index(sourceRow, (int)mMovieAttributes.at(i), sourceParent);
		if (sourceModel()->data(index).toString().contains(filterRegExp()))
			return true;
	}
	return false;
}

//! Convenience method only.
void MvdFilterProxyModel::sortByAttribute(Movida::MovieAttribute attr, Qt::SortOrder order)
{
	sort((int) attr, order);
}

void MvdFilterProxyModel::sort(int column, Qt::SortOrder order)
{
	mSortColumn = column;
	mSortOrder = order;
	QSortFilterProxyModel::sort(column, order);
	emit sorted();
}

int MvdFilterProxyModel::sortColumn() const
{
	return mSortColumn;
}

Movida::MovieAttribute MvdFilterProxyModel::sortAttribute() const
{
	return (Movida::MovieAttribute) sortColumn();
}

Qt::SortOrder MvdFilterProxyModel::sortOrder() const
{
	return mSortOrder;
}
