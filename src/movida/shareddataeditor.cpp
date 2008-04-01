/**************************************************************************
** Filename: shareddataeditor.cpp
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

#include "shareddataeditor.h"
#include "treeview.h"
#include "shareddatamodel.h"
#include <QGridLayout>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>

/*!
	\class MvdSharedDataEditor shareddataeditor.h
	\ingroup movida

	\brief Shared data viewer/editor.
*/


MvdSharedDataEditor::MvdSharedDataEditor(QWidget* parent)
: QWidget(parent)
{
	init();
}

MvdSharedDataEditor::MvdSharedDataEditor(QAbstractItemModel* model, QWidget* parent)
: QWidget(parent)
{
	init();
	Ui::MvdSharedDataEditor::view->setModel(model);
}

MvdSharedDataEditor::~MvdSharedDataEditor()
{

}

void MvdSharedDataEditor::setModel(QAbstractItemModel* model)
{
	Ui::MvdSharedDataEditor::view->setModel(model);
}

//! \internal
void MvdSharedDataEditor::init()
{
	setupUi(this);
	filterEdit->setPlaceHolder(tr("Search this list..."));

	dataRole->addItem(tr("People"), QVariant((uint)Movida::PersonRole));
	dataRole->addItem(tr("Genres"), QVariant((uint)Movida::GenreRole));
	dataRole->addItem(tr("Tags"), QVariant((uint)Movida::TagRole));
	dataRole->addItem(tr("Countries"), QVariant((uint)Movida::CountryRole));
	dataRole->addItem(tr("Languages"), QVariant((uint)Movida::LanguageRole));

	connect(dataRole, SIGNAL(currentIndexChanged(int)), SLOT(updateDataRole()));

	Ui::MvdSharedDataEditor::view->setDragEnabled(true);

	Ui::MvdSharedDataEditor::view->setDragEnabled(true);
	Ui::MvdSharedDataEditor::view->setAcceptDrops(true);
}

//! \internal
void MvdSharedDataEditor::contextMenuEvent(QContextMenuEvent* cme)
{
	QPoint viewpoint = Ui::MvdSharedDataEditor::view->mapFromGlobal(cme->globalPos());
	if (!viewpoint.isNull() && Ui::MvdSharedDataEditor::view->model())
	{
		QModelIndex index = Ui::MvdSharedDataEditor::view->indexAt(viewpoint);
		emit contextMenuRequested(index);
	}
}

void MvdSharedDataEditor::updateDataRole()
{
	MvdSharedDataModel* m = dynamic_cast<MvdSharedDataModel*>(Ui::MvdSharedDataEditor::view->model());
	Q_ASSERT(m);

	Movida::DataRole role = (Movida::DataRole) dataRole->itemData(dataRole->currentIndex()).toUInt();
	m->setRole(role);
}

QAbstractItemModel* MvdSharedDataEditor::model() const
{
	return Ui::MvdSharedDataEditor::view->model();
}

QAbstractItemView* MvdSharedDataEditor::view() const
{
	return Ui::MvdSharedDataEditor::view;
}
