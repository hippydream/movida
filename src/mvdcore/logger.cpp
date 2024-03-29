/**************************************************************************
** Filename: logger.cpp
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

#include "logger.h"

#include "pathresolver.h"

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>
#include <QtCore/QtGlobal>

#include <stdexcept>

using namespace Movida;

Q_GLOBAL_STATIC(QMutex, MvdLoggerLock)

/*!
    \class MvdLogger logger.h
    \ingroup MvdCore Singletons

    <b>Movida::iLog()</b>, <b>Movida::eLog()</b> and <b>Movida::wLog()</b> can be used as a
    convenience methods to write information, warning or error messages.

    \brief Application log handling.
*/


/************************************************************************
    MvdLogger::Private
 *************************************************************************/

//! \internal
class MvdLogger::Private
{
public:
    Private();
    ~Private();

    QTextStream *stream;
    QFile *file;
    static bool html;
};

bool MvdLogger::Private::html = false;

//! \internal
MvdLogger::Private::Private() :
    stream(0),
    file(0)
{ }

//! \internal
MvdLogger::Private::~Private()
{
    if (stream) {
        if (html)
            *stream << MVD_LINEBREAK << "</body>" << MVD_LINEBREAK << "</html>";
        stream->flush();
    }
    delete stream;
    delete file;
}

/************************************************************************
    MvdLogger
 *************************************************************************/

//! \internal Private constructor.
MvdLogger::MvdLogger() :
    QObject(),
    d(new Private)
{
    QString logFilePath = paths().logFile();
    QFileInfo logFileInfo(logFilePath);
    if (logFileInfo.exists()) {
        QString logFilePathOld = logFilePath + QLatin1String(".old");
        QFileInfo logFileInfoOld(logFilePathOld);
        if (logFileInfoOld.exists())
            QFile::remove(logFilePathOld);
        QFile::rename(logFilePath, logFilePathOld);
    }

    d->file = new QFile(logFilePath);
    if (!d->file->open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        delete d->file;
        d->file = 0;
        d->stream = new QTextStream(stderr);
    } else {
        d->stream = new QTextStream(d->file);

        QString sep(MVD_LINEBREAK);
        if (MvdLogger::isUsingHtml()) {
            *(d->stream) << "<html>" << MVD_LINEBREAK << "<body>" << MVD_LINEBREAK;
            sep.prepend("<br />");
        }

        QDateTime dt = QDateTime::currentDateTime();
        QString header = QString(QLatin1String("Movida log: application started at %1"))
            .arg(dt.toString(Qt::ISODate)).append(sep);
        *(d->stream) << header << sep;
    }
}

//! \internal
volatile MvdLogger *MvdLogger::mInstance = 0;
bool MvdLogger::mDestroyed = false;

/*!
    Returns the application unique logger.
*/
MvdLogger &MvdLogger::instance()
{
    if (!mInstance) {
        QMutexLocker locker(MvdLoggerLock());
        if (!mInstance) {
            if (mDestroyed) throw std::runtime_error("Logger: access to dead reference");
            create();
        }
    }

    return (MvdLogger &) * mInstance;
}

//! Destructor.
MvdLogger::~MvdLogger()
{
    delete d;
    mInstance = 0;
    mDestroyed = true;
}

void MvdLogger::create()
{
    // Local static members are instantiated as soon
    // as this function is entered for the first time
    // (Scott Meyers singleton)
    static MvdLogger instance;

    mInstance = &instance;
}

//! Sets whether the log should be written in HTML or plain text.
void MvdLogger::setUseHtml(bool useHtml)
{
    MvdLogger::Private::html = useHtml;
}

//! Returns true if HTML is used when writing the log.
bool MvdLogger::isUsingHtml()
{
    return MvdLogger::Private::html;
}

//! Writes a single char to the log file.
MvdLogger &MvdLogger::operator<<(QChar t)
{
    *(d->stream) << "\'" << t << "\'";
    d->stream->flush();
    return *this;
}

//! Writes the string representation of a bool to the log file.
MvdLogger &MvdLogger::operator<<(bool t)
{
    *(d->stream) << (t ? "true" : "false");
    d->stream->flush();
    return *this;
}

//! Writes a single char to the log file.
MvdLogger &MvdLogger::operator<<(char t)
{
    *(d->stream) << t;
    d->stream->flush();
    return *this;
}

//! Writes a single short to the log file.
MvdLogger &MvdLogger::operator<<(signed short t)
{
    *(d->stream) << t;
    d->stream->flush();
    return *this;
}

//! Writes a single short to the log file.
MvdLogger &MvdLogger::operator<<(unsigned short t)
{
    *(d->stream) << t;
    d->stream->flush();
    return *this;
}

