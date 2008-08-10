/**************************************************************************
** Filename: importresultspage.cpp
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

#include "importdialog.h"
#include "importresultspage.h"
#include "mvdcore/core.h"
#include <QHeaderView>
#include <math.h>

/*!
	\class MvdImportResultsPage importresultspage.h
	\ingroup MovidaShared

	\brief This page shows the movies matching the search query.
*/

//! \todo Move to sharedglobal.h
namespace MovidaShared
{
	enum {
		ItemTypeRole = Qt::UserRole + 1,
		JobIdRole,
		NotesRole
	};
};
using namespace MovidaShared;

/*!
	This Import Wizard page shows search results and allows the user to
	select what to import.
*/
MvdImportResultsPage::MvdImportResultsPage(QWidget* parent)
: MvdImportExportPage(parent), matchId(0), lastSelectedMatches(0), locked(false), currentSection(0)
{
	setTitle(tr("Search results"));
	setSubTitle(tr("Please select the items you want to import.\nYou can confirm each single import after viewing all the movie details in the next page."));
	setCommitPage(true);

	ui.setupUi(this);
	
	ui.results->setHeaderLabels(QStringList() << tr("Title") << tr("Year"));
	ui.results->header()->setResizeMode(0, QHeaderView::Stretch);
	ui.results->header()->setStretchLastSection(false);

	connect( ui.results, SIGNAL(itemSelectionChanged()), 
		this, SLOT(resultsSelectionChanged()) );
	connect( ui.results, SIGNAL(itemChanged(QTreeWidgetItem*, int)), 
		this, SLOT(resultsCheckStateChanged()) );

	// Register fields
	registerField("resultsCount", this, "resultsCount", SIGNAL("resultsCountChanged()"));
	registerField("selectedResultsCount", this, "selectedResultsCount", SIGNAL("selectedResultsCountChanged()"));
}

//! Returns the current number of found results.
int MvdImportResultsPage::resultsCount() const
{
	return countMatches();
}

//! Returns the current number of selected results.
int MvdImportResultsPage::selectedResultsCount() const
{
	int s;
	(void)countMatches(&s);
	return s;
}

//! Override. Unlocks the UI if it was locked.
void MvdImportResultsPage::setBusyStatus(bool busy)
{
	MvdImportExportPage::setBusyStatus(busy);

	if (locked && !busy) {
		setLock(false);
		resultsSelectionChanged();
	}

	if (!busy && ui.results->topLevelItemCount() == 0)
		wizard()->next();

	ui.stack->setCurrentIndex(busy ? 0 : 1);
}

//! Locks or unlocks (part of) the GUI.
void MvdImportResultsPage::setLock(bool lock)
{
	if (lock == locked)
		return;
	locked = lock;
}

//! Initialize page each time it is shown.
void MvdImportResultsPage::initializePage()
{
	ui.results->clear();
	ui.results->setRootIsDecorated(false);
	currentSection = 0;

	// Wait until the search is done.
	setBusyStatus(true);
	setLock(true);
}

//! This method is called when the user hits the "back" button.
void MvdImportResultsPage::cleanupPage()
{
	ui.results->clear();
	ui.results->setRootIsDecorated(false);
	currentSection = 0;

	// Update status message
	resultsSelectionChanged();

	matchId = 0;
	lastSelectedMatches = 0;
}

//! Override.
void MvdImportResultsPage::showMessage(const QString& msg, MovidaShared::MessageType t)
{
	Q_UNUSED(t);
	if (ui.stack->currentIndex() == 1)
		ui.infoLabel->setText(msg);
	else ui.stackedInfoLabel->setText(msg);
}

/*!
	See MvdImportDialog::addMatch(const QString&)
*/
int MvdImportResultsPage::addMatch(QString title, const QString& year, const QString& notes)
{
	title = title.trimmed();

	QTreeWidgetItem* sectionItem = currentSection;
	int id = matchId++;

	QTreeWidgetItem* item = sectionItem ? 
		new QTreeWidgetItem(sectionItem) : new QTreeWidgetItem(ui.results);
	item->setData(0, ItemTypeRole, quint32(StandardItem));
	item->setData(0, JobIdRole, id);
	item->setText(0, title);
	item->setText(1, year);
	if (!notes.isEmpty())
		item->setData(0, NotesRole, notes);
	item->setCheckState(0, Qt::Unchecked);

	emit resultsCountChanged();
	emit resultsCountChanged(matchId);

	return id;
}

