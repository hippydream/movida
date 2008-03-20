/**************************************************************************
** Filename: collectionmetaeditor.h
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

#ifndef MVD_COLLECTIONMETAEDITOR_H
#define MVD_COLLECTIONMETAEDITOR_H

#include "ui_collectionmetaeditor.h"
#include <QDialog>

class MvdMovieCollection;
class QCloseEvent;
class QKeyEvent;

class MvdCollectionMetaEditor : public QDialog, private Ui::MvdCollectionMetaEditor
{
	Q_OBJECT

public:
	MvdCollectionMetaEditor(QWidget* parent = 0);

	void setCollection(MvdMovieCollection* c);
	bool isModified() const;

protected:
	virtual void closeEvent(QCloseEvent* e);
	virtual void keyPressEvent(QKeyEvent* e);

public slots:
	virtual void accept();
	virtual void reject();

private:
	MvdMovieCollection* mCollection;
};

#endif // MVD_COLLECTIONMETAEDITOR_H
