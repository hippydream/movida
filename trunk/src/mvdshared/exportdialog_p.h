/**************************************************************************
** Filename: exportdialog_p.h
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the MvdShared API.  It exists for the convenience
// of Movida.  This header file may change from version to version without notice, 
// or even be removed.
//
// We mean it.
//

#ifndef MVD_EXPORTDIALOG_P_H
#define MVD_EXPORTDIALOG_P_H

#include "exportdialog.h"
#include "exportstartpage.h"
#include <QStringList>

//! \internal
class MvdExportDialog::Private
{
public:
	enum { StartPage, FinalPage };

	MvdExportStartPage* startPage;

	bool closing;
	MvdExportDialog::Result exportResult;
	MvdExportDialog::ErrorType errorType;
};

#endif // MVD_EXPORTDIALOG_P_H
