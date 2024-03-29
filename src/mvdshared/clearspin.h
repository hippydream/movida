/**************************************************************************
** Filename: clearspin.h
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

#ifndef MVD_CLEARSPIN_H
#define MVD_CLEARSPIN_H

#include "sharedglobal.h"

#include <QtGui/QSpinBox>

class MVD_EXPORT_SHARED MvdClearSpin : public QSpinBox
{
    Q_OBJECT

public:
    MvdClearSpin(QWidget * parent = 0);

    virtual void setRange(int min, int max);

    QSize sizeHint() const;

protected:
    virtual bool event(QEvent *event);
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
    virtual void showEvent(QShowEvent *);

protected slots:
    virtual void clearButtonClicked();

private slots:
    void updateClearButton(const QString &text);

private:
    class Private;
    Private *d;
};

#endif // MVD_CLEAREDIT_H
