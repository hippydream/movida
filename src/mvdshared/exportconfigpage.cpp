/**************************************************************************
** Filename: exportconfigpage.cpp
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

#include "exportconfigpage.h"
#include "importresultspage.h"
#include "mvdcore/core.h"
#include <QHeaderView>

/*!
        \class MvdExportConfigPage exportconfigpage.h
        \ingroup MovidaShared

        \brief This page allows the user to customize the export process if
        the selected engine allows it.
*/


/*!
        This Export Wizard page allows the user to customize the export process if
        the selected engine allows it.
*/
MvdExportConfigPage::MvdExportConfigPage(QWidget* parent)
: MvdImportExportPage(parent)
{
    setTitle(tr("Export configuration"));
    setSubTitle(tr("Please select the attributes you want to be exported and eventually their order."));

    ui.setupUi(this);

    ui.results->setHeaderLabels(QStringList() << tr("Attribute"));
    ui.results->header()->setResizeMode(0, QHeaderView::Stretch);
    ui.results->header()->setStretchLastSection(false);

    //connect( ui.results, SIGNAL(itemSelectionChanged()),
    //	this, SLOT(resultsSelectionChanged()) );
    //connect( ui.results, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
    //	this, SLOT(resultsCheckStateChanged()) );
}

//! Initialize page each time it is shown.
void MvdExportConfigPage::initializePage()
{
        wizard()->setButtonText(QWizard::NextButton, tr("&Export"));
}

//! This method is called when the user hits the "back" button.
void MvdExportConfigPage::cleanupPage()
{

}
