/**************************************************************************
** Filename: itemdelegate.h
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

#ifndef MVD_ITEMDELEGATE_H
#define MVD_ITEMDELEGATE_H

#include "guiglobal.h"
#include <QItemDelegate>
#include <QModelIndex>
#include <QLineEdit>
#include <QAbstractItemModel>

/*!
	\class MvdItemDelegate itemdelegate.h
	\ingroup Movida

	\brief QItemDelegate with custom validators.
*/

class MvdItemDelegate : public QItemDelegate
{
	Q_OBJECT

public:
	MvdItemDelegate(QObject* parent = 0)
	: QItemDelegate(parent)
	{

	}

	void setEditorData(QWidget* editor, const QModelIndex& index) const
	{
		if (!editor)
			return;

		QVariant v = index.data(Movida::PlaceholderRole);
		bool isPlaceHolder = v.isNull() ? false : v.toBool();
	
		if (isPlaceHolder)
			return;

		QItemDelegate::setEditorData(editor, index);
	}

	void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
	{
		if (!model || !editor)
			return;

		bool ok, skipDelegate = true;

		Movida::ItemValidator validator = (Movida::ItemValidator) index.data(Movida::ValidationRole).toUInt(&ok);
		if (!ok)
			validator = Movida::NoValidator;

		switch (validator)
		{
			case Movida::UndoEmptyValitator:
			{
				if (QLineEdit* e = qobject_cast<QLineEdit*>(editor))
				{
					skipDelegate = false;
					QString s = e->text().trimmed();
					if (!s.isEmpty())
						model->setData(index, s, Qt::DisplayRole);
				}
			}
			break;
			default:
			{
				if (QLineEdit* e = qobject_cast<QLineEdit*>(editor))
				{
					skipDelegate = false;
					model->setData(index, e->text().trimmed(), Qt::DisplayRole);
				}
			}
		}

		if (skipDelegate)
			QItemDelegate::setModelData(editor, model, index);
	}
};

#endif // MVD_ITEMDELEGATE_H
