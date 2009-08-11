/**************************************************************************
** Filename: smartviewwidget.cpp
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

#include "smartview.h"

#include "guiglobal.h"
#include "mainwindow.h"
#include "smartviewdelegate.h"

#include "mvdcore/core.h"

#include "mvdshared/grafx.h"

/*!
    \class MvdSmartView smartview.h
    \ingroup movida

    \brief Tile based view.
*/


MvdSmartView::MvdSmartView(QWidget *parent) :
    MvdListView(parent)
{
    init();
}

MvdSmartView::MvdSmartView(QAbstractItemModel *model, QWidget *parent) :
    MvdListView(parent)
{
    init();
    setModel(model);
}

MvdSmartView::~MvdSmartView()
{ }

//! \internal
void MvdSmartView::init()
{
    setDragEnabled(true);
    setAcceptDrops(true);

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setModelColumn(0);
    setItemDelegate(new MvdSmartViewDelegate(this));
    setUniformItemSizes(true);
    setFlow(QListView::LeftToRight);
    setResizeMode(QListView::Adjust);
    setMouseTracking(true);
}

void MvdSmartView::keyPressEvent(QKeyEvent *e)
{
    // Redirect the space key to the main window if the quick search bar is visible
    if (e->key() == Qt::Key_Space && Movida::MainWindow->isQuickFilterVisible()) {
        QApplication::postEvent(Movida::MainWindow,
            new QKeyEvent(QEvent::KeyPress, Qt::Key_Space, e->modifiers(), e->text(),
                e->isAutoRepeat(), e->count()));
        e->accept();
    } else MvdListView::keyPressEvent(e);
}

bool MvdSmartView::event(QEvent *e)
{
    switch (e->type()) {
        case QEvent::FontChange:
        case QEvent::StyleChange:
        {
            MvdSmartViewDelegate *delegate = dynamic_cast<MvdSmartViewDelegate *>(itemDelegate());
            if (delegate)
                delegate->forcedUpdate();
        }
        break;

        default:
            ;
    }

    return MvdListView::event(e);
}

void MvdSmartView::contextMenuEvent(QContextMenuEvent *cme)
{
    QPoint viewpoint = mapFromGlobal(cme->globalPos());

    if (!viewpoint.isNull() && model()) {
        QModelIndex index = indexAt(viewpoint);
        emit contextMenuRequested(index);
    }
}

MvdSmartView::ItemSize MvdSmartView::itemSize() const
{
    MvdSmartViewDelegate *delegate = dynamic_cast<MvdSmartViewDelegate *>(itemDelegate());

    if (!delegate)
        return MediumItemSize;
    switch (delegate->itemSize()) {
        case MvdSmartViewDelegate::SmallItemSize:
            return SmallItemSize;

        case MvdSmartViewDelegate::LargeItemSize:
            return LargeItemSize;

        default:
            ;
    }
    return MediumItemSize;
}

void MvdSmartView::setItemSize(ItemSize s)
{
    MvdSmartViewDelegate *delegate = dynamic_cast<MvdSmartViewDelegate *>(itemDelegate());

    if (!delegate)
        return;

    switch (s) {
        case SmallItemSize:
            delegate->setItemSize(MvdSmartViewDelegate::SmallItemSize); break;

        case LargeItemSize:
            delegate->setItemSize(MvdSmartViewDelegate::LargeItemSize); break;

        default:
            delegate->setItemSize(MvdSmartViewDelegate::MediumItemSize); break;
    }
}

void MvdSmartView::startDrag(Qt::DropActions supportedActions)
{
    const int MaxPosters = MvdCore::parameter("movida/d&d/max-pixmaps").toInt();

    QModelIndexList indexes = selectedRows();

    if (indexes.count() > 0) {
        QMimeData *data = model()->mimeData(indexes);
        if (!data)
            return;

        int validIndexes = 0;
        QStringList posters;
        QString title;

        foreach(QModelIndex index, indexes)
        {
            if (!index.isValid())
                continue;

            ++validIndexes;

            if (posters.size() >= MaxPosters)
                continue;

            if (title.isEmpty())
                title = index.data(Movida::UniqueDisplayRole).toString();

            QString s = index.data(Movida::MoviePosterRole).toString();
            if (!s.isEmpty() && !posters.contains(s))
                posters.append(s);
        }

        QRect rect;
        rect.adjust(horizontalOffset(), verticalOffset(), 0, 0);

        QDrag *drag = new QDrag(this);
        drag->setMimeData(data);

        QString msg = validIndexes == 1 && !title.isEmpty() ? title :
                      tr("%1 movies", "Drag&drop pixmap overlay message", validIndexes).arg(validIndexes);
        QPixmap pm = MvdGrafx::moviesDragPixmap(posters, msg, this->font());
        drag->setPixmap(pm);

        // drag->setHotSpot(d->pressedPosition - rect.topLeft());
        Qt::DropAction action = drag->start(supportedActions);
        Q_UNUSED(action);
    }
}

void MvdSmartView::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->source() == this) {
        e->ignore();
        return;
    }

    MvdListView::dragEnterEvent(e);
}
