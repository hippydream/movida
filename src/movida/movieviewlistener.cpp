/**************************************************************************
** Filename: movieviewlistener.cpp
**
** Copyright (C) 2007-2008 Angius Fabrizio. All rights reserved.
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
#include "mainwindow.h"
#include "guiglobal.h"
#include "mvdcore/core.h"
#include <QDragMoveEvent>
#include <QAbstractItemView>
#include <QStatusBar>

/*!
	\class MvdMovieViewListener movieviewlistener.h
	\brief A movie view listener will filter events from multiple movie
	views and handle specific events and - for instance - show relevant
	messages to the user.
*/

MvdMovieViewListener::MvdMovieViewListener(QObject* parent)
: QObject(parent)
{
}

void MvdMovieViewListener::registerView(QAbstractItemView* view)
{
	if (view)
		view->viewport()->installEventFilter(this);
}

bool MvdMovieViewListener::eventFilter(QObject* o, QEvent* _e) {
	QAbstractItemView* view = dynamic_cast<QAbstractItemView*>(o->parent());
	if (!view) return false;

	if (_e->type() == QEvent::DragMove) {
		QDragMoveEvent* e = static_cast<QDragMoveEvent*>(_e);

		if (e->isAccepted()) {
			QModelIndex index = view->indexAt(e->pos());
			if (index.isValid()) {
				QString title = index.data(Movida::UniqueDisplayRole).toString(); // Role is column-independent
				if (index.isValid() && !title.isEmpty()) {
					//! \todo Item under the cursor should show a drop indicator depending on the carried data (i.e. highlight the movie poster area)
					const QString mimeMA = MvdCore::parameter("movida/mime/movie-attributes").toString();
					const QMimeData* md = e->mimeData();
					if (md->hasUrls() && !md->hasFormat(mimeMA)) {
						Movida::MainWindow->statusBar()->showMessage(tr("Drop to set a new movie poster for '%1'").arg(title));
					}
				}
			}
		} else Movida::MainWindow->statusBar()->clearMessage();
	}

	return false;
}
