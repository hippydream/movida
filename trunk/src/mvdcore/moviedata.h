/**************************************************************************
** Filename: moviedata.h
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

#ifndef MVD_MOVIEDATA_H
#define MVD_MOVIEDATA_H

#include "global.h"

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

class QIODevice;

/*!
    \class MvdMovieData moviedata.h
    \ingroup MvdCore

    \brief Temporary storage class for movie data. This can be used when direct access to the data
    is required, with no SD identifiers (i.e. when importing a movie).
*/

class MVD_EXPORT MvdMovieData
{
public:
    struct UrlData {
        UrlData(const QString &u = QString()) :
            url(u),
            isDefault(false) { }

        QString url;
        QString description;
        bool isDefault;
    };

    struct PersonData {
        PersonData(const QString &n = QString()) :
            name(n) { }

        QString name;
        QString imdbId;
        QStringList roles;
        QList<UrlData> urls;

        inline bool operator==(const PersonData &pd) const;
        inline bool operator<(const PersonData &pd) const;
        inline void merge(const PersonData &pd);
        inline void mergeRoles(const QStringList &roles);
    };

    enum PropertyName {
        InvalidProperty = 0,
        Title, OriginalTitle, Year, ImdbId,
        Plot, Notes, StorageId, RunningTime, Rating, ColorMode, Languages,
        Countries, Tags, Genres, Directors, Producers, CrewMembers, Actors,
        Urls, SpecialContents, PosterPath, SeenTag, SpecialTag, LoanedTag
    };

    enum Option {
        NoOption = 0,
        StopAtFirstMovie = 0x01,
        NoXmlEncoding = 0x02,
        EmbedMoviePoster = 0x04
    };
    Q_DECLARE_FLAGS(Options, Option);

    MvdMovieData() :
        runningTime(0),
        rating(0),
        colorMode(Movida::UnknownColorMode)
    { }

    bool isValid() const;
    bool loadFromXml(const QString &path, Options options = NoOption);
    bool writeToXmlFile(const QString &path, Options options = NoOption) const;
    bool writeToXmlDevice(QIODevice *dev, Options options = NoOption) const;
    void writeToXmlString(QString *string, Options options = NoOption) const;

    QString title;
    QString originalTitle;
    QString year;
    QString imdbId;
    QString plot;
    QString notes;
    QString storageId;
    quint16 runningTime;
    quint8 rating;
    Movida::ColorMode colorMode;
    QStringList languages;
    QStringList countries;
    QStringList tags;
    QStringList genres;
    QList<PersonData> directors;
    QList<PersonData> producers;
    QList<PersonData> crewMembers;
    QList<PersonData> actors;
    QList<UrlData> urls;
    QStringList specialContents;
    QString posterPath;
    Movida::Tags specialTags;
    QHash<QString, QVariant> extendedAttributes;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(MvdMovieData::Options)

typedef QList<MvdMovieData> MvdMovieDataList;

#endif // MVD_MOVIEDATA_H
