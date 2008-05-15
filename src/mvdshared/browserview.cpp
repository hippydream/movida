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
#include "mvdcore/core.h"
#include "mvdcore/movie.h"
#include "mvdcore/moviecollection.h"
#include "mvdcore/pathresolver.h"
#include "mvdcore/settings.h"
#include "mvdcore/templatemanager.h"
#include "mvdcore/templatecache.h"
#include <QContextMenuEvent>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QMenu>
#include <QRegExp>
#include <QtWebKit>


//////////////////////////////////////////////////////////////////////////
// MvdBrowserView::Private
//////////////////////////////////////////////////////////////////////////

class MvdBrowserView::Private
{
public:
	enum Context {
		InvalidContext = 0,
		PosterContext,
		RatingContext
	};

	enum Action {
		NoAction = 0,
		SavePoster
	};

	Private(MvdBrowserView* v) : collection(0), q(v)
	{}

	Context context(const QString& s) const;
	void populateContextMenu(QMenu* menu, Context pos) const;
	void contextMenuActionTriggered(const QWebHitTestResult& hit, Action a);

	QAction* backAction;
	QAction* reloadAction;
	QAction* forwardAction;
	MvdMovieCollection* collection;

	Ui::MvdBrowserView ui;
	MvdBrowserView* q;
};

MvdBrowserView::Private::Context MvdBrowserView::Private::context(const QString& s) const
{
	if (s == QLatin1String("poster"))
		return PosterContext;
	else if (s == QLatin1String("rating"))
		return RatingContext;
	return InvalidContext;
}

void MvdBrowserView::Private::populateContextMenu(QMenu* menu, Context pos) const
{
	Q_ASSERT(menu && (int)pos);

	switch (pos) {
	case PosterContext:
	{
		QAction* a = menu->addAction(QIcon(":/images/document-save.svgz"), tr("Save movie poster..."));
		a->setData((uint)SavePoster);
	}
	break;
	case RatingContext:
	{
		QAction* a = menu->addAction(tr("Clear rating"));
		a->setData((uint)PosterContext);
	}
	break;
	default: ;
	}
}

void MvdBrowserView::Private::contextMenuActionTriggered(const QWebHitTestResult& hit, Action a)
{
	switch (a) {
	case SavePoster: 
		{
			QString sourceFile = hit.imageUrl().toLocalFile();
			QString lastDir = Movida::settings().value("movida/browserview/saveposterdialog").toString();
			QString destFile = QFileDialog::getSaveFileName(q, MVD_CAPTION, lastDir, q->tr("PNG image files (*.png)"));
			if (destFile.isEmpty()) return;
			QFile dest(destFile);
			if (dest.exists())
				dest.remove();
			if (!QFile::copy(sourceFile, destFile)) {
				Movida::error(q->tr("Failed to save poster."));
			} else {
				Movida::info(q->tr("Movie poster saved."));
			}
		}
	default: ;
	}
}


//////////////////////////////////////////////////////////////////////////
// MvdBrowserView
//////////////////////////////////////////////////////////////////////////

MvdBrowserView::MvdBrowserView(QWidget* parent)
: QWidget(parent), d(new Private(this))
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

	QWebSettings* ws = d->ui.webView->settings();
	ws->setAttribute(QWebSettings::JavaEnabled, false);
	ws->setAttribute(QWebSettings::PluginsEnabled, false);
	ws->setAttribute(QWebSettings::JavascriptEnabled, false);
	ws->setAttribute(QWebSettings::DeveloperExtrasEnabled, false);

	d->ui.webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

	d->ui.webView->installEventFilter(this);

	connect( d->ui.webView, SIGNAL(linkClicked(QUrl)), this, SIGNAL(linkClicked(QUrl)) );
}

MvdBrowserView::~MvdBrowserView()
{

}

void MvdBrowserView::setMovieCollection(MvdMovieCollection* c)
{
	d->collection = c;
}

void MvdBrowserView::clear()
{
	d->ui.webView->setHtml(QString());
}

void MvdBrowserView::blank()
{
	QString path = Movida::tcache().blank(d->collection);
	if (path.isEmpty())
		clear();
	else d->ui.webView->setUrl(QUrl::fromLocalFile(path));
}

void MvdBrowserView::setUrl(QString url)
{
	bool special = false;
	MvdActionUrl aurl;
	
	QRegExp rx("[a-z]+:[a-z]+");
	if (rx.exactMatch(url)) {
		// Special URL like "about:blank"
		special = true;
	} else {
		aurl = MvdCore::parseActionUrl(url);
		if (!aurl.isValid()) {
			special = true;
			url = QLatin1String("about:blank");
		}
	}

	if (!special) {
		bool ok = false;

		// movida://movie/id/14,23,54,53
		if (aurl.action == QLatin1String("movie")) {
			QStringList sl = aurl.parameter.split(QChar('/'));
			if (sl.size() == 2) {
				QString s = sl.at(0);
				if (s == QLatin1String("id")) {
					sl = sl.at(1).split(QChar(','), QString::SkipEmptyParts);
					QList<mvdid> ids;
					if (!sl.isEmpty()) {
						foreach (QString s, sl) {
							mvdid id = s.toUInt(&ok);
							if (!ok) continue;
							ids.append(id);
						}
					}
					ok = true;
					int c = ids.size();
					if (c == 0)
						ok = false;
					else if (c == 1)
						showMovie(ids.at(0));
					else showMovies(ids);
				}
			}
		} else {

		}

		if (!ok) {
			url = QLatin1String("about:blank");
			special = true;
		}
	} // !special

	if (!special)
		return;

	QStringList list = url.split(QChar(':'));
	Q_ASSERT(list.size() == 2);
	aurl.action = list.at(0);
	aurl.parameter = list.at(1);

	if (aurl.action != QLatin1String("about")) {
		aurl.action = QLatin1String("about");
		aurl.parameter = QLatin1String("blank");
	}

	if (aurl.parameter == QLatin1String("blank")) {
		blank();
	}
}

