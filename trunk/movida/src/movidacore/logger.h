/**************************************************************************
** Filename: logger.h
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

#ifndef MVD_LOGGER_H
#define MVD_LOGGER_H

#include "global.h"

#include <QTextStream>
#include <QString>

class MvdLogger_P;

class MOVIDA_EXPORT MvdLogger : public QObject
{
	Q_OBJECT

public:
	static MvdLogger& instance();
	virtual ~MvdLogger();

	inline MvdLogger &operator<< (QChar t);
	inline MvdLogger &operator<< (bool t);
	inline MvdLogger &operator<< (char t);
	inline MvdLogger &operator<< (signed short t);
	inline MvdLogger &operator<< (unsigned short t);
	inline MvdLogger &operator<< (signed int t);
	inline MvdLogger &operator<< (unsigned int t);
	inline MvdLogger &operator<< (signed long t);
	inline MvdLogger &operator<< (unsigned long t);
	inline MvdLogger &operator<< (qint64 t);
	inline MvdLogger &operator<< (quint64 t);
	inline MvdLogger &operator<< (float t);
	inline MvdLogger &operator<< (double t);
	// Don't inline this or GCC will send a warning
	MvdLogger &operator<< (const char* t);
	// Don't inline this or GCC will send a warning
	MvdLogger &operator<< (const QString& t);
	inline MvdLogger &operator<< (const QLatin1String& t);
	inline MvdLogger &operator<< (const QByteArray& t);
	inline MvdLogger &operator<< (const void* t);
	inline MvdLogger &operator<< (QTextStreamFunction f);
	inline MvdLogger &appendTimestamp(const QString& message = QString());

private:
	MvdLogger();
	static MvdLogger* mInstance;
	MvdLogger_P* d;
};

namespace Movida
{
	MOVIDA_EXPORT extern MvdLogger& iLog();
	MOVIDA_EXPORT extern MvdLogger& wLog();
	MOVIDA_EXPORT extern MvdLogger& eLog();
}

#endif // MVD_LOGGER_H
