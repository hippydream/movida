/**************************************************************************
** Filename: moviecollection.cpp
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

#include "moviecollection.h"

#include "core.h"
#include "global.h"
#include "logger.h"
#include "md5.h"
#include "movie.h"
#include "moviedata.h"
#include "pathresolver.h"
#include "sditem.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QUuid>
#include <QtGui/QPixmap>

#define __COLLECTION_CHANGED \
    if (!d->modified) { d->modified = true; emit modified(); } \
    emit changed();

/*!
    \class MvdMovieCollection moviecollection.h
    \ingroup MvdCore Shared

    \brief Stores movies and handles collection related metadata.
*/

//! \enum MvdMovieCollection::CollectionInfo Collection related metadata.

namespace {
// Allows quick lookup of potentially duplicate entries
struct QuickLookupEntry {
    quint32 ref;     // reference count
    int released;

    inline QuickLookupEntry() :
        ref(0),
        released(0) { }

    inline QuickLookupEntry(int i) :
        ref(0),
        released(i) { }

    inline QuickLookupEntry(const QuickLookupEntry &other) :
        ref(0),
        released(other.released) { }

    inline bool operator==(const QuickLookupEntry &other) const
    { return released == other.released; }

    inline bool operator==(int other) const { return released == other; }

    inline QuickLookupEntry &operator=(const QuickLookupEntry &other)
    { released = other.released; return *this; }

    inline QuickLookupEntry &operator=(int other)
    { released = other; return *this; }

};
}

//! \internal
class MvdMovieCollection::Private
{
public:
    Private();
    Private(const MvdMovieCollection::Private &m);

    typedef QList<QuickLookupEntry> QuickLookupList;
    typedef QHash<QString, QuickLookupList> QuickLookupTable;

    QAtomicInt ref;

    QString name;
    QString owner;
    QString email;
    QString website;
    QString notes;

    mutable QString dataPath;
    mutable QString tempPath;

    // having titles and years stored in another vector too introduces some
    // redundancy but it makes the search for potential duplicate titles
    // pretty faster
    QuickLookupTable quickLookupTable;
    MvdMovieCollection::MovieList movies;

    mvdid id;
    bool modified;

    QString fileName;
    QString path;

    MvdSharedData smd;
};

//! \internal
bool operator==(int i, const QuickLookupEntry &e)
{
    return i == e.released;
}

//! \internal
MvdMovieCollection::Private::Private()
{
    ref = 1;
    id = 1;
    modified = false;
}

//! \internal
MvdMovieCollection::Private::Private(const MvdMovieCollection::Private &m)
{
    ref = 1;

    id = 1;
    modified = false;

    name = m.name;
    owner = m.owner;
    email = m.email;
    website = m.website;
    notes = m.notes;
    dataPath = m.dataPath;
    tempPath = m.tempPath;

    quickLookupTable = m.quickLookupTable;
    movies = m.movies;

    fileName = m.fileName;
    path = m.path;

    smd = m.smd;
}


//////////////////////////////////////////////////////////////////////////


/*!
    Creates a new movie collection.
*/
MvdMovieCollection::MvdMovieCollection() :
    QObject(),
    d(new Private)
{ }

/*!
    Builds a new collection as a copy of an existing one.
*/
MvdMovieCollection::MvdMovieCollection(const MvdMovieCollection &m) :
    QObject(),
    d(m.d)
{
    d->ref.ref();
}

/*!
    Deletes this movie collection.
*/
MvdMovieCollection::~MvdMovieCollection()
{
    if (!d->ref.deref()) {
        emit destroyed();
        if (!d->tempPath.isEmpty())
            Movida::paths().removeDirectoryTree(d->tempPath);
        delete d;
    }
}

/*!
    Assignment operator.
*/
MvdMovieCollection &MvdMovieCollection::operator=(const MvdMovieCollection &other)
{
    qAtomicAssign(d, other.d);
    return *this;
}

//! \internal Forces a detach.
void MvdMovieCollection::detach()
{
    qAtomicDetach(d);
}

//! \internal
bool MvdMovieCollection::isDetached() const
{
    return d->ref == 1;
}

/*!
    Returns a reference to this collection's SD.
*/
MvdSharedData &MvdMovieCollection::sharedData() const
{
    return d->smd;
}

