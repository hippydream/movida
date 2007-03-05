/**************************************************************************
** Filename: collectionsaver.h
** Revision: 1
**
** Copyright (C) 2007 Angius Fabrizio. All rights reserved.
**
** This file is part of the Movida project (http://movida.sourceforge.net/).
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

#include <QObject>

class MOVIDA_EXPORT MvdCollectionSaver
{
public:
	enum ErrorCode
	{
		NoError = 0,
		FileOpenError,
		ZipError,
		TemporaryDirectoryError,
		UnknownError
	};

	static const quint8 version = 1;
	
	static ErrorCode save(
		const MvdMovieCollection& collection, const QString& file);
};

#endif // MVD_COLLECTIONSAVER_H
