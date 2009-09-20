#include "iconengine.h"

//#define MVD_SVGZ_NO_CUSTOM_ENGINE

#include <QtCore/QAtomicInt>
#include <QtCore/QBuffer>
#include <QtCore/QDataStream>
#include <QtCore/QFileInfo>
#include <QtCore/QtDebug>
#include <QtCore/QTime>
#include <QtGui/QApplication>
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#ifdef MVD_SVGZ_NO_CUSTOM_ENGINE
#include <QtGui/QPixmapCache>
#endif
#include <QtGui/QStyle>
#include <QtGui/QStyleOption>
#include <QtSvg/QSvgRenderer>

#include <zlib/zlib.h>

#ifndef MVD_SVGZ_NO_CUSTOM_ENGINE

namespace {
const int GZipReadBuffer = (256 * 1024);

const int GZipOk = 0;
const int GZipInvalidOutputDevice = -1;
const int GZipFileOpenError = -2;
const int GZipInvalidStream = -3;
const int GZipReadError = -4;
const int GZipWriteError = -5;
}

static int inflateFile(const QString &filename, QIODevice *output)
{
    if (!output)
        return GZipInvalidOutputDevice;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug("Failed to open input file: %s", qPrintable(filename));
        return GZipFileOpenError;
    }

    quint64 compressedSize = file.size();

    uInt rep = compressedSize / GZipReadBuffer;
    uInt rem = compressedSize % GZipReadBuffer;
    uInt cur = 0;

    qint64 read;
    quint64 tot = 0;

    char buffer1[GZipReadBuffer];
    char buffer2[GZipReadBuffer];

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
    if ((zret = inflateInit2_(&zstr, MAX_WBITS + 16, ZLIB_VERSION, sizeof(z_stream))) != Z_OK) {
        qWarning("MvdSvgzIconEngine: Failed to initialize zlib (error code %d).", zret);
        return GZipInvalidStream;
    }

    int szDecomp;

    // Decompress until deflate stream ends or end of file
    do {
        read = file.read(buffer1, cur < rep ? GZipReadBuffer : rem);
        if (read == 0)
            break;
        if (read < 0) {
            (void)inflateEnd(&zstr);
            qWarning("MvdSvgzIconEngine: Read error (%s).", qPrintable(file.errorString()));
            return GZipReadError;
        }

        cur++;
        tot += read;

        zstr.avail_in = (uInt)read;
        zstr.next_in = (Bytef *)buffer1;


        // Run inflate() on input until output buffer not full
        do {
            zstr.avail_out = GZipReadBuffer;
            zstr.next_out = (Bytef *)buffer2;;

            zret = inflate(&zstr, Z_NO_FLUSH);

            switch (zret) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                inflateEnd(&zstr);
                qWarning("MvdSvgzIconEngine: zlib failed to decode the file (error code %d)", zret);
                return GZipInvalidStream;

            default:
                ;
            }

            szDecomp = GZipReadBuffer - zstr.avail_out;
            if (output->write(buffer2, szDecomp) != szDecomp) {
                inflateEnd(&zstr);
                qDebug("MvdSvgzIconEngine: Write error (%s).", qPrintable(output->errorString()));
                return GZipWriteError;
            }

        } while (zstr.avail_out == 0);

    } while (zret != Z_STREAM_END);

    inflateEnd(&zstr);

    return GZipOk;
}

static inline QByteArray decompressGZipFile(const QString &fileName)
{
    QBuffer buffer;

    buffer.open(QBuffer::WriteOnly);
    if (inflateFile(fileName, &buffer) != GZipOk)
        return QByteArray();
    return buffer.data();
}

static inline int createKey(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    return ((((((size.width() << 11) | size.height()) << 11) | mode) << 4) | state);
}


//////////////////////////////////////////////////////////////////////////


class MvdSvgzIconEngine::Private : public QSharedData
{
public:
    Private()
    {
    }

    ~Private()
    {
    }

