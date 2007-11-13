/**************************************************************************
** Filename: filterwidget.cpp
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

#include "filterwidget.h"

MvdFilterWidget::MvdFilterWidget(QWidget* parent)
: QWidget(parent)
{
	setupUi(this);
	closeButton->setIcon(QIcon(":/images/misc/filter-close"));
	warningIconLabel->setPixmap(QPixmap(":/images/misc/filter-warning"));

	connect( closeButton, SIGNAL(clicked()), this, SIGNAL(hideRequest()) );
	connect( caseSensitive, SIGNAL(stateChanged(int)), this, SIGNAL(caseSensitivityChanged()) );
}

QLineEdit* MvdFilterWidget::editor() const
{
	return input;
}

//! Shows or hides a label warning the user that no item has been filtered.
void MvdFilterWidget::setNoResultsWarningVisible(bool visible)
{
	warningIconLabel->setVisible(visible);
	warningTextLabel->setVisible(visible);
}

bool MvdFilterWidget::noResultsWarningVisible() const
{
	return warningIconLabel->isVisible();
}

/*!
	Calling this method will not emit the caseSensitivityChanged() signal.
*/
void MvdFilterWidget::setCaseSensitivity(Qt::CaseSensitivity cs)
{
	caseSensitive->setChecked(cs == Qt::CaseSensitive);
}

Qt::CaseSensitivity MvdFilterWidget::caseSensitivity() const
{
	return caseSensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
}
