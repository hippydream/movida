/**************************************************************************
** Filename: movietreeview.h
** Revision: 1
**
** Copyright (C) 2007-2009 Angius Fabrizio. All rights reserved.
**
** This file is part of the Movida project (http://movida.sourceforge.net/).
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

#include "movietreeview.h"

#include "guiglobal.h"

#include "mvdcore/core.h"

#include "mvdshared/grafx.h"

MvdMovieTreeView::MvdMovieTreeView(QWidget *parent) :
    MvdTreeView(parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
}

void MvdMovieTreeView::startDrag(Qt::DropActions supportedActions)
{
    const int MaxPosters = Movida::core().parameter("movida/d&d/max-pixmaps").toInt();

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

void MvdMovieTreeView::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->source() == this) {
        e->ignore();
        return;
    }

    MvdTreeView::dragEnterEvent(e);
}
