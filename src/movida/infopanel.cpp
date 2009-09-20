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

#include "mvdcore/settings.h"
#include "mvdshared/effects.h"

#include <QtCore/QTimer>

namespace {
const int DefaultTempMessageDelay = 3000;
}

class MvdInfoPanel::Private
{
public:
    Private(MvdInfoPanel *p) :
        timer(0),  
        hideOnTemporaryMessageTimeout(true),
        state(PersistentMessage),
        widgetPosition(MvdInfoPanel::WidgetAtBottom),
        q(p)
    {

    }

    QPixmap infoIcon;
    QString persistentText;
    QTimer *timer;
    bool hideOnTemporaryMessageTimeout;
    MvdInfoPanel::WidgetPosition widgetPosition;

    enum State { PersistentMessage, TemporaryMessage } state;

private:
    MvdInfoPanel *q;
};


MvdInfoPanel::MvdInfoPanel(QWidget *parent) :
    QFrame(parent),
    d(new Private(this))
{
    setupUi(this);

    d->timer = new QTimer(this);
    d->timer->setSingleShot(true);
    connect(d->timer, SIGNAL(timeout()), this, SLOT(timeout()));

    closeButton->setIcon(QIcon(":/images/filter-close.png"));

    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Raised);

    QIcon i(":/images/dialog-information.svgz");
    d->infoIcon = i.pixmap(16, 16);
    iconLabel->setPixmap(d->infoIcon);

    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
}

MvdInfoPanel::~MvdInfoPanel()
{
    delete d;
}

/*!
    Sets a new permanent text message. This method won't automatically show the panel
    if it is hidden (as opposed to showPersistentMessage() or showTemporaryMessage()).
*/
void MvdInfoPanel::setPersistentMessage(const QString &s)
{
    d->persistentText = s;
    if (d->state == Private::PersistentMessage)
        messageLabel->setText(s);
}

/*!
    Returns the last permanent message set using setText().
*/
QString MvdInfoPanel::persistentMessage() const
{
    return d->persistentText;
}

void MvdInfoPanel::setVisible(bool v)
{
    QFrame::setVisible(v);
}

void MvdInfoPanel::closeRequest()
{
    close();
    emit closedByUser();
}

/*!
    Sets and shows a persistent message.
    This method will automatically show the panel if necessary.
*/
void MvdInfoPanel::showPersistentMessage(const QString &s)
{
    if (d->state == Private::TemporaryMessage) {
        d->timer->stop();
        d->state = Private::PersistentMessage;
    }

    d->hideOnTemporaryMessageTimeout = false;
    setPersistentMessage(s);

    show();
}

/*!
    Sets and shows a temporary message.
    This method will automatically show the panel if necessary and close it after the
    given timeout.

    The permanent text set using showPersistentMessage()  or setPersistentMessage()
    will be restored after the timeout.
*/
void MvdInfoPanel::showTemporaryMessage(const QString &s, int milliseconds /* = 3000*/)
{
    if (d->state == Private::TemporaryMessage) {
        d->timer->stop();
    }
    d->state = Private::TemporaryMessage;

    messageLabel->setText(s);
    
    show();
    d->timer->start(milliseconds <= 0 ? ::DefaultTempMessageDelay : milliseconds);
}

void MvdInfoPanel::timeout()
{
    d->state = Private::PersistentMessage;
    messageLabel->setText(d->persistentText);
    if (d->hideOnTemporaryMessageTimeout)
        closeWithEffect();
}

bool MvdInfoPanel::event(QEvent *e)
{
    if (e->type() == QEvent::Close) {
        d->hideOnTemporaryMessageTimeout = true;
        if (d->state == Private::TemporaryMessage)
            d->timer->stop();
    }

    return QFrame::event(e);
}

MvdInfoPanel::WidgetPosition MvdInfoPanel::widgetPosition() const
{
    return d->widgetPosition;
}

/*!
    Sets the position of this widget within the window.
    This value is used when the widget is closed or shown with a scroll
    effect, so remember to set it to the correct value.
*/
void MvdInfoPanel::setWidgetPosition(WidgetPosition wp)
{
    d->widgetPosition = wp;
}

/*!
    Shows the panel using a scroll effect (unless effects are disabled
    by the "movida/effects/bars" MvdSettings value.
*/
void MvdInfoPanel::showWithEffect()
{
    show(); // Effects are broken
}

/*!
    Closes the panel using a scroll effect (unless effects are disabled
    by the "movida/effects/bars" MvdSettings value.
*/
void MvdInfoPanel::closeWithEffect()
{
    /*const bool useEffects = Movida::settings().value("movida/effects/bars").toBool();
    if (useEffects) {
        Movida::fadeEffect(this, Movida::DefaultWidgetEffectDuration, true);
    } else {
        close();
    }*/
    // Effects are broken
    close();
}
