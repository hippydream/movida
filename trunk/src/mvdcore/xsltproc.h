/**************************************************************************
** Filename: xsltproc.h
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

#ifndef MVD_XSLTPROC_H
#define MVD_XSLTPROC_H

#include "global.h"

#include <QtCore/QHash>

class QIODevice;

class MVD_EXPORT MvdXsltProc
{
public:
    typedef QHash<QString, QString> ParameterList;

    MvdXsltProc();
    MvdXsltProc(const QString &xslpath);

    bool isOk() const;

    bool loadXslFile(const QString &xslpath);

    QString processText(const QString &txt,
    const ParameterList &params = ParameterList());
    QString processFile(const QString &file,
    const ParameterList &params = ParameterList());

    bool processTextToDevice(const QString &txt, QIODevice *dev,
    const ParameterList &params = ParameterList());
    bool processFileToDevice(const QString &file, QIODevice *dev,
    const ParameterList &params = ParameterList());

private:
    class Private;
    Private *d;
};

#endif // MVD_XSLTPROC_H
