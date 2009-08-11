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

    void setText(const QString &s);
    QString text() const;

    void showTemporaryMessage(const QString &s, int milliseconds = 3000);

    void setVisible(bool v);

public slots:
    void closeImmediately();

signals:
    void closedByUser();

private slots:
    void do_closeImmediately();
    void resetPermanentText();

private:
    QPixmap mInfoIcon;
    QString mPermanentText;
    bool mHideOnTemporaryMessageTimeout;
};

#endif // MVD_INFOPANEL_H
