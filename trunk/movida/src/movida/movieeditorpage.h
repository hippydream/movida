/**************************************************************************
** Filename: movieeditorpage.h
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

#ifndef MVD_MOVIEEDITORPAGE_H
#define MVD_MOVIEEDITORPAGE_H

#include "mpdialogpage.h"
#include "shareddata.h"
#include "moviecollection.h"
#include "movie.h"

class QIcon;

class MvdMovieEditorPage : public MvdMPDialogPage
{
	Q_OBJECT

public:
	MvdMovieEditorPage(MvdMovieCollection* c, QWidget* parent = 0) : 
		MvdMPDialogPage(parent), mCollection(c) {};

	/*!
		Default implementation returns false. Subclasses should return true if 
		the form has been modified and needs to be saved.
	*/
	virtual bool isModified() const { return false; };

	void setMovie(const MvdMovie& movie)
	{
		if (mCollection == 0)
			return;

		mMovie = movie;
		setMovieImpl(mMovie);
	}

	void setMovies(const QList<MvdMovie>& movies)
	{
		if (mCollection == 0)
			return;

		setMoviesImpl(movies);
	}

	/*!
		Stores the current values in a movie object.
		The default implementation always returns true.
		Subclasses should return true only if the form has been successfully validated.
		The modified status should not be checked: you should always store the data,
		except when validation of the form fails.
	*/
	virtual bool store(MvdMovie& movie) { Q_UNUSED(movie); return true; }

protected:
	//! Subclasses have to implement this method to populate the form with data.
	virtual void setMovieImpl(const MvdMovie& movie) = 0;
	//! Subclasses have to implement this method to populate the form with data.
	virtual void setMoviesImpl(const QList<MvdMovie>& movies) = 0;

	MvdSharedData& sd(smdid id) const
	{
		if (MvdSharedData::isHardcoded(id))
			return Movida::globalSD();
		if (mCollection == 0)
			Movida::globalSD();
		return mCollection->smd();
	}

	MvdMovieCollection* mCollection;
	MvdMovie mMovie;
};

#endif // MVD_MOVIEEDITORPAGE_H