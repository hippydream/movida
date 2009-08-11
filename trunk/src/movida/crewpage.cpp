/**************************************************************************
** Filename: crewpage.cpp
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

#include "crewpage.h"

#include "guiglobal.h"

#include "mvdcore/core.h"

#include <QtGui/QIcon>
#include <QtGui/QTreeWidgetItem>

/*!
    \class MvdCrewPage crewpage.h
    \ingroup Movida

    \brief Crew editing widget for movie editing dialog.
*/

/*!
    Creates a new page.
*/
MvdCrewPage::MvdCrewPage(MvdMovieCollection *c, MvdMovieEditor *parent) :
    MvdMovieEditorPage(c, parent)
{
    setupUi(this);

    cast->setMovieCollection(mCollection);
    cast->setDataSource(Movida::ActorRole);
    connect(cast, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));
    connect(cast, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));

    directors->setMovieCollection(mCollection);
    directors->setDataSource(Movida::DirectorRole);
    connect(directors, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));
    connect(directors, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));

    producers->setMovieCollection(mCollection);
    producers->setDataSource(Movida::ProducerRole);
    connect(producers, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));
    connect(producers, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));

    crew->setMovieCollection(mCollection);
    crew->setDataSource(Movida::CrewMemberRole);
    connect(crew, SIGNAL(modifiedStatusChanged(bool)), this, SLOT(updateModifiedStatus()));
    connect(crew, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));

    linkActivated("movida://toggle-view/cast");
    connect(toggleLabel, SIGNAL(linkActivated(const QString &)), this, SLOT(linkActivated(const QString &)));

    moveUpBtn->setIcon(QIcon(":/images/arrow-up.svgz"));
    moveDownBtn->setIcon(QIcon(":/images/arrow-down.svgz"));

    connect(moveUpBtn, SIGNAL(clicked()), this, SLOT(moveUp()));
    connect(moveDownBtn, SIGNAL(clicked()), this, SLOT(moveDown()));
}

/*!
    Returns the title to be used for this page.
*/
QString MvdCrewPage::label()
{
    return tr("Cast and crew");
}

/*!
    Returns the icon to be used for this page.
*/
QIcon MvdCrewPage::icon()
{
    return QIcon();
}

/*!
    Sets data from a single movie.
*/
void MvdCrewPage::setMovieImpl(const MvdMovie &movie)
{
    cast->setMovie(movie);
    cast->setDataSource(Movida::ActorRole);

    directors->setMovie(movie);
    directors->setDataSource(Movida::DirectorRole);

    producers->setMovie(movie);
    producers->setDataSource(Movida::ProducerRole);

    crew->setMovie(movie);
    crew->setDataSource(Movida::CrewMemberRole);
}

bool MvdCrewPage::store(MvdMovie &movie)
{
    cast->store(movie);
    directors->store(movie);
    producers->store(movie);
    crew->store(movie);

    return true;
}

//! \internal
void MvdCrewPage::linkActivated(const QString &url)
{
    MvdActionUrl a = MvdCore::parseActionUrl(url);

    if (!a.isValid())
        return;

    int index = -1;

    if (a.action == "toggle-view") {
        if (a.parameter == "cast") {
            toggleLabel->setText(tr("Currently showing movie <b>cast</b>. Click to show <a href=\"movida://toggle-view/directors\">directors</a>, <a href=\"movida://toggle-view/producers\">producers</a> or other <a href=\"movida://toggle-view/crew\">crew members</a>."));
            index = 0;
        } else if (a.parameter == "directors") {
            toggleLabel->setText(tr("Currently showing movie <b>directors</b>. Click to show <a href=\"movida://toggle-view/cast\">cast</a>, <a href=\"movida://toggle-view/producers\">producers</a> or other <a href=\"movida://toggle-view/crew\">crew members</a>."));
            index = 1;
        } else if (a.parameter == "producers") {
            toggleLabel->setText(tr("Currently showing movie <b>producers</b>. Click to show <a href=\"movida://toggle-view/cast\">cast</a>, <a href=\"movida://toggle-view/directors\">directors</a> or other <a href=\"movida://toggle-view/crew\">crew members</a>."));
            index = 2;
        } else if (a.parameter == "crew") {
            toggleLabel->setText(tr("Currently showing movie <b>crew</b>. Click to show <a href=\"movida://toggle-view/cast\">cast</a>, <a href=\"movida://toggle-view/directors\">directors</a> or <a href=\"movida://toggle-view/producers\">producers</a>."));
            index = 3;
        }

        stack->setCurrentIndex(index);
    }
}

//!
void MvdCrewPage::updateModifiedStatus()
{
    if (cast->isModified() || directors->isModified() ||
        producers->isModified() || crew->isModified())
        setModified(true);
    else setModified(false);
}

void MvdCrewPage::moveUp()
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

void MvdCrewPage::moveDown()
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

void MvdCrewPage::itemSelectionChanged()
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

MvdSDTreeWidget *MvdCrewPage::currentView() const
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
