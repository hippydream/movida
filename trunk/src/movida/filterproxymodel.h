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

    void setQuickFilterAttributes(const QByteArray &alist);
    QByteArray quickFilterAttributes() const;

    int sortColumn() const;
    Movida::MovieAttribute sortAttribute() const;
    Qt::SortOrder sortOrder() const;

    void sortByAttribute(Movida::MovieAttribute attribute, Qt::SortOrder order = Qt::AscendingOrder);
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    virtual bool setFilterAdvancedString(const QString &q);
    virtual QString filterAdvancedString() const;

signals:
    void sorted();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    struct Function {
        Function(Movida::FilterFunction ff, const QStringList &p, bool negated) :
            type(ff),
            parameters(p),
            neg(negated) { }

        Movida::FilterFunction type;
        QStringList parameters;
        bool neg;
    };

    bool rebuildPatterns();
    bool testFunction(int sourceRow, const QModelIndex &sourceParent,
    const Function &function) const;
    inline QList<mvdid> idList(const QStringList &sl) const;

    QList<Movida::MovieAttribute> mMovieAttributes;
    int mSortColumn;
    Qt::SortOrder mSortOrder;

    QString mQuery;
    bool mInvalidQuery;
    QList<Function> mFunctions;
    QStringList mPlainStrings;
};

#endif // MVD_FILTERPROXYMODEL_H
