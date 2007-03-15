/**************************************************************************
** Filename: importdialog.cpp
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

#include "importdialog.h"
#include "labelanimator.h"

#include <QPushButton>

namespace {
	static const int ResultUuidRole = Qt::UserRole + 1;
}

MvdwImportDialog::MvdwImportDialog(QWidget* parent)
: QDialog(parent)
{
	setupUi(this);

	QStringList frames;
	for (int i = 1; i <= 8; ++i)
		frames << QString(":/images/loading_p%1.png").arg(i);
	labelAnimator = new MvdwLabelAnimator(frames, loadingIconLabel, this);

	resultsWidget->setHeaderLabels(QStringList() << tr("Matching movies"));

	// Setup buttons
	QPushButton* closeButton = startButtonBox->addButton(QDialogButtonBox::Close);
	connect( closeButton, SIGNAL(clicked()), this, SLOT(close()) );
	
	searchButton = startButtonBox->addButton(tr("Search"), QDialogButtonBox::ActionRole);
	connect( searchButton, SIGNAL(clicked()), this, SIGNAL(searchTriggered()) );
	searchButton->setEnabled(false);	
	
	closeButton = importButtonBox->addButton(QDialogButtonBox::Close);
	connect( closeButton, SIGNAL(clicked()), this, SLOT(close()) );

	importButton = importButtonBox->addButton(tr("Import"), QDialogButtonBox::ActionRole);
	connect( importButton, SIGNAL(clicked()), this, SIGNAL(importTriggered()) );
	importButton->setEnabled(false);

	backButton = importButtonBox->addButton(tr("New search"), QDialogButtonBox::ActionRole);
	connect( backButton, SIGNAL(clicked()), this, SIGNAL(showStartPageTriggered()) );
	backButton->setEnabled(false);

	connect( resultsWidget, SIGNAL(itemActivated(QTreeWidgetItem*, int)), 
		this, SIGNAL(resultsSelectionChanged()) );
}

void MvdwImportDialog::setSearchButtonEnabled(bool enabled)
{
	searchButton->setEnabled(enabled);
}

void MvdwImportDialog::setBackButtonEnabled(bool enabled)
{
	backButton->setEnabled(enabled);
}

void MvdwImportDialog::setImportButtonEnabled(bool enabled)
{
	importButton->setEnabled(enabled);
}

QWidget* MvdwImportDialog::startPage()
{
	return Ui::MvdwImportDialog::startPageFrame;
}

/*! Clears the results and shows the start page. */
void MvdwImportDialog::showStartPage()
{
	clearResults();
	mainStack->setCurrentIndex(0);
}

void MvdwImportDialog::showImportPage()
{
	mainStack->setCurrentIndex(1);
}

void MvdwImportDialog::setStatus(const QString& s)
{
	loadingLabel->setText(s);
}

void MvdwImportDialog::setBusyStatus(bool busy)
{
	labelAnimator->setPixmapVisible(busy);
}

QUuid MvdwImportDialog::addSearchResult(const QString& displayString)
{
	if (displayString.isEmpty())
		return QUuid();

	QUuid uuid = QUuid::createUuid();
	QTreeWidgetItem* item = new QTreeWidgetItem(resultsWidget);
	item->setText(0, displayString);
	item->setData(0, ResultUuidRole, uuid.toString());
	item->setCheckState(0, Qt::Unchecked);
	return uuid;
}

//! Clears the search results.
void MvdwImportDialog::clearResults()
{
	resultsWidget->clear();
}

//! Updates the UI after some result has been selected or deselected.
void MvdwImportDialog::resultsSelectionChanged()
{
	bool uiChecked = importButton->isEnabled();
	for (int i = 0; i < resultsWidget->topLevelItemCount(); ++i)
	{
		// Stop as soon as we find out that the current UI status is not valid
		bool itemChecked = 
			resultsWidget->topLevelItem(i)->checkState(0) == Qt::Checked;
		if (itemChecked != uiChecked)
		{
			importButton->setEnabled(itemChecked);
			break;
		}
	}
}
