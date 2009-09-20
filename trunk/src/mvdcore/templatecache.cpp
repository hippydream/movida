/**************************************************************************
** Filename: templatecache.cpp
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

#include "templatecache.h"

#include "core.h"
#include "moviecollection.h"
#include "pathresolver.h"
#include "templatemanager.h"

#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/QUrl>

#include <stdexcept>

using namespace Movida;

Q_GLOBAL_STATIC(QMutex, MvdTemplateCacheLock)

namespace {
static const int MaxCachedPages = 100;
}


/*!
    \class MvdTemplateCache templatecache.h
    \ingroup MvdCore Singletons

    <b>Movida::tcache()</b> can be used as a convenience method to access the singleton.

    \brief Caches global or collection specific template instantiations.
*/


/************************************************************************
    MvdTemplateCache::Private
 *************************************************************************/

//! \internal
class MvdTemplateCache::Private
{
public:
    Private() :
        manualCacheId(0) { }

    ~Private() {
        purgeCache();
    };

    void reduceCache();
    void purgeCache();
    QString retrievePath(MvdMovieCollection *collection);

    struct CachedPage {
        CachedPage(quint64 id, const QString &s, MvdMovieCollection *c = 0) :
            id(id),
            collection(c),
            path(s),
            usage(0) { }

        bool operator<(const CachedPage &o) const { return usage > o.usage; }

        bool operator==(const CachedPage &o) const { return id == o.id && collection == o.collection; }

        quint64 id;
        MvdMovieCollection *collection;
        QString path;
        int usage;
    };
    typedef QList<CachedPage> PageCache;

    PageCache automaticCache;
    PageCache manualCache;
    quint64 manualCacheId;

    QString cacheDir;
    QString templateName;
    QString blank;

    QList<MvdMovieCollection *> registeredCollections;
};

QString MvdTemplateCache::Private::retrievePath(MvdMovieCollection *collection)
{
    QString path;

    if (collection) {
        path = collection->metaData(MvdMovieCollection::TempPathInfo).append("/templates/");
    } else {
        if (cacheDir.isEmpty()) {
            cacheDir = Movida::paths().generateTempDir();
        }
        path = QString(cacheDir).append("/templates/");
    }

    if (!QFile::exists(path)) {
        QDir dir;
        dir.mkpath(path);
    }

    return path;
}

void MvdTemplateCache::Private::reduceCache()
{
    qSort(automaticCache);
    int last = automaticCache.size();
    while (automaticCache.size() > (MaxCachedPages / 2)) {
        CachedPage p = automaticCache.takeAt(--last);
        QFile::remove(p.path);
    }
}

void MvdTemplateCache::Private::purgeCache()
{
    while (!automaticCache.isEmpty()) {
        CachedPage p = automaticCache.takeAt(0);
        QFile::remove(p.path);
    }

    while (!manualCache.isEmpty()) {
        CachedPage p = manualCache.takeAt(0);
        QFile::remove(p.path);
    }
}

/************************************************************************
    MvdTemplateCache
 *************************************************************************/

//! \internal
volatile MvdTemplateCache *MvdTemplateCache::mInstance = 0;
bool MvdTemplateCache::mDestroyed = false;

//! \internal Private constructor.
MvdTemplateCache::MvdTemplateCache() :
    d(new Private)
{ }

/*!
    Returns the unique application instance.
*/
MvdTemplateCache &MvdTemplateCache::instance()
{
    if (!mInstance) {
        QMutexLocker locker(MvdTemplateCacheLock());
        if (!mInstance) {
            if (mDestroyed) throw std::runtime_error("Template Cache: access to dead reference");
            create();
        }
    }

    return (MvdTemplateCache &) * mInstance;
}

//! Destructor.
MvdTemplateCache::~MvdTemplateCache()
{
    delete d;
    mInstance = 0;
    mDestroyed = true;
}

void MvdTemplateCache::create()
{
    // Local static members are instantiated as soon
    // as this function is entered for the first time
    // (Scott Meyers singleton)
    static MvdTemplateCache instance;

    mInstance = &instance;
}

QString MvdTemplateCache::blank(MvdMovieCollection *collection)
{
    if (collection)
        registerCollection(collection);

    if (d->blank.isEmpty()) {
        d->blank = d->retrievePath(collection);
        d->blank.append("blank.html");
        if (QFile::exists(d->blank))
            QFile::remove(d->blank);
        bool res = Movida::tmanager().collectionToHtmlFile(
            collection, d->blank, QLatin1String("BrowserView"), d->templateName);
        if (!res) {
            d->blank.clear();
            return QString();
        }
    }

    return d->blank;
}