/*!
	See MvdImportDialog::addSection(const QString&, const QString&)
*/
void MvdImportResultsPage::addSection(QString title, const QString& notes)
{
	title = title.trimmed();

	// Look if the section already exists
	QTreeWidgetItem* sectionItem = 0;
	int topLevelItemIndex = 0;
	for (int i = 0; i < ui.results->topLevelItemCount() && !sectionItem; ++i) {
		QTreeWidgetItem* item = ui.results->topLevelItem(i);
		ItemType type = ItemType(item->data(0, ItemTypeRole).toUInt());
		if (type == SectionItem && item->text(0) == title) {
			sectionItem = item;
			topLevelItemIndex = i;
		}
	}

	if (sectionItem) {
		currentSection = sectionItem;
		return;
	}

	sectionItem = new QTreeWidgetItem(ui.results);
	currentSection = sectionItem;

	// Expand the first top-level section only
	if (countSections() == 0) {
		ui.results->expandItem(sectionItem);
		ui.results->setRootIsDecorated(true);
	}
	sectionItem->setFirstColumnSpanned(true);

	QFont font = sectionItem->font(0);
	font.setBold(true);
	sectionItem->setFont(0, font);
	
	sectionItem->setData(0, ItemTypeRole, quint32(SectionItem));

	if (title.isEmpty())
	{
		int topC = countSections();
		QString autoTitle = topC > 1 ? 
			tr("Results group %1").arg(topC) : tr("Main group");
		sectionItem->setText(0, autoTitle);
	}
	else sectionItem->setText(0, title);

	if (!notes.isEmpty())
		sectionItem->setData(0, NotesRole, notes);
}

/*!
	See MvdImportDialog::addSubSection(const QString&, const QString&)
*/
void MvdImportResultsPage::addSubSection(QString title, const QString& notes)
{
	title = title.trimmed();

	// Get current top level section
	QTreeWidgetItem* sectionItem = currentSection;

	if (!sectionItem) {
		sectionItem = new QTreeWidgetItem(ui.results);
		// Expand the first top-level section only
		ui.results->expandItem(sectionItem);
		QFont font = sectionItem->font(0);
		font.setBold(true);
		sectionItem->setFont(0, font);
		sectionItem->setFirstColumnSpanned(true);
		sectionItem->setData(0, ItemTypeRole, quint32(SectionItem));
		sectionItem->setText(0, tr("Main group"));
	} else {
		// Check if another section with same title exists
		for (int i = 0; i < sectionItem->childCount(); ++i) {
			QTreeWidgetItem* item = sectionItem->child(i);
			ItemType type = ItemType(item->data(0, ItemTypeRole).toUInt());
			if (type == SectionItem && item->text(0) == title) {
				currentSection = item;
				return;
			}
		}
	}

	QTreeWidgetItem* item = new QTreeWidgetItem(sectionItem);
	currentSection = item;

	// Expand the first top-level section only
	if (ui.results->topLevelItemCount() == 0)
		ui.results->expandItem(item);
	item->setFirstColumnSpanned(true);

	QFont font = item->font(0);
	font.setBold(true);
	item->setFont(0, font);
	
	item->setData(0, ItemTypeRole, quint32(SectionItem));

	if (title.isEmpty())
	{
		int topC = countSections(sectionItem);
		QString autoTitle = topC > 1 ? 
			tr("Results sub-group %1").arg(topC) : tr("Main results");
		item->setText(0, autoTitle);
	}
	else item->setText(0, title);

	if (!notes.isEmpty())
		item->setData(0, NotesRole, notes);
}

int MvdImportResultsPage::countMatches(int* selected) const
{
	int c = 0;
	int s = 0;
	for (int i = 0; i < ui.results->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = ui.results->topLevelItem(i);
		ItemType type = ItemType(item->data(0, ItemTypeRole).toUInt());
		if (type == SectionItem)
		{
			for (int j = 0; j < item->childCount(); ++j)
			{
				QTreeWidgetItem* subItem = item->child(j);
				ItemType subType = ItemType(subItem->data(0, ItemTypeRole).toUInt());
				if (subType == SectionItem)
					c += subItem->childCount();
				else {
					c++;
					if (selected && subItem->checkState(0) == Qt::Checked)
						s++;
				}
			}
		}
		else {
			c++;
			if (selected && item->checkState(0) == Qt::Checked)
				s++;
		}
	}
	
	if (selected)
		*selected = s;

	return c;
}

