/**************************************************************************
** Filename: filterwidget.cpp
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

#include "filterwidget.h"

#include "guiglobal.h"

#include "mvdcore/core.h"

#include <QtCore/QDataStream>
#include <QtCore/QHash>
#include <QtCore/QVariant>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QIcon>
#include <QtGui/QPixmap>

MvdFilterWidget::MvdFilterWidget(QWidget *parent) :
    QFrame(parent),
    mMessage(NoMessage)
{
    setupUi(this);
    closeButton->setIcon(QIcon(":/images/filter-close.png"));

    QIcon i(":/images/dialog-warning.svgz");
    mWarning = i.pixmap(16, 16);
    i = QIcon(":/images/dialog-information.svgz");
    mInfo = i.pixmap(16, 16);

    warningIconLabel->setVisible(false);
    warningTextLabel->setVisible(false);

    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Raised);

    input->setPlaceHolder(tr("Enter a filter..."));
    setAcceptDrops(true);

    connect(closeButton, SIGNAL(clicked()), this, SIGNAL(hideRequest()));
    connect(caseSensitive, SIGNAL(stateChanged(int)), this, SIGNAL(caseSensitivityChanged()));
    connect(useOrOperator, SIGNAL(stateChanged(int)), this, SIGNAL(booleanOperatorChanged()));
}

QLineEdit *MvdFilterWidget::editor() const
{
    return input;
}

//! Shows a label warning or informing the user about something. Use MvdFilterWidget::NoMessage to hide the label.
void MvdFilterWidget::setMessage(Message m)
{
    if (mMessage == m)
        return;

    switch (m) {
        case NoResultsWarning:
            warningTextLabel->setText(tr("No movies match the filter criteria."));
            warningIconLabel->setPixmap(mWarning);
            break;

        case SyntaxErrorWarning:
            warningTextLabel->setText(tr("Invalid filter function."));
            warningIconLabel->setPixmap(mWarning);
            break;

        case DropInfo:
            warningTextLabel->setText(tr("Drop on the filter bar to create a filter function."));
            warningIconLabel->setPixmap(mInfo);
            break;

        default:
            ;
    }

    mMessage = m;

    warningIconLabel->setVisible(m != NoMessage);
    warningTextLabel->setVisible(m != NoMessage);
}

MvdFilterWidget::Message MvdFilterWidget::message() const
{
    return mMessage;
}

/*!
    Calling this method will not emit the caseSensitivityChanged() signal.
*/
void MvdFilterWidget::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    caseSensitive->setChecked(cs == Qt::CaseSensitive);
}

Qt::CaseSensitivity MvdFilterWidget::caseSensitivity() const
{
    return caseSensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
}

/*!
    Calling this method will not emit the booleanOperatorChanged() signal.
*/
void MvdFilterWidget::setBooleanOperator(Movida::BooleanOperator op)
{
    useOrOperator->setChecked(op == Movida::OrOperator);
}

Movida::BooleanOperator MvdFilterWidget::booleanOperator() const
{
    return useOrOperator->isChecked() ? Movida::OrOperator : Movida::AndOperator;
}

void MvdFilterWidget::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasFormat(Movida::core().parameter("movida/mime/movie-attributes").toString())) {
        e->acceptProposedAction();
    } else e->ignore();
}

void MvdFilterWidget::dragMoveEvent(QDragMoveEvent *e)
{
    if (e->mimeData()->hasFormat(Movida::core().parameter("movida/mime/movie-attributes").toString())) {
        e->acceptProposedAction();
    } else e->ignore();
}

void MvdFilterWidget::dropEvent(QDropEvent *e)
{
    const QMimeData *md = e->mimeData();
    QString idList =
        QString::fromLatin1(md->data(Movida::core().parameter("movida/mime/movie-attributes").toString()));

    Qt::DropAction dropAction = e->proposedAction();
    bool replaceFilter = dropAction == Qt::MoveAction;

    applySharedDataFilter(idList, replaceFilter);

    e->acceptProposedAction();
}

void MvdFilterWidget::applySharedDataFilter(const QString &itemIds, bool replaceFilter)
{
    QString filter = QString("@%1(%2)")
                         .arg(Movida::filterFunctionName(Movida::SharedDataIdFilter))
                         .arg(itemIds);

    if (replaceFilter)
        editor()->clear();

    if (filter.isEmpty())
        return;

    QString text;
    if (!replaceFilter) {
        text = editor()->text().trimmed();
        if (!text.isEmpty())
            text.append(QLatin1Char(' '));
    }

    text.append(filter);
    editor()->setText(text);
}

bool MvdFilterWidget::event(QEvent *e)
{
    return QFrame::event(e);
}

bool MvdFilterWidget::isEmpty() const
{
    return editor()->text().trimmed().isEmpty();
}

QString MvdFilterWidget::filter() const
{
    return editor()->text();
}
