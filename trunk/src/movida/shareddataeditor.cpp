/**************************************************************************
** Filename: shareddataeditor.cpp
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

#include "shareddataeditor.h"
#include "treeview.h"
#include <QGridLayout>

/*!
	\class MvdSharedDataEditor shareddataeditor.h
	\ingroup movida

	\brief Shared data viewer/editor.
*/


MvdSharedDataEditor::MvdSharedDataEditor(QWidget* parent)
: QDialog(parent)
{
	init();
}

MvdSharedDataEditor::MvdSharedDataEditor(QAbstractItemModel* model, QWidget* parent)
: QDialog(parent)
{
	init();
	view->setModel(model);
}

MvdSharedDataEditor::~MvdSharedDataEditor()
{

}

void MvdSharedDataEditor::setModel(QAbstractItemModel* model)
{
	view->setModel(model);
}

//! \internal
void MvdSharedDataEditor::init()
{
	QGridLayout* layout = new QGridLayout(this);
	layout->setMargin(0);

	view = new MvdTreeView;
	view->setSelectionMode(QAbstractItemView::ExtendedSelection);

	layout->addWidget(view, 0, 0);

	resize(640, 480);
}

//! \internal
void MvdSharedDataEditor::contextMenuEvent(QContextMenuEvent* cme)
{
	QPoint viewpoint = view->mapFromGlobal(cme->globalPos());
	if (!viewpoint.isNull() && view->model())
	{
		QModelIndex index = view->indexAt(viewpoint);
		emit contextMenuRequested(index);
	}
}
