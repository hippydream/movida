/**************************************************************************
** Filename: unzip.cpp
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

#include "unzip.h"
#include "zipentry_p.h"
#include "logger.h"
#include <QMetaObject>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QtGlobal>

using namespace Movida;

/*!
	\class MvdUnZip unzip.h
	\ingroup MvdCore

	\brief PKZip 2.0 compatible archive decompression.
*/

/*!
	\enum MvdUnZip::ErrorCode The result of a decompression operation.
	\value MvdUnZip::NoError No error occurred.
	\value MvdUnZip::ZlibInitError Failed to init or load the zlib library.
	\value MvdUnZip::ZlibError The zlib library returned some error.
	\value MvdUnZip::FileOpenError Unable to create or open a device.
	\value MvdUnZip::FilePartiallyCorruptedError FileCorruptedError zip archive - some files 
	could be extracted.
	\value MvdUnZip::FileCorruptedError FileCorruptedError or invalid zip archive.
	\value MvdUnZip::WrongPasswordError Unable to decrypt a password protected file.
	\value MvdUnZip::NoOpenArchiveError No archive has been opened yet.
	\value MvdUnZip::FileNotFoundError Unable to find the requested file in the archive.
	\value MvdUnZip::ReadError Reading of a file failed.
	\value MvdUnZip::WriteError Writing of a file failed.
	\value MvdUnZip::SeekError Seek failed.
	\value MvdUnZip::CreateDirError Could not create a directory.
	\value MvdUnZip::InvalidDeviceError A null device has been passed as parameter.
	\value MvdUnZip::InvalidArchiveError This is not a valid (or supported) ZIP archive.
	\value MvdUnZip::HeaderConsistencyError Local header record info does not match 
	with the central directory record info. The archive may be corrupted.

	\value MvdUnZip::EncryptedFileSkipped An encrypted file has not extracted
	due to a user request.
	\value MvdUnZip::AllEncryptedFilesSkipped Encrypted files have not extracted
	due to a user request.
*/

/*!
	\enum MvdUnZip::ExtractionOptions Some options for the file extraction 
	methods.
	\value MvdUnZip::ExtractPaths Default. Does not ignore the path of the 
	zipped files.
	\value MvdUnZip::SkipPaths Default. Ignores the path of the zipped files 
	and extracts them all to the same root directory.
*/

//! Local header size (excluding signature, excluding variable length fields)
#define MVD_UNZIP_LOCAL_HEADER_SIZE 26
//! Central Directory file entry size (excluding signature, excluding variable length fields)
#define MVD_UNZIP_CD_ENTRY_SIZE_NS 42
//! Data descriptor size (excluding signature)
#define MVD_UNZIP_DD_SIZE 12
//! End Of Central Directory size (including signature, excluding variable length fields)
#define MVD_UNZIP_EOCD_SIZE 22
//! Local header entry encryption header size
#define MVD_UNZIP_LOCAL_ENC_HEADER_SIZE 12

// Some offsets inside a CD record (excluding signature)
#define MVD_UNZIP_CD_OFF_VERSION 0
#define MVD_UNZIP_CD_OFF_GPFLAG 4
#define MVD_UNZIP_CD_OFF_CMETHOD 6
#define MVD_UNZIP_CD_OFF_MODT 8
#define MVD_UNZIP_CD_OFF_MODD 10
#define MVD_UNZIP_CD_OFF_CRC32 12
#define MVD_UNZIP_CD_OFF_CSIZE 16
#define MVD_UNZIP_CD_OFF_USIZE 20
#define MVD_UNZIP_CD_OFF_NAMELEN 24
#define MVD_UNZIP_CD_OFF_XLEN 26
#define MVD_UNZIP_CD_OFF_COMMLEN 28
#define MVD_UNZIP_CD_OFF_LHOFFSET 38

// Some offsets inside a local header record (excluding signature)
#define MVD_UNZIP_LH_OFF_VERSION 0
#define MVD_UNZIP_LH_OFF_GPFLAG 2
#define MVD_UNZIP_LH_OFF_CMETHOD 4
#define MVD_UNZIP_LH_OFF_MODT 6
#define MVD_UNZIP_LH_OFF_MODD 8
#define MVD_UNZIP_LH_OFF_CRC32 10
#define MVD_UNZIP_LH_OFF_CSIZE 14
#define MVD_UNZIP_LH_OFF_USIZE 18
#define MVD_UNZIP_LH_OFF_NAMELEN 22
#define MVD_UNZIP_LH_OFF_XLEN 24

// Some offsets inside a data descriptor record (excluding signature)
#define MVD_UNZIP_DD_OFF_CRC32 0
#define MVD_UNZIP_DD_OFF_CSIZE 4
#define MVD_UNZIP_DD_OFF_USIZE 8

// Some offsets inside a EOCD record
#define MVD_UNZIP_EOCD_OFF_ENTRIES 6
#define MVD_UNZIP_EOCD_OFF_CDOFF 12
#define MVD_UNZIP_EOCD_OFF_COMMLEN 16

/*!
	Max version handled by this API.
	0x1B = 2.7 --> full compatibility only up to version 2.0 (0x14)
	versions from 2.1 to 2.7 may use unsupported compression methods
	versions after 2.7 may have an incompatible header format
*/
#define MVD_UNZIP_VERSION 0x1B
//! Full compatibility granted until this version
#define MVD_UNZIP_VERSION_STRICT 0x14

//! MVD_CRC32 routine
#define MVD_CRC32(c, b) crcTable[((int)c^b) & 0xff] ^ (c >> 8)

//! Checks if some file has been already extracted.
#define MVD_UNZIP_CHECK_FOR_VALID_DATA \
{\
	if (headers != 0)\
{\
	if (headers->size() != 0)\
	{\
		ec = MvdUnZip::FilePartiallyCorruptedError;\
		eLog() << "MvdUnzip: Corrupted zip archive. Some files can still be extracted.";\
	}\
	else\
	{\
		ec = MvdUnZip::FileCorruptedError;\
		eLog() << "MvdUnzip: Corrupted zip archive. No file can be extracted.";\
	}\
	break;\
}\
		else\
{\
	delete device;\
	device = 0;\
	eLog() << "MvdUnzip: Corrupted or invalid zip archive";\
	ec = MvdUnZip::FileCorruptedError;\
	break;\
}\
}

