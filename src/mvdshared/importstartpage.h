/**************************************************************************
** Filename: importstartpage.h
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

#ifndef MVD_IMPORTSTARTPAGE_H
#define MVD_IMPORTSTARTPAGE_H

#include "importexportpage.h"
#include "searchengine.h"
#include "sharedglobal.h"

class MvdActionLabel;
class MvdClearEdit;

class QComboBox;
class QLabel;
class QPushButton;

class MvdImportStartPage : public MvdImportExportPage
{
    Q_OBJECT

public:
    MvdImportStartPage(QWidget *parent = 0);

    virtual void initializePage();
    void reset();

    int registerEngine(const MvdSearchEngine &engine);

    int engine() const;
    const MvdSearchEngine *engineDescriptor(int id) const;
    QString query() const;

    void updateCompleter(const QStringList &history);

signals:
    void engineConfigurationRequest(int engine);

private slots:
    void engineChanged();
    void controlTriggered(int);

private:
    QList<MvdSearchEngine> engines;

    QLabel *infoLabel;
    QComboBox *engineCombo;
    MvdClearEdit *queryInput;
    MvdActionLabel *controls;
    int configureEngineId;
    int configurePluginId;
};

#endif // MVD_IMPORTSTARTPAGE_H
