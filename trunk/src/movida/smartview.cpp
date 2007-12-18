/**************************************************************************
** Filename: smartview.cpp
** Revision: 3
**
** Copyright (C) 2007 Angius Fabrizio. All rights reserved.
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
: QWidget(parent)
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

//! Returns the current aspect ratio for the tiles.
qreal MvdSmartView::aspectRatio() const
{
	return currentAspectRatio;
}

//! Sets the new aspect ratio for the tiles. Ratio is set to the default value if \p r is 0.
void MvdSmartView::setAspectRatio(qreal r)
{
	currentAspectRatio = r == 0 ? 1 : r;
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
	slider->installEventFilter(this);
	view->installEventFilter(this);
	
	if (style()->objectName().toLower() == "windowsxp") {
		layout()->setContentsMargins(4, 4, 0, 4);
	}

	defaultAspectRatio = currentAspectRatio = 2.7;
	minimumSize = 80;
	maximumSize = 400;

	QSize tilesSize(int(minimumSize * currentAspectRatio), minimumSize);

	// How much can we adjust with the slider?
	int maxDelta = maximumSize - qMax(tilesSize.width(), tilesSize.height());
	int step = maxDelta / 10;
	
	slider->setMinimum(0);
	slider->setMaximum(step * 10);
	slider->setValue(step * 5);
	slider->setTickInterval(step);
	slider->setSingleStep(step);
	slider->setPageStep(step * 2);

	delegate = new MvdSmartViewDelegate(view);
	delegate->setItemSize(tilesSize);

	resizeTiles(maxDelta / 2);
	
	view->setSelectionMode(QAbstractItemView::ExtendedSelection);
	view->setItemDelegate(delegate);
	view->setUniformItemSizes(true);
	view->setFlow(QListView::LeftToRight);
	view->setResizeMode(QListView::Adjust);

	updateSliderStatus(true);

	connect( slider, SIGNAL(valueChanged(int)), this, SLOT(resizeTiles(int)) );
	connect( view, SIGNAL(doubleClicked(QModelIndex)), this, SIGNAL(doubleClicked(QModelIndex)) );
}

bool MvdSmartView::eventFilter(QObject* o, QEvent* e)
{
	if (o == slider) {
		switch (e->type())
		{
		case QEvent::Enter:
			updateSliderStatus();
			return true;
			break;
		case QEvent::Leave:
			updateSliderStatus(true);
			return true;
			break;
		default:
			return false;
		}
	} else if (o == view && e->type() == QEvent::KeyPress) {
		QKeyEvent* ke = static_cast<QKeyEvent*>(e);
		// Redirect the space key to the main window if the quick search bar is visible
		if (ke->key() == Qt::Key_Space && Movida::MainWindow->isQuickFilterVisible()) {
			QApplication::postEvent(Movida::MainWindow, new QKeyEvent(QEvent::KeyPress, Qt::Key_Space, ke->modifiers(), ke->text(), ke->isAutoRepeat(), ke->count()));
			return true;
		}
	}

	return QWidget::eventFilter(o, e);
}

void MvdSmartView::contextMenuEvent(QContextMenuEvent* cme)
{
	QPoint viewpoint = view->mapFromGlobal(cme->globalPos());
	if (!viewpoint.isNull() && view->model())
	{
		QModelIndex index = view->indexAt(viewpoint);
		if (index.isValid())
			emit contextMenuRequested(index);
	}
}

void MvdSmartView::resizeTiles(int offset)
{
	QSize tilesSize(int(minimumSize * currentAspectRatio), minimumSize);

	if (currentAspectRatio > 0)
	{
		tilesSize.rwidth() += offset;
		tilesSize.setHeight( int(tilesSize.width() / currentAspectRatio) );
	}
	else
	{
		tilesSize.rheight() += offset;
		tilesSize.setWidth( int(tilesSize.height() * currentAspectRatio) );
	}

	delegate->setItemSize(tilesSize);

	updateSliderStatus();
}

void MvdSmartView::updateSliderStatus(bool reset)
{
	if (reset)
		sliderStatus->setText(tr("Use the slider to resize the tiles."));
	else
	{
		QSize currentSize = delegate->itemSize();
		sliderStatus->setText(tr("%1x%2").arg(currentSize.width()).arg(currentSize.height()));
	}
}