// zLib authors suggest using larger buffers (128K or 256K) for (de)compression
// (especially for inflate()); we use a 256K buffer here - if you want to 
// use this code on a pre ice age mainframe please change it ;)
#define MVD_UNZIP_READ_BUFFER (256*1024)

/************************************************************************
MvdUnZip_P
*************************************************************************/

//! \internal
class MvdUnZip_P
{
public:
	MvdUnZip_P();

	// Store last 2 used passwords
	QString password1;
	QString password2;

	bool skipAllEncrypted;

	QMap<QString,MvdZipEntry_P*>* headers;

	QIODevice* device;

	char buffer1[MVD_UNZIP_READ_BUFFER];
	char buffer2[MVD_UNZIP_READ_BUFFER];

	unsigned char* uBuffer;
	const quint32* crcTable;

	QObject* callbackObject;
	QString callbackSlot;

	// Central Directory (CD) offset
	quint32 cdOffset;
	// End of Central Directory (EOCD) offset
	quint32 eocdOffset;

	// Number of entries in the Central Directory (as to the EOCD record)
	quint16 cdEntryCount;

	// The number of detected entries that have been skipped because of a non compatible format
	quint16 unsupportedEntryCount;

	QString comment;

	MvdUnZip::ErrorCode openArchive(QIODevice* device);

	MvdUnZip::ErrorCode seekToCentralDirectory();
	MvdUnZip::ErrorCode parseCentralDirectoryRecord();
	MvdUnZip::ErrorCode parseLocalHeaderRecord(const QString& path, 
		MvdZipEntry_P& entry);

	void closeArchive();

	MvdUnZip::ErrorCode extractFile(const QString& path, MvdZipEntry_P& entry, 
		const QDir& dir, MvdUnZip::ExtractionOptions options);
	MvdUnZip::ErrorCode extractFile(const QString& path, MvdZipEntry_P& entry, 
		QIODevice* device, MvdUnZip::ExtractionOptions options);

	MvdUnZip::ErrorCode testPassword(quint32* keys, const QString& file, const MvdZipEntry_P& header);
	bool testKeys(const MvdZipEntry_P& header, quint32* keys);

	bool createDirectory(const QString& path);

	inline void decryptBytes(quint32* keys, char* buffer, qint64 read);

	inline quint32 getULong(const unsigned char* data, quint32 offset) const;
	inline quint64 getULLong(const unsigned char* data, quint32 offset) const;
	inline quint16 getUShort(const unsigned char* data, quint32 offset) const;
	inline int decryptByte(quint32 key2) const;
	inline void updateKeys(quint32* keys, int c) const;
	inline void initKeys(const QString& pwd, quint32* keys) const;

	inline QDateTime convertDateTime(const unsigned char date[2], 
		const unsigned char time[2]) const;
};

//! \internal
MvdUnZip_P::MvdUnZip_P()
{
	skipAllEncrypted = false;
	headers = 0;
	device = 0;

	uBuffer = (unsigned char*) buffer1;
	crcTable = (quint32*) get_crc_table();

	cdOffset = eocdOffset = 0;
	cdEntryCount = 0;
	unsupportedEntryCount = 0;
}

//! \internal Parses a Zip archive.
MvdUnZip::ErrorCode MvdUnZip_P::openArchive(QIODevice* dev)
{
	Q_ASSERT(dev != 0);

	if (device != 0)
		closeArchive();

	device = dev;

	if (!(device->isOpen() || device->open(QIODevice::ReadOnly)))
	{
		delete device;
		device = 0;

		eLog() << "Unable to open device for reading";
		return MvdUnZip::FileOpenError;
	}

	MvdUnZip::ErrorCode ec;

	ec = seekToCentralDirectory();
	if (ec != MvdUnZip::NoError)
	{
		closeArchive();
		return ec;
	}

	//! \todo Ignore CD entry count? CD may be corrupted.
	if (cdEntryCount == 0)
	{
		return MvdUnZip::NoError;
	}

	bool continueParsing = true;

	while (continueParsing)
	{
		if (device->read(buffer1, 4) != 4)
			MVD_UNZIP_CHECK_FOR_VALID_DATA

			if (! (buffer1[0] == 'P' && buffer1[1] == 'K' && buffer1[2] == 0x01  && buffer1[3] == 0x02) )
				break;

		if ( (ec = parseCentralDirectoryRecord()) != MvdUnZip::NoError )
			break;
	}

	if (ec != MvdUnZip::NoError)
		closeArchive();

	return ec;
}

