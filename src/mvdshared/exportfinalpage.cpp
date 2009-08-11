/**************************************************************************
** Filename: exportfinalpage.cpp
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

#include "exportfinalpage.h"

#include "actionlabel.h"
#include "exportdialog.h"
#include "exportdialog_p.h"

#include "movida/guiglobal.h"

#include "mvdcore/core.h"
#include "mvdcore/logger.h"
#include "mvdcore/plugininterface.h"
#include "mvdcore/settings.h"

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
    \class MvdExportFinalPage exportfinalpage.h
    \ingroup MovidaShared

    \brief Last page of the export wizard, showing the results of the export.
*/

MvdExportFinalPage::MvdExportFinalPage(QWidget *parent) :
    MvdImportExportPage(parent),
    mPendingButtonUpdates(false)
{
    setTitle(tr("We are all done!"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/import-wizard/watermark.png"));

    setPreventCloseWhenBusy(true);
    ui.setupUi(this);

    connect(ui.restartWizard, SIGNAL(toggled(bool)), this, SLOT(restartWizardToggled()));
}

//! Override. Locks or unlocks (part of) the GUI.
void MvdExportFinalPage::setBusyStatus(bool busy)
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
        showMessage(tr("Export in progress..."), MovidaShared::InfoMessage);
    } else {
        if (exportDialog()->result() != MvdExportDialog::Success) {
            showMessage(tr("Export failed."), MovidaShared::ErrorMessage);
        } else {
            showMessage(tr("Export finished."), MovidaShared::InfoMessage);
        }
    }
}

//! Override.
void MvdExportFinalPage::showMessage(const QString &msg, MovidaShared::MessageType t)
{
    Q_UNUSED(t);
    ui.messageLabel->setText(msg);
}

void MvdExportFinalPage::initializePage()
{
    showMessage(tr("TODO"), MovidaShared::InfoMessage);
    // setBusyStatus(true);
}

void MvdExportFinalPage::initializePageInternal()
{
    /*QString msg;
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
       msg = tr("Sorry but no movie can be imported because of a problem with the selected search engine.\n\nReinstalling the plugin or the engine could solve the problem.");
       break;
       case MvdImportDialog::InvalidEngineError:
       msg = tr("Sorry but no movie can be imported because the selected search engine is not valid.\n\nReinstalling the plugin or the engine could solve the problem.");
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

       showMessage(msg, MvdShared::InfoMessage);
       ui.filterMovies->setEnabled(hasSomeImport);*/
}

void MvdExportFinalPage::cleanupPage()
{
    //setBusyStatus(false);
}

//! Toggles the finish/new_search button.
void MvdExportFinalPage::restartWizardToggled()
{
    if (mFinishButtonText.isEmpty())
        mFinishButtonText = wizard()->buttonText(QWizard::FinishButton);

    wizard()->setButtonText(QWizard::FinishButton, ui.restartWizard->isChecked() ? tr("&New Export") : mFinishButtonText);
}

/*! Returns false and calls QWizard::restart() if a new search is to be performed. Returns true, causing the wizard to
    close (as this is supposed to be the last page) otherwise.
*/
bool MvdExportFinalPage::validatePage()
{
    if (ui.restartWizard->isChecked()) {
        Q_ASSERT(QMetaObject::invokeMethod(wizard(), "restart", Qt::QueuedConnection));
        return false;
    }

    return true;
}

//! \todo Remove this code if next Qt versions will allow to lock the wizard.
//! Forces the buttons to lock or unlock.
void MvdExportFinalPage::updateButtons()
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
void MvdExportFinalPage::reset()
{
    setBusyStatus(false);
}
