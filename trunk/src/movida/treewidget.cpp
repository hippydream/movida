/**************************************************************************
** Filename: treewidget.cpp
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

#include "treewidget.h"
#include "mvdcore/settings.h"
#include <QApplication>
#include <QCursor>
#include <QHeaderView>
#include <QMenu>
#include <QMouseEvent>
#include <QKeyEvent>

/*!
	\class MvdTreeWidget treewidget.h
	\ingroup gui

	\brief Custom QTreeWidget implementation.
*/

/*!
	\enum MvdTreeWidget::CursorMovement Determines how the next index is
	chosen when editing is in act.

	\value MvdTreeWidget::MoveByRow Choses the item on the same row but next column.
	\value MvdTreeWidget::MoveByColumn Choses the item on the same column but next row.
	\value MvdTreeWidget::MoveByIndex Same as MoveByRow | MoveByColumn, choses the next item on the same row or on the next row if the current row has no more items.
*/


/************************************************************************
MvdTreeWidget_P
*************************************************************************/

//! \internal
class MvdTreeWidget_P
{
public:
	MvdTreeWidget_P() : disableHeaderContextMenu(false), cursorMovement(MvdTreeWidget::MoveByColumn) {}

	bool disableHeaderContextMenu;
	MvdTreeWidget::CursorMovement cursorMovement;
};


/************************************************************************
MvdTreeWidget
*************************************************************************/

/*!
	Creates a new empty data view.
*/
MvdTreeWidget::MvdTreeWidget(QWidget* parent)
: QTreeWidget(parent)
{
	d = new MvdTreeWidget_P();

	QHeaderView* h = header();
	h->installEventFilter(this);
	h->setDefaultAlignment(Qt::AlignLeft);
	h->setSortIndicator(0, Qt::AscendingOrder);

	setSortingEnabled(true);
	setRootIsDecorated(false);
}

/*!
	Deletes this view and releases any used resources.
*/
MvdTreeWidget::~MvdTreeWidget()
{
	delete d;
	header()->removeEventFilter(this);
}

/*!
	Returns true if no header context menu should be shown.
*/
bool MvdTreeWidget::isHeaderContextMenuDisabled() const
{
	return d->disableHeaderContextMenu;
}

/*!
	Enables or disables the header context menu.
*/
void MvdTreeWidget::setHeaderContextMenuDisabled(bool disable)
{
	d->disableHeaderContextMenu = disable;
}

/*!
	Returns the current cursor movement type.

	\sa MvdTreeWidget::setCursorMovement(CursorMovement)
*/
MvdTreeWidget::CursorMovement MvdTreeWidget::cursorMovement() const
{
	return d->cursorMovement;
}

/*!
	The cursor movement type determines how the next index is selected when editing
	is in act. Please refer to the enum values for detailed info.

	Default is MoveByColumn.
*/
void MvdTreeWidget::setCursorMovement(CursorMovement m)
{
	d->cursorMovement = m;
}