/*
	\internal Parses a local header record and makes some consistency check
	with the information stored in the Central Directory record for this entry
	that has been previously parsed.
	\todo Optional consistency check (as a ExtractionOptions flag)

	local file header signature     4 bytes  (0x04034b50)
	version needed to extract       2 bytes
	general purpose bit flag        2 bytes
	compression method              2 bytes
	last mod file time              2 bytes
	last mod file date              2 bytes
	crc-32                          4 bytes
	compressed size                 4 bytes
	uncompressed size               4 bytes
	file name length                2 bytes
	extra field length              2 bytes

	file name (variable size)
	extra field (variable size)
*/
MvdUnZip::ErrorCode MvdUnZip_P::parseLocalHeaderRecord(const QString& path, MvdZipEntry_P& entry)
{
	if (!device->seek(entry.lhOffset))
		return MvdUnZip::SeekError;

	// Test signature
	if (device->read(buffer1, 4) != 4)
		return MvdUnZip::ReadError;

	if ((buffer1[0] != 'P') || (buffer1[1] != 'K') || (buffer1[2] != 0x03) || (buffer1[3] != 0x04))
		return MvdUnZip::InvalidArchiveError;

	if (device->read(buffer1, MVD_UNZIP_LOCAL_HEADER_SIZE) != MVD_UNZIP_LOCAL_HEADER_SIZE)
		return MvdUnZip::ReadError;

	/*
		Check 3rd general purpose bit flag.

		"bit 3: If this bit is set, the fields crc-32, compressed size
		and uncompressed size are set to zero in the local
		header.  The correct values are put in the data descriptor
		immediately following the compressed data."
	*/
	bool hasDataDescriptor = entry.hasDataDescriptor();

	bool checkFailed = false;

	if (!checkFailed)
		checkFailed = entry.compMethod != getUShort(uBuffer, MVD_UNZIP_LH_OFF_CMETHOD);
	if (!checkFailed)
		checkFailed = entry.gpFlag[0] != uBuffer[MVD_UNZIP_LH_OFF_GPFLAG];
	if (!checkFailed)
		checkFailed = entry.gpFlag[1] != uBuffer[MVD_UNZIP_LH_OFF_GPFLAG + 1];
	if (!checkFailed)
		checkFailed = entry.modTime[0] != uBuffer[MVD_UNZIP_LH_OFF_MODT];
	if (!checkFailed)
		checkFailed = entry.modTime[1] != uBuffer[MVD_UNZIP_LH_OFF_MODT + 1];
	if (!checkFailed)
		checkFailed = entry.modDate[0] != uBuffer[MVD_UNZIP_LH_OFF_MODD];
	if (!checkFailed)
		checkFailed = entry.modDate[1] != uBuffer[MVD_UNZIP_LH_OFF_MODD + 1];
	if (!hasDataDescriptor)
	{
		if (!checkFailed)
			checkFailed = entry.crc != getULong(uBuffer, MVD_UNZIP_LH_OFF_CRC32);
		if (!checkFailed)
			checkFailed = entry.szComp != getULong(uBuffer, MVD_UNZIP_LH_OFF_CSIZE);
		if (!checkFailed)
			checkFailed = entry.szUncomp != getULong(uBuffer, MVD_UNZIP_LH_OFF_USIZE);
	}

	if (checkFailed)
		return MvdUnZip::HeaderConsistencyError;

	// Check filename
	quint16 szName = getUShort(uBuffer, MVD_UNZIP_LH_OFF_NAMELEN);
	if (szName == 0)
		return MvdUnZip::HeaderConsistencyError;

	if (device->read(buffer2, szName) != szName)
		return MvdUnZip::ReadError;

	QString filename = QString::fromAscii(buffer2, szName);
	if (filename != path)
	{
		eLog() << "Filename in local header mismatches.";
		return MvdUnZip::HeaderConsistencyError;
	}

	// EncryptedFileSkipped extra field
	quint16 szExtra = getUShort(uBuffer, MVD_UNZIP_LH_OFF_XLEN);
	if (szExtra != 0)
	{
		if (!device->seek(device->pos() + szExtra))
			return MvdUnZip::SeekError;
	}

	entry.dataOffset = device->pos();

	if (hasDataDescriptor)
	{
		/*
			The data descriptor has this OPTIONAL signature: PK\7\8
			We try to skip the compressed data relying on the size set in the
			Central Directory record.
		*/
		if (!device->seek(device->pos() + entry.szComp))
			return MvdUnZip::SeekError;

		// Read 4 bytes and check if there is a data descriptor signature
		if (device->read(buffer2, 4) != 4)
			return MvdUnZip::ReadError;

		bool hasSignature = buffer2[0] == 'P' && buffer2[1] == 'K' && buffer2[2] == 0x07 && buffer2[3] == 0x08;
		if (hasSignature)
		{
			if (device->read(buffer2, MVD_UNZIP_DD_SIZE) != MVD_UNZIP_DD_SIZE)
				return MvdUnZip::ReadError;
		}
		else
		{
			if (device->read(buffer2 + 4, MVD_UNZIP_DD_SIZE - 4) != MVD_UNZIP_DD_SIZE - 4)
				return MvdUnZip::ReadError;
		}

		// DD: crc, compressed size, uncompressed size
		if (
			entry.crc != getULong((unsigned char*)buffer2, MVD_UNZIP_DD_OFF_CRC32) ||
			entry.szComp != getULong((unsigned char*)buffer2, MVD_UNZIP_DD_OFF_CSIZE) ||
			entry.szUncomp != getULong((unsigned char*)buffer2, MVD_UNZIP_DD_OFF_USIZE)
			)
			return MvdUnZip::HeaderConsistencyError;
	}

	return MvdUnZip::NoError;
}

