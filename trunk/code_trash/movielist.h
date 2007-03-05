/****************************************************************************
** Filename: movielist.h
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** DataView subclass for handling of movie lists.
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

#ifndef MOVIDA_MOVIELIST__H
#define MOVIDA_MOVIELIST__H

#include "dataview.h"

class QByteArray;
class MovieCollection;
class MovieListPrivate;

class MovieList : public DataView
{
	Q_OBJECT
		
public:
	MovieList(QWidget* parent);
	~MovieList();

	void setCollection(MovieCollection* c);
	MovieCollection* collection() const;
	
public slots:
	void addMovie(int id);
	void changeMovie(int id);
	void removeMovie(int id);

	void resetColumns();
	
private:
	MovieListPrivate* d;
};

#endif // MOVIDA_MOVIELIST__H