/*!
    Sets some metadata for this collection.
    Setting the data path will result in the previously set data path (if any)
    to be erased.
*/
void MvdMovieCollection::setMetaData(MetaDataType ci, const QString &val)
{
    detach();

    switch (ci) {
        case NameInfo:
            d->name = val;
            break;

        case OwnerInfo:
            d->owner = val;
            break;

        case EMailInfo:
            d->email = val;
            break;

        case WebsiteInfo:
            d->website = val;
            break;

        case DataPathInfo:
            if (!d->dataPath.isEmpty())
                Movida::paths().removeDirectoryTree(d->dataPath);
            d->dataPath = MvdCore::toLocalFilePath(val, true);
            break;

        case TempPathInfo:
            if (!d->tempPath.isEmpty())
                Movida::paths().removeDirectoryTree(d->tempPath);
            d->tempPath = MvdCore::toLocalFilePath(val, true);
            break;

        case NotesInfo:
            d->notes = val;
            break;

        default:
            ;
    }

    __COLLECTION_CHANGED
    emit metaDataChanged((int)ci, val);
}

/*!
    Returns information about this collection.
    Generates a new data path if the data path is requested but none has been
    set yet.
*/
QString MvdMovieCollection::metaData(MetaDataType ci, bool dontCreateDirs) const
{
    switch (ci) {
        case NameInfo:
            return d->name;

        case OwnerInfo:
            return d->owner;

        case EMailInfo:
            return d->email;

        case WebsiteInfo:
            return d->website;

        case DataPathInfo:
            if (d->dataPath.isEmpty() && !dontCreateDirs) {
                MvdMovieCollection *that = const_cast<MvdMovieCollection *>(this);
                that->detach();
                d->dataPath = Movida::paths().generateTempDir().append("persistent") + QDir::separator();
                QDir dir;
                const QString imgPath = d->dataPath + "images" + QDir::separator();
                dir.mkpath(imgPath);
            }
            return d->dataPath;

        case TempPathInfo:
            if (d->tempPath.isEmpty() && !dontCreateDirs) {
                MvdMovieCollection *that = const_cast<MvdMovieCollection *>(this);
                that->detach();
                d->tempPath = Movida::paths().generateTempDir();
            }
            return d->tempPath;

        case NotesInfo:
            return d->notes;

        default:
            ;
    }

    return QString();
}

/*!
    Returns the movie with given ID or an invalid MvdMovie object if the collection
    contains no movie with this ID.
*/
MvdMovie MvdMovieCollection::movie(mvdid id) const
{
    if (id == MvdNull || d->movies.isEmpty())
        return MvdMovie();

    MovieList::ConstIterator itr = d->movies.find(id);
    return itr == d->movies.constEnd() ? MvdMovie() : itr.value();
}

/*!
    Adds a new movie to the database and returns its ID.
    MvdNull is returned if the movie could not be added.
    A movieAdded() signal is emitted if the movie has been added.

    The collection will automatically update references to shared items.
*/
mvdid MvdMovieCollection::addMovie(MvdMovie &movie)
{
    if (!movie.isValid())
        return MvdNull;

    detach();

    QString title = movie.title();
    if (title.isEmpty())
        title = movie.originalTitle();

    title = title.toLower();

    QString released = movie.year();

    bool ok;
    int releasedInt = released.toInt(&ok);
    if (!ok)
        releasedInt = -1;

    if (d->quickLookupTable.isEmpty()) {
        // Add a new quick lookup entry
        MvdMovieCollection::Private::QuickLookupList ql;
        ql.append(releasedInt);
        d->quickLookupTable.insert(title, ql);
    } else {
        MvdMovieCollection::Private::QuickLookupTable::Iterator qltIterator = d->quickLookupTable.find(title);
        if (qltIterator != d->quickLookupTable.end()) {   // need to add a new year
            MvdMovieCollection::Private::QuickLookupList qll = qltIterator.value();

            bool found = false;

            // Look if year already exists to avoid duplicate entries!
            MvdMovieCollection::Private::QuickLookupList::Iterator qll_begin = qll.begin();
            MvdMovieCollection::Private::QuickLookupList::Iterator qll_end = qll.end();
            while (qll_begin != qll_end && !found) {
                if (*qll_begin == releasedInt) {
                    (*qll_begin).ref++;
                    found = true;
                }
                ++qll_begin;
            }

            if (!found)
                qll.append(releasedInt);
        } else { // need to add a new entry!
            MvdMovieCollection::Private::QuickLookupList ql;
            ql.append(releasedInt);
            d->quickLookupTable.insert(title, ql);
        }
    }

    // Set import date-time if missing
    const QString _importDate = Movida::core().parameter("mvdcore/extra-attributes/import-date").toString();
    if (!movie.hasExtendedAttribute(_importDate))
        movie.setExtendedAttribute(_importDate, QDateTime::currentDateTime());

    mvdid movie_id = d->id++;
    d->movies.insert(movie_id, movie);

    MvdSharedData &sd = sharedData();
    QList<mvdid> sharedItems = movie.sharedItemIds();
    foreach(mvdid sd_id, sharedItems)
    {
        sd.addMovieLink(sd_id, movie_id);
    }

    __COLLECTION_CHANGED
    emit movieAdded(movie_id);

    // Need a better way to report the loading status to the GUI. This is terribly slow!
    // QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    return movie_id;
}

