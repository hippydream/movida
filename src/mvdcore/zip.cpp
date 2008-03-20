/**************************************************************************
** Filename: zip.cpp
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

#include "zip.h"
#include "zipentry_p.h"
#include "logger.h"
#include <QMap>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QtGlobal>
#include <QFileInfo>
#include <QCoreApplication>
#include <time.h>

using namespace Movida;

//! Local header size (including signature, excluding variable length fields)
#define MVD_ZIP_LOCAL_HEADER_SIZE 30
//! Encryption header size
#define MVD_ZIP_LOCAL_ENC_HEADER_SIZE 12
//! Data descriptor size (signature included)
#define MVD_ZIP_DD_SIZE_WS 16
//! Central Directory record size (signature included)
#define MVD_ZIP_CD_SIZE 46
//! End of Central Directory record size (signature included)
#define MVD_ZIP_EOCD_SIZE 22

// Some offsets inside a local header record (signature included)
#define MVD_ZIP_LH_OFF_VERS 4
#define MVD_ZIP_LH_OFF_GPFLAG 6
#define MVD_ZIP_LH_OFF_CMET 8
#define MVD_ZIP_LH_OFF_MODT 10
#define MVD_ZIP_LH_OFF_MODD 12
#define MVD_ZIP_LH_OFF_CRC 14
#define MVD_ZIP_LH_OFF_CSIZE 18
#define MVD_ZIP_LH_OFF_USIZE 22
#define MVD_ZIP_LH_OFF_NAMELEN 26
#define MVD_ZIP_LH_OFF_XLEN 28

// Some offsets inside a data descriptor record (including signature)
#define MVD_ZIP_DD_OFF_MVD_CRC32 4
#define MVD_ZIP_DD_OFF_CSIZE 8
#define MVD_ZIP_DD_OFF_USIZE 12

// Some offsets inside a Central Directory record (including signature)
#define MVD_ZIP_CD_OFF_MADEBY 4
#define MVD_ZIP_CD_OFF_VERSION 6
#define MVD_ZIP_CD_OFF_GPFLAG 8
#define MVD_ZIP_CD_OFF_CMET 10
#define MVD_ZIP_CD_OFF_MODT 12
#define MVD_ZIP_CD_OFF_MODD 14
#define MVD_ZIP_CD_OFF_CRC 16
#define MVD_ZIP_CD_OFF_CSIZE 20
#define MVD_ZIP_CD_OFF_USIZE 24
#define MVD_ZIP_CD_OFF_NAMELEN 28
#define MVD_ZIP_CD_OFF_XLEN 30
#define MVD_ZIP_CD_OFF_COMMLEN 32
#define MVD_ZIP_CD_OFF_DISKSTART 34
#define MVD_ZIP_CD_OFF_IATTR 36
#define MVD_ZIP_CD_OFF_EATTR 38
#define MVD_ZIP_CD_OFF_LHOFF 42

// Some offsets inside a EOCD record (including signature)
#define MVD_ZIP_EOCD_OFF_DISKNUM 4
#define MVD_ZIP_EOCD_OFF_CDDISKNUM 6
#define MVD_ZIP_EOCD_OFF_ENTRIES 8
#define MVD_ZIP_EOCD_OFF_CDENTRIES 10
#define MVD_ZIP_EOCD_OFF_CDSIZE 12
#define MVD_ZIP_EOCD_OFF_CDOFF 16
#define MVD_ZIP_EOCD_OFF_COMMLEN 20

//! PKZip version for archives created by this API
#define MVD_ZIP_VERSION 0x14

//! Do not store very small files as the compression headers overhead would be to big
#define MVD_ZIP_COMPRESSION_THRESHOLD 60

//! This macro updates a one-char-only CRC; it's the Info-Zip macro re-adapted
#define MVD_CRC32(c, b) crcTable[((int)c^b) & 0xff] ^ (c >> 8)

// zLib authors suggest using larger buffers (128K or 256K) for (de)compression
// (especially for inflate()); we use a 256K buffer here - if you want to 
// use this code on a pre ice age mainframe please change it ;)
#define MVD_ZIP_READ_BUFFER (256*1024)

/*!
	\class MvdZip zip.h

	\brief MvdZip file compression.

	Some quick usage examples.

	\verbatim
	Suppose you have this directory structure:

	/root/dir1/
	/root/dir1/file1.1
	/root/dir1/file1.2
	/root/dir1/dir1.1/
	/root/dir1/dir1.2/file1.2.1

	EXAMPLE 1:
	myZipInstance.addDirectory("/root/dir1");

	RESULT:
	Beheaves like any common zip software and creates a zip file with this structure:

	dir1/
	dir1/file1.1
	dir1/file1.2
	dir1/dir1.1/
	dir1/dir1.2/file1.2.1

	EXAMPLE 2:
	myZipInstance.addDirectory("/root/dir1", "myRoot/myFolder");

	RESULT:
	Adds a custom root to the paths and creates a zip file with this structure:

	myRoot/myFolder/dir1/
	myRoot/myFolder/dir1/file1.1
	myRoot/myFolder/dir1/file1.2
	myRoot/myFolder/dir1/dir1.1/
	myRoot/myFolder/dir1/dir1.2/file1.2.1

	EXAMPLE 3:
	myZipInstance.addDirectory("/root/dir1", MvdZip::AbsolutePathsOption);

	NOTE:
	Same as calling addDirectory(SOME_PATH, PARENT_PATH_of_SOME_PATH).

	RESULT:
	Preserves absolute paths and creates a zip file with this structure:

	/root/dir1/
	/root/dir1/file1.1
	/root/dir1/file1.2
	/root/dir1/dir1.1/
	/root/dir1/dir1.2/file1.2.1

	EXAMPLE 4:
	myZipInstance.setPassword("hellopass");
	myZipInstance.addDirectory("/root/dir1", "/");

	RESULT:
	Adds and encrypts the files in /root/dir1, creating the following zip structure:

	/dir1/
	/dir1/file1.1
	/dir1/file1.2
	/dir1/dir1.1/
	/dir1/dir1.2/file1.2.1

	EXAMPLE 5:
	myZipInstance.addDirectory("/root/dir1", "myRoot", MvdZip::IgnoreRootOption);

	RESULT:
	The IgnoreRootOption option creates a zip file with the contents of the
	specified directory but without the root of that directory itself:

	myRoot/
	myRoot/file1.1
	myRoot/file1.2
	myRoot/dir1/dir1.1/
	myRoot/dir1/dir1.2/file1.2.1

	\endverbatim
*/

