/**************************************************************************
** Filename: importfinalpage.cpp
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

#include "importfinalpage.h"

#include "actionlabel.h"
#include "importdialog.h"
#include "importdialog_p.h"

#include "movida/guiglobal.h"

#include "mvdcore/core.h"
#include "mvdcore/logger.h"
#include "mvdcore/plugininterface.h"
#include "mvdcore/settings.h"
#include "mvdcore/templatemanager.h"

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QRadioButton>

#ifndef MVD_WINDOW_UTILS
#define MVD_WINDOW_UTILS
# ifdef Q_OS_WIN
#  include <windows.h>
#  define MVD_ENABLE_CLOSE_BTN(Enable) \
    { QWidget *tlw = this; \
      while (tlw && !tlw->isWindow() && tlw->windowType() != Qt::SubWindow) \
          tlw = tlw->parentWidget();\
      HMENU hMenu = GetSystemMenu((HWND)tlw->winId(), FALSE); \
      EnableMenuItem(hMenu, SC_CLOSE, Enable ? (MF_BYCOMMAND | MF_ENABLED) : (MF_BYCOMMAND | MF_GRAYED)); }
# else
#  define MVD_ENABLE_CLOSE_BTN(Enable)
# endif // Q_OS_WIN

#endif // MVD_WINDOW_UTILS

using namespace Movida;

/*!
    \class MvdImportFinalPage importfinalpage.h
    \ingroup MovidaShared

    \brief Last page of the import wizard, showing the results of the import.
*/