/*! \internal Attempts to find the start of the central directory record.

	We seek the file back until we reach the "End Of Central Directory"
	signature PK\5\6.

	end of central dir signature    4 bytes  (0x06054b50)
	number of this disk             2 bytes
	number of the disk with the
	start of the central directory  2 bytes
	total number of entries in the
	central directory on this disk  2 bytes
	total number of entries in
	the central directory           2 bytes
	size of the central directory   4 bytes
	offset of start of central
	directory with respect to
	the starting disk number        4 bytes
	.ZIP file comment length        2 bytes
	--- SIZE UNTIL HERE: MVD_UNZIP_EOCD_SIZE ---
	.ZIP file comment       (variable size)
*/
MvdUnZip::ErrorCode MvdUnZip_P::seekToCentralDirectory()
{
	qint64 length = device->size();
	qint64 offset = length - MVD_UNZIP_EOCD_SIZE;

	if (length < MVD_UNZIP_EOCD_SIZE)
		return MvdUnZip::InvalidArchiveError;

	if (!device->seek( offset ))
		return MvdUnZip::SeekError;

	if (device->read(buffer1, MVD_UNZIP_EOCD_SIZE) != MVD_UNZIP_EOCD_SIZE)
		return MvdUnZip::ReadError;

	bool eocdFound = (buffer1[0] == 'P' && buffer1[1] == 'K' && buffer1[2] == 0x05 && buffer1[3] == 0x06);

	if (eocdFound)
	{
		// Zip file has no comment (the only variable length field in the EOCD record)
		eocdOffset = offset;
	}
	else
	{
		qint64 read;
		char* p = 0;

		offset -= MVD_UNZIP_EOCD_SIZE;

		if (offset <= 0)
			return MvdUnZip::InvalidArchiveError;

		if (!device->seek( offset ))
			return MvdUnZip::SeekError;

		while ((read = device->read(buffer1, MVD_UNZIP_EOCD_SIZE)) >= 0)
		{
			if ( (p = strstr(buffer1, "PK\5\6")) != 0)
			{
				// Seek to the start of the EOCD record so we can read it fully
				// Yes... we could simply read the missing bytes and append them to the buffer
				// but this is far easier so heck it!
				device->seek( offset + (p - buffer1) );
				eocdFound = true;
				eocdOffset = offset + (p - buffer1);

				// Read EOCD record
				if (device->read(buffer1, MVD_UNZIP_EOCD_SIZE) != MVD_UNZIP_EOCD_SIZE)
					return MvdUnZip::ReadError;

				break;
			}

			offset -= MVD_UNZIP_EOCD_SIZE;
			if (offset <= 0)
				return MvdUnZip::InvalidArchiveError;

			if (!device->seek( offset ))
				return MvdUnZip::SeekError;
		}
	}

	if (!eocdFound)
		return MvdUnZip::InvalidArchiveError;

	// Parse EOCD to locate CD offset
	offset = getULong((const unsigned char*)buffer1, MVD_UNZIP_EOCD_OFF_CDOFF + 4);

	cdOffset = offset;

	cdEntryCount = getUShort((const unsigned char*)buffer1, MVD_UNZIP_EOCD_OFF_ENTRIES + 4);

	quint16 commentLength = getUShort((const unsigned char*)buffer1, MVD_UNZIP_EOCD_OFF_COMMLEN + 4);
	if (commentLength != 0)
	{
		QByteArray c = device->read(commentLength);
		if (c.count() != commentLength)
			return MvdUnZip::ReadError;

		comment = c;
	}

	// Seek to the start of the CD record
	if (!device->seek( cdOffset ))
		return MvdUnZip::SeekError;

	return MvdUnZip::NoError;
}

/*!
	\internal Parses a central directory record.

	Central Directory record structure:

	[file header 1]
	.
	.
	.
	[file header n]
	[digital signature] // PKZip 6.2 or later only

	File header:

	central file header signature   4 bytes  (0x02014b50)
	version made by                 2 bytes
	version needed to extract       2 bytes
	general purpose bit flag        2 bytes
	compression method              2 bytes
	last mod file time              2 bytes
	last mod file date              2 bytes
	crc-32                          4 bytes
	compressed size                 4 bytes
	uncompressed size               4 bytes
	file name length                2 bytes
	extra field length              2 bytes
	file comment length             2 bytes
	disk number start               2 bytes
	internal file attributes        2 bytes
	external file attributes        4 bytes
	relative offset of local header 4 bytes

	file name (variable size)
	extra field (variable size)
	file comment (variable size)
*/
MvdUnZip::ErrorCode MvdUnZip_P::parseCentralDirectoryRecord()
{
	// Read CD record
	if (device->read(buffer1, MVD_UNZIP_CD_ENTRY_SIZE_NS) != MVD_UNZIP_CD_ENTRY_SIZE_NS)
		return MvdUnZip::ReadError;

	bool skipEntry = false;

	// Get compression type so we can skip non compatible algorithms
	quint16 compMethod = getUShort(uBuffer, MVD_UNZIP_CD_OFF_CMETHOD);

	// Get variable size fields length so we can skip the whole record
	// if necessary
	quint16 szName = getUShort(uBuffer, MVD_UNZIP_CD_OFF_NAMELEN);
	quint16 szExtra = getUShort(uBuffer, MVD_UNZIP_CD_OFF_XLEN);
	quint16 szComment = getUShort(uBuffer, MVD_UNZIP_CD_OFF_COMMLEN);

	quint32 skipLength = szName + szExtra + szComment;

	MvdUnZip::ErrorCode ec = MvdUnZip::NoError;

	if ((compMethod != 0) && (compMethod != 8))
	{
		eLog() << "Unsupported compression method. Skipping file.";
		skipEntry = true;
	}

	// Header parsing may be a problem if version is bigger than MVD_UNZIP_VERSION
	if (!skipEntry && buffer1[MVD_UNZIP_CD_OFF_VERSION] > MVD_UNZIP_VERSION)
	{
		eLog() << "Unsupported PKZip version. Skipping file.";
		skipEntry = true;
	}

	if (!skipEntry && szName == 0)
	{
		eLog() << "Skipping file with no name.";
		skipEntry = true;
	}

	if (!skipEntry && device->read(buffer2, szName) != szName)
	{
		ec = MvdUnZip::ReadError;
		skipEntry = true;
	}

	if (skipEntry)
	{
		if (ec == MvdUnZip::NoError)
		{
			if (!device->seek( device->pos() + skipLength ))
				ec = MvdUnZip::SeekError;

			unsupportedEntryCount++;
		}

		return ec;
	}

	QString filename = QString::fromAscii(buffer2, szName);

	MvdZipEntry_P* h = new MvdZipEntry_P;
	h->compMethod = compMethod;

	h->gpFlag[0] = buffer1[MVD_UNZIP_CD_OFF_GPFLAG];
	h->gpFlag[1] = buffer1[MVD_UNZIP_CD_OFF_GPFLAG + 1];

	h->modTime[0] = buffer1[MVD_UNZIP_CD_OFF_MODT];
	h->modTime[1] = buffer1[MVD_UNZIP_CD_OFF_MODT + 1];

	h->modDate[0] = buffer1[MVD_UNZIP_CD_OFF_MODD];
	h->modDate[1] = buffer1[MVD_UNZIP_CD_OFF_MODD + 1];

	h->crc = getULong(uBuffer, MVD_UNZIP_CD_OFF_CRC32);
	h->szComp = getULong(uBuffer, MVD_UNZIP_CD_OFF_CSIZE);
	h->szUncomp = getULong(uBuffer, MVD_UNZIP_CD_OFF_USIZE);

	// EncryptedFileSkipped extra field (if any)
	if (szExtra != 0)
	{
		if (!device->seek( device->pos() + szExtra ))
		{
			delete h;
			return MvdUnZip::SeekError;
		}
	}

	// Read comment field (if any)
	if (szComment != 0)
	{
		if (device->read(buffer2, szComment) != szComment)
		{
			delete h;
			return MvdUnZip::ReadError;
		}

		h->comment = QString::fromAscii(buffer2, szComment);
	}

	h->lhOffset = getULong(uBuffer, MVD_UNZIP_CD_OFF_LHOFFSET);

	if (headers == 0)
		headers = new QMap<QString, MvdZipEntry_P*>();
	headers->insert(filename, h);

	return MvdUnZip::NoError;
}