/*!
	\enum MvdZip::ErrorCode The result of a compression operation.
	\value MvdZip::NoError No error occurred.
	\value MvdZip::ZlibInitError Failed to init or load the zlib library.
	\value MvdZip::ZlibError The zlib library returned some error.
	\value MvdZip::FileExistsError The file already exists and will not be overwritten.
	\value MvdZip::FileOpenError Unable to create or open a device.
	\value MvdZip::NoOpenArchiveError CreateArchive() has not been called yet.
	\value MvdZip::FileNotFoundError File or directory does not exist.
	\value MvdZip::ReadError Reading of a file failed.
	\value MvdZip::WriteError Writing of a file failed.
	\value MvdZip::SeekError Seek failed.
*/

/*!
	\enum MvdZip::CompressionLevel Returns the result of a decompression operation.
	\value MvdZip::NoCompression No compression.
	\value MvdZip::Deflate1Compression Deflate compression level 1(lowest compression).
	\value MvdZip::Deflate1Compression Deflate compression level 2.
	\value MvdZip::Deflate1Compression Deflate compression level 3.
	\value MvdZip::Deflate1Compression Deflate compression level 4.
	\value MvdZip::Deflate1Compression Deflate compression level 5.
	\value MvdZip::Deflate1Compression Deflate compression level 6.
	\value MvdZip::Deflate1Compression Deflate compression level 7.
	\value MvdZip::Deflate1Compression Deflate compression level 8.
	\value MvdZip::Deflate1Compression Deflate compression level 9 (maximum compression).
	\value MvdZip::AutoCpuCompression Adapt compression level to CPU speed (faster CPU => better compression).
	\value MvdZip::AutoMimeCompression Adapt compression level to MIME type of the file being compressed.
	\value MvdZip::AutoFullCompression Use both CPU and MIME type detection.
*/


/************************************************************************
MvdZip_P
*************************************************************************/

//! \internal
class MvdZip_P
{
public:
	MvdZip_P();
	~MvdZip_P();

	QMap<QString,MvdZipEntry_P*>* headers;

	QIODevice* device;

	char buffer1[MVD_ZIP_READ_BUFFER];
	char buffer2[MVD_ZIP_READ_BUFFER];

	unsigned char* uBuffer;

	const quint32* crcTable;

	QString comment;
	QString password;

	MvdZip::ErrorCode createArchive(QIODevice* device);
	MvdZip::ErrorCode closeArchive();
	void reset();

	bool zLibInit();

	MvdZip::ErrorCode createEntry(const QFileInfo& file, const QString& root, 
		MvdZip::CompressionLevel level);
	MvdZip::CompressionLevel detectCompressionByMime(const QString& ext);

	inline void encryptBytes(quint32* keys, char* buffer, qint64 read);

	inline void setULong(quint32 v, char* buffer, unsigned int offset);
	inline void updateKeys(quint32* keys, int c) const;
	inline void initKeys(quint32* keys) const;
	inline int decryptByte(quint32 key2) const;

	inline QString extractRoot(const QString& p, MvdZip::CompressionOptions o);
};

//! \internal
MvdZip_P::MvdZip_P()
{
	headers = 0;
	device = 0;

	// keep an unsigned pointer so we avoid to over bloat the code with casts
	uBuffer = (unsigned char*) buffer1;
	crcTable = (quint32*) get_crc_table();
}

//! \internal
MvdZip_P::~MvdZip_P()
{
	closeArchive();
}

//! \internal
MvdZip::ErrorCode MvdZip_P::createArchive(QIODevice* dev)
{
	Q_ASSERT(dev != 0);

	if (device != 0)
		closeArchive();

	device = dev;

	if (!device->isOpen())
	{
		if (!device->open(QIODevice::ReadOnly)) {
			delete device;
			device = 0;
			eLog() << "MvdZip: Unable to open device for writing.";
			return MvdZip::FileOpenError;
		}
	}

	headers = new QMap<QString,MvdZipEntry_P*>;
	return MvdZip::NoError;
}