    QPixmap renderFile(const QString &fileName, const QSize &size) {
        /*qDebug("MvdSvgzIconEngine: Rendering %s at %dx%d", 
            qPrintable(fileName), size.width(), size.height());
        QTime t;
        t.start();*/

        QByteArray data = decompressGZipFile(fileName);
        if (data.isEmpty())
            return QPixmap();

        QSvgRenderer renderer;
        if (!renderer.load(data))
            return QPixmap();

        QSize actualSize = renderer.defaultSize();
        if (!actualSize.isNull())
            actualSize.scale(size, Qt::KeepAspectRatio);   

        QImage img(actualSize, QImage::Format_ARGB32_Premultiplied);
        img.fill(0x00000000);

        QPainter p(&img);
        renderer.render(&p);
        p.end();

        QPixmap pm = QPixmap::fromImage(img);

        // qDebug("MvdSvgzIconEngine: Rendering took %d ms", t.elapsed());

        return pm;
    }

    typedef QHash<int, QPixmap> PixmapCache;
    typedef QHash<int, QString> FileCache;
    PixmapCache pmCache;
    FileCache fileCache;
};


//////////////////////////////////////////////////////////////////////////


MvdSvgzIconEngine::MvdSvgzIconEngine() :
    d(new Private)
{
}

MvdSvgzIconEngine::MvdSvgzIconEngine(const MvdSvgzIconEngine &other) :
    QIconEngineV2(other),
    d(new Private)
{
    d->pmCache = other.d->pmCache;
    d->fileCache = other.d->fileCache;
}

MvdSvgzIconEngine::~MvdSvgzIconEngine()
{
}

QSize MvdSvgzIconEngine::actualSize(const QSize &size, QIcon::Mode, QIcon::State)
{
    return size;
}

QPixmap MvdSvgzIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    const int key = createKey(size, mode, state);
    
    Private::PixmapCache::ConstIterator cache_it = d->pmCache.find(key);
    if (cache_it != d->pmCache.constEnd()) {
        const QPixmap& pixmap = cache_it.value();
        if (pixmap.size() == size)
            return pixmap;
    }

    QPixmap pm;
    const int key_default = createKey(size, QIcon::Normal, QIcon::On);
    cache_it = d->pmCache.find(key_default);
    if (cache_it != d->pmCache.constEnd()) {
        const QPixmap& pixmap = cache_it.value();
        if (pixmap.size() == size)
            pm = pixmap;
    }

    if (pm.isNull()) {
        int k = createKey(QSize(), mode, state);
        QString filename = d->fileCache.value(k);
        if (filename.isEmpty()) {
            k = createKey(QSize(), QIcon::Normal, QIcon::On);
            filename = d->fileCache.value(k);
        }
        if (filename.isEmpty())
            return QPixmap();
        pm = d->renderFile(filename, size);
        if (pm.isNull())
            return QPixmap();
    }
    
    QStyleOption opt(0);
    opt.palette = QApplication::palette();
    
    QPixmap generated = QApplication::style()->generatedIconPixmap(mode, pm, &opt);
    if (!generated.isNull())
        pm = generated;

    d->pmCache.insert(key, pm);

    return pm;
}

void MvdSvgzIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state)
{
    if (pixmap.isNull())
        return;
    const int key = createKey(pixmap.size(), mode, state);
    d->pmCache.insert(key, pixmap);
}

void MvdSvgzIconEngine::addFile(const QString &fileName, const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(size);
    const int key = createKey(QSize(), mode, state);
    d->fileCache.insert(key, fileName);
}

void MvdSvgzIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    painter->drawPixmap(rect, pixmap(rect.size(), mode, state));
}

QString MvdSvgzIconEngine::key() const
{
    return QLatin1String("svgz");
}

QIconEngineV2 *MvdSvgzIconEngine::clone() const
{
    return new MvdSvgzIconEngine(*this);
}

