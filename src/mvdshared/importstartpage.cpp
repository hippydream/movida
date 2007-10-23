/**************************************************************************
** Filename: importstartpage.cpp
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

#include "importstartpage.h"
#include "queryvalidator.h"
#include "settings.h"
#include <QLabel>
#include <QComboBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QCompleter>

/*!
	\class MvdImportStartPage importstartpage.h
	\ingroup MovidaShared

	\brief First page of the import wizard, showing the available engines and a query input widget.
*/

/*!
	This Import Wizard page allows to enter a query, validate it and
	select a search engine (if more than one are available).
*/
MvdImportStartPage::MvdImportStartPage(QWidget* parent)
: MvdImportPage(parent)
{
	setTitle(tr("Movida Internet import wizard"));
	setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/import/watermark.png"));

	infoLabel = new QLabel;
	infoLabel->setWordWrap(true);
	
	engineCombo = new QComboBox;
	queryInput = new QLineEdit;

	infoLabel->setText(tr("Please enter your query and hit the Search button (or press Enter) to start the search."));

	QGridLayout* gridLayout = new QGridLayout(this);
	gridLayout->addWidget(infoLabel, 0, 0, 1, 2);

	gridLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed), 1, 1, 1, 1);

	QGridLayout* queryGridLayout = new QGridLayout();
	QLabel* engineLabel = new QLabel(this);
	engineLabel->setText(tr("Search type:"));
	queryGridLayout->addWidget(engineLabel, 0, 0, 1, 1);
	queryGridLayout->addWidget(engineCombo, 0, 1, 1, 1);
	queryGridLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed), 1, 1, 1, 1);

	connect( engineCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(engineChanged()) );

	QLabel* queryLabel = new QLabel(this);
	queryLabel->setText(tr("Query:"));
	queryGridLayout->addWidget(queryLabel, 2, 0, 1, 1);
	queryGridLayout->addWidget(queryInput, 2, 1, 1, 1);

	gridLayout->addLayout(queryGridLayout, 2, 0, 1, 2);

	gridLayout->addItem(new QSpacerItem(20, 60, QSizePolicy::Minimum, QSizePolicy::Expanding), 3, 0, 1, 1);

	configButton = new QPushButton(this);
	configButton->setText(tr("&Options"));
	configButton->setDisabled(true);
	connect( configButton, SIGNAL(clicked()), this, SLOT(configButtonTriggered()) );

	gridLayout->addItem(new QSpacerItem(60, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 4, 0, 1, 1);
	gridLayout->addWidget(configButton, 4, 1, 1, 1);

	if (Movida::settings().value("movida/use-history").toBool()) {
	
		QStringList history = Movida::settings().value("movida/history/movie-import").toStringList();
		QCompleter* completer = new QCompleter(history, this);
		queryInput->setCompleter(completer);
	}

	// Finish GUI setup
	queryInput->setFocus(Qt::OtherFocusReason);
}

//! See MvdImportDialog::registerEngine(const MvdSearchEngine&)
int MvdImportStartPage::registerEngine(const MvdSearchEngine& engine)
{
	if (engine.name.isEmpty())
		return -1;

	engines.append(engine);
	engineCombo->addItem(engine.name);
	int id = engineCombo->count() - 1;
	
	if (id == 1) // More than two engines now! Update info label.
		infoLabel->setText(tr("Please select a search type, enter your query and hit the Search button (or press Enter) to start the search."));
	
	// Update current validator
	if (id == 0)
		engineChanged();
	engineCombo->setCurrentIndex(0);
	return id;
}

//! \internal
void MvdImportStartPage::engineChanged()
{
	if (engines.isEmpty())
		return;

	const MvdSearchEngine& e = engines.at(engineCombo->currentIndex());

	QRegExp rx(e.validator);
	queryInput->setValidator(new MvdQueryValidator(this));

	configButton->setEnabled(e.canConfigure);
}

//! \internal
void MvdImportStartPage::configButtonTriggered()
{
	emit engineConfigurationRequest(engineCombo->currentIndex());
}

//! Returns the ID of the currently selected search engine.
int MvdImportStartPage::engine() const
{
	return engineCombo->currentIndex();
}

//! Returns the current (possibly trimmed) query.
QString MvdImportStartPage::query() const
{
	return queryInput->text().trimmed();
}

//! Sets a new completer in the query input widget.
void MvdImportStartPage::updateCompleter(const QStringList& history)
{
	queryInput->setCompleter(new QCompleter(history));
}