//! \internal Writes a new entry in the zip file.
MvdZip::ErrorCode MvdZip_P::createEntry(const QFileInfo& file, const QString& root, 
	MvdZip::CompressionLevel level)
{
	//! \todo Automatic level detection (cpu, extension & file size)

	// Directories and very small files are always stored
	// (small files would get bigger due to the compression headers overhead)

	// Need this for zlib
	bool isPNGFile = false;
	bool dirOnly = file.isDir();

	QString entryName = root;

	// Directory entry
	if (dirOnly)
		level = MvdZip::NoCompression;
	else
	{
		entryName.append(file.fileName());

		QString ext = file.completeSuffix().toLower();
		isPNGFile = ext == "png";

		if (file.size() < MVD_ZIP_COMPRESSION_THRESHOLD)
			level = MvdZip::NoCompression;
		else
			switch (level)
		{
			case MvdZip::AutoCpuCompression:
				level = MvdZip::Deflate5Compression;
				break;
			case MvdZip::AutoMimeCompression:
				level = detectCompressionByMime(ext);
				break;
			case MvdZip::AutoFullCompression:
				level = detectCompressionByMime(ext);
				break;
			default:
				;
		}
	}

	// entryName contains the path as it should be written
	// in the zip file records

	// create header and store it to write a central directory later
	MvdZipEntry_P* h = new MvdZipEntry_P;

	h->compMethod = (level == MvdZip::NoCompression) ? 0 : 0x0008;

	// Set encryption bit and set the data descriptor bit
	// so we can use mod time instead of crc for password check
	bool encrypt = !dirOnly && !password.isEmpty();
	if (encrypt)
		h->gpFlag[0] |= 9;

	QDateTime dt = file.lastModified();
	QDate d = dt.date();
	h->modDate[1] = ((d.year() - 1980) << 1) & 254;
	h->modDate[1] |= ((d.month() >> 3) & 1);
	h->modDate[0] = ((d.month() & 7) << 5) & 224;
	h->modDate[0] |= d.day();

	QTime t = dt.time();
	h->modTime[1] = (t.hour() << 3) & 248;
	h->modTime[1] |= ((t.minute() >> 3) & 7);
	h->modTime[0] = ((t.minute() & 7) << 5) & 224;
	h->modTime[0] |= t.second() / 2;

	h->szUncomp = dirOnly ? 0 : file.size();

	// **** Write local file header ****

	// signature
	buffer1[0] = 'P'; buffer1[1] = 'K';
	buffer1[2] = 0x3; buffer1[3] = 0x4;

	// version needed to extract
	buffer1[MVD_ZIP_LH_OFF_VERS] = MVD_ZIP_VERSION;
	buffer1[MVD_ZIP_LH_OFF_VERS + 1] = 0;

	// general purpose flag
	buffer1[MVD_ZIP_LH_OFF_GPFLAG] = h->gpFlag[0];
	buffer1[MVD_ZIP_LH_OFF_GPFLAG + 1] = h->gpFlag[1];

	// compression method
	buffer1[MVD_ZIP_LH_OFF_CMET] = h->compMethod & 0xFF;
	buffer1[MVD_ZIP_LH_OFF_CMET + 1] = (h->compMethod>>8) & 0xFF;

	// last mod file time
	buffer1[MVD_ZIP_LH_OFF_MODT] = h->modTime[0];
	buffer1[MVD_ZIP_LH_OFF_MODT + 1] = h->modTime[1];

	// last mod file date
	buffer1[MVD_ZIP_LH_OFF_MODD] = h->modDate[0];
	buffer1[MVD_ZIP_LH_OFF_MODD + 1] = h->modDate[1];

	// skip crc (4bytes) [14,15,16,17]

	// skip compressed size but include evtl. encryption header (4bytes: [18,19,20,21])
	buffer1[MVD_ZIP_LH_OFF_CSIZE] =
		buffer1[MVD_ZIP_LH_OFF_CSIZE + 1] =
		buffer1[MVD_ZIP_LH_OFF_CSIZE + 2] =
		buffer1[MVD_ZIP_LH_OFF_CSIZE + 3] = 0;

	h->szComp = encrypt ? MVD_ZIP_LOCAL_ENC_HEADER_SIZE : 0;

	// uncompressed size [22,23,24,25]
	setULong(h->szUncomp, buffer1, MVD_ZIP_LH_OFF_USIZE);

	// filename length
	QByteArray entryNameBytes = entryName.toAscii();
	int sz = entryNameBytes.size();

	buffer1[MVD_ZIP_LH_OFF_NAMELEN] = sz & 0xFF;
	buffer1[MVD_ZIP_LH_OFF_NAMELEN + 1] = (sz >> 8) & 0xFF;

	// extra field length
	buffer1[MVD_ZIP_LH_OFF_XLEN] = buffer1[MVD_ZIP_LH_OFF_XLEN + 1] = 0;

	// NoCompression offset to write crc and compressed size
	h->lhOffset = device->pos();
	quint32 crcOffset = h->lhOffset + MVD_ZIP_LH_OFF_CRC;

	if (device->write(buffer1, MVD_ZIP_LOCAL_HEADER_SIZE) != MVD_ZIP_LOCAL_HEADER_SIZE)
	{
		delete h;
		return MvdZip::WriteError;
	}

	// Write out filename
	if (device->write(entryNameBytes) != sz)
	{
		delete h;
		return MvdZip::WriteError;
	}

	// Encryption keys
	quint32 keys[3] = { 0, 0, 0 };

	if (encrypt)
	{
		// **** encryption header ****

		// XOR with PI to ensure better random numbers
		// with poorly implemented rand() as suggested by Info-Zip
		srand(time(NULL) ^ 3141592654UL);
		int randByte;

		initKeys(keys);
		for (int i=0; i<10; ++i)
		{
			randByte = (rand() >> 7) & 0xff;
			buffer1[i] = decryptByte(keys[2]) ^ randByte;
			updateKeys(keys, randByte);
		}

		// Encrypt encryption header
		initKeys(keys);
		for (int i=0; i<10; ++i)
		{
			randByte = decryptByte(keys[2]);
			updateKeys(keys, buffer1[i]);
			buffer1[i] ^= randByte;
		}

		// We don't know the CRC at this time, so we use the modification time
		// as the last two bytes
		randByte = decryptByte(keys[2]);
		updateKeys(keys, h->modTime[0]);
		buffer1[10] ^= randByte;

		randByte = decryptByte(keys[2]);
		updateKeys(keys, h->modTime[1]);
		buffer1[11] ^= randByte;

		// Write out encryption header
		if (device->write(buffer1, MVD_ZIP_LOCAL_ENC_HEADER_SIZE) 
			!= MVD_ZIP_LOCAL_ENC_HEADER_SIZE)
		{
			delete h;
			return MvdZip::WriteError;
		}
	}

	qint64 written = 0;
	quint32 crc = crc32(0L, Z_NULL, 0);

	if (!dirOnly)
	{
		QFile actualFile(file.absoluteFilePath());
		if (!actualFile.open(QIODevice::ReadOnly))
		{
			eLog() << QString("MvdZip: An error occurred while opening %1")
				.arg(file.absoluteFilePath());
			return MvdZip::FileOpenError;
		}

		// Write file data
		qint64 read = 0;
		qint64 totRead = 0;
		qint64 toRead = actualFile.size();

		if (level == MvdZip::NoCompression)
		{
			while ( (read = actualFile.read(buffer1, MVD_ZIP_READ_BUFFER)) > 0 )
			{
				crc = crc32(crc, uBuffer, read);

				if (password != 0)
					encryptBytes(keys, buffer1, read);

				if ( (written = device->write(buffer1, read)) != read )
				{
					actualFile.close();
					delete h;
					return MvdZip::WriteError;
				}
			}
		}
		else
		{
			z_stream zstr;

			// Initialize zalloc, zfree and opaque before calling the init function
			zstr.zalloc = Z_NULL;
			zstr.zfree = Z_NULL;
			zstr.opaque = Z_NULL;

			int zret;

			// Use deflateInit2 with negative windowBits to get raw compression
			if ((zret = deflateInit2_(
				&zstr,
				(int)level,
				Z_DEFLATED,
				-MAX_WBITS,
				8,
				isPNGFile ? Z_RLE : Z_DEFAULT_STRATEGY,
				ZLIB_VERSION,
				sizeof(z_stream)
				)) != Z_OK )
			{
				actualFile.close();
				eLog() << "MvdZip: Could not initialize zlib for compression";
				delete h;
				return MvdZip::ZlibError;
			}

			qint64 compressed;

			int flush = Z_NO_FLUSH;

			do
			{
				read = actualFile.read(buffer1, MVD_ZIP_READ_BUFFER);
				totRead += read;

				if (read == 0)
					break;
				if (read < 0)
				{
					actualFile.close();
					deflateEnd(&zstr);
					eLog() << QString("MvdZip: Error while reading %1").arg(file.absoluteFilePath());
					delete h;
					return MvdZip::ReadError;
				}

				crc = crc32(crc, uBuffer, read);

				zstr.next_in = (Bytef*) buffer1;
				zstr.avail_in = (uInt)read;

				// Tell zlib if this is the last chunk we want to encode
				// by setting the flush parameter to Z_FINISH
				flush = (totRead == toRead) ? Z_FINISH : Z_NO_FLUSH;

				// Run deflate() on input until output buffer not full
				// finish compression if all of source has been read in
				do
				{
					zstr.next_out = (Bytef*) buffer2;
					zstr.avail_out = MVD_ZIP_READ_BUFFER;

					zret = deflate(&zstr, flush);
					// State not clobbered
					Q_ASSERT(zret != Z_STREAM_ERROR);

					// Write compressed data to file and empty buffer
					compressed = MVD_ZIP_READ_BUFFER - zstr.avail_out;

					if (password != 0)
						encryptBytes(keys, buffer2, compressed);

					if (device->write(buffer2, compressed) != compressed)
					{
						deflateEnd(&zstr);
						actualFile.close();
						eLog() << QString("MvdZip: Error while writing %1").arg(file.absoluteFilePath());
						delete h;
						return MvdZip::WriteError;
					}

					written += compressed;

				} while (zstr.avail_out == 0);

				// All input will be used
				Q_ASSERT(zstr.avail_in == 0);

			} while (flush != Z_FINISH);

			// Stream will be complete
			Q_ASSERT(zret == Z_STREAM_END);

			deflateEnd(&zstr);

		} // if (level != STORE)

		actualFile.close();
	}

	// NoCompression end of entry offset
	quint32 current = device->pos();

	// Update crc and compressed size in local header
	if (!device->seek(crcOffset))
	{
		delete h;
		return MvdZip::SeekError;
	}

	h->crc = dirOnly ? 0 : crc;
	h->szComp += written;

	setULong(h->crc, buffer1, 0);
	setULong(h->szComp, buffer1, 4);
	if ( device->write(buffer1, 8) != 8)
	{
		delete h;
		return MvdZip::WriteError;
	}

	// Seek to end of entry
	if (!device->seek(current))
	{
		delete h;
		return MvdZip::SeekError;
	}

	if ((h->gpFlag[0] & 8) == 8)
	{
		// Write data descriptor

		// Signature: PK\7\8
		buffer1[0] = 'P';
		buffer1[1] = 'K';
		buffer1[2] = 0x07;
		buffer1[3] = 0x08;

		// CRC
		setULong(h->crc, buffer1, MVD_ZIP_DD_OFF_MVD_CRC32);

		// Compressed size
		setULong(h->szComp, buffer1, MVD_ZIP_DD_OFF_CSIZE);

		// Uncompressed size
		setULong(h->szUncomp, buffer1, MVD_ZIP_DD_OFF_USIZE);

		if (device->write(buffer1, MVD_ZIP_DD_SIZE_WS) != MVD_ZIP_DD_SIZE_WS)
		{
			delete h;
			return MvdZip::WriteError;
		}
	}

	headers->insert(entryName, h);
	return MvdZip::NoError;
}