//! \internal Closes the archive and resets the internal status.
void MvdUnZip_P::closeArchive()
{
	if (device == 0)
		return;

	skipAllEncrypted = false;

	if (headers != 0)
	{
		qDeleteAll(*headers);
		delete headers;
		headers = 0;
	}

	delete device; device = 0;

	cdOffset = eocdOffset = 0;
	cdEntryCount = 0;
	unsupportedEntryCount = 0;

	comment.clear();
}

//! \internal
MvdUnZip::ErrorCode MvdUnZip_P::extractFile(const QString& path, MvdZipEntry_P& entry, const QDir& dir, MvdUnZip::ExtractionOptions options)
{
	QString name(path);
	QString dirname;
	QString directory;

	int pos = name.lastIndexOf('/');

	// This entry is for a directory
	if (pos == name.length() - 1)
	{
		if (options.testFlag(MvdUnZip::SkipPaths))
			return MvdUnZip::NoError;

		directory = QString("%1/%2").arg(dir.absolutePath()).arg(QDir::cleanPath(name));
		if (!createDirectory(directory))
		{
			eLog() << QString("Unable to create directory: %1").arg(directory);
			return MvdUnZip::CreateDirError;
		}

		return MvdUnZip::NoError;
	}

	// Extract path from entry
	if (pos > 0)
	{
		// get directory part
		dirname = name.left(pos);
		if (options.testFlag(MvdUnZip::SkipPaths))
		{
			directory = dir.absolutePath();
		}
		else
		{
			directory = QString("%1/%2").arg(dir.absolutePath()).arg(QDir::cleanPath(dirname));
			if (!createDirectory(directory))
			{
				eLog() << QString("Unable to create directory: %1").arg(directory);
				return MvdUnZip::CreateDirError;
			}
		}
		name = name.right(name.length() - pos - 1);
	} else directory = dir.absolutePath();

	name = QString("%1/%2").arg(directory).arg(name);

	QFile outFile(name);

	if (!outFile.open(QIODevice::WriteOnly))
	{
		eLog() << QString("Unable to open %1 for writing").arg(name);
		return MvdUnZip::FileOpenError;
	}

	//! \todo Set creation/last_modified date/time

	MvdUnZip::ErrorCode ec = extractFile(path, entry, &outFile, options);

	outFile.close();

	if (ec != MvdUnZip::NoError)
	{
		if (!outFile.remove())
			eLog() << QString("Unable to remove corrupted file: %1").arg(name);
	}

	return ec;
}

//! \internal
MvdUnZip::ErrorCode MvdUnZip_P::extractFile(const QString& path, MvdZipEntry_P& entry, QIODevice* dev, MvdUnZip::ExtractionOptions options)
{
	Q_UNUSED(options);
	Q_ASSERT(dev != 0);

	if (!entry.lhEntryChecked)
	{
		MvdUnZip::ErrorCode ec = parseLocalHeaderRecord(path, entry);
		entry.lhEntryChecked = true;

		if (ec != MvdUnZip::NoError)
			return ec;
	}

	if (!device->seek(entry.dataOffset))
		return MvdUnZip::SeekError;

	// Encryption keys
	quint32 keys[3];

	if (entry.isEncrypted())
	{
		MvdUnZip::ErrorCode e = testPassword(keys, path, entry);
		if (e != MvdUnZip::NoError)
		{
			eLog() << QString("Unable to decrypt %1").arg(path);
			return e;
		}//! Encryption header size
		entry.szComp -= MVD_UNZIP_LOCAL_ENC_HEADER_SIZE; // remove encryption header size
	}

	if (entry.szComp == 0)
	{
		if (entry.crc != 0)
			return MvdUnZip::FileCorruptedError;

		return MvdUnZip::NoError;
	}

	uInt rep = entry.szComp / MVD_UNZIP_READ_BUFFER;
	uInt rem = entry.szComp % MVD_UNZIP_READ_BUFFER;
	uInt cur = 0;

	// extract data
	qint64 read;
	quint64 tot = 0;

	quint32 myCRC = crc32(0L, Z_NULL, 0);

	if (entry.compMethod == 0)
	{
		while ( (read = device->read(buffer1, cur < rep ? MVD_UNZIP_READ_BUFFER : rem)) > 0 )
		{
			if (entry.isEncrypted())
				decryptBytes(keys, buffer1, read);

			myCRC = crc32(myCRC, uBuffer, read);

			if (dev->write(buffer1, read) != read)
				return MvdUnZip::WriteError;

			cur++;
			tot += read;

			if (tot == entry.szComp)
				break;
		}

		if (read < 0)
			return MvdUnZip::ReadError;
	}
	else if (entry.compMethod == 8)
	{
		/* Allocate inflate state */
		z_stream zstr;
		zstr.zalloc = Z_NULL;
		zstr.zfree = Z_NULL;
		zstr.opaque = Z_NULL;
		zstr.next_in = Z_NULL;
		zstr.avail_in = 0;

		int zret;

		// Use inflateInit2 with negative windowBits to get raw decompression
		if ( (zret = inflateInit2_(&zstr, -MAX_WBITS, ZLIB_VERSION, sizeof(z_stream))) != Z_OK )
			return MvdUnZip::ZlibError;

		int szDecomp;

		// Decompress until deflate stream ends or end of file
		do
		{
			read = device->read(buffer1, cur < rep ? MVD_UNZIP_READ_BUFFER : rem);
			if (read == 0)
				break;
			if (read < 0)
			{
				(void)inflateEnd(&zstr);
				return MvdUnZip::ReadError;
			}

			if (entry.isEncrypted())
				decryptBytes(keys, buffer1, read);

			cur++;
			tot += read;

			zstr.avail_in = (uInt) read;
			zstr.next_in = (Bytef*) buffer1;


			// Run inflate() on input until output buffer not full
			do {
				zstr.avail_out = MVD_UNZIP_READ_BUFFER;
				zstr.next_out = (Bytef*) buffer2;;

				zret = inflate(&zstr, Z_NO_FLUSH);

				switch (zret) {
					case Z_NEED_DICT:
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
						inflateEnd(&zstr);
						return MvdUnZip::WriteError;
					default:
						;
				}

				szDecomp = MVD_UNZIP_READ_BUFFER - zstr.avail_out;
				if (dev->write(buffer2, szDecomp) != szDecomp)
				{
					inflateEnd(&zstr);
					return MvdUnZip::ZlibError;
				}

				myCRC = crc32(myCRC, (const Bytef*) buffer2, szDecomp);

			} while (zstr.avail_out == 0);

		}
		while (zret != Z_STREAM_END);

		inflateEnd(&zstr);
	}

	if (myCRC != entry.crc)
		return MvdUnZip::FileCorruptedError;

	return MvdUnZip::NoError;
}