MvdImportFinalPage::MvdImportFinalPage(QWidget *parent) :
    MvdImportExportPage(parent),
    mPendingButtonUpdates(false)
{
    setTitle(tr("We are all done!"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/import-wizard/watermark.png"));

    setPreventCloseWhenBusy(true);
    ui.setupUi(this);

    ui.filterMovies->setChecked(settings().value("movida/import-wizard/auto-create-filter", true).toBool());

    connect(ui.restartWizard, SIGNAL(toggled(bool)), this, SLOT(restartWizardToggled()));
}

//! Override. Locks or unlocks (part of) the GUI.
void MvdImportFinalPage::setBusyStatus(bool busy)
{
    if (busy == busyStatus())
        return;

    MvdImportExportPage::setBusyStatus(busy);

    MVD_ENABLE_CLOSE_BTN(!busy)

    mPendingButtonUpdates = true;

    if (!busy)
        updateButtons();

    ui.closeWizard->setEnabled(!busy);
    ui.restartWizard->setEnabled(!busy);

    if (busy) {
        ui.filterMovies->setEnabled(false);
    } else {
        int importedMovies =  field("importedMoviesCount").toInt();
        bool hasSomeImports = importedMovies > 0;
        ui.filterMovies->setEnabled(hasSomeImports && ui.closeWizard->isChecked());
    }
}

//! Override.
void MvdImportFinalPage::showMessage(const QString &msg, MovidaShared::MessageType t)
{
    Q_UNUSED(t);
    ui.messageLabel->setText(msg);
}

void MvdImportFinalPage::initializePage()
{
    showMessage(tr("Importing movies..."), MovidaShared::InfoMessage);
    setBusyStatus(true);
}

void MvdImportFinalPage::initializePageInternal()
{
    int totalMatches = field("resultsCount").toInt();
    int selectedMatches = field("selectedResultsCount").toInt();
    int importedMovies =  field("importedMoviesCount").toInt();

    QString msg;
    bool hasSomeImport = importedMovies > 0;

    if (importDialog()->result() == MvdImportDialog::CriticalError) {
        hasSomeImport = false;

        switch (importDialog()->errorType()) {
            case MvdImportDialog::NetworkError:
                msg = tr("Sorry but no movie can be imported because of a network error.\n\nPlease try again later or wait for your network to work again.");
                break;

            case MvdImportDialog::FileError:
                msg = tr("Sorry but no movie can be imported because of a system error.\n\nRebooting the computer could solve it. If it fails, please contact the developers using the 'Report error' tool in the 'Help' menu.");
                break;

            case MvdImportDialog::EngineError:
                msg = tr("Sorry but no movie can be imported because of a problem with the selected search engine.\n\nUpdating and reinstalling the plugin or the engine could solve the problem.");
                break;

            case MvdImportDialog::InvalidEngineError:
                msg = tr("Sorry but no movie can be imported because the selected search engine is not valid.\n\nUpdating and reinstalling the plugin or the engine could solve the problem.");
                break;

            default:
                msg = tr("Sorry but no movie can be imported because of an internal error.\n\nIf you think it might be an application error, please contact the developers using the 'Report error' tool in the 'Help' menu.");
        }
    } else {
        if (totalMatches == 0) {
            msg = tr("No movie has been found matching your search criteria.\n\nPlease try to check your spelling, use a different engine or use different key words.");
        } else if (selectedMatches == 0 || importedMovies == 0) {
            msg = tr("No movie has been selected for import.\n\nYou can use the wizard for a different search or just continue having fun with movida.");
        } else {
            msg = tr("%1 movie(s) have been imported with success.", "Number of actually imported movies", importedMovies).arg(importedMovies);
            if (importDialog()->result() == MvdImportDialog::MovieDataFailed) {
                msg.append(tr("\n\nSome movies could not be imported for some movie and you will have to repeat the process, possibly using a different search engine, or add them manually."));
            } else if (importDialog()->result() == MvdImportDialog::MoviePosterFailed) {
                msg.append(tr("\n\nThe movie poster could not be imported for some movie and you will have to import it manually."));
            }
        }
    }

    showMessage(msg, MovidaShared::InfoMessage);
    ui.filterMovies->setEnabled(hasSomeImport);
}

void MvdImportFinalPage::cleanupPage()
{
    //setBusyStatus(false);
}

//! Toggles the finish/new_search button.
void MvdImportFinalPage::restartWizardToggled()
{
    if (mFinishButtonText.isEmpty())
        mFinishButtonText = wizard()->buttonText(QWizard::FinishButton);

    wizard()->setButtonText(QWizard::FinishButton, ui.restartWizard->isChecked() ? tr("&New Search") : mFinishButtonText);

    int importedMovies =  field("importedMoviesCount").toInt();
    bool hasSomeImports = importedMovies > 0;

    ui.filterMovies->setEnabled(hasSomeImports && ui.closeWizard->isChecked());
}

/*! Returns false and calls QWizard::restart() if a new search is to be performed. Returns true, causing the wizard to
    close (as this is supposed to be the last page) otherwise.
*/
bool MvdImportFinalPage::validatePage()
{
    if (ui.restartWizard->isChecked()) {
        Q_ASSERT(QMetaObject::invokeMethod(wizard(), "restart", Qt::QueuedConnection));
        return false;
    }

    settings().setValue("movida/import-wizard/auto-create-filter", ui.filterMovies->isChecked());

    return true;
}

//! Does the actual movie import.
void MvdImportFinalPage::importMovies(const MvdMovieDataList &movies)
{
    setBusyStatus(true);

    for (int i = 0; i < movies.size(); ++i) {
        const MvdMovieData &m = movies.at(i);
        QString s = m.title.isEmpty() ? m.originalTitle : m.title;
        if (movies.size() == 1) {
            showMessage(tr("Importing '%1'...").arg(s), MovidaShared::InfoMessage);
        } else {
            if (i == movies.size() - 1)
                showMessage(tr("Importing the last movie: '%1'...").arg(s), MovidaShared::InfoMessage);
            else showMessage(tr("Importing '%1'. %2 movie(s) remaining...", "# of movies not imported yet", movies.size() - 1 - i).arg(s).arg(movies.size() - 1 - i), MovidaShared::InfoMessage);
        }

        MvdMovieCollection *collection = MvdCore::pluginContext()->collection;
        Q_ASSERT(collection);

        const QString _importDate = MvdCore::parameter("mvdcore/extra-attributes/import-date").toString();

        QHash<QString, QVariant> extra;
        extra.insert(_importDate, QDateTime::currentDateTime());
        mvdid movieId = collection->addMovie(m, extra);
        mImportedMovies << movieId;

        QCoreApplication::processEvents();
    }

    setBusyStatus(false);
    initializePageInternal();
}

//! \todo Remove this code if next Qt versions will allow to lock the wizard.
//! Forces the buttons to lock or unlock.
void MvdImportFinalPage::updateButtons()
{
    if (mPendingButtonUpdates) {
        mPendingButtonUpdates = false;

        bool locked = busyStatus();

        if (QAbstractButton * b = wizard()->button(QWizard::CancelButton))
            b->setEnabled(!locked);
        if (QAbstractButton * b = wizard()->button(QWizard::FinishButton))
            b->setEnabled(!locked);
    }
}

//!
void MvdImportFinalPage::reset()
{
    setBusyStatus(false);
}
