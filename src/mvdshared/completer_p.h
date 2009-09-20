/**************************************************************************
** Filename: completer_p.h
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the MvdShared API.  It exists for the convenience
// of Movida.  This header file may change from version to version without notice,
// or even be removed.
//
// We mean it.
//

#ifndef MVD_COMPLETER_P_H
#define MVD_COMPLETER_P_H

#include "completer.h"

#include <QtCore/QPointer>
#include <QtGui/QAbstractProxyModel>
#include <QtGui/QItemDelegate>
#include <QtGui/QPainter>
#include <QtGui/QTreeView>

class MvdCompletionModel;

class MvdCompleter::Private : public QObject
{
    Q_OBJECT

public:
    Private(MvdCompleter* p) :
        widget(0),
        proxy(0),
        popup(0),
        cs(Qt::CaseSensitive),
        role(Qt::EditRole),
        column(0),
        sorting(MvdCompleter::UnsortedModel),
        wrap(true),
        eatFocusOut(true),
        q(p)
    {
    }

    ~Private()
    {
        delete popup;
    }

    void init(QAbstractItemModel *model = 0);

    QPointer<QWidget> widget;
    MvdCompletionModel *proxy;
    QAbstractItemView *popup;
    MvdCompleter::CompletionMode mode;

    QString prefix;
    Qt::CaseSensitivity cs;
    int role;
    int column;
    MvdCompleter::ModelSorting sorting;
    bool wrap;

    bool eatFocusOut;
    QRect popupRect;

public slots:
    void showPopup(const QRect&);
    void complete(QModelIndex, bool = false);
    void completionSelected(const QItemSelection&);
    void autoResizePopup();
    void setCurrentIndex(QModelIndex, bool = true);

private:
    MvdCompleter* q;
};

class MvdCompletionEngine::Private
{
public:
    Private(MvdCompletionEngine *p) :
        completer(0),
        curRow(-1),
        cost(0),
        q(p)
    {
    }

    MvdMatchData curMatch, historyMatch;
    MvdCompleter *completer;
    QStringList curParts;
    QModelIndex curParent;
    int curRow;

    Cache cache;
    int cost;

private:
    MvdCompletionEngine *q;
};

class MvdCompletionModel::Private : public QObject
{
    Q_OBJECT

public:
    Private(MvdCompletionModel* p) :
        completer(0),
        engine(0),
        showAll(true),
        q(p)
    { }

    ~Private()
    {
        delete engine;
    }

    void createDefaultEngine()
    {
        bool sortedEngine = false;
        switch (completer->modelSorting()) {
        case MvdCompleter::UnsortedModel:
            sortedEngine = false;
            break;
        case MvdCompleter::CaseSensitivelySortedModel:
            sortedEngine = completer->caseSensitivity() == Qt::CaseSensitive;
            break;
        case MvdCompleter::CaseInsensitivelySortedModel:
            sortedEngine = completer->caseSensitivity() == Qt::CaseInsensitive;
            break;
        }

        delete engine;
        if (sortedEngine)
            engine = new MvdSortedModelEngine(completer);
        else
            engine = new MvdUnsortedModelEngine(completer);
    }

    MvdCompleter *completer;
    MvdCompletionEngine *engine;
    bool showAll;

private:
    MvdCompletionModel* q;
};

#endif // MVD_COMPLETER_P_H