//! \internal Creates a new directory and all the needed parent directories.
bool MvdUnZip_P::createDirectory(const QString& path)
{
	QDir d(path);
	if (!d.exists())
	{
		int sep = path.lastIndexOf("/");
		if (sep <= 0) return true;

		if (!createDirectory(path.left(sep)))
			return false;

		if (!d.mkdir(path))
		{
			eLog() << QString("Unable to create directory: %1").arg(path);
			return false;
		}
	}

	return true;
}

/*!
\internal Reads an quint32 (4 bytes) from a byte array starting at given offset.
*/
quint32 MvdUnZip_P::getULong(const unsigned char* data, quint32 offset) const
{
	quint32 res = (quint32) data[offset];
	res |= (((quint32)data[offset+1]) << 8);
	res |= (((quint32)data[offset+2]) << 16);
	res |= (((quint32)data[offset+3]) << 24);

	return res;
}

/*!
\internal Reads an quint64 (8 bytes) from a byte array starting at given offset.
*/
quint64 MvdUnZip_P::getULLong(const unsigned char* data, quint32 offset) const
{
	quint64 res = (quint64) data[offset];
	res |= (((quint64)data[offset+1]) << 8);
	res |= (((quint64)data[offset+2]) << 16);
	res |= (((quint64)data[offset+3]) << 24);
	res |= (((quint64)data[offset+1]) << 32);
	res |= (((quint64)data[offset+2]) << 40);
	res |= (((quint64)data[offset+3]) << 48);
	res |= (((quint64)data[offset+3]) << 56);

	return res;
}

/*!
	\internal Reads an quint16 (2 bytes) from a byte array starting at given offset.
*/
quint16 MvdUnZip_P::getUShort(const unsigned char* data, quint32 offset) const
{
	return (quint16) data[offset] | (((quint16)data[offset+1]) << 8);
}

/*!
	\internal Return the next byte in the pseudo-random sequence
*/
int MvdUnZip_P::decryptByte(quint32 key2) const
{
	quint16 temp = ((quint16)(key2) & 0xffff) | 2;
	return (int)(((temp * (temp ^ 1)) >> 8) & 0xff);
}

/*!
	\internal Update the encryption keys with the next byte of plain text
*/
void MvdUnZip_P::updateKeys(quint32* keys, int c) const
{
	keys[0] = MVD_CRC32(keys[0], c);
	keys[1] += keys[0] & 0xff;
	keys[1] = keys[1] * 134775813L + 1;
	keys[2] = MVD_CRC32(keys[2], ((int)keys[1]) >> 24);
}

/*!
	\internal Initialize the encryption keys and the random header according to
	the given password.
*/
void MvdUnZip_P::initKeys(const QString& pwd, quint32* keys) const
{
	keys[0] = 305419896L;
	keys[1] = 591751049L;
	keys[2] = 878082192L;

	QByteArray pwdBytes = pwd.toAscii();
	int sz = pwdBytes.size();
	const char* ascii = pwdBytes.data();

	for (int i=0; i<sz; ++i)
		updateKeys(keys, (int)ascii[i]);
}

/*!
	\internal Attempts to test a password without actually extracting a file.
	The \p file parameter can be used in the user interface or for debugging purposes
	as it is the name of the encrypted file for wich the password is being tested.
*/
MvdUnZip::ErrorCode MvdUnZip_P::testPassword(quint32* keys, const QString& file, const MvdZipEntry_P& header)
{
	Q_UNUSED(file);

	// read encryption keys
	if (device->read(buffer1, 12) != 12)
		return MvdUnZip::FileCorruptedError;

	// first test cached pwd if available
	if (!password1.isEmpty())
	{
		initKeys(password1, keys);
		if (testKeys(header, keys))
			return MvdUnZip::NoError;
	}

	if (!password2.isEmpty())
	{
		initKeys(password2, keys);
		if (testKeys(header, keys))
			return MvdUnZip::NoError;
	}

	while (true)
	{
		if (callbackObject == 0)
			return MvdUnZip::EncryptedFileSkipped;

		QString pwd;
		MvdUnZip::ErrorCode res;

		QMetaObject::invokeMethod(callbackObject, callbackSlot.toAscii().data(),
			Qt::DirectConnection,
			Q_RETURN_ARG(quint32, (quint32&)res), Q_ARG(QString, pwd));

		switch (res)
		{
		case MvdUnZip::NoError: break;
		case MvdUnZip::EncryptedFileSkipped: return MvdUnZip::EncryptedFileSkipped;
		default: return MvdUnZip::AllEncryptedFilesSkipped;
		}

		// initialize keys
		initKeys(pwd, keys);

		if (testKeys(header, keys))
		{
			if (!password1.isEmpty())
			{
				password1 = pwd;
				return MvdUnZip::NoError;
			}

			if (!password2.isEmpty())
			{
				password2 = pwd;
				return MvdUnZip::NoError;
			}

			password1 = pwd;
			return MvdUnZip::NoError;
		}
	}

	return MvdUnZip::EncryptedFileSkipped;
}

