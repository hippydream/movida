/**************************************************************************
** Filename: extendedinfopage.cpp
**
** Copyright (C) 2007-2009 Angius Fabrizio. All rights reserved.
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

#include "extendedinfopage.h"

#include "guiglobal.h"

#include "mvdcore/core.h"
#include "mvdcore/movie.h"
#include "mvdcore/moviecollection.h"

#include <QtCore/QList>
#include <QtGui/QIcon>
#include <QtGui/QMessageBox>
#include <QtGui/QTreeWidgetItem>

/*!
    \class MvdExtendedInfoPage extendedinfopage.h
    \ingroup Movida

    \brief Movie editor page for genre, country, language and other info.
*/

/*!
    Creates a new page.
*/
MvdExtendedInfoPage::MvdExtendedInfoPage(MvdMovieCollection *c, MvdMovieEditor *parent) :
    MvdMovieEditorPage(c, parent)
{
    setupUi(this);

    genres->setDataRole(Movida::GenreRole);
    genres->setMovieCollection(c);
    connect(genres, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));
    connect(genres, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));

    countries->setDataRole(Movida::CountryRole);
    countries->setMovieCollection(c);
    connect(countries, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));
    connect(countries, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));

    languages->setDataRole(Movida::LanguageRole);
    languages->setMovieCollection(c);
    connect(languages, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));
    connect(languages, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));

    tags->setDataRole(Movida::TagRole);
    tags->setMovieCollection(c);
    connect(tags, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));
    connect(tags, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));

    linkActivated("movida://toggle-view/genres");
    connect(toggleLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(linkActivated(const QString &)));

    moveUpBtn->setIcon(QIcon(":/images/arrow-up.svgz"));
    moveDownBtn->setIcon(QIcon(":/images/arrow-down.svgz"));

    connect(moveUpBtn, SIGNAL(clicked()), this, SLOT(moveUp()));
    connect(moveDownBtn, SIGNAL(clicked()), this, SLOT(moveDown()));
}

/*!
    Returns the title to be used for this page.
*/
QString MvdExtendedInfoPage::label()
{
    return tr("More info");
}

/*!
    Returns the icon to be used for this page.
*/
QIcon MvdExtendedInfoPage::icon()
{
    return QIcon();
}

/*!
    Sets data from a single movie.
*/
void MvdExtendedInfoPage::setMovieImpl(const MvdMovie &movie)
{
    genres->setMovie(movie);
    genres->setDataRole(Movida::GenreRole);

    countries->setMovie(movie);
    countries->setDataRole(Movida::CountryRole);

    languages->setMovie(movie);
    languages->setDataRole(Movida::LanguageRole);

    tags->setMovie(movie);
    tags->setDataRole(Movida::TagRole);
}

bool MvdExtendedInfoPage::store(MvdMovie &movie)
{
    genres->store(movie);
    countries->store(movie);
    languages->store(movie);
    tags->store(movie);

    return true;
}

//! \internal
void MvdExtendedInfoPage::linkActivated(const QString &url)
{
    MvdActionUrl a = MvdCore::parseActionUrl(url);

    if (!a.isValid())
        return;

    int index = -1;

    if (a.action == "toggle-view") {
        if (a.parameter == "genres") {
            toggleLabel->setText(tr("Currently showing movie <b>genres</b>. Click to show production <a href=\"movida://toggle-view/countries\">countries</a>, <a href=\"movida://toggle-view/languages\">languages</a> or <a href=\"movida://toggle-view/tags\">tags</a>."));
            index = 0;
        } else if (a.parameter == "countries") {
            toggleLabel->setText(tr("Currently showing movie production <b>countries</b>. Click to show <a href=\"movida://toggle-view/genres\">genres</a>, <a href=\"movida://toggle-view/languages\">languages</a> or <a href=\"movida://toggle-view/tags\">tags</a>."));
            index = 1;
        } else if (a.parameter == "languages") {
            toggleLabel->setText(tr("Currently showing movie <b>languages</b>. Click to show <a href=\"movida://toggle-view/genres\">genres</a>, production <a href=\"movida://toggle-view/countries\">countries</a> or <a href=\"movida://toggle-view/tags\">tags</a>."));
            index = 2;
        } else if (a.parameter == "tags") {
            toggleLabel->setText(tr("Currently showing movie <b>tags</b>. Click to show <a href=\"movida://toggle-view/genres\">genres</a>, production <a href=\"movida://toggle-view/countries\">countries</a> or <a href=\"movida://toggle-view/languages\">languages</a>."));
            index = 3;
        }

        stack->setCurrentIndex(index);
    }
}

//!
void MvdExtendedInfoPage::updateModifiedStatus()
{
    if (genres->isModified() || countries->isModified() ||
        languages->isModified() || tags->isModified())
        setModified(true);
    else setModified(false);
}

void MvdExtendedInfoPage::moveUp()
{
    MvdSDTreeWidget *v = currentView();

    if (!v) return;
    QList<QTreeWidgetItem *> sel = v->selectedItems();
    if (sel.size() != 1) return;
    QTreeWidgetItem *item = sel.at(0);
    if (v->isPlaceHolder(item)) return;
    int idx = v->indexOfTopLevelItem(item);
    if (idx <= 1)
        return;
    v->takeTopLevelItem(idx);
    v->insertTopLevelItem(idx - 1, item);
    v->setItemSelected(item, true);
    v->setCurrentItem(item);
}

void MvdExtendedInfoPage::moveDown()
{
    MvdSDTreeWidget *v = currentView();

    if (!v) return;
    QList<QTreeWidgetItem *> sel = v->selectedItems();
    if (sel.size() != 1) return;
    QTreeWidgetItem *item = sel.at(0);
    if (v->isPlaceHolder(item)) return;
    int idx = v->indexOfTopLevelItem(item);
    if (idx == 0 || idx == v->topLevelItemCount() - 1)
        return;
    v->takeTopLevelItem(idx);
    v->insertTopLevelItem(idx + 1, item);
    v->setItemSelected(item, true);
    v->setCurrentItem(item);
}

void MvdExtendedInfoPage::itemSelectionChanged()
{
    MvdSDTreeWidget *v = currentView();

    if (!v) return;
    QList<QTreeWidgetItem *> sel = v->selectedItems();
    bool up = true;
    bool down = true;
    if (sel.isEmpty()) {
        up = down = false;
    } else {
        QTreeWidgetItem *item = sel.at(0);
        if (v->isPlaceHolder(item)) {
            up = down = false;
        } else {
            int idx = v->indexOfTopLevelItem(item);
            up = idx > 1;
            down = idx < v->topLevelItemCount() - 1;
        }
    }

    moveUpBtn->setEnabled(up);
    moveDownBtn->setEnabled(down);
}

MvdSDTreeWidget *MvdExtendedInfoPage::currentView() const
{
    MvdSDTreeWidget *v = 0;
    QWidget *cw = stack->currentWidget();
    QObjectList children = cw->children();

    for (int i = 0; i < children.size() && !v; ++i) {
        QObject *o = children.at(i);
        if (o->isWidgetType()) {
            v = qobject_cast<MvdSDTreeWidget *>(o);
        }
    }
    return v;
}
