/**************************************************************************
** Filename: importstartpage.cpp
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

#include "importstartpage.h"
#include "queryvalidator.h"

#include <QLabel>
#include <QComboBox>
#include <QGridLayout>
#include <QLineEdit>

MvdwImportStartPage::MvdwImportStartPage(const QList<MvdwSearchEngine>& _engines, QWidget* parent)
: QWizardPage(parent), engines(_engines)
{
	setTitle(tr("Movida Internet import wizard"));
	setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/import/watermark.png"));

	infoLabel = new QLabel;
	infoLabel->setWordWrap(true);
	
	engineCombo = engines.size() < 2 ? 0 : new QComboBox;
	queryInput = new QLineEdit;

	if (engineCombo)
		infoLabel->setText(tr("Please select a search type, enter your query and hit the Search button (or press Enter) to start the search."));
	else infoLabel->setText(tr("Please enter your query and hit the Search button (or press Enter) to start the search."));

	QGridLayout* gridLayout = new QGridLayout(this);
	gridLayout->addWidget(infoLabel, 0, 0, 1, 2);

	gridLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed), 1, 1, 1, 1);

	QGridLayout* queryGridLayout = new QGridLayout();
	if (engineCombo)
	{
		QLabel* engineLabel = new QLabel(this);
		engineLabel->setText(tr("Search type:"));
		queryGridLayout->addWidget(engineLabel, 0, 0, 1, 1);
		queryGridLayout->addWidget(engineCombo, 0, 1, 1, 1);
		queryGridLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed), 1, 1, 1, 1);

		// Extract engine names
		QStringList engineNames;
		foreach (MvdwSearchEngine e, engines)
			engineNames.append(e.name);
		engineCombo->addItems(engineNames);

		// Setup the query validator for the selected engine
		engineChanged();
		connect( engineCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(engineChanged()) );
	}
	else
	{
		// Setup the query validator for the selected engine
		engineChanged();
	}

	QLabel* queryLabel = new QLabel(this);
	queryLabel->setText(tr("Query:"));
	queryGridLayout->addWidget(queryLabel, 2, 0, 1, 1);
	queryGridLayout->addWidget(queryInput, 2, 1, 1, 1);

	gridLayout->addLayout(queryGridLayout, 2, 0, 1, 2);

	gridLayout->addItem(new QSpacerItem(20, 61, QSizePolicy::Minimum, QSizePolicy::Expanding), 3, 0, 1, 1);

	// Register fields
	registerField("query*", queryInput);
	if (engineCombo)
		registerField("engine", engineCombo);
	else registerField("engine", this);
}

//! The info text should show some basic information about the kind of searches the registered engines do.
void MvdwImportStartPage::setInfoText(const QString& s)
{
	infoLabel->setText(s);
}

QString MvdwImportStartPage::engine() const
{
	if (engineCombo) 
		return engineCombo->currentText();

	return engines.isEmpty() ? QString() : engines.at(0).name;
}

void MvdwImportStartPage::engineChanged()
{
	if (engines.isEmpty())
		return;

	QRegExp rx(engineCombo ? engines.at(engineCombo->currentIndex()).validator :
		engines.at(0).validator);
	queryInput->setValidator(new MvdwQueryValidator(this));

	emit currentEngineChanged();
}
