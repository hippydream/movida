/**************************************************************************
** Filename: treeview.cpp
** Review: 3
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

#include "treeview.h"
#include "mainwindow.h"
#include "mvdcore/settings.h"
#include <QApplication>
#include <QCursor>
#include <QHeaderView>
#include <QMenu>
#include <QMouseEvent>
#include <QString>
#include <QKeyEvent>

/*!
	\class MvdTreeView treeview.h
	\ingroup movida

	\brief Custom QTreeView implementation.
*/


/************************************************************************
MvdTreeView
*************************************************************************/

/*!
	Creates a new empty data view.
*/
MvdTreeView::MvdTreeView(QWidget* parent)
: QTreeView(parent), mDisableHeaderContextMenu(false)
{
	header()->installEventFilter(this);

	setSortingEnabled(true);
	setRootIsDecorated(false);
}

/*!
	Deletes this view and releases any used resources.
*/
MvdTreeView::~MvdTreeView()
{
	header()->removeEventFilter(this);
}

/*!
	Returns true if no header context menu should be shown.
*/
bool MvdTreeView::isHeaderContextMenuDisabled() const
{
	return mDisableHeaderContextMenu;
}

/*!
	Enables or disables the header context menu.
*/
void MvdTreeView::setHeaderContextMenuDisabled(bool disable)
{
	mDisableHeaderContextMenu = disable;
}

/*!
	\internal Handles the header context menu.
*/
bool MvdTreeView::eventFilter(QObject* o, QEvent* e)
{
	switch (e->type())
	{
		case QEvent::ContextMenu:
		{
			if (mDisableHeaderContextMenu || !isEnabled())
				return true;
				
			QContextMenuEvent* me = (QContextMenuEvent*)e;
			
			int columnCount = header()->count();

			if (columnCount != 0)
			{
				// do not show a menu if we only have one column
				if (columnCount != 1)
					showHeaderContext(me->pos());
			}
			return true;
		}
		default:
			;
	}

	return QTreeView::eventFilter(o, e);
}

/*!
	\internal Shows a context menu to select visible columns.
*/
void MvdTreeView::showHeaderContext(const QPoint& p)
{
	QHeaderView* h = header();

	if (h == 0)
		return;

	int currentLogical = h->logicalIndexAt(p);
	Q_UNUSED(currentLogical)
	
	QMenu context(this);
	
	//! \todo Should menus have a title (or sort of)?
	QAction* _fakeAction = context.addAction(tr("Visible columns"));
	QFont font = _fakeAction->font();
    font.setBold(true);
    _fakeAction->setFont(font);

	context.addSeparator();

	QAbstractItemModel* mod = model();

	// Column names are first sorted by their name
	QMap<QString, int> map;
	QHash<QAction*, int> hash;

	for (int i = 0; i < h->count(); ++i)
	{
		QString txt;
		if (mod == 0)
		{
			txt = QString::number(i);
		}
		else
		{
			QVariant v = mod->headerData(i, Qt::Horizontal, Qt::DisplayRole);
			if (v.type() == QVariant::String)
				txt = v.toString();
			else txt = QString::number(i);
		}

		map.insert(txt, i );
	}

	for (QMap<QString,int>::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it)
	{
		QString txt = it.key();
		int idx = it.value();

		QAction* act = context.addAction(txt);
		act->setCheckable(true);
		act->setChecked( !h->isSectionHidden( h->logicalIndex(idx) ) );

		//! \todo Highlight current column in context menu?
		/*
		if (idx == currentLogical)
		{
			QFont font = act->font();
			font.setItalic(true);
			act->setFont(font);
		}
		*/

		hash.insert(act, idx);
	}

	QAction* _res = context.exec(header()->mapToGlobal(p));
	if (_res == 0 || _res == _fakeAction)
		return;

	int col = hash[_res];
	h->setSectionHidden( h->logicalIndex(col), !_res->isChecked());
	if (_res->isChecked())
		resizeColumnToContents(col);
}

//! \internal
void MvdTreeView::contextMenuEvent(QContextMenuEvent* e)
{
	QModelIndex index = indexAt(e->pos());
	emit contextMenuRequested(index, e->reason());
}

//! \internal
void MvdTreeView::keyPressEvent(QKeyEvent* e)
{
	if (e->key() == Qt::Key_Space && Movida::MainWindow->isQuickFilterVisible()) {
		e->accept();
		return;
	}

	e->ignore();
	QTreeView::keyPressEvent(e);
}

//! \internal
void MvdTreeView::setModel(QAbstractItemModel* model)
{
	QTreeView::setModel(model);
	
	if (model != 0)
	{
		connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), 
			this, SIGNAL(itemSelectionChanged()));
	}
}

/*!
	Returns true if the view's model has any selected row.
*/
bool MvdTreeView::hasSelectedRows() const
{
	QItemSelectionModel* sel = selectionModel();
	return sel != 0 && sel->selectedRows().count();
}

/*!
	Returns the view's selected rows.
*/
QModelIndexList MvdTreeView::selectedRows() const
{
	QItemSelectionModel* sel = selectionModel();
	return sel != 0 ? sel->selectedRows() : QModelIndexList();
}

//! Returns the first selected index or a null QModelIndex.
QModelIndex MvdTreeView::selectedIndex() const
{
	QItemSelectionModel* sel = selectionModel();
	if (sel != 0)
	{
		QModelIndexList list = sel->selectedIndexes();
		if (!list.isEmpty())
			return list.at(0);
	}

	return QModelIndex();
}

//! Returns the selected indexes or a null QModelIndex.
QModelIndexList MvdTreeView::selectedIndexes() const
{
	QItemSelectionModel* sel = selectionModel();
	return sel != 0 ? sel->selectedIndexes() : QModelIndexList();
}

void MvdTreeView::selectIndex(const QModelIndex& idx)
{
	if (!idx.isValid())
		return;

	QItemSelectionModel* sel = selectionModel();
	if (sel == 0)
		return;

	sel->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}