//! Writes a single int to the log file.
MvdLogger &MvdLogger::operator<<(signed int t)
{
    *(d->stream) << t;
    d->stream->flush();
    return *this;
}

//! Writes a single int to the log file.
MvdLogger &MvdLogger::operator<<(unsigned int t)
{
    *(d->stream) << t;
    d->stream->flush();
    return *this;
}

//! Writes a single long to the log file.
MvdLogger &MvdLogger::operator<<(signed long t)
{
    *(d->stream) << t;
    d->stream->flush();
    return *this;
}

//! Writes a single long to the log file.
MvdLogger &MvdLogger::operator<<(unsigned long t)
{
    *(d->stream) << t;
    d->stream->flush();
    return *this;
}

//! Writes a single qint64 to the log file.
MvdLogger &MvdLogger::operator<<(qint64 t)
{
    *(d->stream) << QString::number(t);
    d->stream->flush();
    return *this;
}

//! Writes a single quint64 to the log file.
MvdLogger &MvdLogger::operator<<(quint64 t)
{
    *(d->stream) << QString::number(t);
    d->stream->flush();
    return *this;
}

//! Writes a single float to the log file.
MvdLogger &MvdLogger::operator<<(float t)
{
    *(d->stream) << t;
    d->stream->flush();
    return *this;
}

//! Writes a single double to the log file.
MvdLogger &MvdLogger::operator<<(double t)
{
    *(d->stream) << t;
    d->stream->flush();
    return *this;
}

//! Writes a string to the log file.
MvdLogger &MvdLogger::operator<<(const char *t)
{
    if (MvdLogger::isUsingHtml())
        *(d->stream) << QString(t).replace(MVD_LINEBREAK, QString("<br />").append(MVD_LINEBREAK));
    else *(d->stream) << t;
    d->stream->flush();
    return *this;
}

//! Writes a string to the log file.
MvdLogger &MvdLogger::operator<<(const QString &t)
{
    if (MvdLogger::isUsingHtml())
        *(d->stream) << QString(t).replace(MVD_LINEBREAK, QString("<br />").append(MVD_LINEBREAK));
    else *(d->stream) << t;
    d->stream->flush();
    return *this;
}

//! Writes a string to the log file.
MvdLogger &MvdLogger::operator<<(const QLatin1String &t)
{
    if (MvdLogger::isUsingHtml())
        *(d->stream) << QString(t).replace(MVD_LINEBREAK, QString("<br />").append(MVD_LINEBREAK));
    else *(d->stream) << t.latin1();
    d->stream->flush();
    return *this;
}

//! Writes a byte array to the log file.
MvdLogger &MvdLogger::operator<<(const QByteArray &t)
{
    *(d->stream) << t;
    d->stream->flush();
    return *this;
}

//! Writes a void pointer to the log file.
MvdLogger &MvdLogger::operator<<(const void *t)
{
    *(d->stream) << t;
    d->stream->flush();
    return *this;
}

//! Writes a QTextStreamFunction to the log file.
MvdLogger &MvdLogger::operator<<(QTextStreamFunction f)
{
    *(d->stream) << f;
    d->stream->flush();
    return *this;
}

/*!
    This is mainly for internal use.
    Writes the current timestamp in ISO format between square brackets,
    evtl. followed by " - " and \p message (if \p message is not empty).
    An additional whitespace is added after the closing bracket.
    Example: "[2007-01-02T18:11:00 - MYMESSAGE] "
*/
MvdLogger &MvdLogger::appendTimestamp(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);

    *(d->stream) << (MvdLogger::isUsingHtml() ? QString("<br />").append(MVD_LINEBREAK) : MVD_LINEBREAK) << (message.isEmpty() ?
                                                                                                             QString("[%1] ").arg(timestamp) :
                                                                                                             QString("[%1 - %2] ").arg(timestamp).arg(message));
    return *this;
}

/*!
    Convenience method to access the MvdLogger singleton.
    Prepends date and message type (error in this case) to the message.
*/
MvdLogger &Movida::eLog()
{
    QString s = MvdLogger::instance().isUsingHtml() ? "<span style='color:red;'>ERROR</span>" : "ERROR";

    return MvdLogger::instance().appendTimestamp(s);
}

/*!
    Convenience method to access the MvdLogger singleton.
    Prepends date and message type (warning in this case) to the message.
*/
MvdLogger &Movida::wLog()
{
    QString s = MvdLogger::instance().isUsingHtml() ? "<span style='color:orange;'>WARNING</span>" : "WARNING";

    return MvdLogger::instance().appendTimestamp(s);
}

/*!
    Convenience method to access the MvdLogger singleton.
    Prepends date and message type (info in this case) to the message.
*/
MvdLogger &Movida::iLog()
{
    return MvdLogger::instance().appendTimestamp("INFO");
}