/*!
    Creates a new MvdMovie object from a movie data object, registering
    shared data with the SD and then adds the movie to the collection.
    MvdNull is returned if the movie could not be added.
    A movieAdded() signal is emitted if the movie has been added.
*/
mvdid MvdMovieCollection::addMovie(const MvdMovieData &movie,
    const QHash<QString, QVariant> &extra)
{
    if (!movie.isValid())
        return MvdNull;

    MvdMovie m;
    m.setTitle(movie.title);
    m.setOriginalTitle(movie.originalTitle);
    m.setYear(movie.year);
    m.setImdbId(movie.imdbId);
    m.setPlot(movie.plot);
    m.setNotes(movie.notes);
    m.setStorageId(movie.storageId);
    m.setRunningTime(movie.runningTime);
    m.setRating(movie.rating);
    m.setColorMode(movie.colorMode);
    m.setExtendedAttributes(movie.extendedAttributes);
    m.addExtendedAttributes(extra);

    for (int i = 0; i < movie.languages.size(); ++i) {
        const QString &s = movie.languages.at(i);
        mvdid id = sharedData().addItem(MvdSdItem(Movida::LanguageRole, s));
        m.addLanguage(id);
    }

    for (int i = 0; i < movie.countries.size(); ++i) {
        const QString &s = movie.countries.at(i);
        mvdid id = sharedData().addItem(MvdSdItem(Movida::CountryRole, s));
        m.addCountry(id);
    }

    for (int i = 0; i < movie.tags.size(); ++i) {
        const QString &s = movie.tags.at(i);
        mvdid id = sharedData().addItem(MvdSdItem(Movida::TagRole, s));
        m.addTag(id);
    }

    for (int i = 0; i < movie.genres.size(); ++i) {
        const QString &s = movie.genres.at(i);
        mvdid id = sharedData().addItem(MvdSdItem(Movida::GenreRole, s));
        m.addGenre(id);
    }

    for (int i = 0; i < movie.directors.size(); ++i) {
        const MvdMovieData::PersonData &p = movie.directors.at(i);
        MvdSdItem sdi(Movida::PersonRole, p.name);
        sdi.id = p.imdbId;
        for (int j = 0; j < p.urls.size(); ++j) {
            const MvdMovieData::UrlData &ud = p.urls.at(j);
            sdi.urls.append(MvdSdItem::Url(ud.url, ud.description, ud.isDefault));
        }
        mvdid id = sharedData().addItem(sdi);
        m.addDirector(id);
    }

    for (int i = 0; i < movie.producers.size(); ++i) {
        const MvdMovieData::PersonData &p = movie.producers.at(i);
        MvdSdItem sdi(Movida::PersonRole, p.name);
        sdi.id = p.imdbId;
        for (int j = 0; j < p.urls.size(); ++j) {
            const MvdMovieData::UrlData &ud = p.urls.at(j);
            sdi.urls.append(MvdSdItem::Url(ud.url, ud.description, ud.isDefault));
        }
        mvdid id = sharedData().addItem(sdi);
        m.addProducer(id);
    }

    for (int i = 0; i < movie.crewMembers.size(); ++i) {
        const MvdMovieData::PersonData &p = movie.crewMembers.at(i);
        MvdSdItem sdi(Movida::PersonRole, p.name);
        sdi.id = p.imdbId;
        for (int j = 0; j < p.urls.size(); ++j) {
            const MvdMovieData::UrlData &ud = p.urls.at(j);
            sdi.urls.append(MvdSdItem::Url(ud.url, ud.description, ud.isDefault));
        }
        mvdid id = sharedData().addItem(sdi);
        m.addCrewMember(id, p.roles);
    }

    for (int i = 0; i < movie.actors.size(); ++i) {
        const MvdMovieData::PersonData &p = movie.actors.at(i);
        MvdSdItem sdi(Movida::PersonRole, p.name);
        sdi.id = p.imdbId;
        for (int j = 0; j < p.urls.size(); ++j) {
            const MvdMovieData::UrlData &ud = p.urls.at(j);
            sdi.urls.append(MvdSdItem::Url(ud.url, ud.description, ud.isDefault));
        }
        mvdid id = sharedData().addItem(sdi);
        m.addActor(id, p.roles);
    }

    for (int i = 0; i < movie.urls.size(); ++i) {
        const MvdMovieData::UrlData &u = movie.urls.at(i);
        m.addUrl(MvdUrl(u.url, u.description, u.isDefault));
    }

    m.setSpecialContents(movie.specialContents);

    if (!movie.posterPath.isEmpty())
        m.setPoster(addImage(movie.posterPath, MoviePosterImage));

    return addMovie(m);
}

