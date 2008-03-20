/**************************************************************************
** Filename: smartview.cpp
**
** Copyright (C) 2007 Angius Fabrizio. All rights reserved.
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
#include "smartviewdelegate.h"
#include "mainwindow.h"

/*!
	\class MvdSmartView smartview.h
	\ingroup movida

	\brief Tile based view.
*/


MvdSmartView::MvdSmartView(QWidget* parent)
: QWidget(parent)
{
	init();
}

MvdSmartView::MvdSmartView(QAbstractItemModel* model, QWidget* parent)
: QWidget(parent), delegate(0)
{
	init();
	view->setModel(model);
}

MvdSmartView::~MvdSmartView()
{

}

void MvdSmartView::setModel(QAbstractItemModel* model)
{
	view->setModel(model);
}

/*!
	Returns the spacing for this view, i.e. the distance between icons.
*/
int MvdSmartView::viewSpacing() const
{
	return view->spacing();
}

//! Sets the spacing for this view.
void MvdSmartView::setViewSpacing(int spacing)
{
	view->setSpacing(spacing);
}

//! Returns the selection model used by this view.
QItemSelectionModel* MvdSmartView::selectionModel() const
{
	return view->selectionModel();
}

//! Sets a shared selection model for this view.
void MvdSmartView::setSelectionModel(QItemSelectionModel* selectionModel)
{
	view->setSelectionModel(selectionModel);
}

//! \internal
void MvdSmartView::init()
{
	setupUi(this);
	view->installEventFilter(this);

	delegate = new MvdSmartViewDelegate(view);

	view->setSelectionMode(QAbstractItemView::ExtendedSelection);
	view->setItemDelegate(delegate);
	view->setUniformItemSizes(true);
	view->setFlow(QListView::LeftToRight);
	view->setResizeMode(QListView::Adjust);
	view->setMouseTracking(true);
	connect( view, SIGNAL(doubleClicked(QModelIndex)), this, SIGNAL(doubleClicked(QModelIndex)) );
}

bool MvdSmartView::eventFilter(QObject* o, QEvent* e)
{
	if (o == view) {
		switch (e->type()) {
		case QEvent::KeyPress: {
			QKeyEvent* ke = static_cast<QKeyEvent*>(e);
			// Redirect the space key to the main window if the quick search bar is visible
			if (ke->key() == Qt::Key_Space && Movida::MainWindow->isQuickFilterVisible()) {
				QApplication::postEvent(Movida::MainWindow, new QKeyEvent(QEvent::KeyPress, Qt::Key_Space, ke->modifiers(), ke->text(), ke->isAutoRepeat(), ke->count()));
				return true;
			}
		}
		break;
		case QEvent::FontChange:
		case QEvent::StyleChange: {
			if (delegate)
				delegate->forcedUpdate();
		}
		break;
		default: ;
		}
	}

	return QWidget::eventFilter(o, e);
}

void MvdSmartView::contextMenuEvent(QContextMenuEvent* cme)
{
	QPoint viewpoint = view->mapFromGlobal(cme->globalPos());
	if (!viewpoint.isNull() && view->model()) {
		QModelIndex index = view->indexAt(viewpoint);
		emit contextMenuRequested(index);
	}
}

MvdSmartView::ItemSize MvdSmartView::itemSize() const
{
	if (!delegate)
		return MediumItemSize;
	switch (delegate->itemSize()) {
	case MvdSmartViewDelegate::SmallItemSize: return SmallItemSize;
	case MvdSmartViewDelegate::LargeItemSize: return LargeItemSize;
	default: ;
	}
	return MediumItemSize;
}

void MvdSmartView::setItemSize(ItemSize s)
{
	if (!delegate)
		return;

	switch (s) {
	case SmallItemSize: delegate->setItemSize(MvdSmartViewDelegate::SmallItemSize); break;
	case LargeItemSize: delegate->setItemSize(MvdSmartViewDelegate::LargeItemSize); break;
	default: delegate->setItemSize(MvdSmartViewDelegate::MediumItemSize); break;
	}
}
