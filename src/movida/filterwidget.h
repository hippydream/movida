/**************************************************************************
** Filename: filterwidget.h
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

#ifndef MVD_FILTERWIDGET_H
#define MVD_FILTERWIDGET_H

#include "ui_filterwidget.h"

#include "shareddatamodel.h"

#include <QtGui/QWidget>

class QLineEdit;
class QPixmap;

class MvdFilterWidget : public QFrame, private Ui::MvdFilterWidget
{
    Q_OBJECT

public:
    enum Message {
        NoMessage = 0,
        NoResultsWarning,
        SyntaxErrorWarning,
        DropInfo
    };

    MvdFilterWidget(QWidget *parent = 0);

    QLineEdit *editor() const;

    void setMessage(Message m);
    Message message() const;

    void setCaseSensitivity(Qt::CaseSensitivity cs);
    Qt::CaseSensitivity caseSensitivity() const;

    void setBooleanOperator(Movida::BooleanOperator op);
    Movida::BooleanOperator booleanOperator() const;

    void applySharedDataFilter(const QString &itemIds, bool replaceFilter);

    bool isEmpty() const;
    QString filter() const;

protected:
    virtual bool event(QEvent *e);
    virtual void dragEnterEvent(QDragEnterEvent *e);
    virtual void dragMoveEvent(QDragMoveEvent *e);
    virtual void dropEvent(QDropEvent *e);

signals:
    void hideRequest();
    void caseSensitivityChanged();
    void booleanOperatorChanged();

private:
    Message mMessage;
    QPixmap mWarning;
    QPixmap mInfo;
};

#endif // MVD_FILTERWIDGET_H
