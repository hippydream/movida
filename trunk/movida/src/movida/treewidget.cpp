/**************************************************************************
** Filename: treewidget.cpp
** Revision: 1
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

#include "treewidget.h"
#include "settings.h"

#include <QApplication>
#include <QCursor>
#include <QHeaderView>
#include <QMenu>
#include <QMouseEvent>
#include <QString>

/*!
	\class MvdTreeWidget treewidget.h
	\ingroup gui

	\brief Custom QTreeWidget implementation.
*/


/************************************************************************
MvdTreeWidget_P
*************************************************************************/

//! \internal
class MvdTreeWidget_P
{
public:
	MvdTreeWidget_P() : disableHeaderContextMenu(false) {}

	bool disableHeaderContextMenu;
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
