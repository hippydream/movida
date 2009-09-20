/**************************************************************************
** Filename: movieviewlistener.cpp
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

#include "movieviewlistener.h"

#include "guiglobal.h"
#include "mainwindow.h"
#include "mainwindow_p.h"
#include "shareddataeditor.h"
#include "smartview.h"

#include "mvdcore/core.h"
#include "mvdcore/settings.h"

#include <QtCore/QMimeData>
#include <QtGui/QAbstractItemView>
#include <QtGui/QDropEvent>
#include <QtGui/QStatusBar>

/*!
    \class MvdMovieViewListener movieviewlistener.h
    \brief A movie view listener will filter events from multiple movie
    views and handle specific events and - for instance - show relevant
    messages to the user.
*/

MvdMovieViewListener::MvdMovieViewListener(MvdMainWindow *parent) :
    QObject(parent), mMainWindow(parent)
{
    Q_ASSERT(parent);
}

void MvdMovieViewListener::registerView(QAbstractItemView *view)
{
    if (view)
        view->viewport()->installEventFilter(this);
}

bool MvdMovieViewListener::eventFilter(QObject *o, QEvent *e)
{
    MvdMainWindow::Private *d = mMainWindow->d;
    MvdMainWindow *q = mMainWindow;
    Q_ASSERT(d);
    Q_ASSERT(q);

    QAbstractItemView *view = qobject_cast<QAbstractItemView *>(o->parent());
    Q_ASSERT(view);

    switch (e->type()) {
    case QEvent::DragMove:
    {
        QDragMoveEvent *dragEvent = static_cast<QDragMoveEvent *>(e);

        if (e->isAccepted()) {
            QModelIndex index = view->indexAt(dragEvent->pos());
            if (index.isValid()) {
                QString title = index.data(Movida::UniqueDisplayRole).toString(); // Role is column-independent
                if (index.isValid() && !title.isEmpty()) {
                    //! \todo Item under the cursor should show a drop indicator depending on the carried data (i.e. highlight the movie poster area)
                    const QString mimeMA = Movida::core().parameter("movida/mime/movie-attributes").toString();
                    const QMimeData *md = dragEvent->mimeData();
                    if (md->hasUrls() && !md->hasFormat(mimeMA)) {
                        Movida::MainWindow->statusBar()->showMessage(tr("Drop to set a new movie poster for '%1'").arg(title));
                    }
                }
            }
        } else Movida::MainWindow->statusBar()->clearMessage();
    }
    break;

    case QEvent::DragEnter:
    if (!d->mDraggingSharedData) {
        QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent *>(e);
        if (dragEvent->source() == d->mSharedDataEditor->view()) {
            // Dirty dirty dirty trick to detect the end of a drag operation.
            // Any attempt to use DragLeave or Drop failed.
            connect(dragEvent->mimeData(), SIGNAL(destroyed()), d, SLOT(sdeDragEnded()));
            d->sdeDragStarted();
        }

    }
    break;

    case QEvent::Drop:
    {
        qDebug("Drop!");
    }
    break;

    case QEvent::Wheel:
    if (view == d->mSmartView) {
        if (qApp->keyboardModifiers() == Qt::ControlModifier) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(e);

            bool zoomInReq = wheelEvent->delta() >= 0;
            if (!Movida::settings().value("movida/movie-view/wheel-up-magnifies").toBool())
                zoomInReq = !zoomInReq; // Invert direction

            QAction *za = zoomInReq ? d->mA_ViewModeZoomIn : d->mA_ViewModeZoomOut;

            int numDegrees = qAbs(wheelEvent->delta()) / 8;
            int numSteps = numDegrees / 15;

            for (int i = 0; i < numSteps && za->isEnabled(); ++i) {
                if (zoomInReq)
                    q->zoomIn();
                else q->zoomOut();
            }
            return true; // Filter out or the view will scroll!
        }
    }
    break;

    default: ;
    }

    return false;
}
