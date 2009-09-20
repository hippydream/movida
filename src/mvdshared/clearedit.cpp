/**************************************************************************
** Filename: clearedit.cpp
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

#include "clearedit.h"

#include <QtGui/QApplication>
#include <QtGui/QStyle>
#include <QtGui/QStyleOptionFrameV2>
#include <QtGui/QToolButton>

#include <cmath>

// From QLineEdit
#define verticalMargin 1
#define horizontalMargin 2

namespace {
//! Check out style issues with Oxygen
static int spacing()
{
    QStyle *s = QApplication::style();

    if (s->inherits("OxygenStyle"))
        return 3;
    return 2;
}

}

class MvdClearEdit::Private
{
public:
    Private() :
        clearButton(0)
    { }

    QToolButton *clearButton;
    QSize pixmapSize;
    QPixmap pixmap;
    QString defaultValue;
};

MvdClearEdit::MvdClearEdit(QWidget *parent) :
    MvdLineEdit(parent),
    d(new Private)
{
    init(QPixmap(":/images/clear-edit.png"), tr("Clear text."));
}

MvdClearEdit::MvdClearEdit(const QPixmap &pm, const QString &tip, QWidget *parent) :
    MvdLineEdit(parent),
    d(new Private)
{
    init(pm, tip);
}

MvdClearEdit::~MvdClearEdit()
{
    delete d;
}

void MvdClearEdit::init(const QPixmap &pixmap, const QString &tip)
{
    d->pixmap = pixmap;
    d->pixmapSize = pixmap.size();

    d->clearButton = new QToolButton(this);
    d->clearButton->setToolTip(tip);
    d->clearButton->setIcon(QIcon(pixmap));
    d->clearButton->setIconSize(pixmap.size());
    d->clearButton->setCursor(Qt::PointingHandCursor);
    d->clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    d->clearButton->setFocusPolicy(Qt::NoFocus);
    d->clearButton->hide();

    connect(d->clearButton, SIGNAL(clicked()), this, SLOT(on_buttonClicked()));
    connect(d->clearButton, SIGNAL(clicked()), this, SIGNAL(embeddedActionTriggered()));
    connect(this, SIGNAL(textChanged(const QString &)), this, SLOT(updateButton(const QString &)));

    QString styleSheet = QLatin1String("MvdClearEdit {");
    styleSheet += QString(" padding-right: %1;").arg(d->pixmapSize.width() + 2 * ::spacing());

    if (style()->inherits("OxygenStyle"))   // Focus effect is too close to the text! Add more spacing.
        styleSheet += QString(" padding-left: %1;").arg(::spacing());

    styleSheet += QLatin1String("}");
    setStyleSheet(styleSheet);
}

QAbstractButton *MvdClearEdit::button() const
{
    return d->clearButton;
}

QPixmap MvdClearEdit::pixmap() const
{
    return d->pixmap;
}

QSize MvdClearEdit::pixmapSize() const
{
    return d->pixmapSize;
}

/*! Sets a new pixmap for the embedded button. The pixmap is scaled to
    MvdClearEdit::pixmapSize() if necessary (currently always 14x10).
*/
void MvdClearEdit::setPixmap(const QPixmap& pm)
{
    if (d->pixmap.data_ptr() == const_cast<QPixmap&>(pm).data_ptr())
        return;
    d->pixmap = pm.scaled(d->pixmapSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    d->clearButton->setIcon(d->pixmap);
}

QString MvdClearEdit::toolTip() const
{
    return d->clearButton->toolTip();
}

void MvdClearEdit::setToolTip(const QString& tip)
{
    d->clearButton->setToolTip(tip);
}

void MvdClearEdit::resizeEvent(QResizeEvent *e)
{
    MvdLineEdit::resizeEvent(e);

    QStyleOptionFrameV2 opt;
    initStyleOption(&opt);

    QRect r = style()->subElementRect(QStyle::SE_LineEditContents, &opt, this);

    int vscroll;
    QFontMetrics fm = fontMetrics();
    Qt::Alignment va = QStyle::visualAlignment(layoutDirection(), QFlag(alignment()));
    switch (va & Qt::AlignVertical_Mask) {
        case Qt::AlignBottom:
            vscroll = r.y() + r.height() - fm.height() - verticalMargin;
            break;

        case Qt::AlignTop:
            vscroll = r.y() + verticalMargin;
            break;

        default:
            //center
            vscroll = r.y() + (r.height() - fm.height() + 1) / 2;
            break;
    }

    QRect lineRect(r.x() + horizontalMargin * 3, vscroll, r.width() - 2 * horizontalMargin, fm.height());

    d->clearButton->move(
        lineRect.right(),
        (int)ceil((rect().height() - d->pixmapSize.height()) / 2.0)
        );
}

QSize MvdClearEdit::sizeHint() const
{
    return MvdLineEdit::sizeHint();
}

void MvdClearEdit::updateButton(const QString &text)
{
    d->clearButton->setVisible(!text.isEmpty());
}

void MvdClearEdit::on_buttonClicked()
{
    clear();
}


//////////////////////////////////////////////////////////////////////////


class MvdResetEdit::Private
{
public:
    Private(MvdResetEdit *p) :
      defaultValueSet(false),
      state(ClearButton),
      q(p)
    { }

    QString defaultValue;
    bool defaultValueSet;
    
    enum State { ClearButton, ResetButton } state;

    void setState(State s) {
        if (state == s)
            return;
        state = s;

        if (s == ResetButton) {
            static const QPixmap pm = QPixmap(":/images/reset-edit.png");
            q->setPixmap(pm);
            q->setToolTip(MvdResetEdit::tr("Reset to default value."));
        } else {
            static const QPixmap pm = QPixmap(":/images/clear-edit.png");
            q->setPixmap(pm);
            q->setToolTip(MvdClearEdit::tr("Clear text."));
        }
    }

private:
    MvdResetEdit *q;
};


MvdResetEdit::MvdResetEdit(QWidget *parent) :
    MvdClearEdit(parent),
    d(new Private(this))
{
}

MvdResetEdit::~MvdResetEdit()
{
    delete d;
}

void MvdResetEdit::on_buttonClicked()
{
    if (d->state == Private::ResetButton)
        setText(d->defaultValue);
    else clear();
}


/*! Sets a string that will be set when the reset button is clicked.
    The string will be initialized the first time you call setText()
    so it's usually not necessary to call this method.
*/
void MvdResetEdit::setDefaultValue(const QString &s)
{
    if (d->defaultValue == s && d->defaultValueSet)
        return;

    d->defaultValue = s;
    d->defaultValueSet = true;

    if (text() == d->defaultValue)
        d->setState(Private::ClearButton);
    else d->setState(Private::ResetButton);
}

//! Returns the default value string, if any.
QString MvdResetEdit::defaultValue() const
{
    return d->defaultValue;
}

void MvdResetEdit::setTextCalled()
{
    MvdLineEdit::setTextCalled();

    if (!d->defaultValueSet)
        setDefaultValue(text());
}

void MvdResetEdit::updateButton(const QString &text)
{
    button()->setVisible(true);
    if (d->defaultValueSet) {
        const bool hasDefault = text == d->defaultValue;
        d->setState(hasDefault ? Private::ClearButton : Private::ResetButton);
        if (d->defaultValue.trimmed().isEmpty() && text.trimmed().isEmpty())
            button()->setVisible(false);
    } else {
        MvdClearEdit::updateButton(text);
        d->setState(Private::ClearButton);
    }
}
