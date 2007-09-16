/****************************************************************************
** Filename: movielist.cpp
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

#include <QList>
#include <QTreeWidgetItem>
#include <QHeaderView>

#include "movielist.h"
#include "columnids.h"
#include "moviecollection.h"
#include "movie.h"

/*!
	\class MovieList movielist.h

	\brief DataView subclass for handling of movie lists.
*/


/************************************************************************
Private interface
*************************************************************************/

//! \internal
class MovieListPrivate
{
public:
	MovieListPrivate();

	MovieCollection* collection;
};

//! \internal
MovieListPrivate::MovieListPrivate()
{
	collection = 0;
}

/************************************************************************
Public interface
*************************************************************************/


/*!
	Creates a new empty movie list.
*/
MovieList::MovieList(QWidget* parent)
: DataView(parent)
{
	d = new MovieListPrivate();

	resetColumns();
	setObjectName("movie_list");	
}

/*!
*/
MovieList::~MovieList()
{
	delete d;
}

/*!
	Adds a new movie to the list.
*/
void MovieList::addMovie(int id)
{
	Q_UNUSED(id)

/*	const Movie* movie = mMC->getMovie(id);
	if (movie == 0)
		return;

	// enum InfoType { IT_TITLE, IT_ORIG_TITLE, IT_RYEAR, IT_PYEAR, IT_VERSION, IT_LENGTH, IT_IMDB_ID, IT_PLOT, IT_NOTES };
	const QString* info = 0;
	
	// movies need at least a localized title!
	info = movie->getInfo(Movie::IT_TITLE);
	if (info == 0)
		return;

	QTreeWidgetItem* item = new QTreeWidgetItem(this);
	int colId = columnID(MovidaUCD::UCD_ID);
	item->setData(colId, DATAVIEW_ROLE_ID, QVariant(id));

	id = columnID(MovidaUCD::UCD_TITLE);
	item->setText(colId, *info);*/
}

/*!
	Updates a movie in the list.
*/
void MovieList::changeMovie(int id)
{
	Q_UNUSED(id)
}

/*!
	Removes a movie from the list.
*/
void MovieList::removeMovie(int id)
{
	Q_UNUSED(id)
}

/*!
	Resets column visibility, position and width to their default values.
*/
void MovieList::resetColumns()
{
	clearColumns();

	QList<DataView::ColumnDescriptor> cols;
	DataView::ColumnDescriptor cd;

	// default visible title, or.title, r.year, directors, length, media_type

	cd.cid = MovidaCID::Id;
	cols << cd;
	
	cd.cid = MovidaCID::Title;
	cols << cd;

	cd.cid = MovidaCID::OriginalTitle;
	cols << cd;

	cd.cid = MovidaCID::Directors;
	cols << cd;
	
	cd.cid = MovidaCID::ReleaseYear;
	cols << cd;

	cd.isVisible = false;

	cd.cid = MovidaCID::ProductionYear;
	cols << cd;

	cd.isVisible = true;

	cd.cid = MovidaCID::Length;
	cols << cd;
	
	cd.cid = MovidaCID::MediaType;
	cols << cd;

	cd.isVisible = false;

	cd.cid = MovidaCID::Cast;
	cols << cd;

	cd.cid = MovidaCID::Producers;
	cols << cd;

	setColumns(cols);
}

/*!
	Returns the current collection.
*/
MovieCollection* MovieList::collection() const
{
	return d->collection;
}

/*!
	Sets a new collection.
*/
void MovieList::setCollection(MovieCollection* c)
{
	d->collection = c;
}