//! \internal
int MvdZip_P::decryptByte(quint32 key2) const
{
	quint16 temp = ((quint16)(key2) & 0xffff) | 2;
	return (int)(((temp * (temp ^ 1)) >> 8) & 0xff);
}

//! \internal Writes an quint32 (4 bytes) to a byte array at given offset.
void MvdZip_P::setULong(quint32 v, char* buffer, unsigned int offset)
{
	buffer[offset+3] = ((v >> 24) & 0xFF);
	buffer[offset+2] = ((v >> 16) & 0xFF);
	buffer[offset+1] = ((v >> 8) & 0xFF);
	buffer[offset] = (v & 0xFF);
}

//! \internal Initializes decryption keys using a password.
void MvdZip_P::initKeys(quint32* keys) const
{
	// Encryption keys initialization constants are taken from the
	// PKZip file format specification docs
	keys[0] = 305419896L;
	keys[1] = 591751049L;
	keys[2] = 878082192L;

	QByteArray pwdBytes = password.toAscii();
	int sz = pwdBytes.size();
	const char* ascii = pwdBytes.data();

	for (int i=0; i<sz; ++i)
		updateKeys(keys, (int)ascii[i]);
}

//! \internal Updates encryption keys.
void MvdZip_P::updateKeys(quint32* keys, int c) const
{
	keys[0] = MVD_CRC32(keys[0], c);
	keys[1] += keys[0] & 0xff;
	keys[1] = keys[1] * 134775813L + 1;
	keys[2] = MVD_CRC32(keys[2], ((int)keys[1]) >> 24);
}

