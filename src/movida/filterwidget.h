/**************************************************************************
** Filename: filterwidget.h
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

#ifndef MVD_FILTERWIDGET_H
#define MVD_FILTERWIDGET_H

#include "ui_filterwidget.h"
#include <QWidget>

class QLineEdit;

class MvdFilterWidget : public QWidget, private Ui::MvdFilterWidget
{
	Q_OBJECT
	
public:
	MvdFilterWidget(QWidget* parent = 0);
	
	QLineEdit* editor() const;

	void setNoResultsWarningVisible(bool visible);
	bool noResultsWarningVisible() const;

	void setCaseSensitivity(Qt::CaseSensitivity cs);
	Qt::CaseSensitivity caseSensitivity() const;

	void setMatchWholeWords(bool match);
	bool matchWholeWords() const;

signals:
	void hideRequest();
	void caseSensitivityChanged();
	void matchWholeWordsChanged();
};

#endif // MVD_FILTERWIDGET_H
