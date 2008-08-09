/**************************************************************************
** Filename: collectionmetaeditor.cpp
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

#include "collectionmetaeditor.h"
#include "mvdcore/moviecollection.h"
#include <QCloseEvent>
#include <QKeyEvent>
#include <QMessageBox>

MvdCollectionMetaEditor::MvdCollectionMetaEditor(QWidget* parent)
: QDialog(parent), mCollection(0)
{
	setupUi(this);
	setWindowTitle(tr("Collection properties"));
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

	if (isModified()) {
		mCollection->setMetaData(MvdMovieCollection::NameInfo, name->text().trimmed());
		mCollection->setMetaData(MvdMovieCollection::OwnerInfo, owner->text().trimmed());
		mCollection->setMetaData(MvdMovieCollection::EMailInfo, mail->text().trimmed());
		mCollection->setMetaData(MvdMovieCollection::WebsiteInfo, website->text().trimmed());
		mCollection->setMetaData(MvdMovieCollection::NotesInfo, notes->toPlainText().trimmed());
	}

	QDialog::accept();
}

void MvdCollectionMetaEditor::reject()
{
	setResult(QDialog::Rejected);
	close();
}

void MvdCollectionMetaEditor::keyPressEvent(QKeyEvent* e)
{
	switch (e->key())
	{
	case Qt::Key_Escape:
		close();
		e->ignore();
		return;
	}

	QDialog::keyPressEvent(e);
}

void MvdCollectionMetaEditor::closeEvent(QCloseEvent* e)
{
	if (isModified()) {
		if (QMessageBox::question(this, MVD_CAPTION, tr("The collection information has been modified. Are you sure you want to discard the changes?"), 
			QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes)
			e->ignore();
			return;
	}

	QDialog::close();
}

//!
bool MvdCollectionMetaEditor::isModified() const
{
	if (!mCollection)
		return false;

	return
		mCollection->metaData(MvdMovieCollection::NameInfo) != name->text().trimmed() ||
		mCollection->metaData(MvdMovieCollection::OwnerInfo) != owner->text().trimmed() ||
		mCollection->metaData(MvdMovieCollection::EMailInfo) != mail->text().trimmed() ||
		mCollection->metaData(MvdMovieCollection::WebsiteInfo) != website->text().trimmed() ||
		mCollection->metaData(MvdMovieCollection::NotesInfo) != notes->toPlainText().trimmed();
}
