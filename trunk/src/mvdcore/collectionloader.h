/**************************************************************************
** Filename: collectionloader.h
**
** Copyright (C) 2007-2008 Angius Fabrizio. All rights reserved.
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

#ifndef MVD_COLLECTIONLOADER_H
#define MVD_COLLECTIONLOADER_H

#include "global.h"
#include <QObject>
#include <QHash>

class MvdMovieCollection;

class MVD_EXPORT MvdCollectionLoader : QObject
{
	Q_OBJECT

public:
	struct Info {
		Info() : expectedMovieCount(0) {}

		int expectedMovieCount;
		QHash<QString,QString> metadata;
	};

	enum ProgressType {
		CollectionInfo,
		ProgressInfo
	};

	enum StatusCode
	{
		NoError = 0,
		InvalidCollectionError,
		FileOpenError,
		FileNotFoundError,
		ZipError,
		InvalidFileError,
		TemporaryDirectoryError,
		UnknownError
	};

	MvdCollectionLoader(QObject* parent = 0);
	~MvdCollectionLoader();

	void setProgressHandler(QObject* receiver, const char* member);
	StatusCode load(MvdMovieCollection* collection, QString file = QString());

private slots:
	void extractionProgress(int);

private:
	class Private;
	Private* d;
};

#endif // MVD_COLLECTIONLOADER_H