/*!
    Changes the movie with specified ID.
    \p movie must be a valid movie (see MvdMovie::isValid())
    A movieChanged() signal is emitted if the movie has been changed.

    The collection will automatically update references to shared items.
*/
void MvdMovieCollection::updateMovie(mvdid id, const MvdMovie &movie)
{
    if (d->movies.isEmpty())
        return;

    if (id == MvdNull || !movie.isValid())
        return;

    MovieList::Iterator oldMovieItr = d->movies.find(id);
    if (oldMovieItr == d->movies.end())
        return;

    detach();

    // get title & year of new movie
    QString title = movie.title();
    if (title.isEmpty())
        title = movie.originalTitle();

    title = title.toLower();

    QString released = movie.year();

    bool ok;
    int releasedInt = released.toInt(&ok);
    if (!ok)
        releasedInt = -1;

    const MvdMovie &oldMovie = oldMovieItr.value();

    MvdSharedData &sd = sharedData();
    QList<mvdid> sharedItems = oldMovie.sharedItemIds();
    foreach(mvdid sd_id, sharedItems)
    {
        sd.removeMovieLink(sd_id, id);
    }

    // remove title & year of old movie from the lookup table
    if (!d->quickLookupTable.isEmpty()) {
        QString oldTitle = oldMovie.title();
        if (oldTitle.isEmpty())
            oldTitle = oldMovie.originalTitle();

        oldTitle = oldTitle.toLower();

        QString oldReleased = oldMovie.year();
        int oldReleasedInt = oldReleased.toInt(&ok);
        if (!ok)
            oldReleasedInt = -1;

        if ((oldTitle != title) || (oldReleasedInt != releasedInt)) {
            for (MvdMovieCollection::Private::QuickLookupTable::Iterator qltIterator = d->quickLookupTable.begin();
                 qltIterator != d->quickLookupTable.end(); ++qltIterator) {
                if (qltIterator.key() == oldTitle) {
                    MvdMovieCollection::Private::QuickLookupList &qll = qltIterator.value();

                    // found similar movie title, now search for the correct entry
                    for (MvdMovieCollection::Private::QuickLookupList::Iterator qllIterator = qll.begin();
                         qllIterator != qll.end(); ++qllIterator) {
                        if (oldReleasedInt == *qllIterator) {
                            if (--(*qllIterator).ref == 0)
                                qll.erase(qllIterator);

                            break;
                        }
                    }

                    if (qll.isEmpty())
                        d->quickLookupTable.erase(qltIterator);

                    break;
                }
            }
        }
    }

    // add new title and year to the lookup map
    MvdMovieCollection::Private::QuickLookupTable::Iterator qltIterator = d->quickLookupTable.find(title);
    if (qltIterator != d->quickLookupTable.end()) {
        MvdMovieCollection::Private::QuickLookupList qll = qltIterator.value();

        bool found = false;

        // look if year already exists to avoid duplicate entries!
        for (MvdMovieCollection::Private::QuickLookupList::Iterator qllIterator = qll.begin();
             qllIterator != qll.end(); ++qllIterator) {
            if (*qllIterator == releasedInt) {
                (*qllIterator).ref++;
                found = true;
                break;
            }
        }

        if (!found)
            qll.append(releasedInt);
    } else {   // need to add a new entry!
        MvdMovieCollection::Private::QuickLookupList ql;
        ql.append(releasedInt);
        d->quickLookupTable.insert(title, ql);
    }

    d->movies.erase(oldMovieItr);     //! \todo CHECK IF WE REALLY NEED TO CALL REMOVE
    d->movies.insert(id, movie);

    sharedItems = movie.sharedItemIds();
    foreach(mvdid sd_id, sharedItems)
    {
        sd.addMovieLink(sd_id, id);
    }

    __COLLECTION_CHANGED
    emit movieChanged(id);
}