bool MvdSvgzIconEngine::read(QDataStream &in)
{
    // QSharedDataPointer will delete the old pointer
    d = new Private;
    
    int num_entries;
    in >> num_entries;
    for (int i = 0; i < num_entries; ++i) {
        if (in.atEnd()) {
            d->pmCache.clear();
            return false;
        }
        int key;
        QPixmap pm;
        in >> key;
        in >> pm;
        if (!pm.isNull())
            d->pmCache.insert(key, pm);
    }
    in >> num_entries;
    for (int i = 0; i < num_entries; ++i) {
        if (in.atEnd()) {
            d->pmCache.clear();
            d->fileCache.clear();
            return false;
        }
        int key;
        QString file;
        in >> key;
        in >> file;
        if (!file.isEmpty())
            d->fileCache.insert(key, file);
    }
    return true;
}

bool MvdSvgzIconEngine::write(QDataStream &out) const
{
    out << d->pmCache.size();
    Private::PixmapCache::ConstIterator pmBegin = d->pmCache.constBegin();
    Private::PixmapCache::ConstIterator pmEnd = d->pmCache.constEnd();
    while (pmBegin != pmEnd) {
        out << pmBegin.key();
        out << pmBegin.value();
        ++pmBegin;
    }
    out << d->fileCache.size();
    Private::FileCache::ConstIterator fBegin = d->fileCache.constBegin();
    Private::FileCache::ConstIterator fEnd = d->fileCache.constEnd();
    while (fBegin != fEnd) {
        out << fBegin.key();
        out << fBegin.value();
        ++fBegin;
    }
    return true;
}

#else // MVD_SVGZ_CUSTOM_ENGINE

class MvdSvgzIconEngine::Private : public QSharedData
{
public:
    Private() : 
        svgFiles(0),
        svgBuffers(0),
        addedPixmaps(0)
    {
        stepSerialNum();
    }

    ~Private()
    {
        delete addedPixmaps;
        delete svgBuffers;
        delete svgFiles;
    }

    static int hashKey(QIcon::Mode mode, QIcon::State state)
    {
        return (((mode)<<4)|state);
    }

    QString pmcKey(const QSize &size, QIcon::Mode mode, QIcon::State state)
    {
        return QLatin1String("$mvd_svgicon_")
            + QString::number(serialNum, 16).append(QLatin1Char('_'))
            + QString::number((((((size.width()<<11)|size.height())<<11)|mode)<<4)|state, 16);
    }
    
    void stepSerialNum()
    {
        serialNum = lastSerialNum.fetchAndAddRelaxed(1);
    };
    
    void loadData(QSvgRenderer *renderer, QIcon::Mode mode, QIcon::State state);

    QHash<int, QString>* svgFiles;
    QHash<int, QByteArray> *svgBuffers;
    QHash<int, QPixmap> *addedPixmaps;
    int serialNum;
    static QAtomicInt lastSerialNum;
};

void MvdSvgzIconEngine::Private::loadData(QSvgRenderer *renderer, QIcon::Mode mode, 
    QIcon::State state)
{
    QByteArray buf;
    if (svgBuffers) {
        buf = svgBuffers->value(hashKey(mode, state));
        if (buf.isEmpty())
            buf = svgBuffers->value(hashKey(QIcon::Normal, QIcon::Off));
    }

    if (!buf.isEmpty()) {
#ifndef QT_NO_COMPRESS
        buf = qUncompress(buf);
#endif
        renderer->load(buf);

    } else {
        QString svgFile;
        if (svgFiles) {
            svgFile = svgFiles->value(hashKey(mode, state));
            if (svgFile.isEmpty())
                svgFile = svgFiles->value(hashKey(QIcon::Normal, QIcon::Off));
        }
        if (!svgFile.isEmpty())
            renderer->load(svgFile);
    }
}

QAtomicInt MvdSvgzIconEngine::Private::lastSerialNum;

static inline int pmKey(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    return ((((((size.width()<<11)|size.height())<<11)|mode)<<4)|state);
}


//////////////////////////////////////////////////////////////////////////


