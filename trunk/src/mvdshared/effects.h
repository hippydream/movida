/**************************************************************************
** Filename: effects.h
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

#ifndef MVD_EFFECTS_H
#define MVD_EFFECTS_H

#include "sharedglobal.h"

class QWidget;

class MVD_EXPORT_SHARED MvdEffects
{
public:
    enum Direction {
        LeftScroll  = 0x0001,
        RightScroll = 0x0002,
        UpScroll    = 0x0004,
        DownScroll  = 0x0008
    };

    typedef uint DirFlags;
};

namespace Movida {
const int DefaultWidgetEffectDuration = 150;
extern void MVD_EXPORT_SHARED scrollEffect(QWidget*, MvdEffects::DirFlags dir = MvdEffects::DownScroll, int time = -1);
extern void MVD_EXPORT_SHARED fadeEffect(QWidget*, int time = -1, bool fadeOut = false);
}

#endif // MVD_EFFECTS_H
