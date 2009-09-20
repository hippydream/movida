/**************************************************************************
** Filename: lineedit.cpp
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

#include "lineedit.h"

#include "completer.h"

#include <QtCore/QPointer>
#include <QtGui/QAbstractItemView>
#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtGui/QStyleOptionFrameV2>

// From QLineEdit
#define verticalMargin 1
#define horizontalMargin 2

class MvdLineEdit::Private
{
public:
    Private(MvdLineEdit *p) :
        completer(0),
        textEdited(false),
        q(p)
    {

    }

    bool advanceToEnabledItem(int dir);
    void complete(int key);

    QPointer<MvdCompleter> completer;
    QString placeHolder;

    bool textEdited;

private:
    MvdLineEdit *q;
};

// looks for an enabled item iterating forward(dir=1)/backward(dir=-1) from the
// current row based. dir=0 indicates a new completion prefix was set.
bool MvdLineEdit::Private::advanceToEnabledItem(int dir)
{
    int start = completer->currentRow();
    if (start == -1)
        return false;
    int i = start + dir;
    if (dir == 0) dir = 1;
    do {
        if (!completer->setCurrentRow(i)) {
            if (!completer->wrapAround())
                break;
            i = i > 0 ? 0 : completer->completionCount() - 1;
        } else {
            QModelIndex currentIndex = completer->currentIndex();
            if (completer->completionModel()->flags(currentIndex) & Qt::ItemIsEnabled)
                return true;
            i += dir;
        }
    } while (i != start);

    completer->setCurrentRow(start); // restore
    return false;
}

void MvdLineEdit::Private::complete(int key)
{
    if (!completer || q->isReadOnly() || q->echoMode() != QLineEdit::Normal)
        return;

    const int selstart = q->selectionStart();
    const int selend = selstart + q->selectedText().length();
    const QString text = q->text();

    if (completer->completionMode() == MvdCompleter::InlineCompletion) {
        if (key == Qt::Key_Backspace)
            return;
        int n = 0;
        if (key == Qt::Key_Up || key == Qt::Key_Down) {
            if (selend != 0 && selend != text.length())
                return;
            QString prefix = q->hasSelectedText() ? text.left(selstart) : text;
            if (text.compare(completer->currentCompletion(), completer->caseSensitivity()) != 0
                || prefix.compare(completer->completionPrefix(), completer->caseSensitivity()) != 0) {
                    completer->setCompletionPrefix(prefix);
            } else {
                n = (key == Qt::Key_Up) ? -1 : +1;
            }
        } else {
            completer->setCompletionPrefix(text);
        }
        if (!advanceToEnabledItem(n))
            return;
    } else {

        if (text.isEmpty()) {
            completer->popup()->hide();
            return;
        }

        completer->setCompletionPrefix(text);
    }

    completer->complete();
}


//////////////////////////////////////////////////////////////////////////


MvdLineEdit::MvdLineEdit(QWidget *parent) :
    QLineEdit(parent), d(new Private(this))
{
    init();
}

MvdLineEdit::MvdLineEdit(const QString &contents, QWidget *parent) :
    QLineEdit(contents, parent), d(new Private(this))
{
    init();
}

MvdLineEdit::~MvdLineEdit()
{
    delete d;
}

void MvdLineEdit::init()
{
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(on_textChanged()));
    connect(this, SIGNAL(textEdited(QString)), this, SLOT(on_textEdited()));
}

QSize MvdLineEdit::sizeHint() const
{
    // Same as QLineEdit::sizeHint() but using QApplication's QStyle
    // to avoid issues with the style sheet style (it will often return a 
    // smaller size)

    int tleft, ttop, tright, tbottom;
    getContentsMargins(&tleft, &ttop, &tright, &tbottom);
    int cleft, ctop, cright, cbottom;
    getContentsMargins(&cleft, &ctop, &cright, &cbottom);

    ensurePolished();
    QFontMetrics fm(font());
    int h = qMax(fm.lineSpacing(), 14) + 2*verticalMargin
        + ttop + tbottom
        + ctop + cbottom;
    int w = fm.width(QLatin1Char('x')) * 17 + 2*horizontalMargin
        + tleft + tright
        + cleft + cright; // "some"
    QStyleOptionFrameV2 opt;
    initStyleOption(&opt);
    return (QApplication::style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).
        expandedTo(QApplication::globalStrut()), this));
}

bool MvdLineEdit::event(QEvent *e)
{
    return QLineEdit::event(e);
}

/*!
    Sets this line edit to provide auto completions from the completer, \a c.
    The completion mode is set using MvdCompleter::setCompletionMode().

    To use a MvdCompleter with a QValidator or QLineEdit::inputMask, you need to
    ensure that the model provided to MvdCompleter contains valid entries. You can
    use the QSortFilterProxyModel to ensure that the MvdCompleter's model contains
    only valid entries.

    If \a c == 0, setAdvancedCompleter() removes the current completer, effectively
    disabling auto completion.

    Please note that any MvdCompleter set with setCompleter() will be disabled
    if an advanced completer is available.

    \sa MvdCompleter
*/
void MvdLineEdit::setAdvancedCompleter(MvdCompleter *c)
{
    if (c == d->completer)
        return;
    if (d->completer) {
        disconnect(d->completer, 0, this, 0);
        d->completer->setWidget(0);
        if (d->completer->parent() == this)
            delete d->completer;
    }
    d->completer = c;
    if (!c)
        return;
    setCompleter(0); // Ensure no QCompleter will be used
    if (c->widget() == 0)
        c->setWidget(this);
    if (hasFocus()) {
        QObject::connect(d->completer, SIGNAL(activated(QString)),
            this, SLOT(completionActivated(QString)));
        QObject::connect(d->completer, SIGNAL(highlighted(QString)),
            this, SLOT(completionHighlighted(QString)));
    }
}

