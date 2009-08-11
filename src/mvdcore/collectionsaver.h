/**************************************************************************
** Filename: collectionsaver.h
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

#ifndef MVD_COLLECTIONSAVER_H
#define MVD_COLLECTIONSAVER_H

#include "global.h"
#include "moviecollection.h"

#include <QtCore/QObject>

class MVD_EXPORT MvdCollectionSaver : public QObject
{
    Q_OBJECT

public:
    enum StatusCode {
        NoError = 0,
        InvalidCollectionError,
        InvalidFileError,
        FileOpenError,
        ZipError,
        TemporaryDirectoryError,
        UnknownError
    };

    MvdCollectionSaver(QObject * parent = 0);
    virtual ~MvdCollectionSaver();

    void setProgressHandler(QObject *receiver, const char *member);
    StatusCode save(MvdMovieCollection *collection, QString file = QString());

private:
    class Private;
    Private *d;
};

#endif // MVD_COLLECTIONSAVER_H
