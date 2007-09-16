/**************************************************************************
** Filename: linkspage.h
** Revision: 1
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

#ifndef MVD_LINKSPAGE_H
#define MVD_LINKSPAGE_H

#include "ui_linkspage.h"
#include "movieeditorpage.h"

class QIcon;

class MvdLinksPage : public MvdMovieEditorPage, private Ui::MvdLinksPage
{
	Q_OBJECT
		
public:
	MvdLinksPage(MvdMovieCollection* c, QWidget* parent = 0);
	
	void reset();
	QString label();
	QIcon icon();

	bool isModified() const;

	void setMovieImpl(const MvdMovie& movie);
	void setMoviesImpl(const QList<MvdMovie>& movies);

	bool store(MvdMovie& movie);

private slots:
	void imdbIdChanged(const QString& text);
	void openImdbPage();

private:
	bool mModified;
};

#endif // MVD_LINKSPAGE_H
