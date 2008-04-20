/**************************************************************************
** Filename: browserview.h
**
** Copyright (C) 2007-2008 Angius Fabrizio. All rights reserved.
**
** This file is part of the Movida project (http://movida.42cows.org/).
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

#ifndef MVD_BROWSERVIEW_H
#define MVD_BROWSERVIEW_H

#include "ui_browserview.h"
#include "mvdcore/global.h"
#include <QWidget>
#include <QList>

class MvdMovieCollection;
class QAction;

class MvdBrowserView : public QWidget, protected Ui::MvdBrowserView
{
	Q_OBJECT

public:
	MvdBrowserView(QWidget* parent = 0);
	virtual ~MvdBrowserView();

	void setMovieCollection(MvdMovieCollection* c);
	void setMovie(mvdid id);

public slots:
	void clear();
	void setHtml(const QString& s);

	bool eventFilter(QObject* o, QEvent* e);

private slots:
	void invalidateMovie(mvdid id);

private:
	struct CachedPage {
		CachedPage(mvdid id) : movie(id), usage(0) {}
		CachedPage(mvdid id, const QString& s) : movie(id), path(s), usage(0) {}
		bool operator<(const CachedPage& o) const { return usage > o.usage; }
		bool operator==(const CachedPage& o) const { return movie == o.movie; }

		mvdid movie;
		QString path;
		int usage;
	};
	typedef QList<CachedPage> PageCache;

	void updateCache();

	QAction* mBackAction;
	QAction* mReloadAction;
	QAction* mForwardAction;
	MvdMovieCollection* mCollection;
	PageCache mCache;
};

#endif // MVD_BROWSERVIEW_H
