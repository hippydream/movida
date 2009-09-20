/**************************************************************************
** Filename: filterproxymodel.h
**
** Copyright (C) 2007-2009 Angius Fabrizio. All rights reserved.
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

#ifndef MVD_FILTERPROXYMODEL_H
#define MVD_FILTERPROXYMODEL_H

#include "guiglobal.h"

#include <QtCore/QByteArray>
#include <QtGui/QSortFilterProxyModel>

class MvdFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    MvdFilterProxyModel(QObject *parent = 0);
    virtual ~MvdFilterProxyModel();

    void setQuickFilterAttributes(const QByteArray &alist);
    QByteArray quickFilterAttributes() const;

    int sortColumn() const;
    Movida::MovieAttribute sortAttribute() const;
    Qt::SortOrder sortOrder() const;

    void sortByAttribute(Movida::MovieAttribute attribute, Qt::SortOrder order = Qt::AscendingOrder);
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    virtual bool setFilterAdvancedString(const QString &q);
    virtual QString filterAdvancedString() const;

    virtual void setSourceModel(QAbstractItemModel *sourceModel);

signals:
    void sorted();

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

protected slots:
    virtual void onSourceDataChanged(const QModelIndex &, const QModelIndex &);
    virtual void onSourceRowsAboutToBeRemoved(const QModelIndex &, int, int);
    virtual void onSourceModelDestroyed(QObject*);

private:
    class Private;
    Private *d;
};

#endif // MVD_FILTERPROXYMODEL_H
