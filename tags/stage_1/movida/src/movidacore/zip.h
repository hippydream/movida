/**************************************************************************
** Filename: zip.h
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

#ifndef MVD_ZIP_H
#define MVD_ZIP_H

#include "global.h"

#include <QtGlobal>
#include <QMap>

#include <zlib/zlib.h>

class MvdZip_P;
class MvdLogger;

class QIODevice;
class QFile;
class QDir;
class QStringList;
class QString;


class MVD_EXPORT MvdZip
{
public:
	enum ErrorCode
	{
		NoError = 0,
		ZlibInitError,
		ZlibError,
		FileExistsError,
		FileOpenError,
		NoOpenArchiveError,
		FileNotFoundError,
		ReadError,
		WriteError,
		SeekError
	};

	enum CompressionLevel
	{
		NoCompression,
		Deflate1Compression = 1, Deflate2Compression, Deflate3Compression, 
		Deflate4Compression, Deflate5Compression, Deflate6Compression, 
		Deflate7Compression, Deflate8Compression, Deflate9Compression,
		AutoCpuCompression, AutoMimeCompression, AutoFullCompression
	};

	enum CompressionOption
	{
		//! Does not preserve absolute paths in the zip file when adding a file/directory (default)
		RelativePathsOption = 0x0001,
		//! Preserve absolute paths
		AbsolutePathsOption = 0x0002,
		//! Do not store paths. All the files are put in the (evtl. user defined) root of the zip file
		IgnorePathsOption = 0x0004,
		//! Do not write the name of the root directory of the directory being compressed
		IgnoreRootOption = 0x0008
	};
	Q_DECLARE_FLAGS(CompressionOptions, CompressionOption)

	MvdZip();
	virtual ~MvdZip();

	bool isOpen() const;

	ErrorCode createArchive(const QString& file, bool overwrite = true);
	ErrorCode createArchive(QIODevice* device);

	QString archiveComment() const;
	void setArchiveComment(const QString& comment);

	void setPassword(const QString& pwd);
	void clearPassword();
	QString password() const;

	MvdZip::ErrorCode addDirectoryContents(const QString& path, 
		CompressionLevel level = AutoFullCompression);
	MvdZip::ErrorCode addDirectoryContents(const QString& path, const QString& root, 
		CompressionLevel level = AutoFullCompression);

	MvdZip::ErrorCode addDirectory(const QString& path, 
		CompressionOptions options = RelativePathsOption, CompressionLevel level = AutoFullCompression);
	MvdZip::ErrorCode addDirectory(const QString& path, const QString& root, 
		CompressionLevel level = AutoFullCompression);
	MvdZip::ErrorCode addDirectory(const QString& path, const QString& root, 
		CompressionOptions options = RelativePathsOption, CompressionLevel level = AutoFullCompression);

	MvdZip::ErrorCode closeArchive();

	QString formatError(ErrorCode c) const;

private:
	MvdZip_P* d;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(MvdZip::CompressionOptions)

#endif // MVD_ZIP_H
