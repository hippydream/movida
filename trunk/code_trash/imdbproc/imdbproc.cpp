/****************************************************************************
** Filename: imdbproc.cpp
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** imdbproc main window.
**
** Copyright (C) 2006 Angius Fabrizio. All rights reserved.
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
**********************************************************************/

#include "imdbproc.h"

#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

/*!
\class ImdbProc imdbproc.h

\brief imdbproc main window.
*/

ImdbProc::ImdbProc()
: QMainWindow()
{
	setupUi(this);

	processButton->setDisabled(true);

	connect( input, SIGNAL(textChanged(const QString&)), this, SLOT(updateUi()) );
	connect( output, SIGNAL(textChanged(const QString&)), this, SLOT(updateUi()) );

	connect( inputBrowse, SIGNAL(clicked()), this, SLOT(browseInput()) );
	connect( outputBrowse, SIGNAL(clicked()), this, SLOT(browseOutput()) );

	connect( processButton, SIGNAL(clicked()), this, SLOT(process()) );
}

void ImdbProc::updateUi()
{
	QFileInfo in(input->text());

	bool inputOk = in.isFile() && !output->text().isEmpty();
	processButton->setEnabled(inputOk);
}

void ImdbProc::browseInput()
{
	QString fname = QFileDialog::getOpenFileName(this, "ImdbProc", 
		mLastIn.isEmpty() ? mLastOut : mLastIn, "*.list");

	if (fname.isEmpty())
		return;

	QFileInfo info(fname);
	mLastIn = info.absolutePath();

	input->setText(info.absoluteFilePath());
}

void ImdbProc::browseOutput()
{
	QString fname = QFileDialog::getSaveFileName(this, "ImdbProc", 
		mLastIn.isEmpty() ? mLastIn : mLastOut);

	if (fname.isEmpty())
		return;

	QFileInfo info(fname);
	mLastOut = info.absolutePath();

	output->setText(info.absoluteFilePath());
}

void ImdbProc::process()
{
	QFile inFile(input->text());

	if (!inFile.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(this, "ImdbProc", tr("Unable to open input file."));
		return;
	}

	QFile outFile(output->text());
	if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		QMessageBox::warning(this, "ImdbProc", tr("Unable to open output file."));
		return;
	}

	QTextStream in(&inFile);
	QTextStream out(&outFile);

	QString line;

	statusBar()->showMessage(tr("Parsing header: %1").arg(input->text()));

	// Find data start
	while ( !(line = in.readLine()).isNull() )
	{
		if (line.trimmed() == "Name\t\t\tTitles")
		{
			// Skip the "----\t\t\t------" line
			line = in.readLine();
			break;
		}
	}

	int minMovieCount = minMovies->value();
	QString imdbName;
	int movieCount = 0;
	mItemCount = 0;

	// Process data
	while ( !(line = in.readLine()).isNull() )
	{
		if (line.isEmpty())
		{
			// Block end
			if (!imdbName.isEmpty())
			{
				if (movieCount >= minMovieCount)
					writeItem(out, imdbName);

				imdbName.clear();
			}

			movieCount = 0;
		}
		else if (line.startsWith("\t\t\t"))
		{
			if (imdbName.isEmpty())
				continue;
			if (!line.trimmed().isEmpty())
				movieCount++;
		}
		else
		{
			// Parse name
			int idx = line.indexOf("\t");
			if (idx <= 0)
				continue;
			imdbName = line.left(idx);
			movieCount++;
		}
	}

	// Write last item
	if (!imdbName.isEmpty() && movieCount >= minMovieCount)
		writeItem(out, imdbName);

	inFile.close();
	outFile.close();

	QMessageBox::information(this, "ImdbProc", 
		(mItemCount == 0 ? tr("No items found.") : tr("%1 items found.").arg(mItemCount)));
}

void ImdbProc::writeItem(QTextStream& out, const QString& item)
{
	out << item << "\r\n";
	mItemCount++;
}