/*!
	\internal Returns the number of sub sections.
	\p if section is 0 this is the number of top level sections.
*/
int MvdImportResultsPage::countSections(const QTreeWidgetItem* section) const
{
	int c = 0;
	for (int i = 0; i < (section ? section->childCount() : ui.results->topLevelItemCount()); ++i)
	{
		QTreeWidgetItem* item = section ? section->child(i) : ui.results->topLevelItem(i);
		ItemType type = ItemType(item->data(0, ItemTypeRole).toUInt());
		if (type == SectionItem)
			c++;
	}
	return c;
}

//! \internal Updates the status according to the currently selected search result.
void MvdImportResultsPage::resultsSelectionChanged()
{
	if (busyStatus())
		return;

	int selectedMatches;
	int matches = countMatches(&selectedMatches);

	bool showItemCount = false;
	QString text;

	QList<QTreeWidgetItem*> list = ui.results->selectedItems();
	if (list.isEmpty())
		showItemCount = true;
	else
	{
		const QTreeWidgetItem* item = list.at(0);
		ItemType type = ItemType(item->data(0, ItemTypeRole).toUInt());
		if (type == StandardItem)
		{
			text = item->data(0, NotesRole).toString();
			showItemCount = text.isEmpty();
		}
		else showItemCount = true;
	}

	if (showItemCount)
	{
		if (matches == 0)
			showMessage(tr("No matches found."), MovidaShared::InfoMessage);
		else if (selectedMatches == 0)
			showMessage(tr("%1 match(es) found. None have been selected for import.", "Found results", matches).arg(matches), MovidaShared::InfoMessage);
		else if (selectedMatches == matches)
			showMessage(tr("%1 match(es) found and selected for import.", "Found results.", matches).arg(matches), MovidaShared::InfoMessage);
		else showMessage(tr("%1 of %2 matches have been selected for import.", "Found results. # of selected messages is given for plural form.", selectedMatches).arg(selectedMatches).arg(matches), MovidaShared::InfoMessage);
	}
	else {
		showMessage(text, MovidaShared::InfoMessage);
		Q_ASSERT(QMetaObject::invokeMethod(this, "ensureItemVisible", Qt::QueuedConnection));
	}

	if (selectedMatches != lastSelectedMatches) {
		lastSelectedMatches = selectedMatches;
		emit selectedResultsCountChanged();
		emit selectedResultsCountChanged(selectedMatches);
	}
}

//! \internal Ensures the selected item is visible in the results view.
void MvdImportResultsPage::ensureItemVisible()
{
	QList<QTreeWidgetItem*> list = ui.results->selectedItems();
	if (list.isEmpty())
		return;
	ui.results->scrollToItem(list.at(0));
}

//! Returns a list containing the requested jobs.
QList<int> MvdImportResultsPage::jobs() const
{
	QList<int> list;
	for (int i = 0; i <ui. results->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* topItem = ui.results->topLevelItem(i);
		ItemType topType = ItemType(topItem->data(0, ItemTypeRole).toUInt());
 		if (topType == SectionItem)
		{
			for (int j = 0; j < topItem->childCount(); ++j)
			{
				QTreeWidgetItem* subItem = topItem->child(j);
				ItemType subType = ItemType(subItem->data(0, ItemTypeRole).toUInt());
				if (subType == SectionItem)
				{
					for (int k = 0; k < subItem->childCount(); ++k)
					{
						QTreeWidgetItem* item = subItem->child(k);
						ItemType type = ItemType(item->data(0, ItemTypeRole).toUInt());
						if (type == StandardItem && item->checkState(0) == Qt::Checked)
							list << item->data(0, JobIdRole).toInt();
					}
				}
				else if (subItem->checkState(0) == Qt::Checked)
					list << subItem->data(0, JobIdRole).toInt();
			}
		}
		else if (topItem->checkState(0) == Qt::Checked)
			list << topItem->data(0, JobIdRole).toInt();
	}

	return list;
}

//! \internal
void MvdImportResultsPage::resultsCheckStateChanged()
{
	// Update selected item count
	resultsSelectionChanged();
}

//! Sets the current progress.
void MvdImportResultsPage::setProgress(int v)
{
	ui.progressBar->setValue(v);
}

//! Sets the maximum progress value.
void MvdImportResultsPage::setProgressMaximum(int m)
{
	ui.progressBar->setMaximum(m);
}

//! Returns the current progress value.
int MvdImportResultsPage::progress() const
{
	return ui.progressBar->value();
}