//! \internal Encrypts a byte array.
void MvdZip_P::encryptBytes(quint32* keys, char* buffer, qint64 read)
{
	char t;

	for (int i=0; i<(int)read; ++i)
	{
		t = buffer[i];
		buffer[i] ^= decryptByte(keys[2]);
		updateKeys(keys, t);
	}
}

//! \internal Detects the best compression level for a given file extension.
MvdZip::CompressionLevel MvdZip_P::detectCompressionByMime(const QString& ext)
{
	// files really hard to compress
	if ((ext == "png") ||
		(ext == "jpg") ||
		(ext == "jpeg") ||
		(ext == "mp3") ||
		(ext == "ogg") ||
		(ext == "ogm") ||
		(ext == "avi") ||
		(ext == "mov") ||
		(ext == "rm") ||
		(ext == "ra") ||
		(ext == "zip") ||
		(ext == "rar") ||
		(ext == "bz2") ||
		(ext == "gz") ||
		(ext == "7z") ||
		(ext == "z") ||
		(ext == "jar")
		) return MvdZip::NoCompression;

	// files slow and hard to compress
	if ((ext == "exe") ||
		(ext == "bin") ||
		(ext == "rpm") ||
		(ext == "deb")
		) return MvdZip::Deflate2Compression;

	return MvdZip::Deflate9Compression;
}

