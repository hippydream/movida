/**************************************************************************
** Filename: logger.h
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

#ifndef MVD_LOGGER_H
#define MVD_LOGGER_H

#include "global.h"
#include <QTextStream>
#include <QString>

class MvdLogger_P;

class MVD_EXPORT MvdLogger : public QObject
{
	Q_OBJECT

public:
	static MvdLogger& instance();
	virtual ~MvdLogger();

	static void setUseHtml(bool useHtml);
	static bool isUsingHtml();

	MvdLogger &operator<< (QChar t);
	MvdLogger &operator<< (bool t);
	MvdLogger &operator<< (char t);
	MvdLogger &operator<< (signed short t);
	MvdLogger &operator<< (unsigned short t);
	MvdLogger &operator<< (signed int t);
	MvdLogger &operator<< (unsigned int t);
	MvdLogger &operator<< (signed long t);
	MvdLogger &operator<< (unsigned long t);
	MvdLogger &operator<< (qint64 t);
	MvdLogger &operator<< (quint64 t);
	MvdLogger &operator<< (float t);
	MvdLogger &operator<< (double t);
	MvdLogger &operator<< (const char* t);
	MvdLogger &operator<< (const QString& t);
	MvdLogger &operator<< (const QLatin1String& t);
	MvdLogger &operator<< (const QByteArray& t);
	MvdLogger &operator<< (const void* t);
	MvdLogger &operator<< (QTextStreamFunction f);
	inline MvdLogger &appendTimestamp(const QString& message = QString());

private:
	MvdLogger();
	static MvdLogger* mInstance;
	MvdLogger_P* d;
};

namespace Movida
{
	MVD_EXPORT extern MvdLogger& iLog();
	MVD_EXPORT extern MvdLogger& wLog();
	MVD_EXPORT extern MvdLogger& eLog();
}

#endif // MVD_LOGGER_H
