/**************************************************************************
** Filename: searchengine.h
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

#ifndef MVD_SEARCHENGINE_H
#define MVD_SEARCHENGINE_H

#include "sharedglobal.h"

#include <QtCore/QString>
#include <QtCore/QUrl>

/*!
    \class MvdSearchEngine searchengine.h
    \ingroup MovidaShared

    \brief Describes a local/network search engine.
*/

class MvdSearchEngine
{
public:
    enum EngineCapability {
        NoEngineCapability = 0,
        MultipleSearchCapability = 1
    };
    Q_DECLARE_FLAGS(EngineCapabilities, EngineCapability);

    //! Creates a new search engine.
    MvdSearchEngine() :
        canConfigure(false) { }

    //! Creates a new search engine with given name.
    MvdSearchEngine(const QString &displayName) :
        name(displayName),
        canConfigure(false)
    { }

    QString name;
    QString validator;
    EngineCapabilities capabilities;

    bool canConfigure;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(MvdSearchEngine::EngineCapabilities)

#endif // MVD_SEARCHENGINE_H
