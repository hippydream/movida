/**************************************************************************
** Filename: shareddataview.cpp
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

#include "shareddataview.h"

#include "filterwidget.h"
#include "guiglobal.h"
#include "mainwindow.h"
#include "shareddatamodel.h"

#include "mvdcore/core.h"

#include "mvdshared/grafx.h"

MvdSharedDataView::MvdSharedDataView(QWidget *parent) :
    MvdTreeView(parent)
{
    setSelectionBehavior(SelectRows);
    setSelectionMode(ExtendedSelection);

    connect(this, SIGNAL(activated(QModelIndex)), this, SLOT(onItemActivated(QModelIndex)));
}

MvdSharedDataView::~MvdSharedDataView()
{
}

void MvdSharedDataView::startDrag(Qt::DropActions supportedActions)
{
    const int MaxValues = Movida::core().parameter("movida/d&d/max-values").toInt();

    MvdSharedDataModel *m = dynamic_cast<MvdSharedDataModel *>(model());

    if (!m)
        return;

    Movida::DataRole role = m->role();

    QStringList values;
    QList<mvdid> ids;
    QModelIndexList indexes = selectedRows();
    if (indexes.count() > 0) {
        QMimeData *data = model()->mimeData(indexes);
        if (!data)
            return;

        int validIndexes = 0;
        foreach(QModelIndex index, indexes)
        {
            if (!index.isValid())
                continue;

            ++validIndexes;

            mvdid id = index.data(Movida::IdRole).toUInt();
            ids.append(id);

            if (values.size() >= MaxValues)
                continue;

            QString s = index.data(Movida::UniqueDisplayRole).toString();

            if (!s.isEmpty() && !values.contains(s))
                values.append(s);
        }

        if (validIndexes == 0)
            return;

        QDrag *drag = new QDrag(this);
        drag->setMimeData(data);

        if (validIndexes > 1) {
            qSort(values);
            QString msg;
            switch (role) {
                case Movida::PersonRole:
                    msg = tr("%1 persons: ", "Shared data D&D", validIndexes); break;

                case Movida::CountryRole:
                    msg = tr("%1 countries: ", "Shared data D&D", validIndexes); break;

                case Movida::GenreRole:
                    msg = tr("%1 genres: ", "Shared data D&D", validIndexes); break;

                case Movida::LanguageRole:
                    msg = tr("%1 languages: ", "Shared data D&D", validIndexes); break;

                case Movida::TagRole:
                    msg = tr("%1 tags: ", "Shared data D&D", validIndexes); break;

                default:
                    ;
            }
            msg = msg.arg(validIndexes);
            QString s = values.at(0);
            values.replace(0, s.prepend(msg));
        }

        if (validIndexes > values.size())
            values.append(QString("..."));

        QPixmap pm = MvdGrafx::sharedDataDragPixmap(values.join(QLatin1String(", ")), this->font());
        drag->setPixmap(pm);

        //! \todo Offset should ensure that the pixmap is visible on any OS/Style
        drag->setHotSpot(drag->hotSpot() - QPoint(14, 10));

        Qt::DropAction action = drag->start(supportedActions);
        Q_UNUSED(action);
    }
}

void MvdSharedDataView::onItemActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    mvdid id = index.data(Movida::IdRole).toUInt();
    if (id == MvdNull)
        return;

    Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
    emit itemActivated(id, mods != Qt::ControlModifier);
}
