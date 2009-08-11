/**************************************************************************
** Filename: exportstartpage.h
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

#ifndef MVD_EXPORTSTARTPAGE_H
#define MVD_EXPORTSTARTPAGE_H

#include "exportengine.h"
#include "importexportpage.h"
#include "sharedglobal.h"

class MvdActionLabel;
class MvdClearEdit;

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QRadioButton;
class QToolButton;

class MvdExportStartPage : public MvdImportExportPage
{
    Q_OBJECT

public:
    MvdExportStartPage(QWidget *parent = 0);
    virtual ~MvdExportStartPage();

    virtual void initializePage();
    virtual void cleanupPage();
    void reset();

    int registerEngine(const MvdExportEngine &engine);
    int currentEngineId() const;
    MvdExportEngine currentEngine() const;
    MvdExportEngine::EngineOptions currentEngineOptions() const;

    MvdExportDialog::ExportType exportType() const;
    QUrl exportUrl() const;

    bool configStepRequired() const;

    virtual bool isComplete() const;

signals:
    void engineConfigurationRequest(int engine);

private slots:
    void engineChanged();
    void controlTriggered(int);
    void browseForUrl();

private:
    QLabel *mInfoLabel;
    QComboBox *mEngineCombo;
    MvdClearEdit *mUrl;
    QToolButton *mUrlBrowse;
    QRadioButton *mExportSelectedButton;
    QRadioButton *mExportAllButton;
    QCheckBox *mCustomizeAttributes;
    MvdActionLabel *mControls;
    int mConfigureEngineId;
    int mConfigurePluginId;
    QList<MvdExportEngine> mEngines;
    QString mNextButtonText;
    QString mLastDir;
};

#endif // MVD_EXPORTSTARTPAGE_H