/*!
    Removes the movie with given ID from the database.
    A movieRemoved() signal is emitted if the movie has been removed.

    The collection will automatically update references to shared items.
*/
void MvdMovieCollection::removeMovie(mvdid id)
{
    if (id == MvdNull || d->movies.isEmpty())
        return;

    MovieList::Iterator movieIterator = d->movies.find(id);
    if (movieIterator == d->movies.end())
        return;

    detach();

    const MvdMovie &movie = movieIterator.value();

    MvdSharedData &sd = sharedData();
    QList<mvdid> sharedItems = movie.sharedItemIds();
    foreach(mvdid sd_id, sharedItems)
    {
        sd.removeMovieLink(sd_id, id);
    }

    // Remove quick lookup entry
    if (!d->quickLookupTable.isEmpty()) {
        QString title = movie.title();
        if (title.isEmpty())
            title = movie.originalTitle();

        title = title.toLower();

        QString released = movie.year();

        bool ok;
        int releasedInt = released.toInt(&ok);
        if (!ok)
            releasedInt = -1;

        MvdMovieCollection::Private::QuickLookupTable::Iterator qltIterator = d->quickLookupTable.find(title);
        if (qltIterator != d->quickLookupTable.end()) {
            MvdMovieCollection::Private::QuickLookupList qll = qltIterator.value();

            for (MvdMovieCollection::Private::QuickLookupList::Iterator qllIterator = qll.begin();
                 qllIterator != qll.end(); ++qllIterator) {
                if (releasedInt == *qllIterator) {
                    if (--(*qllIterator).ref == 0)
                        qll.erase(qllIterator);

                    break;
                }
            }

            if (qll.isEmpty())
                d->quickLookupTable.erase(qltIterator);
        }
    }

    d->movies.erase(movieIterator);

    __COLLECTION_CHANGED
    emit movieRemoved(id);
}

/*!
    Clears the database and emits the collectionCleared() signal.
    The modified status is set to true and a collectionModified() signal
    is emitted if the collection was not modified before clearing it.
    Note: No metadata is removed!
*/
void MvdMovieCollection::clear()
{
    detach();
    d->movies.clear();
    d->quickLookupTable.clear();
    d->id = 1;

    __COLLECTION_CHANGED
    emit cleared();
}

/*!
    Returns true if the database has been modified.
*/
bool MvdMovieCollection::isModified() const
{
    return d->modified;
}

/*!
    Sets the modified status to false.
    No collectionModified() signal is emitted.
    Useful when loading a collection or after the collection has been saved.
*/
void MvdMovieCollection::setModifiedStatus(bool modified)
{
    detach();
    d->modified = modified;
}

/*!
    Returns all the movies stored in this collection.
*/
MvdMovieCollection::MovieList MvdMovieCollection::movies() const
{
    return d->movies;
}

/*!
    Convenience method, returns the ids of all movies stored in this collection.
*/
QList<mvdid> MvdMovieCollection::movieIds() const
{
    return d->movies.keys();
}

/*!
    Returns true if the collection contains a movie with given title
    (case insensitive compare) and production year.
    The title is either the localized title or the original title if
    the localized is missing.
*/
bool MvdMovieCollection::contains(const QString &title, int year) const
{
    if (d->quickLookupTable.isEmpty())
        return false;

    if (title.isEmpty() || year < 0)
        return false;

    QString lTitle = title.toLower();

    MvdMovieCollection::Private::QuickLookupTable::ConstIterator qltIterator = d->quickLookupTable.find(lTitle);
    if (qltIterator == d->quickLookupTable.constEnd())
        return false;

    MvdMovieCollection::Private::QuickLookupList qll = qltIterator.value();

    for (MvdMovieCollection::Private::QuickLookupList::ConstIterator qllIterator = qll.begin();
         qllIterator != qll.constEnd(); ++qllIterator)
        if (*qllIterator == year)
            return true;

    return false;
}

