/**************************************************************************
** Filename: base64.h
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

#ifndef MVD_BASE64_H
#define MVD_BASE64_H

#include "global.h"
#include <QFlags>

class QString;
class QBitArray;
class QByteArray;

class MOVIDA_EXPORT MvdBase64
{
public:
	enum EncodingOption { NoOptions = 0x00, BreakLongLines = 0x01 };
	Q_DECLARE_FLAGS(EncodingOptions, EncodingOption)

	static QByteArray decode(const QString& encoded);
	static QBitArray decode(const QString& encoded, quint32 size);
	static QString encode(const QByteArray& decoded, 
		EncodingOptions options = NoOptions);
	static QString encode(const QBitArray& decoded, 
		EncodingOptions options = NoOptions);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(MvdBase64::EncodingOptions)

#endif // MVD_BASE64_H
