/**************************************************************************
** Filename: browserview.cpp
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

#include "browserview.h"
#include "ui_browserview.h"
#include "mvdcore/moviecollection.h"
#include "mvdcore/movie.h"
#include "mvdcore/templatemanager.h"
#include <QGridLayout>
#include <QtWebKit>

namespace {
	static const int MaxCachedPages = 100;
};


class MvdBrowserView::Private
{
public:
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

	Private() : collection(0)
	{}

	void updateCache();

	QAction* backAction;
	QAction* reloadAction;
	QAction* forwardAction;
	MvdMovieCollection* collection;
	PageCache cache;
	QString templateName;

	Ui::MvdBrowserView ui;
};

MvdBrowserView::MvdBrowserView(QWidget* parent)
: QWidget(parent), d(new Private)
{
	d->ui.setupUi(this);

	d->backAction = d->ui.webView->pageAction(QWebPage::Back);
	d->backAction->setIcon(QIcon(":/images/arrow-left.svgz"));
	d->ui.backButton->setDefaultAction(d->backAction);
	
	d->reloadAction = d->ui.webView->pageAction(QWebPage::Reload);
	d->reloadAction->setIcon(QIcon(":/images/reload.svgz"));
	d->ui.reloadButton->setDefaultAction(d->reloadAction);

	d->forwardAction = d->ui.webView->pageAction(QWebPage::Forward);
	d->forwardAction->setIcon(QIcon(":/images/arrow-right.svgz"));
	d->ui.forwardButton->setDefaultAction(d->forwardAction);

	// webView->installEventFilter(this);
}

MvdBrowserView::~MvdBrowserView()
{

}

void MvdBrowserView::setMovieCollection(MvdMovieCollection* c)
{
	d->collection = c;
	clear();

	if (c) {
		connect (c, SIGNAL(movieRemoved(mvdid)), SLOT(invalidateMovie(mvdid)));
		connect (c, SIGNAL(movieChanged(mvdid)), SLOT(invalidateMovie(mvdid)));
	}
}

void MvdBrowserView::clear()
{
	d->ui.webView->setUrl(QUrl("about:blank"));
}

void MvdBrowserView::setHtml(const QString& s)
{
	d->ui.webView->setHtml(s);
}

void MvdBrowserView::setMovie(mvdid id)
{
	clear();
	if (id == MvdNull || !d->collection)
		return;

	QString path;
	int idx = d->cache.indexOf(Private::CachedPage(id));
	if (idx >= 0) {
		Private::CachedPage& cp = d->cache[idx];
		++cp.usage;
		path = cp.path;
	}

	if (path.isEmpty()) {
		// Parse and cache HTML
		path = d->collection->metaData(MvdMovieCollection::TempPathInfo).append("/templates/");
		if (!QFile::exists(path)) {
			QDir dir;
			dir.mkpath(path);
		}

		path.append(QString("bvt_%1.html").arg(id));
		if (QFile::exists(path)) QFile::remove(path);

		bool res = Movida::tmanager().movieToHtmlFile(d->collection->movie(id), *d->collection, 
			path, QLatin1String("BrowserView"), d->templateName);

		if (res) {
			if (d->cache.size() == MaxCachedPages)
				d->updateCache();
			d->cache.append(Private::CachedPage(id, path));
		}
		else path.clear();
	}
	
	if (!path.isEmpty()) {
		d->ui.webView->setUrl(QUrl::fromLocalFile(path));
	}
}

bool MvdBrowserView::eventFilter(QObject* o, QEvent* e)
{
	/*if (o == webView) {
		switch (e->type()) {
		case QEvent::ContextMenu: return true;
		}
	}*/

	return QWidget::eventFilter(o, e);
}

void MvdBrowserView::invalidateMovie(mvdid id)
{
	// Remove movie from cache
	int idx = d->cache.indexOf(Private::CachedPage(id));
	if (idx < 0)
		return;

	QString path = d->cache[idx].path;
	QFile::remove(path);
	d->cache.removeAt(idx);
}

void MvdBrowserView::Private::updateCache()
{
	qSort(cache);
	int last = cache.size();
	while (cache.size() > (MaxCachedPages / 2)) {
		CachedPage p = cache.takeAt(--last);
		QFile::remove(p.path);
	}
}
