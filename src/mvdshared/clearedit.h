/**************************************************************************
** Filename: clearedit.h
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

#ifndef MVD_CLEAREDIT_H
#define MVD_CLEAREDIT_H

#include "sharedglobal.h"
#include "lineedit.h"

class QAbstractButton;

class MVD_EXPORT_SHARED MvdClearEdit : public MvdLineEdit
{
    Q_OBJECT

public:
    MvdClearEdit(QWidget *parent = 0);
    virtual ~MvdClearEdit();

    QSize pixmapSize() const;

    QPixmap pixmap() const;
    void setPixmap(const QPixmap& pm);

    QString toolTip() const;
    void setToolTip(const QString& tip);

    virtual QSize sizeHint() const;

signals:
    void embeddedActionTriggered();

protected:
    MvdClearEdit(const QPixmap &pixmap, const QString &tip, QWidget * parent = 0);

    QAbstractButton *button() const;

    virtual void resizeEvent(QResizeEvent *);

protected slots:
    virtual void on_buttonClicked();
    virtual void updateButton(const QString &text);

private:
    void init(const QPixmap &pixmap, const QString &tip);

    class Private;
    Private *d;
};

//////////////////////////////////////////////////////////////////////////

class MVD_EXPORT_SHARED MvdResetEdit : public MvdClearEdit
{
    Q_OBJECT

public:
    MvdResetEdit(QWidget *parent = 0);
    virtual ~MvdResetEdit();

    void setDefaultValue(const QString &s);
    QString defaultValue() const;

protected:
    virtual void setTextCalled();

protected slots:
    virtual void on_buttonClicked();
    virtual void updateButton(const QString &text);

private:
    class Private;
    friend class Private;
    Private *d;
};

#endif // MVD_CLEAREDIT_H
