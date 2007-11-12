/**************************************************************************
** Filename: collectionmetaeditor.cpp
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

#include "collectionmetaeditor.h"
#include "moviecollection.h"

MvdCollectionMetaEditor::MvdCollectionMetaEditor(QWidget* parent)
: QDialog(parent), mCollection(0)
{
	setupUi(this);
}

void MvdCollectionMetaEditor::setCollection(MvdMovieCollection* c)
{
	if (!c)
		return;

	mCollection = c;

	name->setText(c->metaData(MvdMovieCollection::NameInfo));
	owner->setText(c->metaData(MvdMovieCollection::OwnerInfo));
	mail->setText(c->metaData(MvdMovieCollection::EMailInfo));
	website->setText(c->metaData(MvdMovieCollection::WebsiteInfo));
	notes->setText(c->metaData(MvdMovieCollection::NotesInfo));
}

void MvdCollectionMetaEditor::accept()
{
	//! \todo validate
	if (!mCollection)
		return;

	mCollection->setMetaData(MvdMovieCollection::NameInfo, name->text().trimmed());
	mCollection->setMetaData(MvdMovieCollection::OwnerInfo, owner->text().trimmed());
	mCollection->setMetaData(MvdMovieCollection::EMailInfo, mail->text().trimmed());
	mCollection->setMetaData(MvdMovieCollection::WebsiteInfo, website->text().trimmed());
	mCollection->setMetaData(MvdMovieCollection::NotesInfo, notes->toPlainText().trimmed());
}