/*!
	\internal Tests a set of keys on the encryption header.
*/
bool MvdUnZip_P::testKeys(const MvdZipEntry_P& header, quint32* keys)
{
	char lastByte;

	// decrypt encryption header
	for (int i=0; i<11; ++i)
		updateKeys(keys, lastByte = buffer1[i] ^ decryptByte(keys[2]));
	updateKeys(keys, lastByte = buffer1[11] ^ decryptByte(keys[2]));

	// if there is an extended header (bit in the gp flag) buffer[11] is a byte from the file time
	// with no extended header we have to check the crc high-order byte
	char c = ((header.gpFlag[0] & 0x08) == 8) ? header.modTime[1] : header.crc >> 24;

	return (lastByte == c);
}

/*!
	\internal Decrypts an array of bytes long \p read.
*/
void MvdUnZip_P::decryptBytes(quint32* keys, char* buffer, qint64 read)
{
	for (int i=0; i<(int)read; ++i)
		updateKeys(keys, buffer[i] ^= decryptByte(keys[2]));
}

/*!
	\internal Converts date and time values from ZIP format to a QDateTime object.
*/
QDateTime MvdUnZip_P::convertDateTime(const unsigned char date[2], const unsigned char time[2]) const
{
	QDateTime dt;

	// Usual PKZip low-byte to high-byte order

	// Date: 7 bits = years from 1980, 4 bits = month, 5 bits = day
	quint16 year = (date[1] >> 1) & 127;
	quint16 month = ((date[1] << 3) & 14) | ((date[0] >> 5) & 7);
	quint16 day = date[0] & 31;

	// Time: 5 bits hour, 6 bits minutes, 5 bits seconds with a 2sec precision
	quint16 hour = (time[1] >> 3) & 31;
	quint16 minutes = ((time[1] << 3) & 56) | ((time[0] >> 5) & 7);
	quint16 seconds = (time[0] & 31) * 2;

	dt.setDate(QDate(1980 + year, month, day));
	dt.setTime(QTime(hour, minutes, seconds));
	return dt;
}


/************************************************************************
MvdUnZip
*************************************************************************/

/*!
	Creates a new Zip file decompressor.
*/
MvdUnZip::MvdUnZip()
{
	d = new MvdUnZip_P;
}

/*!
	Closes any open archive and releases used resources.
*/
MvdUnZip::~MvdUnZip()
{
	closeArchive();
	delete d;
}

/*!
	Sets a slot to be called to retrieve the password for encrypted files.
*/
void MvdUnZip::setPasswordHandler(QObject* obj, const char* member)
{
	if (!obj || !member)
		return;

	d->callbackSlot = QString::fromAscii(member);
	d->callbackObject = d->callbackSlot.isEmpty() ? 0 : obj;
}

/*!
	Returns true if there is an open archive.
*/
bool MvdUnZip::isOpen() const
{
	return d->device != 0;
}

/*!
	Opens a zip archive and reads the files list. Closes any previously opened archive.
*/
MvdUnZip::ErrorCode MvdUnZip::openArchive(const QString& filename)
{
	QFile* file = new QFile(filename);

	if (!file->exists()) {
		delete file;
		return MvdUnZip::FileNotFoundError;
	}

	if (!file->open(QIODevice::ReadOnly)) {
		delete file;
		return MvdUnZip::FileOpenError;
	}

	return openArchive(file);
}

/*!
	Opens a zip archive and reads the entries list.
	Closes any previously opened archive.
	\warning The class takes ownership of the device so don't delete it!
*/
MvdUnZip::ErrorCode MvdUnZip::openArchive(QIODevice* device)
{
	if (device == 0)
	{
		eLog() << "Invalid device.";
		return MvdUnZip::InvalidDeviceError;
	}

	return d->openArchive(device);
}

/*!
	Closes the archive and releases all the used resources (like cached passwords).
*/
void MvdUnZip::closeArchive()
{
	d->closeArchive();
}

QString MvdUnZip::archiveComment() const
{
	if (d->device == 0)
		return QString();
	return d->comment;
}

/*!
	Returns a locale translated error string for a given error code.
*/
QString MvdUnZip::formatError(MvdUnZip::ErrorCode c) const
{
	switch (c)
	{
	case NoError: return QCoreApplication::translate("MvdUnZip", "ZIP operation completed successfully."); break;
	case ZlibInitError: return QCoreApplication::translate("MvdUnZip", "Failed to initialize or load zlib library."); break;
	case ZlibError: return QCoreApplication::translate("MvdUnZip", "zlib library error."); break;
	case FileOpenError: return QCoreApplication::translate("MvdUnZip", "Unable to create or open file."); break;
	case FilePartiallyCorruptedError: return QCoreApplication::translate("MvdUnZip", "Partially corrupted archive. Some files might be extracted."); break;
	case FileCorruptedError: return QCoreApplication::translate("MvdUnZip", "FileCorruptedError archive."); break;
	case WrongPasswordError: return QCoreApplication::translate("MvdUnZip", "Wrong password."); break;
	case NoOpenArchiveError: return QCoreApplication::translate("MvdUnZip", "No archive has been created yet."); break;
	case FileNotFoundError: return QCoreApplication::translate("MvdUnZip", "File or directory does not exist."); break;
	case ReadError: return QCoreApplication::translate("MvdUnZip", "File read error."); break;
	case WriteError: return QCoreApplication::translate("MvdUnZip", "File write error."); break;
	case SeekError: return QCoreApplication::translate("MvdUnZip", "File seek error."); break;
	case CreateDirError: return QCoreApplication::translate("MvdUnZip", "Unable to create a directory."); break;
	case InvalidDeviceError: return QCoreApplication::translate("MvdUnZip", "Invalid device."); break;
	case InvalidArchiveError: return QCoreApplication::translate("MvdUnZip", "Invalid or incompatible zip archive."); break;
	case HeaderConsistencyError: return QCoreApplication::translate("MvdUnZip", "Inconsistent headers. Archive might be corrupted."); break;
	default: ;
	}

	return QCoreApplication::translate("MvdUnZip", "Unknown error.");
}

