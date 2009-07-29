/**************************************************************************
** Filename: utils.h
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

#ifndef MVD_UTILS_H
#define MVD_UTILS_H

#include "global.h"
#include <QString>
#include <QVariant>

namespace Movida
{
    MVD_EXPORT QStringList splitArgs(const QString& s, int idx);
    MVD_EXPORT QVariant stringToVariant(const QString& s);
    MVD_EXPORT QString variantToString(const QVariant& v);
}

#endif // MVD_UTILS_H
