/**************************************************************************
** Filename: smartview.h
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

#ifndef MVD_SMARTVIEW_H
#define MVD_SMARTVIEW_H

#include "ui_smartview.h"
#include <QWidget>

class MvdSmartViewDelegate;
class QListView;
class QItemSelectionModel;

class MvdSmartView : public QWidget, protected Ui::MvdSmartView
{
	Q_OBJECT

public:
	MvdSmartView(QWidget* parent = 0);
	MvdSmartView(QAbstractItemModel* model, QWidget* parent = 0);
	virtual ~MvdSmartView();
	
	void setModel(QAbstractItemModel* model);

	int viewSpacing() const;
	void setViewSpacing(int spacing);

	qreal aspectRatio() const;
	void setAspectRatio(qreal r);

	QItemSelectionModel* setSelectionModel() const;
	void setSelectionModel(QItemSelectionModel* selectionModel);

signals:
	void contextMenuRequested(const QModelIndex& index);

protected:
	bool eventFilter(QObject* o, QEvent* e);
	void contextMenuEvent(QContextMenuEvent* cme);

private slots:
	void resizeTiles(int offset);
	void updateSliderStatus(bool reset = false);

private:
	void init();

	MvdSmartViewDelegate* delegate;
	qreal currentAspectRatio;
	qreal defaultAspectRatio;

	int minimumSize;
	int maximumSize;
};

#endif // MVD_SMARTVIEW_H