MvdSvgzIconEngine::MvdSvgzIconEngine() :
    d(new Private)
{
}

MvdSvgzIconEngine::MvdSvgzIconEngine(const MvdSvgzIconEngine &other) :
    QIconEngineV2(other), d(new Private)
{
    if (other.d->svgFiles)
        d->svgFiles = new QHash<int, QString>(*other.d->svgFiles);
    d->svgFiles = other.d->svgFiles;
    if (other.d->svgBuffers)
        d->svgBuffers = new QHash<int, QByteArray>(*other.d->svgBuffers);
    if (other.d->addedPixmaps)
        d->addedPixmaps = new QHash<int, QPixmap>(*other.d->addedPixmaps);
}

MvdSvgzIconEngine::~MvdSvgzIconEngine()
{
}

QSize MvdSvgzIconEngine::actualSize(const QSize &size, QIcon::Mode mode,
    QIcon::State state)
{
   QPixmap pm = pixmap(size, mode, state);
    if (pm.isNull())
        return QSize();
    return pm.size();
}

QPixmap MvdSvgzIconEngine::pixmap(const QSize &size, QIcon::Mode mode,
    QIcon::State state)
{
    QPixmap pm;
    QString pmckey(d->pmcKey(size, mode, state));
    if (QPixmapCache::find(pmckey, pm))
        return pm;

    if (d->addedPixmaps) {
        pm = d->addedPixmaps->value(d->hashKey(mode, state));
        if (!pm.isNull() && pm.size() == size)
            return pm;
    }

    /*QString fileName = !d->svgFiles ? QString() : d->svgFiles->isEmpty() ? QString() : d->svgFiles->begin().value();
    qDebug("MvdSvgzIconEngine: Rendering %s at %dx%d", 
        qPrintable(fileName), size.width(), size.height());
    QTime t;
    t.start();*/

    QSvgRenderer renderer;
    d->loadData(&renderer, mode, state);
    if (!renderer.isValid())
        return pm;

    QSize actualSize = renderer.defaultSize();
    if (!actualSize.isNull())
        actualSize.scale(size, Qt::KeepAspectRatio);

    QImage img(actualSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);

    QPainter p(&img);
    renderer.render(&p);
    p.end();

    pm = QPixmap::fromImage(img);

    //qDebug("MvdSvgzIconEngine: Rendering took %d ms", t.elapsed());

    QStyleOption opt(0);
    opt.palette = QApplication::palette();
    
    QPixmap generated = QApplication::style()->generatedIconPixmap(mode, pm, &opt);
    if (!generated.isNull())
        pm = generated;

    if (!pm.isNull()) {
        QPixmapCache::insert(pmckey, pm);
    }
    
    return pm;
}

void MvdSvgzIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode mode,
    QIcon::State state)
{
    if (!d->addedPixmaps)
        d->addedPixmaps = new QHash<int, QPixmap>;
    d->stepSerialNum();
    d->addedPixmaps->insert(d->hashKey(mode, state), pixmap);
}

void MvdSvgzIconEngine::addFile(const QString &fileName, const QSize &,
    QIcon::Mode mode, QIcon::State state)
{
    if (!fileName.isEmpty()) {
        QString abs = fileName;
        if (fileName.at(0) != QLatin1Char(':'))
            abs = QFileInfo(fileName).absoluteFilePath();
        if (abs.endsWith(QLatin1String(".svg"), Qt::CaseInsensitive)
#ifndef QT_NO_COMPRESS
            || abs.endsWith(QLatin1String(".svgz"), Qt::CaseInsensitive)
            || abs.endsWith(QLatin1String(".svg.gz"), Qt::CaseInsensitive))
#endif
        {
            QSvgRenderer renderer(abs);
            if (renderer.isValid()) {
                if (!d->svgFiles)
                    d->svgFiles = new QHash<int, QString>;
                d->stepSerialNum();
                d->svgFiles->insert(d->hashKey(mode, state), abs);
            }
        } else {
            QPixmap pm(abs);
            if (!pm.isNull())
                addPixmap(pm, mode, state);
        }
    }
}
void MvdSvgzIconEngine::paint(QPainter *painter, const QRect &rect,
    QIcon::Mode mode, QIcon::State state)
{
    painter->drawPixmap(rect, pixmap(rect.size(), mode, state));
}