QString MvdTemplateCache::movie(MvdMovieCollection *collection, mvdid id)
{
    QString path;

    if (!collection || id == MvdNull)
        return path;

    registerCollection(collection);
    int idx = d->automaticCache.indexOf(Private::CachedPage(id, QString(), collection));
    if (idx >= 0) {
        Private::CachedPage &cp = d->automaticCache[idx];
        ++cp.usage;
        path = cp.path;
    }

    //! \todo support multiple collections

    if (path.isEmpty()) {
        // Parse and cache HTML
        path = d->retrievePath(false);
        path.append(QString("bvt_%1.html").arg(id));
        if (QFile::exists(path)) QFile::remove(path);

        bool res = Movida::tmanager().movieToHtmlFile(collection->movie(id), *collection,
            path, QLatin1String("BrowserView"), d->templateName);

        if (res) {
            if (d->automaticCache.size() == MaxCachedPages)
                d->reduceCache();
            d->automaticCache.append(Private::CachedPage(id, path, collection));
        } else path.clear();
    }

    return path;
}

int MvdTemplateCache::cacheMovieData(const MvdMovieData &md)
{
    QString path = d->retrievePath(0);

    if (!QFile::exists(path)) {
        QDir dir;
        dir.mkpath(path);
    }

    quint64 id = ++d->manualCacheId;
    path.append(QString("bvrt_%1.html").arg(id));
    if (QFile::exists(path)) QFile::remove(path);

    bool res = Movida::tmanager().movieDataToHtmlFile(md,
        path, QLatin1String("BrowserView"), d->templateName);

    if (res) {
        d->manualCache.append(Private::CachedPage(id, path));
        return id;
    }

    return -1;
}

QString MvdTemplateCache::movieData(int id)
{
    if (id < 0) {
        return QString();
    }

    foreach(const Private::CachedPage & p, d->manualCache)
    {
        if (p.id == (quint64)id) {
            return p.path;
        }
    }

    return QString();
}

void MvdTemplateCache::clearCachedMovieData(int id)
{
    for (int i = 0; i < d->manualCache.size(); ++i) {
        const Private::CachedPage &p = d->manualCache.at(i);
        if (p.id == (quint64)id) {
            d->manualCache.removeAt(i);
            break;
        }
    }
}

void MvdTemplateCache::invalidateMovie(mvdid id)
{
    MvdMovieCollection *collection = qobject_cast<MvdMovieCollection *>(sender());
    // Remove movie from cache
    int idx = d->automaticCache.indexOf(Private::CachedPage(id, QString(), collection));

    if (idx < 0)
        return;

    QString path = d->automaticCache[idx].path;
    QFile::remove(path);
    d->automaticCache.removeAt(idx);
}

void MvdTemplateCache::invalidateCollection()
{
    // Remove blank page from cache
    if (d->blank.isEmpty()) return;
    QFile::remove(d->blank);
    d->blank.clear();
}

void MvdTemplateCache::registerCollection(MvdMovieCollection *collection)
{
    if (collection) {
        if (d->registeredCollections.indexOf(collection) >= 0) return;
        connect(collection, SIGNAL(movieRemoved(mvdid)), SLOT(invalidateMovie(mvdid)));
        connect(collection, SIGNAL(movieChanged(mvdid)), SLOT(invalidateMovie(mvdid)));
        connect(collection, SIGNAL(changed()), SLOT(invalidateCollection()));
        connect(collection, SIGNAL(destroyed(QObject *)), SLOT(deregisterCollection(QObject *)));
        d->registeredCollections.append(collection);
    }
}

void MvdTemplateCache::deregisterCollection(QObject *o)
{
    MvdMovieCollection *collection = static_cast<MvdMovieCollection *>(o);

    if (collection) {
        d->registeredCollections.removeAll(collection);
        for (int i = 0; i < d->automaticCache.size(); ++i) {
            const Private::CachedPage &p = d->automaticCache.takeAt(i);
            if (p.collection != collection) continue;
            QFile::remove(p.path);
            d->automaticCache.removeAt(i);
            --i;
        }
    }
}

QString MvdTemplateCache::cacheDirectory() const
{
    return d->cacheDir;
}

//! Convenience method to access the MvdTemplateCache singleton.
MvdTemplateCache &Movida::tcache()
{
    return MvdTemplateCache::instance();
}
