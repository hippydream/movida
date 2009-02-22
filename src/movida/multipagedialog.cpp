/**************************************************************************
** Filename: multipagedialog.cpp
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

#include "multipagedialog.h"
#include "mpdialogpage.h"
#include "guiglobal.h"
#include "mvdcore/core.h"
#include <QVariant>
#include <QPalette>

/*!
        \class MvdMultiPageDialog complexdlg.h
        \ingroup Movida

        \brief Base class for a multi page dialog.
*/


/*!
        Creates a new empty dialog.
*/
MvdMultiPageDialog::MvdMultiPageDialog(QWidget* parent)
: QDialog(parent), mDiscardChanges(false)
{
        setupUi(this);
        MVD_WINDOW_ICON

        setAdvancedControlsVisible(false);
        connect(contents, SIGNAL(currentChanged(int)), this, SLOT(emit_currentPageChanged()));
}

/*!
        Adds a new page to the dialog and returns an internally assigned id (or a
        negative value if the page could not be added).
*/
int MvdMultiPageDialog::addPage(MvdMPDialogPage* p)
{
        if (p == 0)
                return -1;

        p->setContentsMargins(10, 10, 10, 10);
        int id = contents->addTab(p, p->label());

        connect( p, SIGNAL(externalActionTriggered(const QString&, const QVariant&)),
                this, SIGNAL(externalActionTriggered(const QString&, const QVariant&)) );

        connect( p, SIGNAL(validationStateChanged()),
                this, SLOT(do_validationStateChanged()) );

        connect( p, SIGNAL(modifiedStateChanged()),
                this, SLOT(do_modifiedStateChanged()) );

        if (!p->isValid())
                do_validationStateChanged(p);
        if (p->isModified())
                do_modifiedStateChanged(p);

        return id;
}

//! Shows a specific page.
void MvdMultiPageDialog::showPage(MvdMPDialogPage* p)
{
        contents->setCurrentWidget(p);
}

//! Returns a pointer to the button container.
QDialogButtonBox* MvdMultiPageDialog::buttonBox() const
{
        return Ui::MvdMultiPageDialog::buttonBox;
}

//! Returns a pointer to the advanced control's button container.
QDialogButtonBox* MvdMultiPageDialog::advancedButtonBox() const
{
        return Ui::MvdMultiPageDialog::advButtonBox;
}

//! \internal
void MvdMultiPageDialog::do_validationStateChanged(MvdMPDialogPage* p)
{
        MvdMPDialogPage* page = p ? p : qobject_cast<MvdMPDialogPage*>(sender());
        if (page)
        {
                if (page->isValid())
                        contents->setTabText(contents->indexOf(page), page->label());
                else contents->setTabText(contents->indexOf(page), page->label().append(" (!)"));

                validationStateChanged(page);
        }
}

/*!
        This slot is called whenever a dialog page changes its validation state.
        Subclasses may override it to enable or disable UI elements.
*/
void MvdMultiPageDialog::validationStateChanged(MvdMPDialogPage* page)
{
        Q_UNUSED(page);
}

//! \internal
void MvdMultiPageDialog::do_modifiedStateChanged(MvdMPDialogPage* p)
{
        MvdMPDialogPage* page = p ? p : qobject_cast<MvdMPDialogPage*>(sender());
        if (page)
        {
                modifiedStateChanged(page);
        }
}

/*!
        This slot is called whenever a dialog page changes its modified state.
        Subclasses may override it to enable or disable UI elements.
*/
void MvdMultiPageDialog::modifiedStateChanged(MvdMPDialogPage* page)
{
        Q_UNUSED(page);
}

//! Sets a subtitle for this dialog. A subtitle is usually displayed next to the window title.
void MvdMultiPageDialog::setSubtitle(const QString& s)
{
        QString title = windowTitle();

        if (!mSubtitle.isEmpty())
                title.truncate( title.length() - mSubtitle.length() - 3 );

        mSubtitle = s;

        if (!mSubtitle.isEmpty())
                title.append(" - ").append(mSubtitle);

        setWindowTitle(title);
}

//! Returns the subtitle for this dialog. A subtitle is usually displayed between square brackets next to the window title.
QString MvdMultiPageDialog::subtitle() const
{
        return mSubtitle;
}

//!
void MvdMultiPageDialog::setAdvancedControlsVisible(bool visible)
{
        advancedControls->setVisible(visible);
}

//!
bool MvdMultiPageDialog::advancedControlsVisible() const
{
        return advancedControls->isVisible();
}

//!
MvdMPDialogPage* MvdMultiPageDialog::currentPage() const
{
        return qobject_cast<MvdMPDialogPage*>(contents->currentWidget());
}

//!
int MvdMultiPageDialog::currentIndex() const
{
        return contents->currentIndex();
}

//! \internal
void MvdMultiPageDialog::emit_currentPageChanged()
{
        emit currentPageChanged(currentPage());
}

//! \internal
void MvdMultiPageDialog::showEvent(QShowEvent* event)
{
        Q_UNUSED(event);
        if (MvdMPDialogPage* page = qobject_cast<MvdMPDialogPage*>(contents->widget(0)))
                page->setMainWidgetFocus();
}