/*!
	Closes the current archive and writes out pending data.
*/
MvdZip::ErrorCode MvdZip_P::closeArchive()
{
	// Close current archive by writing out central directory
	// and free up resources

	if (device == 0)
		return MvdZip::NoError;

	if (headers == 0)
		return MvdZip::NoError;

	const MvdZipEntry_P* h;

	unsigned int sz;
	quint32 szCentralDir = 0;
	quint32 offCentralDir = device->pos();

	for (QMap<QString,MvdZipEntry_P*>::ConstIterator itr = headers->constBegin(); itr != headers->constEnd(); ++itr)
	{
		h = itr.value();

		// signature
		buffer1[0] = 'P';
		buffer1[1] = 'K';
		buffer1[2] = 0x01;
		buffer1[3] = 0x02;

		// version made by  (currently only MS-DOS/FAT - no symlinks or other stuff supported)
		buffer1[MVD_ZIP_CD_OFF_MADEBY] = buffer1[MVD_ZIP_CD_OFF_MADEBY + 1] = 0;

		// version needed to extract
		buffer1[MVD_ZIP_CD_OFF_VERSION] = MVD_ZIP_VERSION;
		buffer1[MVD_ZIP_CD_OFF_VERSION + 1] = 0;

		// general purpose flag
		buffer1[MVD_ZIP_CD_OFF_GPFLAG] = h->gpFlag[0];
		buffer1[MVD_ZIP_CD_OFF_GPFLAG + 1] = h->gpFlag[1];

		// compression method
		buffer1[MVD_ZIP_CD_OFF_CMET] = h->compMethod & 0xFF;
		buffer1[MVD_ZIP_CD_OFF_CMET + 1] = (h->compMethod >> 8) & 0xFF;

		// last mod file time
		buffer1[MVD_ZIP_CD_OFF_MODT] = h->modTime[0];
		buffer1[MVD_ZIP_CD_OFF_MODT + 1] = h->modTime[1];

		// last mod file date
		buffer1[MVD_ZIP_CD_OFF_MODD] = h->modDate[0];
		buffer1[MVD_ZIP_CD_OFF_MODD + 1] = h->modDate[1];

		// crc (4bytes) [16,17,18,19]
		setULong(h->crc, buffer1, MVD_ZIP_CD_OFF_CRC);

		// compressed size (4bytes: [20,21,22,23])
		setULong(h->szComp, buffer1, MVD_ZIP_CD_OFF_CSIZE);

		// uncompressed size [24,25,26,27]
		setULong(h->szUncomp, buffer1, MVD_ZIP_CD_OFF_USIZE);

		// filename
		QByteArray fileNameBytes = itr.key().toAscii();
		sz = fileNameBytes.size();
		buffer1[MVD_ZIP_CD_OFF_NAMELEN] = sz & 0xFF;
		buffer1[MVD_ZIP_CD_OFF_NAMELEN + 1] = (sz >> 8) & 0xFF;

		// extra field length
		buffer1[MVD_ZIP_CD_OFF_XLEN] = buffer1[MVD_ZIP_CD_OFF_XLEN + 1] = 0;

		// file comment length
		buffer1[MVD_ZIP_CD_OFF_COMMLEN] = buffer1[MVD_ZIP_CD_OFF_COMMLEN + 1] = 0;

		// disk number start
		buffer1[MVD_ZIP_CD_OFF_DISKSTART] = buffer1[MVD_ZIP_CD_OFF_DISKSTART + 1] = 0;

		// internal file attributes
		buffer1[MVD_ZIP_CD_OFF_IATTR] = buffer1[MVD_ZIP_CD_OFF_IATTR + 1] = 0;

		// external file attributes
		buffer1[MVD_ZIP_CD_OFF_EATTR] =
			buffer1[MVD_ZIP_CD_OFF_EATTR + 1] =
			buffer1[MVD_ZIP_CD_OFF_EATTR + 2] =
			buffer1[MVD_ZIP_CD_OFF_EATTR + 3] = 0;

		// relative offset of local header [42->45]
		setULong(h->lhOffset, buffer1, MVD_ZIP_CD_OFF_LHOFF);

		if (device->write(buffer1, MVD_ZIP_CD_SIZE) != MVD_ZIP_CD_SIZE)
		{
			//! \todo See if we can detect QFile objects using the Qt Meta Object System
			/*
			if (!device->remove())
			eLog() << tr("Unable to delete corrupted archive: %1").arg(device->fileName());
			*/
			return MvdZip::WriteError;
		}

		// Write out filename
		if ((unsigned int)device->write(fileNameBytes) != sz)
		{
			//! \todo SAME AS ABOVE: See if we can detect QFile objects using the Qt Meta Object System
			/*
			if (!device->remove())
			eLog() << tr("Unable to delete corrupted archive: %1").arg(device->fileName());
			*/
			return MvdZip::WriteError;
		}

		szCentralDir += (MVD_ZIP_CD_SIZE + sz);

	} // central dir headers loop


	// Write end of central directory

	// signature
	buffer1[0] = 'P';
	buffer1[1] = 'K';
	buffer1[2] = 0x05;
	buffer1[3] = 0x06;

	// number of this disk
	buffer1[MVD_ZIP_EOCD_OFF_DISKNUM] = buffer1[MVD_ZIP_EOCD_OFF_DISKNUM + 1] = 0;

	// number of disk with central directory
	buffer1[MVD_ZIP_EOCD_OFF_CDDISKNUM] = buffer1[MVD_ZIP_EOCD_OFF_CDDISKNUM + 1] = 0;

	// number of entries in this disk
	sz = headers->count();
	buffer1[MVD_ZIP_EOCD_OFF_ENTRIES] = sz & 0xFF;
	buffer1[MVD_ZIP_EOCD_OFF_ENTRIES + 1] = (sz >> 8) & 0xFF;

	// total number of entries
	buffer1[MVD_ZIP_EOCD_OFF_CDENTRIES] = buffer1[MVD_ZIP_EOCD_OFF_ENTRIES];
	buffer1[MVD_ZIP_EOCD_OFF_CDENTRIES + 1] = buffer1[MVD_ZIP_EOCD_OFF_ENTRIES + 1];

	// size of central directory [12->15]
	setULong(szCentralDir, buffer1, MVD_ZIP_EOCD_OFF_CDSIZE);

	// central dir offset [16->19]
	setULong(offCentralDir, buffer1, MVD_ZIP_EOCD_OFF_CDOFF);

	// ZIP file comment length
	QByteArray commentBytes = comment.toAscii();
	quint16 commentLength = commentBytes.size();

	if (commentLength == 0)
	{
		buffer1[MVD_ZIP_EOCD_OFF_COMMLEN] = buffer1[MVD_ZIP_EOCD_OFF_COMMLEN + 1] = 0;
	}
	else
	{
		buffer1[MVD_ZIP_EOCD_OFF_COMMLEN] = commentLength & 0xFF;
		buffer1[MVD_ZIP_EOCD_OFF_COMMLEN + 1] = (commentLength >> 8) & 0xFF;
	}

	if (device->write(buffer1, MVD_ZIP_EOCD_SIZE) != MVD_ZIP_EOCD_SIZE)
	{
		//! \todo SAME AS ABOVE: See if we can detect QFile objects using the Qt Meta Object System
		return MvdZip::WriteError;
	}

	if (commentLength != 0)
	{
		if ((unsigned int)device->write(commentBytes) != commentLength)
		{
			//! \todo SAME AS ABOVE: See if we can detect QFile objects using the Qt Meta Object System
			return MvdZip::WriteError;
		}
	}

	return MvdZip::NoError;
}

