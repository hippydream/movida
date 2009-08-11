/**************************************************************************
** Filename: importfinalpage.h
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

#ifndef MVD_IMPORTFINAL_H
#define MVD_IMPORTFINAL_H

#include "ui_importfinalpage.h"

#include "importexportpage.h"
#include "sharedglobal.h"

#include "mvdcore/moviedata.h"

class QLabel;
class QRadioButton;

class MvdImportFinalPage : public MvdImportExportPage
{
    Q_OBJECT

public:
    MvdImportFinalPage(QWidget *parent = 0);

    void showMessage(const QString &msg, MovidaShared::MessageType t);

    void initializePage();
    void cleanupPage();
    void setBusyStatus(bool busy);
    void reset();

    virtual bool validatePage();
    virtual void updateButtons();

    QList<mvdid> importedMovies() const { return mImportedMovies; }

    bool filterImportedMovies() const { return ui.filterMovies->isChecked(); }

public slots:
    virtual void importMovies(const MvdMovieDataList &movies);

private slots:
    void restartWizardToggled();
    void initializePageInternal();

private:
    Ui::MvdImportFinalPage ui;

    bool mPendingButtonUpdates;
    QString mFinishButtonText;
    QList<mvdid> mImportedMovies;
};

#endif // MVD_IMPORTFINAL_H
