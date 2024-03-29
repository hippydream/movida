/**************************************************************************
** Filename: exportconfigpage.h
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

#ifndef MVD_EXPORTCONFIGPAGE_H
#define MVD_EXPORTCONFIGPAGE_H

#include "ui_exportconfigpage.h"

#include "importexportpage.h"
#include "sharedglobal.h"

class MvdExportConfigPage : public MvdImportExportPage
{
    Q_OBJECT

public:
    MvdExportConfigPage(QWidget *parent = 0);

    void initializePage();
    void cleanupPage();

private:
    Ui::MvdExportConfigPage ui;
};

#endif // MVD_EXPORTCONFIGPAGE_H
