/**************************************************************************
** Filename: unzip.h
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

#ifndef MVD_UNZIP_H
#define MVD_UNZIP_H

#include "global.h"

#include <QtGlobal>
#include <QDateTime>
#include <QMap>

#include <zlib/zlib.h>

class MvdUnZip_P;
class QIODevice;
class QFile;
class QDir;
class QStringList;
class QObject;

class MOVIDA_EXPORT MvdUnZip
{
public:
	enum ErrorCode
	{
		NoError,
		ZlibInitError,
		ZlibError,
		FileOpenError,
		FilePartiallyCorruptedError,
		FileCorruptedError,
		WrongPasswordError,
		NoOpenArchiveError,
		FileNotFoundError,
		ReadError,
		WriteError,
		SeekError,
		CreateDirError,
		InvalidDeviceError,
		InvalidArchiveError,
		HeaderConsistencyError,

		EncryptedFileSkipped,
		AllEncryptedFilesSkipped
	};

	enum ExtractionOption
	{
		//! Extracts paths (default)
		ExtractPaths = 0x0001,
		//! Ignores paths and extracts all the files to the same directory
		SkipPaths = 0x0002
	};
	Q_DECLARE_FLAGS(ExtractionOptions, ExtractionOption)

		enum CompressionMethod
	{
		NoCompression, Deflated, UnknownCompression
	};

	enum FileType
	{
		File, Directory
	};

	typedef struct ZipEntry
	{
		ZipEntry();

		QString filename;
		QString comment;

		quint32 compressedSize;
		quint32 uncompressedSize;
		quint32 crc32;

		QDateTime lastModified;

		CompressionMethod compression;
		FileType type;

		bool encrypted;
	};

	MvdUnZip();
	virtual ~MvdUnZip();

	void setPasswordHandler(QObject* obj, const char* member);

	bool isOpen() const;

	MvdUnZip::ErrorCode openArchive(const QString& filename);
	MvdUnZip::ErrorCode openArchive(QIODevice* device);
	void closeArchive();

	QString archiveComment() const;

	QString formatError(MvdUnZip::ErrorCode c) const;

	bool contains(const QString& file) const;

	QStringList fileList() const;
	QList<ZipEntry> entryList() const;

	MvdUnZip::ErrorCode extractAll(const QString& dirname, 
		ExtractionOptions options = ExtractPaths);
	MvdUnZip::ErrorCode extractAll(const QDir& dir, 
		ExtractionOptions options = ExtractPaths);

	MvdUnZip::ErrorCode extractFile(const QString& filename, const QString& dirname, 
		ExtractionOptions options = ExtractPaths);
	MvdUnZip::ErrorCode extractFile(const QString& filename, const QDir& dir, 
		ExtractionOptions options = ExtractPaths);
	MvdUnZip::ErrorCode extractFile(const QString& filename, QIODevice* device, 
		ExtractionOptions options = ExtractPaths);

	MvdUnZip::ErrorCode extractFiles(const QStringList& filenames, const QString& dirname, 
		ExtractionOptions options = ExtractPaths);
	MvdUnZip::ErrorCode extractFiles(const QStringList& filenames, const QDir& dir, 
		ExtractionOptions options = ExtractPaths);

private:
	MvdUnZip_P* d;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(MvdUnZip::ExtractionOptions)

#endif // MVD_UNZIP_H
