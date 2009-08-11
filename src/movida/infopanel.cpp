/**************************************************************************
** Filename: infopanel.cpp
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

#include "infopanel.h"

#include <QtCore/QTimer>

namespace {
const int DefaultTempMessageDelay = 3000;
}

MvdInfoPanel::MvdInfoPanel(QWidget *parent) :
    QFrame(parent),
    mHideOnTemporaryMessageTimeout(true)
{
    setupUi(this);
    closeButton->setIcon(QIcon(":/images/filter-close.png"));

    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Raised);

    QIcon i(":/images/dialog-information.svgz");
    mInfoIcon = i.pixmap(16, 16);
    iconLabel->setPixmap(mInfoIcon);

    connect(closeButton, SIGNAL(clicked()), this, SLOT(do_closeImmediately()));
}

MvdInfoPanel::~MvdInfoPanel()
{ }

/*!
    Sets a new permanent text message. This method won't automatically show the panel
    if it is hidden (as opposed to showTemporaryMessage()).
*/
void MvdInfoPanel::setText(const QString &s)
{
    messageLabel->setText(s);
    mPermanentText = s;
}

/*!
    Returns the last permanent message set using setText().
*/
QString MvdInfoPanel::text() const
{
    return mPermanentText;
}

void MvdInfoPanel::setVisible(bool v)
{
    mHideOnTemporaryMessageTimeout = !v;
    QFrame::setVisible(v);
}

void MvdInfoPanel::closeImmediately()
{
    QFrame::setVisible(false);
}

void MvdInfoPanel::do_closeImmediately()
{
    closeImmediately();
    emit closedByUser();
}

/*!
    Sets and shows a temporary message.
    This method will automatically show the panel if necessary and close it after the
    given timeout unless setVisible() has been called to show the panel explicitly.

    The permanent text set using setText() is always restored after the timeout.
*/
void MvdInfoPanel::showTemporaryMessage(const QString &s, int milliseconds /* = 3000*/)
{
    QFrame::setVisible(true);

    mPermanentText = messageLabel->text();
    messageLabel->setText(s);

    if (milliseconds < 200)
        milliseconds = ::DefaultTempMessageDelay;

    QTimer::singleShot(milliseconds, this, SLOT(resetPermanentText()));
}

void MvdInfoPanel::resetPermanentText()
{
    if (mHideOnTemporaryMessageTimeout) {
        closeImmediately();
    }

    messageLabel->setText(mPermanentText);
}
