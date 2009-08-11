/**************************************************************************
** Filename: importdialog.h
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

#ifndef MVD_IMPORTDIALOG_H
#define MVD_IMPORTDIALOG_H

#include "searchengine.h"
#include "sharedglobal.h"

#include "mvdcore/moviedata.h"

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtGui/QDialog>
#include <QtGui/QPushButton>
#include <QtGui/QWizard>

class MvdImportFinalPage;
class MvdImportResultsPage;
class MvdImportStartPage;

class MVD_EXPORT_SHARED MvdImportDialog : public QWizard
{
    Q_OBJECT

public:
    //! What happened?
    enum Result {
        /*! No error occurred. */
        Success = 0,
        /* Movie data import failed for some movie */
        MovieDataFailed,
        /* Movie poster import failed for some movie */
        MoviePosterFailed,
        /* No search could be performed or some other critical error occurred. */
        CriticalError
    };

    //! Why did it happen?
    enum ErrorType {
        UnknownError = 0,
        NetworkError,
        FileError,
        InvalidEngineError,
        EngineError
    };

    MvdImportDialog(QWidget *parent = 0);

    virtual int nextId() const;

    int registerEngine(const MvdSearchEngine &engine);

    void showMessage(const QString &msg, MovidaShared::MessageType type = MovidaShared::InfoMessage);

    void setImportSteps(quint8 s);
    void setNextImportStep();

    void setSearchSteps(quint8 s);
    void setNextSearchStep();

    int addMatch(const QString &title, const QString &year, const QString &notes = QString());
    void addMovieData(const MvdMovieData &md);

    void addSection(const QString &title, const QString &notes = QString());
    void addSubSection(const QString &title, const QString &notes = QString());

    void setErrorType(ErrorType type);
    ErrorType errorType() const;

    void done(Result res = Success);
    Result result() const;

    void accept();

    bool preventCloseWhenBusy() const;
    bool isBusy() const;

    bool confirmCloseWizard();

public slots:
    virtual void reject();

protected:
    void closeEvent(QCloseEvent *e);
    void keyPressEvent(QKeyEvent *e);

signals:
    void engineConfigurationRequest(int engine);
    void searchRequest(const QString &query, int engine);
    void importRequest(const QList<int> &matches);
    void resetRequest();

private slots:
    void pageChanged(int newPage);

private:
    class Private;
    Private *d;
};

#endif // MVD_IMPORTDIALOG_H
