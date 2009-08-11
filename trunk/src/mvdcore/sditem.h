/**************************************************************************
** Filename: sditem.h
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

#ifndef MVD_SDITEM_H
#define MVD_SDITEM_H

#include "global.h"

#include <QtCore/QList>
#include <QtCore/QString>

/*!
    \class MvdSdItem sditem.h
    \ingroup MvdCore

    \brief A MvdSdItem represents a data item that can be shared between multiple movies or other items.
    As an example, a MvdSdItem can represent a person and thus be associated to a specific movie as an
    actor and to another movie as a director.
    Each item can have an identifier (i.e. an IMDb id) thus allowing similar items to be identified without
    comparing more complex fields (i.e. person names with first name, last name and possible variants).
*/

//! Internal data structure for shared data items.
class MvdSdItem
{
public:
    struct Url {
        inline Url() :
            isDefault(false) { }

        inline Url(const QString &aurl, const QString &adescription = QString(), bool adefault = false) :
            url(aurl),
            description(adescription),
            isDefault(adefault) { }

        inline bool operator==(const Url &o) const
        {
            return url.trimmed().toLower() == o.url.trimmed().toLower();
        }

        inline bool operator<(const Url &o) const
        {
            return url.trimmed().toLower() < o.url.trimmed().toLower();
        }

        inline bool exactMatch(const Url &o) const
        {
            if (!(*this == o))
                return false;
            return description.trimmed() == o.description.trimmed()
                   && isDefault == o.isDefault;
        }

        QString url;
        QString description;
        bool isDefault;
    };

    inline MvdSdItem() :
        role(Movida::NoRole) { }

    inline MvdSdItem(Movida::DataRole arole, QString avalue, QString adescription = QString(), QString aid = QString()) :
        role(arole),
        value(avalue),
        description(adescription),
        id(aid) { }

    inline bool operator==(const MvdSdItem &o) const
    {
        if (!id.isEmpty() && !o.id.isEmpty())
            return id.toLower() == o.id.toLower();
        return value.toLower() == o.value.toLower();
    }

    //! This defines what this item is about (a person, an URL, a movie genre, etc.).
    Movida::DataRole role;

    //! I.e. a person name or a URL
    QString value;
    //! I.e. a title for a URL or some note
    QString description;
    //! I.e. an IMDb id
    QString id;

    //! Movies referencing this item.
    QList<mvdid> movies;
    //! Persons referencing this item.
    QList<mvdid> persons;

    //! Interesting URLs for this item.
    QList<Url> urls;
};

typedef MvdSdItem::Url MvdUrl;

#endif // MVD_SDITEM_H