QString MvdSvgzIconEngine::key() const
{
    return QLatin1String("svgz");
}

QIconEngineV2 *MvdSvgzIconEngine::clone() const
{
    return new MvdSvgzIconEngine(*this);
}

bool MvdSvgzIconEngine::read(QDataStream &in)
{
    // QSharedDataPointer will destroy the old pointer for us
    d = new Private;
    d->svgBuffers = new QHash<int, QByteArray>;

    if (in.version() >= QDataStream::Qt_4_4) {
        int isCompressed;
        QHash<int, QString> fileNames;  // For memory optimization later
        in >> fileNames >> isCompressed >> *d->svgBuffers;
#ifndef QT_NO_COMPRESS
        if (!isCompressed) {
            foreach(int key, d->svgBuffers->keys())
                d->svgBuffers->insert(key, qCompress(d->svgBuffers->value(key)));
        }
#else
        if (isCompressed) {
            qWarning("MvdSvgzIconEngine: Can not decompress SVG data");
            d->svgBuffers->clear();
        }
#endif
        int hasAddedPixmaps;
        in >> hasAddedPixmaps;
        if (hasAddedPixmaps) {
            d->addedPixmaps = new QHash<int, QPixmap>;
            in >> *d->addedPixmaps;
        }
    }
    else {
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
                d->svgBuffers->insert(d->hashKey(QIcon::Normal, QIcon::Off), data);
        }
        in >> num_entries;
        for (int i=0; i<num_entries; ++i) {
            if (in.atEnd())
                return false;
            in >> pixmap;
            in >> mode;
            in >> state;
            // The pm list written by 4.3 is buggy and/or useless, so ignore.
            //addPixmap(pixmap, QIcon::Mode(mode), QIcon::State(state));
        }
    }
    return true;
}

bool MvdSvgzIconEngine::write(QDataStream &out) const
{
    if (out.version() >= QDataStream::Qt_4_4) {
        int isCompressed = 0;
#ifndef QT_NO_COMPRESS
        isCompressed = 1;
#endif
        QHash<int, QByteArray> svgBuffers;
        if (d->svgBuffers)
            svgBuffers = *d->svgBuffers;
        if (d->svgFiles) {
            foreach(int key, d->svgFiles->keys()) {
                QByteArray buf;
                QFile f(d->svgFiles->value(key));
                if (f.open(QIODevice::ReadOnly))
                    buf = f.readAll();
#ifndef QT_NO_COMPRESS
                buf = qCompress(buf);
#endif
                svgBuffers.insert(key, buf);
            }
            out << *(d->svgFiles) << isCompressed << svgBuffers;
        }
        if (d->addedPixmaps)
            out << (int)1 << *d->addedPixmaps;
        else
            out << (int)0;
    }
    else {
        QByteArray buf;
        if (d->svgBuffers)
            buf = d->svgBuffers->value(d->hashKey(QIcon::Normal, QIcon::Off));
        if (buf.isEmpty()) {
            QString svgFile = d->svgFiles ? d->svgFiles->value(d->hashKey(QIcon::Normal, QIcon::Off)) : QString();
            if (!svgFile.isEmpty()) {
                QFile f(svgFile);
                if (f.open(QIODevice::ReadOnly))
                    buf = f.readAll();
            }
        }
#ifndef QT_NO_COMPRESS
        buf = qCompress(buf);
#endif
        out << buf;
        // 4.3 has buggy handling of added pixmaps, so don't write any
        out << (int)0;
    }
    return true;
}

#endif // MVD_SVGZ_CUSTOM_ENGINE
