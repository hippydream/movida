/**************************************************************************
** Filename: moviecollection.h
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

#ifndef MVD_MOVIECOLLECTION_H
#define MVD_MOVIECOLLECTION_H

#include "global.h"
#include "shareddata.h"

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

class MvdLogger;
class MvdMovie;
class MvdMovieData;

class MVD_EXPORT MvdMovieCollection : public QObject
{
    Q_OBJECT

public:
    MvdMovieCollection();
    MvdMovieCollection(const MvdMovieCollection &m);
    virtual ~MvdMovieCollection();
    MvdMovieCollection &operator=(const MvdMovieCollection &m);

    typedef QHash<mvdid, MvdMovie> MovieList;

    enum MetaDataType {
        NameInfo, OwnerInfo, EMailInfo, WebsiteInfo, NotesInfo,
        DataPathInfo, TempPathInfo,
        InvalidInfo
    };

    enum ImageCategory {
        MoviePosterImage, GenericImage
    };

    MvdSharedData &sharedData() const;

    void setMetaData(MetaDataType ci, const QString &val);
    QString metaData(MetaDataType ci, bool dontCreateDirs = false) const;

    int count() const;
    bool isEmpty() const;

    MovieList movies() const;
    QList<mvdid> movieIds() const;
    MvdMovie movie(mvdid id) const;

    mvdid addMovie(MvdMovie &movie);
    void updateMovie(mvdid id, const MvdMovie &movie);
    void removeMovie(mvdid id);

    mvdid addMovie(const MvdMovieData &movie,
    const QHash<QString, QVariant> &extra = (QHash<QString, QVariant>()));

    bool contains(const QString &title, int year) const;

    void clear();

    bool isModified() const;
    void setModifiedStatus(bool modified);

    QString fileName() const;
    void setFileName(const QString &f);

    QString path() const;
    void setPath(const QString &p);

    QString addImage(const QString &path, ImageCategory category = GenericImage);

    void clearPersistentData();

signals:
    void movieAdded(mvdid id);
    void movieChanged(mvdid id);
    void movieRemoved(mvdid id);
    void metaDataChanged(int t, const QString &v);
    void changed();
    void cleared();
    void modified();
    void saved();
    void destroyed();

private:
    class Private;
    Private *d;

public:
    typedef Private *DataPtr;
    inline DataPtr &data_ptr() { return d; }

    void detach();
    bool isDetached() const;
};
Q_DECLARE_SHARED(MvdMovieCollection)

#endif // MVD_MOVIECOLLECTION_H