/*!
	Returns true if the archive contains a file with the given path and name.
*/
bool MvdUnZip::contains(const QString& file) const
{
	if (d->headers == 0)
		return false;

	return d->headers->contains(file);
}

/*!
	Returns complete paths of files and directories in this archive.
*/
QStringList MvdUnZip::fileList() const
{
	return d->headers == 0 ? QStringList() : d->headers->keys();
}

/*!
	Returns information for each (correctly parsed) entry of this archive.
*/
QList<MvdUnZip::ZipEntry> MvdUnZip::entryList() const
{
	QList<MvdUnZip::ZipEntry> list;

	if (d->headers != 0)
	{
		for (QMap<QString,MvdZipEntry_P*>::ConstIterator it = d->headers->constBegin(); it != d->headers->constEnd(); ++it)
		{
			const MvdZipEntry_P* entry = it.value();
			Q_ASSERT(entry != 0);

			ZipEntry z;

			z.filename = it.key();
			if (!entry->comment.isEmpty())
				z.comment = entry->comment;
			z.compressedSize = entry->szComp;
			z.uncompressedSize = entry->szUncomp;
			z.crc32 = entry->crc;
			z.lastModified = d->convertDateTime(entry->modDate, entry->modTime);

			z.compression = entry->compMethod == 0 ? NoCompression : entry->compMethod == 8 ? Deflated : UnknownCompression;
			z.type = z.filename.endsWith("/") ? Directory : File;

			z.encrypted = entry->isEncrypted();

			list.append(z);
		}
	}

	return list;
}

/*!
	Extracts the whole archive to a directory.
*/
MvdUnZip::ErrorCode MvdUnZip::extractAll(const QString& dirname, ExtractionOptions options)
{
	return extractAll(QDir(dirname), options);
}

/*!
	Extracts the whole archive to a directory.
*/
MvdUnZip::ErrorCode MvdUnZip::extractAll(const QDir& dir, ExtractionOptions options)
{
	// this should only happen if we didn't call openArchive() yet
	if (d->device == 0)
		return NoOpenArchiveError;

	if (d->headers == 0)
		return NoError;

	bool end = false;
	for (QMap<QString,MvdZipEntry_P*>::Iterator itr = d->headers->begin(); itr != d->headers->end(); ++itr)
	{
		MvdZipEntry_P* entry = itr.value();
		Q_ASSERT(entry != 0);

		if ((entry->isEncrypted()) && d->skipAllEncrypted)
			continue;

		switch (d->extractFile(itr.key(), *entry, dir, options))
		{
		case FileCorruptedError:
			eLog() << "Removing corrupted entry" << itr.key();
			d->headers->erase(itr++);
			if (itr == d->headers->end())
				end = true;
			break;
		case CreateDirError:
			break;
		case EncryptedFileSkipped:
			break;
		case AllEncryptedFilesSkipped:
			d->skipAllEncrypted = true;
			break;
		default:
			;
		}

		if (end)
			break;
	}

	return NoError;
}

/*!
	Extracts a single file to a directory.
*/
MvdUnZip::ErrorCode MvdUnZip::extractFile(const QString& filename, const QString& dirname, ExtractionOptions options)
{
	return extractFile(filename, QDir(dirname), options);
}

/*!
	Extracts a single file to a directory.
*/
MvdUnZip::ErrorCode MvdUnZip::extractFile(const QString& filename, const QDir& dir, ExtractionOptions options)
{
	QMap<QString,MvdZipEntry_P*>::Iterator itr = d->headers->find(filename);
	if (itr != d->headers->end())
	{
		MvdZipEntry_P* entry = itr.value();
		Q_ASSERT(entry != 0);
		return d->extractFile(itr.key(), *entry, dir, options);
	}

	return FileNotFoundError;
}

/*!
	Extracts a single file to a directory.
*/
MvdUnZip::ErrorCode MvdUnZip::extractFile(const QString& filename, QIODevice* dev, ExtractionOptions options)
{
	if (dev == 0)
		return InvalidDeviceError;

	QMap<QString,MvdZipEntry_P*>::Iterator itr = d->headers->find(filename);
	if (itr != d->headers->end()) {
		MvdZipEntry_P* entry = itr.value();
		Q_ASSERT(entry != 0);
		return d->extractFile(itr.key(), *entry, dev, options);
	}

	return FileNotFoundError;
}

/*!
	Extracts a list of files.
	Stops extraction at the first error (but continues if a file does not exist in the archive).
*/
MvdUnZip::ErrorCode MvdUnZip::extractFiles(const QStringList& filenames, const QString& dirname, ExtractionOptions options)
{
	QDir dir(dirname);
	ErrorCode ec;

	for (QStringList::ConstIterator itr = filenames.constBegin(); itr != filenames.constEnd(); ++itr)
	{
		ec = extractFile(*itr, dir, options);
		if (ec == FileNotFoundError)
			continue;
		if (ec != NoError)
			return ec;
	}

	return NoError;
}

/*!
	Extracts a list of files.
	Stops extraction at the first error (but continues if a file does not exist in the archive).
*/
MvdUnZip::ErrorCode MvdUnZip::extractFiles(const QStringList& filenames, const QDir& dir, ExtractionOptions options)
{
	ErrorCode ec;

	for (QStringList::ConstIterator itr = filenames.constBegin(); itr != filenames.constEnd(); ++itr)
	{
		ec = extractFile(*itr, dir, options);
		if (ec == FileNotFoundError)
			continue;
		if (ec != NoError)
			return ec;
	}

	return NoError;
}

/*!
	ZipEntry constructor - initialize data. Type is set to File.
*/
MvdUnZip::ZipEntry::ZipEntry()
{
	compressedSize = uncompressedSize = crc32 = 0;
	compression = NoCompression;
	type = File;
	encrypted = false;
}
