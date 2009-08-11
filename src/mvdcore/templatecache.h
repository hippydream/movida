/**************************************************************************
** Filename: templatecache.h
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

#ifndef MVD_TEMPLATECACHE_H
#define MVD_TEMPLATECACHE_H

#include "global.h"
#include "moviedata.h"

class MvdMovieCollection;

class MVD_EXPORT MvdTemplateCache : public QObject
{
    Q_OBJECT

public:
    static MvdTemplateCache &instance();

    QString blank(MvdMovieCollection *collection);
    QString movie(MvdMovieCollection *collection, mvdid id);
    int cacheMovieData(const MvdMovieData &md);
    QString movieData(int id);
    void clearCachedMovieData(int id);

private slots:
    void invalidateMovie(mvdid id);
    void invalidateCollection();
    void deregisterCollection(QObject *o);

private:
    MvdTemplateCache();
    MvdTemplateCache(const MvdTemplateCache &);
    MvdTemplateCache &operator=(const MvdTemplateCache &);
    virtual ~MvdTemplateCache();

    void registerCollection(MvdMovieCollection *collection);

    static void create();
    static volatile MvdTemplateCache *mInstance;
    static bool mDestroyed;

    class Private;
    Private *d;
};

namespace Movida {
MVD_EXPORT extern MvdTemplateCache &tcache();
}

#endif // MVD_TEMPLATECACHE_H