//! \internal
void MvdZip_P::reset()
{
	comment.clear();

	if (headers != 0)
	{
		qDeleteAll(*headers);
		delete headers;
		headers = 0;
	}

	delete device; device = 0;
}

/*!
	\internal Returns the path of the parent directory,
	i.e. extractRoot("/home/blue/.movida") will return
	"/home".
*/
QString MvdZip_P::extractRoot(const QString& p, MvdZip::CompressionOptions o)
{
	Q_UNUSED(o);

	QDir d(QDir::cleanPath(p));
	if (!d.exists())
		return QString();

	if (!d.cdUp())
		return QString();

	return d.absolutePath();
}


/************************************************************************
MvdZip
*************************************************************************/

/*!
	Creates a new Zip file compressor.
*/
MvdZip::MvdZip()
{
	d = new MvdZip_P;
}

/*!
	Closes any open archive and releases used resources.
*/
MvdZip::~MvdZip()
{
	closeArchive();
	delete d;
}

/*!
	Returns true if there is an open archive.
*/
bool MvdZip::isOpen() const
{
	return d->device != 0;
}

/*!
	Sets the password to be used for the next files being added!
	Files added before calling this method will use the previously
	set password (if any).
	Closing the archive won't clear the password!
*/
void MvdZip::setPassword(const QString& pwd)
{
	d->password = pwd;
}

//! Convenience method, clears the current password.
void MvdZip::clearPassword()
{
	d->password.clear();
}

//! Returns the currently used password.
QString MvdZip::password() const
{
	return d->password;
}

/*!
	Attempts to create a new Zip archive. If \p overwrite is true and the file
	already exist it will be overwritten.
	Any open archive will be closed.
*/
MvdZip::ErrorCode MvdZip::createArchive(const QString& filename, bool overwrite)
{
	QFile* file = new QFile(filename);

	if (file->exists() && !overwrite) {
		delete file;
		return MvdZip::FileExistsError;
	}

	if (!file->open(QIODevice::WriteOnly)) {
		delete file;
		return MvdZip::FileOpenError;
	}

	MvdZip::ErrorCode ec = createArchive(file);
	if (ec != MvdZip::NoError) {
		file->remove();
	}

	return ec;
}

/*!
	Attempts to create a new Zip archive. If there is another open archive this will be closed.
	\warning The class takes ownership of the device!
*/
MvdZip::ErrorCode MvdZip::createArchive(QIODevice* device)
{
	if (device == 0)
	{
		eLog() << "MvdZip: Invalid device.";
		return MvdZip::FileOpenError;
	}

	return d->createArchive(device);
}

/*!
	Returns the current archive comment.
*/
QString MvdZip::archiveComment() const
{
	return d->comment;
}

/*!
	Sets the comment for this archive. Note: createArchive() should have been
	called before.
*/
void MvdZip::setArchiveComment(const QString& comment)
{
	if (d->device != 0)
		d->comment = comment;
}

/*!
	Convenience method, same as calling
	MvdZip::addDirectory(const QString&,const QString&,CompressionLevel)
	with an empty \p root parameter (or with the parent directory of \p path if the
	AbsolutePathsOption options is set).

	The ExtractionOptions are checked in the order they are defined in the zip.h heaser file.
	This means that the last one overwrites the previous one (if some conflict occurs), i.e.
	MvdZip::IgnorePathsOption | MvdZip::AbsolutePathsOption would be interpreted as MvdZip::IgnorePathsOption.
*/
MvdZip::ErrorCode MvdZip::addDirectory(const QString& path, CompressionOptions options, CompressionLevel level)
{
	return addDirectory(path, QString(), options, level);
}

/*!
	Convenience method, same as calling MvdZip::addDirectory(const QString&,
	const QString&,CompressionOptions,CompressionLevel)
	with the MvdZip::RelativePathsOption flag as compression option.
*/
MvdZip::ErrorCode MvdZip::addDirectory(const QString& path, const QString& root, CompressionLevel level)
{
	return addDirectory(path, root, MvdZip::RelativePathsOption, level);
}

/*!
	Convenience method, same as calling MvdZip::addDirectory(const QString&,
	const QString&,CompressionOptions,CompressionLevel)
	with the MvdZip::IgnorePathsOption flag as compression option and an empty \p root parameter.
*/
MvdZip::ErrorCode MvdZip::addDirectoryContents(const QString& path, CompressionLevel level)
{
	return addDirectory(path, QString(), IgnorePathsOption, level);
}

/*!
	Convenience method, same as calling MvdZip::addDirectory(const QString&,
	const QString&,CompressionOptions,CompressionLevel)
	with the MvdZip::IgnorePathsOption flag as compression option.
*/
MvdZip::ErrorCode MvdZip::addDirectoryContents(const QString& path, 
	const QString& root, CompressionLevel level)
{
	return addDirectory(path, root, IgnorePathsOption, level);
}

