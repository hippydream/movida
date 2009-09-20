/**************************************************************************
** Filename: utils.h
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

#ifndef MVD_UTILS_H
#define MVD_UTILS_H

#include "global.h"

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtGui/QStandardItemModel>

namespace Movida {
MVD_EXPORT QStringList splitArgs(const QString &s, int idx);
MVD_EXPORT QVariant stringToVariant(const QString &s);
MVD_EXPORT QString variantToString(const QVariant &v);

template <typename T>
T variantValue(const QVariant &v, T defaultValue) {
    return v.isNull() ? defaultValue : v.value<T>();
}


/*! Simple functor using QString::localeAwareCompare().

    Sample usage:
    QStringList list;
    qSort(list.begin(), list.end(), LocaleAwareSorter());
*/
struct LocaleAwareSorter {
    inline bool operator()(const QString &a, const QString &b) const {
        return QString::localeAwareCompare(a, b) < 0;
    }
};

MVD_EXPORT QString normalized(const QString &s);
MVD_EXPORT QString &normalize(QString &s);

} // Movida namespace


/////////////////////////////////////////////////////////////////////


class MVD_EXPORT MvdNormalizedItemModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit MvdNormalizedItemModel(QObject *parent = 0);
    MvdNormalizedItemModel(int rows, int columns, QObject *parent = 0);
    virtual ~MvdNormalizedItemModel();

    virtual QModelIndexList match(const QModelIndex &start, int role,
        const QVariant &value, int hits = 1,
        Qt::MatchFlags flags =
        Qt::MatchFlags(Qt::MatchStartsWith|Qt::MatchWrap)) const;

protected:
    virtual bool compare(const QVariant &a, const QVariant &b) const;
};

#endif // MVD_UTILS_H
