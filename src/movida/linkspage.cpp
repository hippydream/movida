/**************************************************************************
** Filename: linkspage.cpp
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

#include "linkspage.h"
#include "guiglobal.h"
#include "movie.h"
#include "core.h"
#include "itemdelegate.h"
#include <QIcon>
#include <QRegExp>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QHeaderView>
#include <QMenu>
#include <QShortcut>

/*!
	\class MvdLinksPage linkspage.h
	\ingroup Movida

	\brief Link editing widget for the movie editor dialog.
	\todo ensure no duplicates can be added
*/

/*!
	Creates a new page.
*/
MvdLinksPage::MvdLinksPage(MvdMovieCollection* c, MvdMovieEditor* parent)
: MvdMovieEditorPage(c, parent), mLocked(false)
{
	setupUi(this);

	linksWidget->setItemDelegate(new MvdItemDelegate(this));
	linksWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
	linksWidget->setHeaderLabels(QStringList() << tr("URL") << tr("Description"));
	linksWidget->header()->resizeSection(0, linksWidget->header()->defaultSectionSize() * 2);

	connect(imdbInput, SIGNAL(textChanged(QString)), this, SLOT(imdbIdChanged(QString)));
	connect(imdbButton, SIGNAL(clicked()), this, SLOT(openImdbPage()));

	connect(linksWidget, SIGNAL(contextMenuRequested(QTreeWidgetItem*, int)), this, SLOT(contextMenuRequested(QTreeWidgetItem*, int)));
	connect(linksWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(itemChanged(QTreeWidgetItem*, int)));

	connect(imdbInput, SIGNAL(textEdited(QString)), this, SLOT(updateModifiedStatus()));
	connect(linksWidget->model(), SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(updateModifiedStatus()));
	connect(linksWidget->model(), SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(updateModifiedStatus()));

	mLocked = true;
	appendPlaceHolder();
	mLocked = false;

	QShortcut* deleteUrlSC = new QShortcut(QKeySequence(tr("Del", "Delete selected URLs")), this);
	connect(deleteUrlSC, SIGNAL(activated()), this, SLOT(deleteSelectedUrls()));
	QShortcut* openUrlSC = new QShortcut(QKeySequence(tr("Ctrl+O", "Open selected URLs")), this);
	connect(openUrlSC, SIGNAL(activated()), this, SLOT(openSelectedUrls()));
}

/*!
	Returns the title to be used for this page.
*/
QString MvdLinksPage::label()
{
	return tr("Links");
}

/*!
	Returns the icon to be used for this page.
*/
QIcon MvdLinksPage::icon()
{
	return QIcon(":/images/preferences/log.png");
}

void MvdLinksPage::setMovieImpl(const MvdMovie& movie)
{
	mLocked = true;

	linksWidget->clear();

	bool hasDefault = false;

	mDefaultUrls = movie.urls();
	for (int i = 0; i < mDefaultUrls.size(); ++i)
	{
		const MvdUrl& url = mDefaultUrls.at(i);
		MvdTreeWidgetItem* item = new MvdTreeWidgetItem(linksWidget);
		item->setText(0, url.url);
		item->setText(1, url.description);
		if (hasDefault)
			item->setCheckState(0, Qt::Unchecked);
		else 
		{
			item->setCheckState(0, url.isDefault ? Qt::Checked : Qt::Unchecked);
			hasDefault = url.isDefault;
		}
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		item->setData(0, Movida::ValidationRole, (quint16) Movida::UndoEmptyValitator);
	}

	// This will possibly remove multiple urls with "default" set to true.
	mDefaultUrls = urls();

	appendPlaceHolder();

	mDefaultImdbId = movie.imdbId();
	imdbInput->setText(mDefaultImdbId);

	mLocked = false;
}

void MvdLinksPage::setMoviesImpl(const QList<MvdMovie>& movies)
{
}

/*!
	Enables or disables the "Visit IMDb page" button when the IMDb id changes.
*/
void MvdLinksPage::imdbIdChanged(const QString& text)
{
	QString pattern = MvdCore::parameter("mvdp://mvdcore/imdb-id-regexp").toString();
	QRegExp rx(pattern);

	imdbButton->setEnabled(rx.exactMatch(text));
}

/*!
	Attempts to open the movies IMDb page in the system's default browser.
*/
void MvdLinksPage::openImdbPage()
{
	QString url = MvdCore::parameter("mvdp://movida/imdb-movie-url").toString();
	QDesktopServices::openUrl(url.arg(imdbInput->text()));
}

bool MvdLinksPage::store(MvdMovie& movie)
{
	//! \todo Consider doing some basic URL validation here.
	movie.setUrls(urls());

	QString pattern = MvdCore::parameter("mvdp://mvdcore/imdb-id-regexp").toString();
	QRegExp rx(pattern);
	QString imdb = imdbInput->text();

	if (imdb.isEmpty())
		return true;

	if (rx.exactMatch(imdb))
		movie.setImdbId(imdb);
	else
	{
		int res = QMessageBox::question(this, MVD_CAPTION, tr("The IMDb id is invalid. Do you want to ignore it?"),
			QMessageBox::Yes, QMessageBox::No);
		if (res != QMessageBox::Yes)
			return false;
	}

	return true;
}

