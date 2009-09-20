/**************************************************************************
** Filename: infopanel.h
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

#ifndef MVD_INFOPANEL_H
#define MVD_INFOPANEL_H

#include "ui_infopanel.h"

#include <QtGui/QFrame>

class QFrame;

class MvdInfoPanel : public QFrame, protected Ui::MvdInfoPanel
{
    Q_OBJECT

public:
    MvdInfoPanel(QWidget *parent = 0);
    virtual ~MvdInfoPanel();

    void setPersistentMessage(const QString &s);
    QString persistentMessage() const;

    void showPersistentMessage(const QString &s);
    void showTemporaryMessage(const QString &s, int milliseconds = 3000);

    enum WidgetPosition {
        WidgetAtTop, WidgetAtBottom
    };

    WidgetPosition widgetPosition() const;
    void setWidgetPosition(WidgetPosition wp);
    
    void showWithEffect();
    void closeWithEffect();

signals:
    void closedByUser();

protected:
    virtual bool event(QEvent *e);

private slots:
    void closeRequest();
    void timeout();

private:
    // Make private to force using show*Message() and close() and avoid
    // confusion
    void setVisible(bool v);

    class Private;
    Private *d;
};

#endif // MVD_INFOPANEL_H
