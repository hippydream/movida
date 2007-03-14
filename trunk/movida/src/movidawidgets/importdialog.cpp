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

MvdwImportDialog::MvdwImportDialog(QWidget* parent)
: QDialog(parent)
{
	setupUi(this);
	QStringList frames;
	for (int i = 1; i <= 8; ++i)
		frames << QString(":/images/loading_p%1.png").arg(i);
	new MvdwLabelAnimator(frames, loadingPixmapLabel, this);
}

QDialogButtonBox* MvdwImportDialog::buttonBox()
{
	return Ui::MvdwImportDialog::buttonBox;
}

QWidget* MvdwImportDialog::startPage()
{
	return Ui::MvdwImportDialog::startPage;
}

void MvdwImportDialog::showNextPage()
{
	mainStack->setCurrentIndex(mainStack->currentIndex() + 1);
}

void MvdwImportDialog::showPreviousPage()
{
	mainStack->setCurrentIndex(mainStack->currentIndex() - 1);
}

void MvdwImportDialog::setStatus(const QString& s)
{
	loadingLabel->setText(s);
}
