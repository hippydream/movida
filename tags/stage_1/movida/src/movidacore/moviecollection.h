/**************************************************************************
** Filename: moviecollection.h
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

#ifndef MVD_MOVIECOLLECTION_H
#define MVD_MOVIECOLLECTION_H

#include "global.h"
#include "shareddata.h"

#include <QList>
#include <QHash>
#include <QString>
#include <QStringList>

class MvdMovieCollection_P;
class MvdMovie;
class MvdLogger;

class MVD_EXPORT MvdMovieCollection : public QObject
{
	Q_OBJECT

public:
	MvdMovieCollection();
	MvdMovieCollection(const MvdMovieCollection& m);
	virtual ~MvdMovieCollection();
	MvdMovieCollection& operator=(const MvdMovieCollection& m);

	typedef QHash<movieid, MvdMovie> MovieTable;
	
	enum CollectionInfo {
		NameInfo, OwnerInfo, EMailInfo, WebsiteInfo, NotesInfo, DataPathInfo
	};

	enum ImageCategory {
		MoviePosterImage, GenericImage
	};
	
	MvdSharedData& smd() const;

	void setInfo(CollectionInfo ci, const QString& val);
	QString info(CollectionInfo ci);

	int count() const;
	bool isEmpty() const;

	MovieTable movies() const;
	QList<movieid> movieIds() const;
	MvdMovie movie(movieid id) const;

	movieid addMovie(const MvdMovie& movie);
	void updateMovie(movieid id, const MvdMovie& movie);
	void removeMovie(movieid id);
	
	bool contains(const QString& title, int year) const;
	
	void clear();
	
	bool isModified() const;
	void setModifiedStatus(bool modified);
	
	QString fileName() const;
	void setFileName(const QString& f);
	
	QString path() const;
	void setPath(const QString& p);

	bool save(const QString& file = QString());
	bool load(const QString& file);

	QString addImage(const QString& path, ImageCategory category = GenericImage);

	void clearPersistentData();

signals:
	void movieAdded(movieid id);
	void movieChanged(movieid id);
	void movieRemoved(movieid id);
	
	void cleared();
	void modified();
	void saved();
	
private:
	MvdMovieCollection_P* d;
public:
	typedef MvdMovieCollection_P* DataPtr;
	inline DataPtr& data_ptr() { return d; }
	void detach();
	bool isDetached() const;
};
MVD_DECLARE_SHARED(MvdMovieCollection)

#endif // MVD_MOVIECOLLECTION_H
