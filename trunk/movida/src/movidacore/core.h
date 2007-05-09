/**************************************************************************
** Filename: core.h
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

#ifndef MVD_CORE_H
#define MVD_CORE_H

#include "global.h"
#include <QVariant>

#include <libxml/xmlerror.h>

class MvdCore_P;

class MVD_EXPORT MvdCore
{
public:
	static bool initCore();
	static void loadStatus();
	static void storeStatus();

	static QVariant parameter(const QString& name);
	static void registerParameters(const QHash<QString,QVariant>& p);

	static quint32 atoui32(const char* c);
	static QString replaceNewLine(QString text);
	static QByteArray toLatin1PercentEncoding(const QString& input, 
		const QByteArray& exclude = QByteArray(), 
		const QByteArray& include = QByteArray());
	static QString decodeXmlEntities(QString s);

private:
	static MvdCore_P* d;
	static bool MvdCoreInitialized;
};

namespace Movida
{
	void xmlStructuredErrorHandler(void* userData, xmlErrorPtr error);
}

#endif // MVD_CORE_H
