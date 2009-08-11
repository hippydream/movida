/**************************************************************************
** Filename: exportfinalpage.h
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

#ifndef MVD_EXPORTFINAL_H
#define MVD_EXPORTFINAL_H

#include "ui_exportfinalpage.h"

#include "importexportpage.h"
#include "sharedglobal.h"

class QLabel;
class QRadioButton;

class MvdExportFinalPage : public MvdImportExportPage
{
    Q_OBJECT

public:
    MvdExportFinalPage(QWidget *parent = 0);

    void showMessage(const QString &msg, MovidaShared::MessageType t);

    void initializePage();
    void cleanupPage();
    void setBusyStatus(bool busy);
    void reset();

    virtual bool validatePage();
    virtual void updateButtons();

private slots:
    void restartWizardToggled();
    void initializePageInternal();

private:
    Ui::MvdExportFinalPage ui;

    bool mPendingButtonUpdates;
    QString mFinishButtonText;
};

#endif // MVD_EXPORTFINAL_H