/*!
    Returns the number of movies stored in this collection.
*/
int MvdMovieCollection::count() const
{
    return d->movies.count();
}

/*!
    Convenience method, returns true if the collection is empty.
*/
bool MvdMovieCollection::isEmpty() const
{
    return d->movies.isEmpty();
}

/*!
    Sets the full path to the collection file.
*/
void MvdMovieCollection::setPath(const QString &p)
{
    detach();
    d->path = p;
}

/*!
    Returns the full path to the collection file.
*/
QString MvdMovieCollection::path() const
{
    return d->path;
}

/*!
    Sets the name of the collection file.
*/
void MvdMovieCollection::setFileName(const QString &p)
{
    detach();
    d->fileName = p;
}

/*!
    Returns the name of the collection file.
*/
QString MvdMovieCollection::fileName() const
{
    return d->fileName;
}

/*!
    Adds a new image to the persistent data directory of this collection and
    returns a collection-wide filename. Use this filename to access the
    image file in Movida (i.e. in MvdMovie::setPoster()).

    The \p category parameter is used to process files that will be used for
    specific purposes (i.e. big movie posters are scaled for higher performance).

    Returns a null string if \p path is not a valid file.
*/
QString MvdMovieCollection::addImage(const QString &path,
    MvdMovieCollection::ImageCategory category)
{
    if (!QFile::exists(path))
        return QString();

    // Init data path if necessary
    if (d->dataPath.isEmpty()) {
        metaData(DataPathInfo);
    } else {
        // Check if the file is already part of the collection's persistent data storage
        QFileInfo info(path);
        QString thisFilePath = Movida::core().toLocalFilePath(info.absolutePath(), true);
        QString storagePath = Movida::core().toLocalFilePath(d->dataPath + "/images/", true);
#ifdef Q_OS_WIN
        if (!QString::compare(thisFilePath, storagePath, Qt::CaseInsensitive))
#else
        if (!QString::compare(thisFilePath, storagePath, Qt::CaseSensitive))
#endif
            return info.fileName();
    }

    // Check if file has been already added
    QString srcHash = MvdMd5::hashFile(path);
    if (QFile::exists(d->dataPath + "/images/" + srcHash))
        return srcHash;

    // Compute internal filename
    QString internalName = MvdMd5::hashFile(path);
    // fall back to UUID
    if (internalName.isEmpty())
        internalName = QUuid::createUuid().toString();

    // Process image if necessary
    if (category == MoviePosterImage) {
        int maxKB = Movida::core().parameter("mvdcore/max-poster-kb").toInt();

        QFileInfo fi(path);
        if (fi.size() > maxKB) {
            // Scale image
            //! \todo Set a limit for the file size or this might take some time!!
            QPixmap pm(path);
            if (pm.isNull())
                return QString();

            QSize maxSize = Movida::core().parameter("mvdcore/max-poster-size").toSize();
            pm = pm.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            if (!pm.save(d->dataPath + "/images/" + internalName, "PNG")) {
                Movida::wLog() << QString("MvdCollection: Failed to save %1")
                    .arg(d->dataPath + "/images/" + internalName);
                return QString();
            }
        }
    }

    if (internalName.isEmpty()) {
        QFile srcFile(path);
        if (!srcFile.copy(d->dataPath + "/images/" + internalName)) {
            Movida::wLog() << QString("Failed to copy %1 to %2: %3")
                .arg(path).arg(d->dataPath + "/images/" + internalName)
                .arg(srcFile.errorString());
            return QString();
        }
    }

    Movida::iLog() << QString("MvdCollection: Added persistent image: %1")
        .arg(d->dataPath + "/images/" + internalName);

    return internalName;
}

/*!
    Removes any persistent data stored in the system's temporary directory.
*/
void MvdMovieCollection::clearPersistentData()
{
    if (d->dataPath.isEmpty())
        return;

    // The data path is a subdirectory of the collection's temp directory
    // but we want to remove everything so we CD up once.
    QDir dp(d->dataPath);
    dp.cdUp();

    Movida::paths().removeDirectoryTree(dp.absolutePath());
}
