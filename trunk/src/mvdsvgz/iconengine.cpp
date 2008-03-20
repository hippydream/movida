/**************************************************************************
** Filename: iconengine.cpp
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

#include "iconengine.h"
#include "zlib/zlib.h"
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>
#include <QPixmapCache>
#include <QStyle>
#include <QApplication>
#include <QStyleOption>
#include <QFileInfo>
#include <QDataStream>
#include <QBuffer>
#include <QtDebug>

#define GZ_READ_BUFFER (256*1024)

#define GZ_OK 0
#define GZ_INVALID_OUTPUT_DEVICE -1
#define GZ_FILE_OPEN_ERROR -2
#define GZ_INVALID_STREAM -3
#define GZ_READ_ERROR -4
#define GZ_WRITE_ERROR -5


struct MvdSvgzCacheEntry
{
	MvdSvgzCacheEntry()
		: mode(QIcon::Normal), state(QIcon::Off) {}
	MvdSvgzCacheEntry(const QPixmap &pm, QIcon::Mode m = QIcon::Normal, QIcon::State s = QIcon::Off)
		: pixmap(pm), mode(m), state(s) {}

	QPixmap pixmap;
	QIcon::Mode mode;
	QIcon::State state;
};

class MvdSvgzIconEnginePrivate : public QSharedData
{
public:
	explicit MvdSvgzIconEnginePrivate()
	{
		render = new QSvgRenderer;
	}
	~MvdSvgzIconEnginePrivate()
	{
		delete render;
		render = 0;
	}
	static int inflateFile(const QString& filename, QIODevice* output)
	{
		if (!output)
			return GZ_INVALID_OUTPUT_DEVICE;

		QFile file(filename);
		if (!file.open(QIODevice::ReadOnly)) {
			qDebug("Failed to open input file: %s", qPrintable(filename));
			return GZ_FILE_OPEN_ERROR;
		}

		quint64 compressedSize = file.size();

		uInt rep = compressedSize / GZ_READ_BUFFER;
		uInt rem = compressedSize % GZ_READ_BUFFER;
		uInt cur = 0;

		qint64 read;
		quint64 tot = 0;

		char buffer1[GZ_READ_BUFFER];
		char buffer2[GZ_READ_BUFFER];

		/* Allocate inflate state */
		z_stream zstr;
		zstr.zalloc = Z_NULL;
		zstr.zfree = Z_NULL;
		zstr.opaque = Z_NULL;
		zstr.next_in = Z_NULL;
		zstr.avail_in = 0;

		int zret;

		/*
			 windowBits can also be greater than 15 for optional gzip decoding. Add
			32 to windowBits to enable zlib and gzip decoding with automatic header
			detection, or add 16 to decode only the gzip format (the zlib format will
			return a Z_DATA_ERROR.  If a gzip stream is being decoded, strm->adler is
			a crc32 instead of an adler32.
		*/
		if ( (zret = inflateInit2_(&zstr, MAX_WBITS + 16, ZLIB_VERSION, sizeof(z_stream))) != Z_OK ) {
			qDebug("Failed to initialize zlib");
			return GZ_INVALID_STREAM;
		}

		int szDecomp;

		// Decompress until deflate stream ends or end of file
		do {
			read = file.read(buffer1, cur < rep ? GZ_READ_BUFFER : rem);
			if (read == 0)
				break;
			if (read < 0) {
				(void)inflateEnd(&zstr);
				qDebug("Read error");
				return GZ_READ_ERROR;
			}

			cur++;
			tot += read;

			zstr.avail_in = (uInt) read;
			zstr.next_in = (Bytef*) buffer1;


			// Run inflate() on input until output buffer not full
			do {
				zstr.avail_out = GZ_READ_BUFFER;
				zstr.next_out = (Bytef*) buffer2;;

				zret = inflate(&zstr, Z_NO_FLUSH);

				switch (zret) {
					case Z_NEED_DICT:
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
						inflateEnd(&zstr);
						qDebug("zlib failed to decode file");
						return GZ_INVALID_STREAM;
					default:
						;
				}

				szDecomp = GZ_READ_BUFFER - zstr.avail_out;
				if (output->write(buffer2, szDecomp) != szDecomp) {
					inflateEnd(&zstr);
					qDebug("Write error");
					return GZ_WRITE_ERROR;
				}

			} while (zstr.avail_out == 0);

		} while (zret != Z_STREAM_END);

		inflateEnd(&zstr);

		return GZ_OK;
	}
	static inline QByteArray decompressGZipFile(const QString& fileName)
	{
		QBuffer buffer;
		buffer.open(QBuffer::WriteOnly);
		if (inflateFile(fileName, &buffer) != GZ_OK)
			return QByteArray();
		return buffer.data();
	}
	static inline int createKey(const QSize &size, QIcon::Mode mode, QIcon::State state)
	{
		return ((((((size.width()<<11)|size.height())<<11)|mode)<<4)|state);
	}

	QSvgRenderer* render;
	QHash<int, MvdSvgzCacheEntry> svgCache;
	QString svgFile;
};

