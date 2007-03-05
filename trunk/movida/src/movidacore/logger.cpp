/**************************************************************************
** Filename: logger.cpp
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

#include "logger.h"
#include "pathresolver.h"

#include <QFile>
#include <QDateTime>
#include <QtGlobal>

using namespace Movida;

/*!
	\class MvdLogger logger.h
	\ingroup movidacore

	\brief Application log handling.
*/


/************************************************************************
MvdLogger_P
*************************************************************************/

//! \internal
class MvdLogger_P
{
public:
	MvdLogger_P();
	~MvdLogger_P();

	QTextStream* stream;
	QFile* file;
};

//! \internal
MvdLogger_P::MvdLogger_P()
: stream(0), file(0)
{
}

//! \internal
MvdLogger_P::~MvdLogger_P()
{
	if (stream)
		stream->flush();
	delete stream;
	delete file;
}


/************************************************************************
MvdLogger
*************************************************************************/

//! \internal Private constructor.
MvdLogger::MvdLogger()
: d(new MvdLogger_P)
{
	d->file = new QFile( paths().logFile() );
	if (!d->file->open(QIODevice::ReadWrite | QIODevice::Truncate))
	{
		delete d->file;
		d->file = 0;
		d->stream = new QTextStream(stderr);
	}
	else
	{
		d->stream = new QTextStream(d->file);

		QDateTime dt = QDateTime::currentDateTime();
		QString header = tr("Movida log: application started at %1")
			.arg(dt.toString(Qt::ISODate)).append("\r\n");
		*(d->stream) << header;
		header = QString().fill('-', header.length());
		*(d->stream) << header << "\r\n";
	}
}

//! \internal
MvdLogger* MvdLogger::mInstance = 0;

/*!
	Returns the application unique logger.
*/
MvdLogger& MvdLogger::instance()
{
	if (mInstance == 0)
		mInstance = new MvdLogger();
	return *mInstance;
}

//!
MvdLogger::~MvdLogger()
{
	delete d;
}

//! Writes a single char to the log file.
MvdLogger& MvdLogger::operator<< (QChar t)
{
	*(d->stream) << "\'" << t << "\'";
	d->stream->flush();
	return *this;
}

//! Writes the string representation of a bool to the log file.
MvdLogger& MvdLogger::operator<< (bool t)
{
	*(d->stream) << (t ? "true" : "false");
	d->stream->flush();
	return *this;
}

//! Writes a single char to the log file.
MvdLogger& MvdLogger::operator<< (char t)
{
	*(d->stream) << t;
	d->stream->flush();
	return *this;
}

//! Writes a single short to the log file.
MvdLogger& MvdLogger::operator<< (signed short t)
{
	*(d->stream) << t;
	d->stream->flush();
	return *this;
}

//! Writes a single short to the log file.
MvdLogger& MvdLogger::operator<< (unsigned short t)
{
	*(d->stream) << t;
	d->stream->flush();
	return *this;
}

//! Writes a single int to the log file.
MvdLogger& MvdLogger::operator<< (signed int t)
{
	*(d->stream) << t;
	d->stream->flush();
	return *this;
}

//! Writes a single int to the log file.
MvdLogger& MvdLogger::operator<< (unsigned int t)
{
	*(d->stream) << t;
	d->stream->flush();
	return *this;
}

//! Writes a single long to the log file.
MvdLogger& MvdLogger::operator<< (signed long t)
{
	*(d->stream) << t;
	d->stream->flush();
	return *this;
}

//! Writes a single long to the log file.
MvdLogger& MvdLogger::operator<< (unsigned long t)
{
	*(d->stream) << t;
	d->stream->flush();
	return *this;
}

//! Writes a single qint64 to the log file.
MvdLogger& MvdLogger::operator<< (qint64 t)
{
	*(d->stream) << QString::number(t);
	d->stream->flush();
	return *this;
}

//! Writes a single quint64 to the log file.
MvdLogger& MvdLogger::operator<< (quint64 t)
{
	*(d->stream) << QString::number(t);
	d->stream->flush();
	return *this;
}

//! Writes a single float to the log file.
MvdLogger& MvdLogger::operator<< (float t)
{
	*(d->stream) << t;
	d->stream->flush();
	return *this;
}

//! Writes a single double to the log file.
MvdLogger& MvdLogger::operator<< (double t)
{
	*(d->stream) << t;
	d->stream->flush();
	return *this;
}

//! Writes a string to the log file.
MvdLogger& MvdLogger::operator<< (const char* t)
{
	*(d->stream) << t;
	d->stream->flush();
	return *this;
}

//! Writes a string to the log file.
MvdLogger& MvdLogger::operator<< (const QString& t)
{
	*(d->stream) << "\"" << t  << "\"";
	d->stream->flush();
	return *this;
}

//! Writes a string to the log file.
MvdLogger& MvdLogger::operator<< (const QLatin1String& t)
{
	*(d->stream) << "\""  << t.latin1() << "\"";
	d->stream->flush();
	return *this;
}

//! Writes a byte array to the log file.
MvdLogger& MvdLogger::operator<< (const QByteArray& t)
{
	*(d->stream) << "\"" << t << "\"";
	d->stream->flush();
	return *this;
}

//! Writes a void pointer to the log file.
MvdLogger& MvdLogger::operator<< (const void* t)
{
	*(d->stream) << t;
	d->stream->flush();
	return *this;
}

//! Writes a QTextStreamFunction to the log file.
MvdLogger& MvdLogger::operator<< (QTextStreamFunction f)
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
MvdLogger& MvdLogger::appendTimestamp(const QString& message)
{
	QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
	*(d->stream) << MVD_LINEBREAK << (message.isEmpty() ? 
		QString("[%1] ").arg(timestamp) : 
		QString("[%1 - %2] ").arg(timestamp).arg(message));
	return *this;
}

/*!
	Convenience method to access the MvdLogger singleton.
	Prepends date and message type (error in this case) to the message.
*/
MvdLogger& Movida::eLog()
{
	return MvdLogger::instance().appendTimestamp("ERROR");
}

/*!
	Convenience method to access the MvdLogger singleton.
	Prepends date and message type (warning in this case) to the message.
*/
MvdLogger& Movida::wLog()
{
	return MvdLogger::instance().appendTimestamp("WARNING");
}

/*!
	Convenience method to access the MvdLogger singleton.
	Prepends date and message type (info in this case) to the message.
*/
MvdLogger& Movida::iLog()
{
	return MvdLogger::instance().appendTimestamp("INFO");
}
