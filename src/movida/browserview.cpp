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
#include <QGridLayout>
#include <QtWebKit>

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

void MvdBrowserView::clear()
{
	webView->setUrl(QUrl("about:blank"));
}

void MvdBrowserView::setHtml(const QString& s)
{
	webView->setHtml(s);
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