/*!
	Shows the movies with given ids in the view using the currently 
	selected template. This method will call showMovie(mvdid) if the
	ids list contains only one id and blank() if the list is empty.

	Pages for multiple movies are never cached.

	See setMovieData(const MvdMovieData&) to show a movie that is not part
	of any collection.

	\see setMovieCollection(MvdMovieCollection*)
*/
void MvdBrowserView::showMovies(const QList<mvdid>& ids)
{
	if (ids.isEmpty()) {
		blank();
		return;
	} else if (ids.count() == 1) {
		showMovie(ids.at(0));
		return;
	}

	d->ui.webView->setHtml(QLatin1String("<html><body><h1>Multiple movies selected</h1></body></html>"));
}

/*!
	Shows the movie with given id in the view using the currently 
	selected template.

	The movie is first looked up in a cache and eventually retrieved from 
	the last set collection and added to a cache.

	Cached movies are removed automatically as soon as they are removed from
	the collection and updated as soon they are updated in the collection,
	so you only need to care about calling setMovie(mvdid) to tell the view
	what movie to show.

	See setMovieData(const MvdMovieData&) to show a movie that is not part
	of any collection.

	\see setMovieCollection(MvdMovieCollection*)
*/
void MvdBrowserView::showMovie(mvdid id)
{
	if (id == MvdNull || !d->collection) {
		blank();
		return;
	}

	QString path = Movida::tcache().movie(d->collection, id);
	if (path.isEmpty()) {
		blank();
	} else {
		d->ui.webView->setUrl(QUrl::fromLocalFile(path));
	}
}

/*!
	Caches a movie data object and returns an internal identifier that can
	be used to show that movie in the view with showMovieData(int).

	Do not use showMovie(mvdid) with the IDs returned by this method or you
	will end up showing the wrong movie or no movie at all.

	Use clearCachedMovieData(int) to remove the movie from the cache.

	Use showMovie(mvdid) if the movie is part of a collection.

	\see setMovieData(const MvdMovieData& md)
	\see clearCachedMovieData(int id)
*/
int MvdBrowserView::cacheMovieData(const MvdMovieData& md)
{
	if (!md.isValid())
		return -1;

	return Movida::tcache().cacheMovieData(md);
}

/*!
	Shows a previously cached movie data object in the view.

	\see cacheMovieData(const MvdMovieData&)
*/
void MvdBrowserView::showMovieData(int id)
{
	QString path = Movida::tcache().movieData(id);
	if (path.isEmpty())
		blank();
	else
		d->ui.webView->setUrl(QUrl::fromLocalFile(path));
}

void MvdBrowserView::clearCachedMovieData(int id)
{
	Movida::tcache().clearCachedMovieData(id);
}

bool MvdBrowserView::eventFilter(QObject* o, QEvent* e)
{
	if (o == d->ui.webView) {
		switch (e->type()) {
		case QEvent::ContextMenu: {
			QContextMenuEvent* cme = static_cast<QContextMenuEvent*>(e);
			showContextMenu(cme);
			return true;
			}
		default: ;
		}
	}

	return QWidget::eventFilter(o, e);
}

void MvdBrowserView::showContextMenu(QContextMenuEvent* e)
{
	Q_ASSERT(e);
	QPoint pos = e->pos();
	QWebFrame* f = frame();
	if (!f) return;
	
	QWebHitTestResult hit = f->hitTestContent(pos);

	//! \todo Bug report submitted to Trolltech on 05/05/2008
	// if (hit.isNull()) return;

	QMenu* menu = new QMenu;
	QUrl url = hit.imageUrl();
	if (!url.isEmpty() && d->collection) {
		QUrl dp = QUrl::fromLocalFile(d->collection->metaData(MvdMovieCollection::DataPathInfo));
		if (url.path().startsWith(dp.path())) {
			d->populateContextMenu(menu, Private::PosterContext);
		}
	}

	url = hit.linkUrl();
	if (!url.isEmpty() && d->collection) {
		if (url.scheme() == QLatin1String("movida")) {
			MvdActionUrl aurl = MvdCore::parseActionUrl(url);
			if (aurl.isValid() && aurl.action == QLatin1String("context") && !aurl.parameter.isEmpty()) {
				d->populateContextMenu(menu, d->context(aurl.parameter));
			}
		}
	}

	QAction* res = menu->isEmpty() ? 0 : menu->exec(d->ui.webView->mapToGlobal(pos));

	if (!res) {
		delete menu;
		return;
	}

	bool ok;
	uint id = res->data().toUInt(&ok);
	delete menu;

	if (!ok || !id) return;
	d->contextMenuActionTriggered(hit, (Private::Action) id);
}

QWebFrame* MvdBrowserView::frame() const
{
	return d->ui.webView->page()->mainFrame();
}

QWebPage* MvdBrowserView::page() const
{
	return d->ui.webView->page();
}

QWebView* MvdBrowserView::webView() const
{
	return d->ui.webView;
}

void MvdBrowserView::setControlsVisible(bool visible)
{
	d->ui.controls->setVisible(visible);
}

bool MvdBrowserView::controlsVisible() const
{
	return d->ui.controls->isVisible();
}