/*!
	\internal Handles the header context menu.
*/
bool MvdTreeWidget::eventFilter(QObject* o, QEvent* e)
{
	switch (e->type())
	{
		case QEvent::ContextMenu:
		{
			if (d->disableHeaderContextMenu || !isEnabled())
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

	return QTreeWidget::eventFilter(o, e);
}

void MvdTreeWidget::keyPressEvent(QKeyEvent* e)
{
	if (model() && (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right 
		|| e->key() == Qt::Key_Up || e->key() == Qt::Key_Down))
	{
		QModelIndex idx = currentIndex();
		if (e->key() == Qt::Key_Left && idx.column() > 0)
			idx = model()->index(idx.row(), idx.column() - 1, idx.parent());
		else if(e->key() == Qt::Key_Right && idx.column() < model()->columnCount() - 1)
			idx = model()->index(idx.row(), idx.column() + 1, idx.parent());
		else if (e->key() == Qt::Key_Up && idx.row() > 0)
			idx = model()->index(idx.row() - 1, idx.column(), idx.parent());
		else if (e->key() == Qt::Key_Down && idx.row() < model()->rowCount() - 1)
			idx = model()->index(idx.row() + 1, idx.column(), idx.parent());
		setCurrentIndex(idx);
	}
	else QTreeWidget::keyPressEvent(e);
}

/*!
	\internal Shows a context menu to select visible columns.
*/
void MvdTreeWidget::showHeaderContext(const QPoint& p)
{
	QHeaderView* h = header();

	if (h == 0)
		return;

	int currentLogical = h->logicalIndexAt(p);
	Q_UNUSED(currentLogical)
	
	QMenu context(this);
	
	//! \todo Should menus have a title (or sort of)?
	QAction* _fakeAction = context.addAction("Visible columns");
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
void MvdTreeWidget::contextMenuEvent(QContextMenuEvent* e)
{
	QModelIndex index = indexAt(e->pos());

	if (!index.isValid())
		emit contextMenuRequested(0, 0);
	else
	{
		QTreeWidgetItem* item = topLevelItem(index.row());
		emit contextMenuRequested(item, index.column());
	}
}

//! Reimplementation to ensure custom handling of the MovePrevious and MoveNext hints.
QModelIndex MvdTreeWidget::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
	Q_UNUSED(modifiers);

	executeDelayedItemsLayout();

	QModelIndex current = currentIndex();
	if (!model() || !current.isValid())
		return QTreeWidget::moveCursor(cursorAction, modifiers);

	switch (cursorAction)
	{
	case MoveNext:
		if (d->cursorMovement & MoveByRow && current.column() < model()->columnCount() - 1)
			return model()->index(current.row(), current.column() + 1, current.parent());
		if (d->cursorMovement & MoveByColumn && current.row() < model()->rowCount() - 1)
			return model()->index(current.row() + 1, d->cursorMovement & MoveByRow ? 0 : current.column(), current.parent());
		break;
	case MovePrevious:
		if (d->cursorMovement & MoveByRow && current.column() > 0)
			return model()->index(current.row(), current.column() - 1, current.parent());
		if (d->cursorMovement & MoveByColumn && current.row() > 0)
			return model()->index(current.row() - 1, d->cursorMovement & MoveByRow ? model()->columnCount() - 1 : current.column(), current.parent());
		break;
	default:
		return QTreeWidget::moveCursor(cursorAction, modifiers);
	}

	return current;
}

QList<QTreeWidgetItem*> MvdTreeWidget::filteredSelectedItems() const
{
	QList<QTreeWidgetItem*> sel = selectedItems();
	for (int i = 0; i < sel.size(); ++i)
	{
		if (isPlaceHolder(sel.at(i)))
			sel.removeAt(i);
	}

	return sel;
}

QList<mvdid> MvdTreeWidget::filteredSelectedIds() const
{
	QList<QTreeWidgetItem*> sel = selectedItems();
	QList<mvdid> ids;
	for (int i = 0; i < sel.size(); ++i)
	{
		QTreeWidgetItem* item = sel.at(i);
		if (isPlaceHolder(item))
			continue;
		bool ok;
		mvdid id = item->data(0, Movida::IdRole).toUInt(&ok);
		if (ok && id != MvdNull)
			ids.append(id);
	}

	return ids;
}

QList<QTreeWidgetItem*> MvdTreeWidget::itemsById(const QList<mvdid>& ids) const
{
	QList<QTreeWidgetItem*> items;

	for (int i = 0; i < topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = topLevelItem(i);
		if (isPlaceHolder(item))
			continue;
		bool ok;
		mvdid id = item->data(0, Movida::IdRole).toUInt(&ok);
		if (!ok || id == MvdNull)
			continue;
		if (ids.contains(id))
			items.append(item);
	}
	return items;
}

bool MvdTreeWidget::isPlaceHolder(QTreeWidgetItem* item) const
{
	if (!item)
		return false;

	QVariant v = item->data(0, Movida::PlaceholderRole);
	return !v.isNull() && v.toBool();
}