/*!
    Returns the current MvdCompleter that provides completions.
*/
MvdCompleter *MvdLineEdit::advancedCompleter() const
{
    return d->completer;
}

void MvdLineEdit::completionActivated(const QString &text)
{
    setText(text);
}

void MvdLineEdit::completionHighlighted(const QString &newText)
{
    if (d->completer->completionMode() != MvdCompleter::InlineCompletion)
        setText(newText);
    else {
        const QString text = this->text();
        int c = cursorPosition();
        setText(text.left(c) + newText.mid(c));
        setSelection(text.length(), c - newText.length());
    }
}

void MvdLineEdit::keyPressEvent(QKeyEvent *e)
{
    bool inlineCompletionAccepted = false;

    const int selstart = selectionStart();
    const int selend = selstart + selectedText().length();
    const QString text = this->text();

    if (d->completer) {
        setCompleter(0); // Ensure no QCompleter will be used
    }

    if (d->completer) {

        MvdCompleter::CompletionMode completionMode = d->completer->completionMode();
        const bool hasPopupCompletion = completionMode == MvdCompleter::PopupCompletion 
            || completionMode == MvdCompleter::UnfilteredPopupCompletion;

        if (hasPopupCompletion && d->completer->popup() && d->completer->popup()->isVisible()) {

            // The following keys are forwarded by the completer to the widget
            // Ignoring the events lets the completer provide suitable default behavior
            switch (e->key()) {
            case Qt::Key_Escape:
                e->ignore();
                return;

            case Qt::Key_Enter:
            case Qt::Key_Return:
            case Qt::Key_F4:
                d->completer->popup()->hide(); // just hide. will end up propagating to parent

            default:
                break; // normal key processing
           }

        } else if (completionMode == MvdCompleter::InlineCompletion) {

            switch (e->key()) {
            case Qt::Key_Enter:
            case Qt::Key_Return:
            case Qt::Key_F4:
                if (!d->completer->currentCompletion().isEmpty() && selend > selstart
                    && selend == text.length()) {
                    setText(d->completer->currentCompletion());
                    inlineCompletionAccepted = true;
                    return;
                }
            default:
                break; // normal key processing
            }
        }
    }

    bool unknown = false;

    if (e == QKeySequence::MoveToNextChar) {
#if !defined(Q_WS_WIN)
        if (hasSelectedText()) {
#else
        if (hasSelectedText() && d->completer
            && d->completer->completionMode() == MvdCompleter::InlineCompletion) {
#endif
            setCursorPosition(selend);
        } else {
            cursorForward(0, layoutDirection() == Qt::LeftToRight ? 1 : -1);
        }
        return;
    } else if (e == QKeySequence::MoveToPreviousChar) {
#if !defined(Q_WS_WIN)
        if (hasSelectedText()) {
#else
        if (hasSelectedText() && d->completer
            && d->completer->completionMode() == MvdCompleter::InlineCompletion) {
#endif
            setCursorPosition(selstart);
        } else {
            cursorBackward(0, layoutDirection() == Qt::LeftToRight ? 1 : -1);
        }
        return;
    } else {
        if (e->modifiers() & Qt::ControlModifier) {
            switch (e->key()) {
                case Qt::Key_Backspace:
                case Qt::Key_E:
                case Qt::Key_U:
                    break;
                case Qt::Key_Up:
                case Qt::Key_Down:
                    d->complete(e->key());
                    return;
                default:
                    unknown = true;
            }
        } else {
            switch (e->key()) {
            case Qt::Key_Backspace:
                if (!isReadOnly()) {
                    backspace();
                    d->complete(Qt::Key_Backspace);
                    return;
                }
            default:
                unknown = true;
            }
        }
    }

    if (e->key() == Qt::Key_Direction_L || e->key() == Qt::Key_Direction_R) {
        setLayoutDirection((e->key() == Qt::Key_Direction_L) ? Qt::LeftToRight : Qt::RightToLeft);
        QEvent evnt(QEvent::FontChange);
        qApp->sendEvent(this, &evnt); // this will call d->updateTextLayout()
        update();
        unknown = false;
    }

    QLineEdit::keyPressEvent(e);

    if (unknown && !isReadOnly()) {
        QString t = e->text();
        if (!t.isEmpty() && t.at(0).isPrint()) {
            d->complete(e->key());
        }
    }
}

void MvdLineEdit::focusInEvent(QFocusEvent *e)
{
    if (d->completer)
        setCompleter(0); // Ensure no QCompleter will be used

    QLineEdit::focusInEvent(e);

    if (d->completer) {
        d->completer->setWidget(this);
        QObject::connect(d->completer, SIGNAL(activated(QString)),
            this, SLOT(completionActivated(QString)));
        QObject::connect(d->completer, SIGNAL(highlighted(QString)),
            this, SLOT(completionHighlighted(QString)));
    }
}

//! Sets a string to be displayed as place holder when the widget contains no text.
void MvdLineEdit::setPlaceHolder(const QString &s)
{
    d->placeHolder = s.trimmed();
}

//! Returns the current place holder string, if any.
QString MvdLineEdit::placeHolder() const
{
    return d->placeHolder;
}

void MvdLineEdit::paintEvent(QPaintEvent *e)
{
    QLineEdit::paintEvent(e);

    // Draw place holder
    if (d->placeHolder.isEmpty() || !text().isEmpty() || hasFocus())
        return;

    QPainter p(this);

    QRect r;
    const QPalette &pal = palette();

    QStyleOptionFrameV2 panel;
    initStyleOption(&panel);
    r = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
    p.setClipRect(r);

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

    p.setPen(pal.color(QPalette::Disabled, QPalette::Text));
    p.drawText(lineRect, d->placeHolder);
}

void MvdLineEdit::on_textChanged()
{
    if (!d->textEdited) {
        setTextCalled();
    }
    d->textEdited = false;
}

void MvdLineEdit::on_textEdited()
{
    d->textEdited = true;
}

void MvdLineEdit::setTextCalled()
{
    
}
