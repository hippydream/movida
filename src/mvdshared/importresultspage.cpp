/**************************************************************************
** Filename: importresultspage.cpp
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

#include "importresultspage.h"
#include "importdialog.h"
#include "core.h"
#include <QLabel>
#include <QTreeWidget>
#include <QGridLayout>
#include <QPushButton>
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
: MvdImportPage(parent), matchId(0), lastSelectedMatches(0), locked(false)
{
	setTitle(tr("Search results"));
	setSubTitle(tr("Please select the items you want to import."));

	QGridLayout* gridLayout = new QGridLayout(this);
	
	results = new QTreeWidget(this);
	results->setHeaderLabels(QStringList() << tr("Title") << tr("Year"));
	results->setRootIsDecorated(false);
	
	results->header()->setResizeMode(0, QHeaderView::Stretch);
	results->header()->setStretchLastSection(false);

	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	sizePolicy.setHorizontalStretch(5);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(results->sizePolicy().hasHeightForWidth());
	results->setSizePolicy(sizePolicy);
	
	gridLayout->addWidget(results, 0, 0, 1, 1);
	
	infoLabel = new QLabel(this);
	infoLabel->setWordWrap(true);

	sizePolicy = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	sizePolicy.setHorizontalStretch(2);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(infoLabel->sizePolicy().hasHeightForWidth());
	infoLabel->setSizePolicy(sizePolicy);
	// infoLabel->setMargin(10);
	
	gridLayout->addWidget(infoLabel, 1, 0, 1, 1);

	connect( results, SIGNAL(itemSelectionChanged()), 
		this, SLOT(resultsSelectionChanged()) );
	connect( results, SIGNAL(itemChanged(QTreeWidgetItem*, int)), 
		this, SLOT(resultsCheckStateChanged()) );

	// Register fields
	registerField("resultsCount", this, "resultsCount", "resultsCountChanged");
	registerField("selectedResultsCount", this, "selectedResultsCount", "selectedResultsCountChanged");
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
	if (locked && !busy)
	{
		setLock(false);
		resultsSelectionChanged();
	}

	MvdImportPage::setBusyStatus(busy);
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
	results->clear();
	results->setRootIsDecorated(false);

	// Wait until the search is done.
	setBusyStatus(true);
	setLock(true);
}

//! This method is called when the user hits the "back" button.
void MvdImportResultsPage::cleanupPage()
{
	results->clear();
	results->setRootIsDecorated(false);

	// Update status message
	resultsSelectionChanged();

	matchId = 0;
	lastSelectedMatches = 0;
}

//! Override.
void MvdImportResultsPage::showMessage(const QString& msg, MvdImportDialog::MessageType t)
{
	Q_UNUSED(t);
	infoLabel->setText(msg);
}

/*!
	See MvdImportDialog::addMatch(const QString&)
*/
int MvdImportResultsPage::addMatch(const QString& title, const QString& year, const QString& notes)
{
	// Get current section (if any)
	QTreeWidgetItem* sectionItem = 0;
	for (int i = results->topLevelItemCount() - 1; i >= 0 && !sectionItem; --i)
	{
		QTreeWidgetItem* item = results->topLevelItem(i);
		ItemType type = ItemType(item->data(0, ItemTypeRole).toUInt());
		if (type == SectionItem)
			sectionItem = item;
	}
	if (sectionItem)
	{
		for (int i = sectionItem->childCount() - 1; i >= 0; --i)
		{
			QTreeWidgetItem* item = sectionItem->child(i);
			ItemType type = ItemType(item->data(0, ItemTypeRole).toUInt());
			if (type == SectionItem)
			{
				sectionItem = item;
				break;
			}
		}
	}

	int id = matchId++;

	QTreeWidgetItem* item = sectionItem ? 
		new QTreeWidgetItem(sectionItem) : new QTreeWidgetItem(results);
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
void MvdImportResultsPage::addSection(const QString& title, const QString& notes)
{
	QTreeWidgetItem* item = new QTreeWidgetItem(results);
	// Expand the first top-level section only
	if (countSections() == 0) {
		results->expandItem(item);
		results->setRootIsDecorated(true);
	}
	item->setFirstColumnSpanned(true);

	QFont font = item->font(0);
	font.setBold(true);
	item->setFont(0, font);
	
	item->setData(0, ItemTypeRole, quint32(SectionItem));

	if (title.isEmpty())
	{
		int topC = countSections();
		QString autoTitle = topC > 1 ? 
			tr("Results group %1").arg(topC) : tr("Main group");
		item->setText(0, autoTitle);
	}
	else item->setText(0, title);

	if (!notes.isEmpty())
		item->setData(0, NotesRole, notes);
}

/*!
	See MvdImportDialog::addSubSection(const QString&, const QString&)
*/
void MvdImportResultsPage::addSubSection(const QString& title, const QString& notes)
{
	// Get current top level section
	QTreeWidgetItem* sectionItem = 0;
	int topLevelItemIndex = 0;
	for (int i = results->topLevelItemCount() - 1; i >= 0 && !sectionItem; --i)
	{
		QTreeWidgetItem* item = results->topLevelItem(i);
		ItemType type = ItemType(item->data(0, ItemTypeRole).toUInt());
		if (type == SectionItem)
		{
			sectionItem = item;
			topLevelItemIndex = i;
		}
	}

	if (!sectionItem)
	{
		sectionItem = new QTreeWidgetItem(results);
		// Expand the first top-level section only
		results->expandItem(sectionItem);
		QFont font = sectionItem->font(0);
		font.setBold(true);
		sectionItem->setFont(0, font);
		sectionItem->setFirstColumnSpanned(true);
		sectionItem->setData(0, ItemTypeRole, quint32(SectionItem));
		sectionItem->setText(0, tr("Main group"));
	}

	QTreeWidgetItem* item = new QTreeWidgetItem(sectionItem);
	// Expand the first top-level section only
	if (topLevelItemIndex == 0)
		results->expandItem(item);
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
	for (int i = 0; i < results->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* item = results->topLevelItem(i);
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
	for (int i = 0; i < (section ? section->childCount() : results->topLevelItemCount()); ++i)
	{
		QTreeWidgetItem* item = section ? section->child(i) : results->topLevelItem(i);
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

	QList<QTreeWidgetItem*> list = results->selectedItems();
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
			showMessage(tr("No matches found."), MvdImportDialog::InfoMessage);
		else if (selectedMatches == 0)
			showMessage(tr("%1 match(es) found. None have been selected for import.", "Found results", matches).arg(matches), MvdImportDialog::InfoMessage);
		else if (selectedMatches == matches)
			showMessage(tr("%1 match(es) found and selected for import.", "Found results.", matches).arg(matches), MvdImportDialog::InfoMessage);
		else showMessage(tr("%1 of %2 matches have been selected for import.", "Found results. # of selected messages is given for plural form.", selectedMatches).arg(selectedMatches).arg(matches), MvdImportDialog::InfoMessage);
	}
	else {
		showMessage(text, MvdImportDialog::InfoMessage);
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
	QList<QTreeWidgetItem*> list = results->selectedItems();
	if (list.isEmpty())
		return;
	results->scrollToItem(list.at(0));
}

//! Returns a list containing the requested jobs.
QList<int> MvdImportResultsPage::jobs() const
{
	QList<int> list;
	for (int i = 0; i < results->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* topItem = results->topLevelItem(i);
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
