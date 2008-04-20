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
#include "mvdcore/moviecollection.h"
#include "mvdcore/movie.h"
#include "mvdcore/templatemanager.h"
#include <QGridLayout>
#include <QtWebKit>

namespace {
	static const int MaxCachedPages = 100;
};

MvdBrowserView::MvdBrowserView(QWidget* parent)
: QWidget(parent)
{
	setupUi(this);

	mBackAction = webView->pageAction(QWebPage::Back);
	mBackAction->setIcon(QIcon(":/images/arrow-left.svgz"));
	backButton->setDefaultAction(mBackAction);
	
	mReloadAction = webView->pageAction(QWebPage::Reload);
	mReloadAction->setIcon(QIcon(":/images/reload.svgz"));
	reloadButton->setDefaultAction(mReloadAction);

	mForwardAction = webView->pageAction(QWebPage::Forward);
	mForwardAction->setIcon(QIcon(":/images/arrow-right.svgz"));
	forwardButton->setDefaultAction(mForwardAction);

	// webView->installEventFilter(this);
}

MvdBrowserView::~MvdBrowserView()
{

}

void MvdBrowserView::setMovieCollection(MvdMovieCollection* c)
{
	mCollection = c;
	clear();

	if (c) {
		connect (c, SIGNAL(movieRemoved(mvdid)), SLOT(invalidateMovie(mvdid)));
		connect (c, SIGNAL(movieChanged(mvdid)), SLOT(invalidateMovie(mvdid)));
	}
}

void MvdBrowserView::clear()
{
	webView->setUrl(QUrl("about:blank"));
}

void MvdBrowserView::setHtml(const QString& s)
{
	webView->setHtml(s);
}

void MvdBrowserView::setMovie(mvdid id)
{
	clear();
	if (id == MvdNull || !mCollection)
		return;

	QString path;
	int idx = mCache.indexOf(CachedPage(id));
	if (idx >= 0) {
		CachedPage& cp = mCache[idx];
		++cp.usage;
		path = cp.path;
	}

	if (path.isEmpty()) {
		// Parse and cache HTML
		path = mCollection->metaData(MvdMovieCollection::TempPathInfo).append("/templates/");
		if (!QFile::exists(path)) {
			QDir dir;
			dir.mkpath(path);
		}

		path.append(QString("bvt_%1.html").arg(id));
		if (QFile::exists(path)) QFile::remove(path);

		bool res = Movida::tmanager().movieToHtmlFile(mCollection->movie(id), *mCollection, path, "Blue");
		if (res) {
			if (mCache.size() == MaxCachedPages)
				updateCache();
			mCache.append(CachedPage(id, path));
		}
		else path.clear();
	}
	
	if (!path.isEmpty()) {
		webView->setUrl(QUrl::fromLocalFile(path));
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
	int idx = mCache.indexOf(CachedPage(id));
	if (idx < 0)
		return;

	QString path = mCache[idx].path;
	QFile::remove(path);
	mCache.removeAt(idx);
}

void MvdBrowserView::updateCache()
{
	qSort(mCache);
	int last = mCache.size();
	while (mCache.size() > (MaxCachedPages / 2)) {
		CachedPage p = mCache.takeAt(--last);
		QFile::remove(p.path);
	}
}