MvdSvgzIconEngine::MvdSvgzIconEngine()
: d(new MvdSvgzIconEnginePrivate)
{
}

MvdSvgzIconEngine::MvdSvgzIconEngine(const MvdSvgzIconEngine& other)
: QIconEngineV2(other), d(new MvdSvgzIconEnginePrivate)
{
	d->render->load(MvdSvgzIconEnginePrivate::decompressGZipFile(other.d->svgFile));
	d->svgCache = other.d->svgCache;
}

MvdSvgzIconEngine::~MvdSvgzIconEngine()
{
}

QSize MvdSvgzIconEngine::actualSize(const QSize& size, QIcon::Mode, QIcon::State )
{
	return size;
}

QPixmap MvdSvgzIconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state)
{
	int index = MvdSvgzIconEnginePrivate::createKey(size, mode, state);
	if (d->svgCache.contains(index))
		return d->svgCache.value(index).pixmap;
	QImage img(size, QImage::Format_ARGB32_Premultiplied);
	img.fill(0x00000000);
	QPainter p(&img);
	d->render->render(&p);
	p.end();
	QPixmap pm = QPixmap::fromImage(img);
	QStyleOption opt(0);
	opt.palette = QApplication::palette();
	QPixmap generated = QApplication::style()->generatedIconPixmap(mode, pm, &opt);
	if (!generated.isNull())
		pm = generated;

	d->svgCache.insert(index, MvdSvgzCacheEntry(pm, mode, state));

	return pm;
}

void MvdSvgzIconEngine::addPixmap(const QPixmap& pixmap, QIcon::Mode mode, QIcon::State state)
{
	int index = MvdSvgzIconEnginePrivate::createKey(pixmap.size(), mode, state);
	d->svgCache.insert(index, pixmap);
}

void MvdSvgzIconEngine::addFile(const QString& fileName, const QSize&, QIcon::Mode, QIcon::State)
{
	if (!fileName.isEmpty()) {
		QString abs = fileName;
		if (fileName.at(0) != QLatin1Char(':'))
			abs = QFileInfo(fileName).absoluteFilePath();

		d->svgFile = abs;
		d->render->load(MvdSvgzIconEnginePrivate::decompressGZipFile(abs));
		//qDebug()<<"loaded "<<abs<<", isOK = "<<d->render->isValid();
	}
}

void MvdSvgzIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state)
{
	painter->drawPixmap(rect, pixmap(rect.size(), mode, state));
}

QString MvdSvgzIconEngine::key() const
{
	return QLatin1String("svgz");
}

QIconEngineV2* MvdSvgzIconEngine::clone() const
{
	return new MvdSvgzIconEngine(*this);
}

bool MvdSvgzIconEngine::read(QDataStream& in)
{
	QPixmap pixmap;
	QByteArray data;
	uint mode;
	uint state;
	int num_entries;

	in >> data;
	if (!data.isEmpty()) {
#ifndef QT_NO_COMPRESS
		data = qUncompress(data);
#endif
		if (!data.isEmpty())
			d->render->load(data);
	}
	in >> num_entries;
	for (int i=0; i<num_entries; ++i) {
		if (in.atEnd()) {
			d->svgCache.clear();
			return false;
		}
		in >> pixmap;
		in >> mode;
		in >> state;
		addPixmap(pixmap, QIcon::Mode(mode), QIcon::State(state));
	}
	return true;
}

bool MvdSvgzIconEngine::write(QDataStream& out) const
{
	if (!d->svgFile.isEmpty()) {
		QFile file(d->svgFile);
		if (file.open(QIODevice::ReadOnly))
#ifndef QT_NO_COMPRESS
			out << qCompress(file.readAll());
#else
			out << file.readAll();
#endif
		else
			out << QByteArray();
	} else {
		out << QByteArray();
	}
	QList<int> keys = d->svgCache.keys();
	out << keys.size();
	for (int i=0; i<keys.size(); ++i) {
		out << d->svgCache.value(keys.at(i)).pixmap;
		out << (uint) d->svgCache.value(keys.at(i)).mode;
		out << (uint) d->svgCache.value(keys.at(i)).state;
	}
	return true;
}