/*!
	Recursively adds files contained in \p dir to the archive, using \p root as 
	name for the root folder.
	Stops adding files if some error occurs.

	The ExtractionOptions are checked in the order they are defined in the zip.h heaser file.
	This means that the last one overwrites the previous one (if some conflict occurs), i.e.
	MvdZip::IgnorePathsOption | MvdZip::AbsolutePathsOption would be interpreted as MvdZip::IgnorePathsOption.

	The \p root parameter is ignored with the MvdZip::IgnorePathsOption parameter and used as 
	path prefix (a trailing / is always added as directory separator!) otherwise 
	(even with MvdZip::AbsolutePathsOption set!).
*/
MvdZip::ErrorCode MvdZip::addDirectory(const QString& path, const QString& root, 
	CompressionOptions options, CompressionLevel level)
{
	// eLog() << QString("addDir(path=%1, root=%2)").arg(path, root);

	// Bad boy didn't call createArchive() yet :)
	if (d->device == 0)
		return MvdZip::NoOpenArchiveError;

	QDir dir(path);
	if (!dir.exists())
		return MvdZip::FileNotFoundError;

	// ActualRoot is the path to be written in the zip records
	// Remove any trailing separator
	QString actualRoot = root.trimmed();

	// Preserve Unix root
	if (actualRoot != "/")
	{
		while (actualRoot.endsWith("/") || actualRoot.endsWith("\\"))
			actualRoot.truncate(actualRoot.length() - 1);
	}

	// QDir::cleanPath() fixes some issues with QDir::dirName()
	QFileInfo current(QDir::cleanPath(path));

	if (!actualRoot.isEmpty() && actualRoot != "/")
		actualRoot.append("/");

	/* This part is quite confusing and needs some test or check */
	/* An attempt to compress the / root directory evtl. using a root prefix should be a good test */
	if (options.testFlag(AbsolutePathsOption) && !options.testFlag(IgnorePathsOption))
	{
		QString absolutePath = d->extractRoot(path, options);

		// addDir("/home/blue/.movida", "myroot") -> absolutePath = "/home/"
		if (!absolutePath.isEmpty() && absolutePath != "/")
			absolutePath.append("/");

		// addDir("/home/blue/.movida", "myroot") -> actualRoot = "myroot/home/"
		if (absolutePath.startsWith("/"))
			actualRoot.append(absolutePath.right(absolutePath.length() -1 ));
		else
			actualRoot.append(absolutePath);
	}

	if ( !(options.testFlag(IgnorePathsOption) || options.testFlag(IgnoreRootOption)) )
	{
		actualRoot.append(QDir(current.absoluteFilePath()).dirName())
			.append("/");
	}

	// actualRoot now contains the path of the file relative to the zip archive
	// with a trailing /

	QFileInfoList list = dir.entryInfoList(
		QDir::Files |
		QDir::Dirs |
		QDir::NoDotAndDotDot |
		QDir::NoSymLinks);

	ErrorCode ec = MvdZip::NoError;
	bool filesAdded = false;

	CompressionOptions recursionOptions;
	if (options.testFlag(IgnorePathsOption))
		recursionOptions |= IgnorePathsOption;
	else recursionOptions |= RelativePathsOption;

	for (int i = 0; i < list.size() && ec == MvdZip::NoError; ++i)
	{
		QFileInfo info = list.at(i);

		if (info.isDir())
		{
			// Recursion :)
			ec = addDirectory(info.absoluteFilePath(), actualRoot, recursionOptions, level);
		}
		else
		{
			ec = d->createEntry(info, actualRoot, level);
			filesAdded = true;
		}
	}


	// We need an explicit record for this dir
	// Non-empty directories don't need it because they have a path component in the filename
	if (!filesAdded && !options.testFlag(IgnorePathsOption))
		ec = d->createEntry(current, actualRoot, level);

	return ec;
}

/*!
	Closes the archive and writes any pending data.
*/
MvdZip::ErrorCode MvdZip::closeArchive()
{
	MvdZip::ErrorCode ec = d->closeArchive();
	d->reset();
	return ec;
}

/*!
	Returns a locale translated error string for a given error code.
*/
QString MvdZip::formatError(MvdZip::ErrorCode c) const
{
	switch (c)
	{
	case NoError: return QCoreApplication::translate("MvdZip", "ZIP operation completed successfully."); break;
	case ZlibInitError: return QCoreApplication::translate("MvdZip", "Failed to initialize or load zlib library."); break;
	case ZlibError: return QCoreApplication::translate("MvdZip", "zlib library error."); break;
	case FileOpenError: return QCoreApplication::translate("MvdZip", "Unable to create or open file."); break;
	case NoOpenArchiveError: return QCoreApplication::translate("MvdZip", "No archive has been created yet."); break;
	case FileNotFoundError: return QCoreApplication::translate("MvdZip", "File or directory does not exist."); break;
	case ReadError: return QCoreApplication::translate("MvdZip", "File read error."); break;
	case WriteError: return QCoreApplication::translate("MvdZip", "File write error."); break;
	case SeekError: return QCoreApplication::translate("MvdZip", "File seek error."); break;
	default: ;
	}

	return QCoreApplication::translate("MvdZip", "Unknown error.");
}
