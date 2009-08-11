/**************************************************************************
** Filename: messagebox.h
**
** Copyright (C) 2007-2009-2008 Angius Fabrizio. All rights reserved.
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

#include "messagebox.h"

#include <QtGui/QApplication>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QPushButton>
#include <QtGui/QStyle>

namespace {
QString wt(const QString &title)
{
    QString s = QApplication::applicationName();

    if (!title.isEmpty())
        s.append(QLatin1String(" - ")).append(title);
    return s;
}

QString bt(QMessageBox::StandardButton sb)
{
    switch (sb) {
        case QMessageBox::Ok:
            return MvdMessageBox::tr("&OK");

        case QMessageBox::Save:
            return MvdMessageBox::tr("&Save");

        case QMessageBox::SaveAll:
            return MvdMessageBox::tr("Save All");

        case QMessageBox::Open:
            return MvdMessageBox::tr("&Open");

        case QMessageBox::Yes:
            return MvdMessageBox::tr("&Yes");

        case QMessageBox::YesToAll:
            return MvdMessageBox::tr("Y&es to All");

        case QMessageBox::No:
            return MvdMessageBox::tr("&No");

        case QMessageBox::NoToAll:
            return MvdMessageBox::tr("N&o to All");

        case QMessageBox::Abort:
            return MvdMessageBox::tr("&Abort");

        case QMessageBox::Retry:
            return MvdMessageBox::tr("&Retry");

        case QMessageBox::Ignore:
            return MvdMessageBox::tr("&Ignore");

        case QMessageBox::Close:
            return MvdMessageBox::tr("&Close");

        case QMessageBox::Cancel:
            return MvdMessageBox::tr("&Cancel");

        case QMessageBox::Discard:
            return MvdMessageBox::tr("&Discard");

        case QMessageBox::Help:
            return MvdMessageBox::tr("&Help");

        case QMessageBox::Apply:
            return MvdMessageBox::tr("&Apply");

        case QMessageBox::Reset:
            return MvdMessageBox::tr("&Reset");

        case QMessageBox::RestoreDefaults:
            return MvdMessageBox::tr("&Restore Defaults");

        default:
            ;
    }
    return QString();
}

QIcon bi(QMessageBox::StandardButton sb)
{
    QStyle *s = QApplication::style();

    switch (sb) {
        case QMessageBox::Ok:
            return s->standardIcon(QStyle::SP_DialogOkButton);

        case QMessageBox::Save:
            return s->standardIcon(QStyle::SP_DialogSaveButton);

        case QMessageBox::Open:
            return s->standardIcon(QStyle::SP_DialogOpenButton);

        case QMessageBox::Yes:
            return s->standardIcon(QStyle::SP_DialogYesButton);

        case QMessageBox::No:
            return s->standardIcon(QStyle::SP_DialogNoButton);

        case QMessageBox::Abort:
            return s->standardIcon(QStyle::SP_DialogCancelButton);

        case QMessageBox::Close:
            return s->standardIcon(QStyle::SP_DialogCloseButton);

        case QMessageBox::Cancel:
            return s->standardIcon(QStyle::SP_DialogCancelButton);

        case QMessageBox::Discard:
            return s->standardIcon(QStyle::SP_DialogDiscardButton);

        case QMessageBox::Help:
            return s->standardIcon(QStyle::SP_DialogHelpButton);

        case QMessageBox::Apply:
            return s->standardIcon(QStyle::SP_DialogApplyButton);

        case QMessageBox::Reset:
            return s->standardIcon(QStyle::SP_DialogResetButton);

        case QMessageBox::RestoreDefaults:
            return s->standardIcon(QStyle::SP_DialogResetButton);

        default:
            ;
    }
    return QIcon();
}

QMessageBox::StandardButton messageBox(QMessageBox::Icon icon, const QString &title,
    const QString &text, QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton, QWidget *parent)
{

    QMessageBox msgBox(icon, wt(title), text, QMessageBox::NoButton, parent);

    QDialogButtonBox *buttonBox = qFindChild<QDialogButtonBox *>(&msgBox);

    Q_ASSERT(buttonBox);

    uint mask = QMessageBox::FirstButton;
    while (mask <= QMessageBox::LastButton) {
        uint sb = buttons & mask;
        mask <<= 1;
        if (!sb)
            continue;

        QPushButton *button = msgBox.addButton((QMessageBox::StandardButton)sb);
        button->setText(bt((QMessageBox::StandardButton)sb));
        button->setIcon(bi((QMessageBox::StandardButton)sb));

        // Choose the first accept role as the default
        if (msgBox.defaultButton())
            continue;

        if ((defaultButton == QMessageBox::NoButton && buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
            || (defaultButton != QMessageBox::NoButton && sb == uint(defaultButton)))
            msgBox.setDefaultButton(button);
    }

    if (msgBox.exec() == -1)
        return QMessageBox::Cancel;
    return msgBox.standardButton(msgBox.clickedButton());
}

} // anonymous namespace


////////////////////////////////////////////////////////////////////////////////


MvdMessageBox::MvdMessageBox(QWidget *parent) :
    QMessageBox(parent)
{ }

MvdMessageBox::MvdMessageBox(Icon icon, const QString &title, const QString &text,
    StandardButtons buttons, QWidget *parent, Qt::WindowFlags f) :
    QMessageBox(icon, title, text, buttons, parent, f)
{ }

MvdMessageBox::~MvdMessageBox()
{ }

QMessageBox::StandardButton MvdMessageBox::critical(QWidget *parent, const QString &title,
    const QString &text, StandardButtons buttons, StandardButton defaultButton)
{
    return messageBox(QMessageBox::Critical, title, text, buttons, defaultButton, parent);
}

QMessageBox::StandardButton MvdMessageBox::information(QWidget *parent, const QString &title,
    const QString &text, StandardButtons buttons, StandardButton defaultButton)
{
    return messageBox(QMessageBox::Information, title, text, buttons, defaultButton, parent);
}

QMessageBox::StandardButton MvdMessageBox::question(QWidget *parent, const QString &title,
    const QString &text, StandardButtons buttons, StandardButton defaultButton)
{
    return messageBox(QMessageBox::Question, title, text, buttons, defaultButton, parent);
}

QMessageBox::StandardButton MvdMessageBox::warning(QWidget *parent, const QString &title,
    const QString &text, StandardButtons buttons, StandardButton defaultButton)
{
    return messageBox(QMessageBox::Warning, title, text, buttons, defaultButton, parent);
}