void MvdLinksPage::contextMenuRequested(QTreeWidgetItem* item, int column)
{
	QMenu menu;
	QAction* deleteUrlAction = 0, * openUrlAction = 0;

	QList<QTreeWidgetItem*> sel = linksWidget->filteredSelectedItems();

	if (!sel.isEmpty())
	{
		openUrlAction = menu.addAction(tr("&Open selected URL(s)", "", sel.count()), this, SLOT(openSelectedUrls()));
		openUrlAction->setShortcut(QKeySequence(tr("Ctrl+O", "Open selected URLs")));
		deleteUrlAction = menu.addAction(tr("&Delete selected URL(s)", "", sel.count()), this, SLOT(deleteSelectedUrls()));
		deleteUrlAction->setShortcut(QKeySequence(tr("Del", "Delete selected URLs")));
	}
	else return;

	menu.exec(QCursor::pos());
}

void MvdLinksPage::itemChanged(QTreeWidgetItem* item, int column)
{
	if (mLocked || !item)
		return;

	mLocked = true;

	if (column == 0 && item->checkState(0) == Qt::Checked)
	{
		for (int i = 0; i < linksWidget->topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* item2 = linksWidget->topLevelItem(i);
			if (item2 != item)
				item2->setCheckState(0, Qt::Unchecked);
		}
	}

	QVariant v = item->data(0, Movida::PlaceholderRole);
	bool isPlaceHolder = !v.isNull() && v.toBool();

	if (isPlaceHolder)
	{
		QString url = item->text(0).trimmed();
		QString description = item->text(1).trimmed();
		if ((column != 0 && description.isEmpty()) || url.isEmpty())
		{
			// Undo changes and restore placeholder
			item->setText(0, tr("New link"));
			item->setText(1, QString());
		}
		else
		{
			if (column != 0)
				item->setText(0, "http://");

			item->setData(0, Movida::PlaceholderRole, QVariant());
			item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
			
			QVariant c = item->data(0, Movida::TextColorBackupRole);
			item->setData(0, Movida::TextColorBackupRole, QVariant());
			item->setData(0, Qt::TextColorRole, c);
			c = item->data(1, Movida::TextColorBackupRole);
			item->setData(1, Movida::TextColorBackupRole, QVariant());
			item->setData(1, Qt::TextColorRole, c);

			QVariant f = item->data(0, Movida::FontBackupRole);
			item->setData(0, Movida::FontBackupRole, QVariant());
			item->setFont(0, qVariantValue<QFont>(f));
			f = item->data(1, Movida::FontBackupRole);
			item->setData(1, Movida::FontBackupRole, QVariant());
			item->setFont(1, qVariantValue<QFont>(f));

			appendPlaceHolder();
		}
	}

	mLocked = false;
}

void MvdLinksPage::appendPlaceHolder()
{
	MvdTreeWidgetItem* item = new MvdTreeWidgetItem(linksWidget);
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	item->setCheckState(0, Qt::Unchecked);
	item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
	
	QFont f = item->font(0);
	item->setData(0, Movida::FontBackupRole, f);
	f.setItalic(true);
	item->setFont(0, f);

	f = item->font(1);
	item->setData(1, Movida::FontBackupRole, f);
	f.setItalic(true);
	item->setFont(1, f);

	item->setText(0, tr("New link"));
	item->setData(0, Movida::PlaceholderRole, true);

	item->setData(0, Movida::TextColorBackupRole, item->data(0, Qt::TextColorRole));
	item->setData(0, Qt::TextColorRole, qVariantFromValue<QColor>(QColor("#585858")));
	item->setData(1, Movida::TextColorBackupRole, item->data(1, Qt::TextColorRole));
	item->setData(1, Qt::TextColorRole, qVariantFromValue<QColor>(QColor("#585858")));

	item->setData(0, Movida::ValidationRole, (quint16) Movida::UndoEmptyValitator);
}

QList<MvdUrl> MvdLinksPage::urls() const
{
	QList<MvdUrl> list;

	for (int i = 0; i < linksWidget->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = linksWidget->topLevelItem(i);

		QVariant v = item->data(0, Movida::PlaceholderRole);
		if (v.isNull() || !v.toBool())
		{
			MvdUrl url(item->text(0), item->text(1), item->checkState(0) == Qt::Checked);
			list.append(url);
		}
	}

	return list;
}

//!
void MvdLinksPage::updateModifiedStatus()
{
	if (mDefaultImdbId != imdbInput->text().trimmed())
	{
		setModified(true);
		return;
	}

	QList<MvdUrl> currentUrls = urls();

	if (mDefaultUrls.size() != currentUrls.size())
	{
		setModified(true);
		return;
	}
	
	QList<MvdUrl> defaultUrls(mDefaultUrls);

	qSort(defaultUrls);
	qSort(currentUrls);

	for (int i = 0; i < currentUrls.size(); ++i)
	{
		if (!currentUrls.at(i).exactMatch(defaultUrls.at(i)))
		{
			setModified(true);
			return;
		}
	}

	setModified(false);
}

//!
void MvdLinksPage::deleteSelectedUrls()
{
	QList<QTreeWidgetItem*> sel = linksWidget->filteredSelectedItems();
	qDeleteAll(sel);
}

//!
void MvdLinksPage::openSelectedUrls()
{
	QList<QTreeWidgetItem*> sel = linksWidget->filteredSelectedItems();
	for (int i = 0; i < sel.size(); ++i)
	{
		QDesktopServices::openUrl(sel.at(i)->text(0));
	}
}
