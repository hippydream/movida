/**************************************************************************
** Filename: completer.cpp
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

/*!
    \class MvdCompleter
    \ingroup MovidaShared
    \brief The MvdCompleter class is very similar to MvdCompleter but offers
    advanced features and allows subclasses to extend the class.

    This class will become deprecated if future versions of MvdCompleter will
    fix its current limitations (mainly the impossobility to customize the
    completer in a subclass - e.g. to implement locale aware completion).

    You can use MvdCompleter to provide auto completions in MvdLineEdit
    and MvdComboBox.

    When the user starts typing a word, MvdCompleter suggests possible ways of
    completing the word, based on a word list. The word list is
    provided as a QAbstractItemModel. (For simple applications, where
    the word list is static, you can pass a QStringList to
    MvdCompleter's constructor.)

    \tableofcontents

    \section1 Basic Usage

    A MvdCompleter is used typically with a MvdLineEdit or MvdComboBox.
    For example, here's how to provide auto completions from a simple
    word list in a MvdLineEdit:

    \snippet doc/src/snippets/code/src_gui_util_MvdCompleter.cpp 0

    A QDirModel can be used to provide auto completion of file names.
    For example:

    \snippet doc/src/snippets/code/src_gui_util_MvdCompleter.cpp 1

    To set the model on which MvdCompleter should operate, call
    setModel(). By default, MvdCompleter will attempt to match the \l
    {completionPrefix}{completion prefix} (i.e., the word that the
    user has started typing) against the Qt::EditRole data stored in
    column 0 in the  model case sensitively. This can be changed
    using setCompletionRole(), setCompletionColumn(), and
    setCaseSensitivity().

    If the model is sorted on the column and role that are used for completion,
    you can call setModelSorting() with either
    MvdCompleter::CaseSensitivelySortedModel or
    MvdCompleter::CaseInsensitivelySortedModel as the argument. On large models,
    this can lead to significant performance improvements, because MvdCompleter
    can then use binary search instead of linear search.

    The model can be a \l{QAbstractListModel}{list model},
    a \l{QAbstractTableModel}{table model}, or a
    \l{QAbstractItemModel}{tree model}. Completion on tree models
    is slightly more involved and is covered in the \l{Handling
    Tree Models} section below.

    The completionMode() determines the mode used to provide completions to
    the user.

    \section1 Iterating Through Completions

    To retrieve a single candidate string, call setCompletionPrefix()
    with the text that needs to be completed and call
    currentCompletion(). You can iterate through the list of
    completions as below:

    \snippet doc/src/snippets/code/src_gui_util_MvdCompleter.cpp 2

    completionCount() returns the total number of completions for the
    current prefix. completionCount() should be avoided when possible,
    since it requires a scan of the entire model.

    \section1 The Completion Model

    completionModel() return a list model that contains all possible
    completions for the current completion prefix, in the order in which
    they appear in the model. This model can be used to display the current
    completions in a custom view. Calling setCompletionPrefix() automatically
    refreshes the completion model.

    \section1 Handling Tree Models

    MvdCompleter can look for completions in tree models, assuming
    that any item (or sub-item or sub-sub-item) can be unambiguously
    represented as a string by specifying the path to the item. The
    completion is then performed one level at a time.

    Let's take the example of a user typing in a file system path.
    The model is a (hierarchical) QDirModel. The completion
    occurs for every element in the path. For example, if the current
    text is \c C:\Wind, MvdCompleter might suggest \c Windows to
    complete the current path element. Similarly, if the current text
    is \c C:\Windows\Sy, MvdCompleter might suggest \c System.

    For this kind of completion to work, MvdCompleter needs to be able to
    split the path into a list of strings that are matched at each level.
    For \c C:\Windows\Sy, it needs to be split as "C:", "Windows" and "Sy".
    The default implementation of splitPath(), splits the completionPrefix
    using QDir::separator() if the model is a QDirModel.

    To provide completions, MvdCompleter needs to know the path from an index.
    This is provided by pathFromIndex(). The default implementation of
    pathFromIndex(), returns the data for the \l{Qt::EditRole}{edit role}
    for list models and the absolute file path if the mode is a QDirModel.

    \sa QAbstractItemModel, MvdLineEdit, MvdComboBox, {Completer Example}
*/

#include "completer_p.h"

#include <QtCore/QEvent>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QDirModel>
#include <QtGui/QHeaderView>
#include <QtGui/QHeaderView>
#include <QtGui/QKeyEvent>
#include <QtGui/QListView>
#include <QtGui/QScrollBar>
#include <QtGui/QStringListModel>

#include <climits>


void MvdCompleter::Private::init(QAbstractItemModel *m)
{
    proxy = new MvdCompletionModel(q, q);

    QObject::connect(proxy, SIGNAL(rowsAdded()), this, SLOT(autoResizePopup()));

    q->setModel(m);
    q->setCompletionMode(MvdCompleter::PopupCompletion);
}

void MvdCompleter::Private::setCurrentIndex(QModelIndex index, bool select)
{
    if (!q->popup())
        return;
    if (!select) {
        popup->selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
    } else {
        if (!index.isValid())
            popup->selectionModel()->clear();
        else
            popup->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select
                                                            | QItemSelectionModel::Rows);
    }
    index = popup->selectionModel()->currentIndex();
    if (!index.isValid())
        popup->scrollToTop();
    else
        popup->scrollTo(index, QAbstractItemView::PositionAtTop);
}

void MvdCompleter::Private::completionSelected(const QItemSelection& selection)
{
    QModelIndex index;
    if (!selection.indexes().isEmpty())
        index = selection.indexes().first();

    complete(index, true);
}

void MvdCompleter::Private::complete(QModelIndex index, bool highlighted)
{
    QString completion;

    if (!index.isValid() || (!proxy->showAll() && (index.row() >= proxy->engine()->matchCount()))) {
        completion = prefix;
    } else {
        QModelIndex si = proxy->mapToSource(index);
        si = si.sibling(si.row(), column); // for clicked()
        completion = q->pathFromIndex(si);
#ifndef QT_NO_DIRMODEL
        // add a trailing separator in inline
        if (mode == MvdCompleter::InlineCompletion) {
            if (qobject_cast<QDirModel *>(proxy->sourceModel()) && QFileInfo(completion).isDir())
                completion += QDir::separator();
        }
#endif
    }

    // Emit signals
    if (highlighted) {
        q->highlighted(index);
        q->highlighted(completion);
    } else {
        q->activated(index);
        q->activated(completion);
    }
}

void MvdCompleter::Private::autoResizePopup()
{
    if (!popup || !popup->isVisible())
        return;
    showPopup(popupRect);
}

void MvdCompleter::Private::showPopup(const QRect& rect)
{
    const QRect screen = QApplication::desktop()->availableGeometry(widget);
    Qt::LayoutDirection dir = widget->layoutDirection();
    QPoint pos;
    int rw, rh, w;
    int h = (popup->sizeHintForRow(0) * qMin(7, popup->model()->rowCount()) + 3) + 3;
    QScrollBar *hsb = popup->horizontalScrollBar();
    if (hsb && hsb->isVisible())
        h += popup->horizontalScrollBar()->sizeHint().height();

    if (rect.isValid()) {
        rh = rect.height();
        w = rw = rect.width();
        pos = widget->mapToGlobal(dir == Qt::RightToLeft ? rect.bottomRight() : rect.bottomLeft());
    } else {
        rh = widget->height();
        rw = widget->width();
        pos = widget->mapToGlobal(QPoint(0, widget->height() - 2));
        w = widget->width();
    }

    if ((pos.x() + rw) > (screen.x() + screen.width()))
        pos.setX(screen.x() + screen.width() - w);
    if (pos.x() < screen.x())
        pos.setX(screen.x());
    if (((pos.y() + rh) > (screen.y() + screen.height())) && ((pos.y() - h - rh) >= 0))
        pos.setY(pos.y() - qMax(h, popup->minimumHeight()) - rh + 2);

    popup->setGeometry(pos.x(), pos.y(), w, h);

    if (!popup->isVisible())
        popup->show();
}


///////////////////////////////////////////////////////////////////////////////


/*!
    Constructs a completer object with the given \a parent.
*/
MvdCompleter::MvdCompleter(QObject *parent) :
    QObject(parent),
    d(new Private(this))
{
    d->init();
}

/*!
    Constructs a completer object with the given \a parent that provides completions
    from the specified \a model.
*/
MvdCompleter::MvdCompleter(QAbstractItemModel *model, QObject *parent) :
    QObject(parent),
    d(new Private(this))
{
    d->init(model);
}

/*!
    Constructs a MvdCompleter object with the given \a parent that uses the specified
    \a list as a source of possible completions.
*/
MvdCompleter::MvdCompleter(const QStringList& list, QObject *parent) :
    QObject(parent),
    d(new Private(this))
{
    d->init(new QStringListModel(list, this));
}

/*!
    Destroys the completer object.
*/
MvdCompleter::~MvdCompleter()
{
    delete d;
}

/*!
    Sets the widget for which completion are provided for to \a widget. This
    function is automatically called when a MvdCompleter is set on a MvdLineEdit
    using MvdLineEdit::setCompleter() or on a MvdComboBox using
    MvdComboBox::setCompleter(). The widget needs to be set explicitly when
    providing completions for custom widgets.

    \sa widget(), setModel(), setPopup()
 */
void MvdCompleter::setWidget(QWidget *widget)
{
    if (d->widget)
        d->widget->removeEventFilter(this);
    d->widget = widget;
    if (d->widget)
        d->widget->installEventFilter(this);
    if (d->popup) {
        d->popup->hide();
        d->popup->setFocusProxy(d->widget);
    }
}

/*!
    Returns the widget for which the completer object is providing completions.

    \sa setWidget()
 */
QWidget *MvdCompleter::widget() const
{
    return d->widget;
}

/*!
    Sets the model which provides completions to \a model. The \a model can
    be list model or a tree model. If a model has been already previously set
    and it has the MvdCompleter as its parent, it is deleted.

    For convenience, if \a model is a QDirModel, MvdCompleter switches its
    caseSensitivity to Qt::CaseInsensitive on Windows and Qt::CaseSensitive
    on other platforms.

    \sa completionModel(), modelSorting, {Handling Tree Models}
*/
void MvdCompleter::setModel(QAbstractItemModel *model)
{
    QAbstractItemModel *oldModel = d->proxy->sourceModel();
    d->proxy->setSourceModel(model);
    if (d->popup)
        setPopup(d->popup); // set the model and make new connections
    if (oldModel && oldModel->QObject::parent() == this)
        delete oldModel;

    if (qobject_cast<QDirModel *>(model)) {
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
        setCaseSensitivity(Qt::CaseInsensitive);
#else
        setCaseSensitivity(Qt::CaseSensitive);
#endif
    }
}

/*!
    Returns the model that provides completion strings.

    \sa completionModel()
*/
QAbstractItemModel *MvdCompleter::model() const
{
    return d->proxy->sourceModel();
}

/*!
    \enum MvdCompleter::CompletionMode

    This enum specifies how completions are provided to the user.

    \value PopupCompletion            Current completions are displayed in a popup window.
    \value InlineCompletion           Completions appear inline (as selected text).
    \value UnfilteredPopupCompletion  All possible completions are displayed in a popup window
                                      with the most likely suggestion indicated as current.

    \sa setCompletionMode()
*/

/*!
    \property MvdCompleter::completionMode
    \brief how the completions are provided to the user

    The default value is MvdCompleter::PopupCompletion.
*/
void MvdCompleter::setCompletionMode(MvdCompleter::CompletionMode mode)
{
    d->mode = mode;
    d->proxy->setFiltered(mode != MvdCompleter::UnfilteredPopupCompletion);

    if (mode == MvdCompleter::InlineCompletion) {
        if (d->widget)
            d->widget->removeEventFilter(this);
        if (d->popup) {
            d->popup->deleteLater();
            d->popup = 0;
        }
    } else {
        if (d->widget)
            d->widget->installEventFilter(this);
    }
}

MvdCompleter::CompletionMode MvdCompleter::completionMode() const
{
    return d->mode;
}

/*!
    Sets the popup used to display completions to \a popup. MvdCompleter takes
    ownership of the view.

    A QListView is automatically created when the completionMode() is set to
    MvdCompleter::PopupCompletion or MvdCompleter::UnfilteredPopupCompletion. The
    default popup displays the completionColumn().

    Ensure that this function is called before the view settings are modified.
    This is required since view's properties may require that a model has been
    set on the view (for example, hiding columns in the view requires a model
    to be set on the view).

    \sa popup()
*/
void MvdCompleter::setPopup(QAbstractItemView *popup)
{
    Q_ASSERT(popup != 0);
    if (d->popup) {
        QObject::disconnect(d->popup->selectionModel(), 0, this, 0);
        QObject::disconnect(d->popup, 0, this, 0);
    }
    if (d->popup != popup)
        delete d->popup;
    if (popup->model() != d->proxy)
        popup->setModel(d->proxy);
#ifdef Q_OS_MAC
     popup->show();
#else
     popup->hide();
#endif
    popup->setParent(0, Qt::Popup);

    Qt::FocusPolicy origPolicy = Qt::NoFocus;
    if (d->widget)
        origPolicy = d->widget->focusPolicy();
    popup->setFocusPolicy(Qt::NoFocus);
    if (d->widget)
        d->widget->setFocusPolicy(origPolicy);

    popup->setFocusProxy(d->widget);
    popup->installEventFilter(this);
    popup->setItemDelegate(new MvdCompleterItemDelegate(popup));
    if (QListView *listView = qobject_cast<QListView *>(popup)) {
        listView->setModelColumn(d->column);
    }

    QObject::connect(popup, SIGNAL(clicked(QModelIndex)),
                     d, SLOT(complete(QModelIndex)));
    QObject::connect(popup, SIGNAL(clicked(QModelIndex)), popup, SLOT(hide()));

    QObject::connect(popup->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                     d, SLOT(completionSelected(QItemSelection)));
    d->popup = popup;
}

/*!
    Returns the popup used to display completions.

    \sa setPopup()
*/
QAbstractItemView *MvdCompleter::popup() const
{
    if (!d->popup && completionMode() != MvdCompleter::InlineCompletion) {
        QListView *listView = new QListView;
        listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        listView->setSelectionBehavior(QAbstractItemView::SelectRows);
        listView->setSelectionMode(QAbstractItemView::SingleSelection);
        listView->setModelColumn(d->column);
        MvdCompleter *that = const_cast<MvdCompleter*>(this);
        that->setPopup(listView);
    }

    return d->popup;
}

/*!
  \reimp
*/
bool MvdCompleter::event(QEvent *ev)
{
    return QObject::event(ev);
}

/*!
  \reimp
*/
bool MvdCompleter::eventFilter(QObject *o, QEvent *e)
{
    if (d->eatFocusOut && o == d->widget && e->type() == QEvent::FocusOut) {
        if (d->popup && d->popup->isVisible())
            return true;
    }

    if (o != d->popup)
        return QObject::eventFilter(o, e);

    switch (e->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);

        QModelIndex curIndex = d->popup->currentIndex();
        QModelIndexList selList = d->popup->selectionModel()->selectedIndexes();

        const int key = ke->key();
        // In UnFilteredPopup mode, select the current item
        if ((key == Qt::Key_Up || key == Qt::Key_Down) && selList.isEmpty() && curIndex.isValid()
            && d->mode == MvdCompleter::UnfilteredPopupCompletion) {
              d->setCurrentIndex(curIndex);
              return true;
        }

        // Handle popup navigation keys. These are hardcoded because up/down might make the
        // widget do something else (lineedit cursor moves to home/end on mac, for instance)
        switch (key) {
        case Qt::Key_End:
        case Qt::Key_Home:
            if (ke->modifiers() & Qt::ControlModifier)
                return false;
            break;

        case Qt::Key_Up:
            if (!curIndex.isValid()) {
                int rowCount = d->proxy->rowCount();
                QModelIndex lastIndex = d->proxy->index(rowCount - 1, 0);
                d->setCurrentIndex(lastIndex);
                return true;
            } else if (curIndex.row() == 0) {
                if (d->wrap)
                    d->setCurrentIndex(QModelIndex());
                return true;
            }
            return false;

        case Qt::Key_Down:
            if (!curIndex.isValid()) {
                QModelIndex firstIndex = d->proxy->index(0, 0);
                d->setCurrentIndex(firstIndex);
                return true;
            } else if (curIndex.row() == d->proxy->rowCount() - 1) {
                if (d->wrap)
                    d->setCurrentIndex(QModelIndex());
                return true;
            }
            return false;

        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            return false;
        }

        // Send the event to the widget. If the widget accepted the event, do nothing
        // If the widget did not accept the event, provide a default implementation
        d->eatFocusOut = false;
        (static_cast<QObject *>(d->widget))->event(ke);
        d->eatFocusOut = true;
        if (!d->widget || e->isAccepted() || !d->popup->isVisible()) {
            // widget lost focus, hide the popup
            if (d->widget && (!d->widget->hasFocus()
#ifdef QT_KEYPAD_NAVIGATION
                || (QApplication::keypadNavigationEnabled() && !d->widget->hasEditFocus())
#endif
                ))
                d->popup->hide();
            if (e->isAccepted())
                return true;
        }

        // default implementation for keys not handled by the widget when popup is open
        switch (key) {
#ifdef QT_KEYPAD_NAVIGATION
        case Qt::Key_Select:
            if (!QApplication::keypadNavigationEnabled())
                break;
#endif
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Tab:
            d->popup->hide();
            if (curIndex.isValid())
                d->complete(curIndex);
            break;

        case Qt::Key_F4:
            if (ke->modifiers() & Qt::AltModifier)
                d->popup->hide();
            break;

        case Qt::Key_Backtab:
        case Qt::Key_Escape:
            d->popup->hide();
            break;

        default:
            break;
        }

        return true;
    }

    case QEvent::MouseButtonPress: {
        if (!d->popup->underMouse()) {
            d->popup->hide();
            return true;
        }
        }
        return false;

    case QEvent::InputMethod:
    case QEvent::ShortcutOverride:
        QApplication::sendEvent(d->widget, e);
        break;

    default:
        return false;
    }
    return false;
}

/*!
    For MvdCompleter::PopupCompletion and MvdCompletion::UnfilteredPopupCompletion
    modes, calling this function displays the popup displaying the current
    completions. By default, if \a rect is not specified, the popup is displayed
    on the bottom of the widget(). If \a rect is specified the popup is
    displayed on the left edge of the rectangle.

    For MvdCompleter::InlineCompletion mode, the highlighted() signal is fired
    with the current completion.
*/
void MvdCompleter::complete(const QRect& rect)
{
    QModelIndex idx = d->proxy->currentIndex(false);
    if (d->mode == MvdCompleter::InlineCompletion) {
        if (idx.isValid())
            d->complete(idx, true);
        return;
    }

    Q_ASSERT(d->widget != 0);
    if ((d->mode == MvdCompleter::PopupCompletion && !idx.isValid())
        || (d->mode == MvdCompleter::UnfilteredPopupCompletion && d->proxy->rowCount() == 0)) {
        if (d->popup)
            d->popup->hide(); // no suggestion, hide
        return;
    }

    popup();
    if (d->mode == MvdCompleter::UnfilteredPopupCompletion)
        d->setCurrentIndex(idx, false);

    d->showPopup(rect);
    d->popupRect = rect;
}

/*!
    Sets the current row to the \a row specified. Returns true if successful;
    otherwise returns false.

    This function may be used along with currentCompletion() to iterate
    through all the possible completions.

    \sa currentCompletion(), completionCount()
*/
bool MvdCompleter::setCurrentRow(int row)
{
    return d->proxy->setCurrentRow(row);
}

/*!
    Returns the current row.

    \sa setCurrentRow()
*/
int MvdCompleter::currentRow() const
{
    return d->proxy->currentRow();
}

/*!
    Returns the number of completions for the current prefix. For an unsorted
    model with a large number of items this can be expensive. Use setCurrentRow()
    and currentCompletion() to iterate through all the completions.
*/
int MvdCompleter::completionCount() const
{
    return d->proxy->completionCount();
}

/*!
    \enum MvdCompleter::ModelSorting

    This enum specifies how the items in the model are sorted.

    \value UnsortedModel                    The model is unsorted.
    \value CaseSensitivelySortedModel       The model is sorted case sensitively.
    \value CaseInsensitivelySortedModel     The model is sorted case insensitively.

    \sa setModelSorting()
*/

/*!
    \property MvdCompleter::modelSorting
    \brief the way the model is sorted

    By default, no assumptions are made about the order of the items
    in the model that provides the completions.

    If the model's data for the completionColumn() and completionRole() is sorted in
    ascending order, you can set this property to \l CaseSensitivelySortedModel
    or \l CaseInsensitivelySortedModel. On large models, this can lead to
    significant performance improvements because the completer object can
    then use a binary search algorithm instead of linear search algorithm.

    The sort order (i.e ascending or descending order) of the model is determined
    dynamically by inspecting the contents of the model.

    \bold{Note:} The performance improvements described above cannot take place
    when the completer's \l caseSensitivity is different to the case sensitivity
    used by the model's when sorting.

    \sa setCaseSensitivity(), MvdCompleter::ModelSorting
*/
void MvdCompleter::setModelSorting(MvdCompleter::ModelSorting sorting)
{
    if (d->sorting == sorting)
        return;
    d->sorting = sorting;
    d->proxy->updateEngine();
    d->proxy->invalidate();
}

MvdCompleter::ModelSorting MvdCompleter::modelSorting() const
{
    return d->sorting;
}

/*!
    \property MvdCompleter::completionColumn
    \brief the column in the model in which completions are searched for.

    If the popup() is a QListView, it is automatically setup to display
    this column.

    By default, the match column is 0.

    \sa completionRole, caseSensitivity
*/
void MvdCompleter::setCompletionColumn(int column)
{
    if (d->column == column)
        return;
    if (QListView *listView = qobject_cast<QListView *>(d->popup))
        listView->setModelColumn(column);
    d->column = column;
    d->proxy->invalidate();
}

int MvdCompleter::completionColumn() const
{
    return d->column;
}

/*!
    \property MvdCompleter::completionRole
    \brief the item role to be used to query the contents of items for matching.

    The default role is Qt::EditRole.

    \sa completionColumn, caseSensitivity
*/
void MvdCompleter::setCompletionRole(int role)
{
    if (d->role == role)
        return;
    d->role = role;
    d->proxy->invalidate();
}

int MvdCompleter::completionRole() const
{
    return d->role;
}

/*!
    \property MvdCompleter::wrapAround
    \brief the completions wrap around when navigating through items
    \since 4.3

    The default is true.
*/
void MvdCompleter::setWrapAround(bool wrap)
{
    if (d->wrap == wrap)
        return;
    d->wrap = wrap;
}

bool MvdCompleter::wrapAround() const
{
    return d->wrap;
}

/*!
    \property MvdCompleter::caseSensitivity
    \brief the case sensitivity of the matching

    The default is Qt::CaseSensitive.

    \sa completionColumn, completionRole, modelSorting
*/
void MvdCompleter::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    if (d->cs == cs)
        return;
    d->cs = cs;
    d->proxy->updateEngine();
    d->proxy->invalidate();
}

Qt::CaseSensitivity MvdCompleter::caseSensitivity() const
{
    return d->cs;
}

/*!
    \property MvdCompleter::completionPrefix
    \brief the completion prefix used to provide completions.

    The completionModel() is updated to reflect the list of possible
    matches for \a prefix.
*/
void MvdCompleter::setCompletionPrefix(const QString &prefix)
{
    if (d->prefix == prefix)
        return;
    d->prefix = prefix;
    d->proxy->filter(splitPath(prefix));
}

QString MvdCompleter::completionPrefix() const
{
    return d->prefix;
}

/*!
    Returns the model index of the current completion in the completionModel().

    \sa setCurrentRow(), currentCompletion(), model()
*/
QModelIndex MvdCompleter::currentIndex() const
{
    return d->proxy->currentIndex(false);
}

/*!
    Returns the current completion string. This includes the \l completionPrefix.
    When used alongside setCurrentRow(), it can be used to iterate through
    all the matches.

    \sa setCurrentRow(), currentIndex()
*/
QString MvdCompleter::currentCompletion() const
{
    return pathFromIndex(d->proxy->currentIndex(true));
}

/*!
    Returns the completion model. The completion model is a read-only list model
    that contains all the possible matches for the current completion prefix.
    The completion model is auto-updated to reflect the current completions.

    \sa completionPrefix, model()
*/
MvdCompletionModel *MvdCompleter::completionModel() const
{
    return d->proxy;
}

/*!
    Returns the path for the given \a index. The completer object uses this to
    obtain the completion text from the underlying model.

    The default implementation returns the \l{Qt::EditRole}{edit role} of the
    item for list models. It returns the absolute file path if the model is a
    QDirModel.

    \sa splitPath()
*/
QString MvdCompleter::pathFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return QString();

    QAbstractItemModel *sourceModel = d->proxy->sourceModel();
    if (!sourceModel)
        return QString();

    QDirModel *dirModel = qobject_cast<QDirModel *>(sourceModel);
    if (!dirModel)
        return sourceModel->data(index, d->role).toString();

    QModelIndex idx = index;
    QStringList list;
    do {
        QString t = sourceModel->data(idx, Qt::EditRole).toString();
        list.prepend(t);
        QModelIndex parent = idx.parent();
        idx = parent.sibling(parent.row(), index.column());
    } while (idx.isValid());

#if !defined(Q_OS_WIN) || defined(Q_OS_WINCE)
    if (list.count() == 1) // only the separator or some other text
        return list[0];
    list[0].clear() ; // the join below will provide the separator
#endif

    return list.join(QDir::separator());
}

/*!
    Splits the given \a path into strings that are used to match at each level
    in the model().

    The default implementation of splitPath() splits a file system path based on
    QDir::separator() when the sourceModel() is a QDirModel.

    When used with list models, the first item in the returned list is used for
    matching.

    \sa pathFromIndex(), {Handling Tree Models}
*/
QStringList MvdCompleter::splitPath(const QString& path) const
{
    bool isDirModel = false;

    isDirModel = qobject_cast<QDirModel *>(d->proxy->sourceModel()) != 0;

    if (!isDirModel || path.isEmpty())
        return QStringList(completionPrefix());

    QString pathCopy = QDir::toNativeSeparators(path);
    QString sep = QDir::separator();
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    if (pathCopy == QLatin1String("\\") || pathCopy == QLatin1String("\\\\"))
        return QStringList(pathCopy);
    QString doubleSlash(QLatin1String("\\\\"));
    if (pathCopy.startsWith(doubleSlash))
        pathCopy = pathCopy.mid(2);
    else
        doubleSlash.clear();
#endif

    QRegExp re(QLatin1String("[") + QRegExp::escape(sep) + QLatin1String("]"));
    QStringList parts = pathCopy.split(re);

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    if (!doubleSlash.isEmpty())
        parts[0].prepend(doubleSlash);
#else
    if (pathCopy[0] == sep[0]) // readd the "/" at the beginning as the split removed it
        parts[0] = QDir::fromNativeSeparators(QString(sep[0]));
#endif

    return parts;
}

/*!
    \fn void MvdCompleter::activated(const QModelIndex& index)

    This signal is sent when an item in the popup() is activated by the user.
    (by clicking or pressing return). The item's \a index in the completionModel()
    is given.

*/

/*!
    \fn void MvdCompleter::activated(const QString &text)

    This signal is sent when an item in the popup() is activated by the user (by
    clicking or pressing return). The item's \a text is given.

*/

/*!
    \fn void MvdCompleter::highlighted(const QModelIndex& index)

    This signal is sent when an item in the popup() is highlighted by
    the user. It is also sent if complete() is called with the completionMode()
    set to MvdCompleter::InlineCompletion. The item's \a index in the completionModel()
    is given.
*/

/*!
    \fn void MvdCompleter::highlighted(const QString &text)

    This signal is sent when an item in the popup() is highlighted by
    the user. It is also sent if complete() is called with the completionMode()
    set to MvdCompleter::InlineCompletion. The item's \a text is given.
*/


///////////////////////////////////////////////////////////////////////////////


MvdCompletionEngine::MvdCompletionEngine(MvdCompleter *c) :
    d(new Private(this))
{
    d->completer = c;
}

MvdCompletionEngine::~MvdCompletionEngine()
{
    delete d;
}

MvdCompleter *MvdCompletionEngine::completer() const
{
    return d->completer;
}

MvdMatchData& MvdCompletionEngine::currentMatch() const
{
    return d->curMatch;
}

MvdMatchData& MvdCompletionEngine::historyMatch() const
{
    return d->historyMatch;
}

int MvdCompletionEngine::matchCount() const
{
    return d->curMatch.indices.count() + d->historyMatch.indices.count();
}

int MvdCompletionEngine::currentRow() const
{
    return d->curRow;
}

void MvdCompletionEngine::setCurrentRow(int n)
{
    d->curRow = n;
}

int MvdCompletionEngine::currentPartsCount() const
{
    return d->curParts.size();
}

QStringList MvdCompletionEngine::currentParts() const
{
    return d->curParts;
}

void MvdCompletionEngine::clearCache()
{
    d->cache.clear();
}

QModelIndex MvdCompletionEngine::currentParent() const
{
    return d->curParent;
}


///////////////////////////////////////////////////////////////////////////////


MvdCompletionModel::MvdCompletionModel(MvdCompleter *c, QObject *parent) :
    QAbstractProxyModel(parent),
    d(new Private(this))
{
    d->completer = c;
    d->createDefaultEngine();
}

MvdCompletionModel::~MvdCompletionModel()
{
    delete d;
}

void MvdCompletionModel::updateEngine()
{
    d->createDefaultEngine();
}

MvdCompleter *MvdCompletionModel::completer() const
{
    return d->completer;
}

MvdCompletionEngine *MvdCompletionModel::engine() const
{
    return d->engine;
}

bool MvdCompletionModel::showAll() const
{
    return d->showAll;
}

int MvdCompletionModel::currentRow() const
{
    return d->engine->currentRow();
}

int MvdCompletionModel::columnCount(const QModelIndex &) const
{
    QAbstractItemModel* m = sourceModel();
    return m ? m->columnCount() : 0;
}

void MvdCompletionModel::setSourceModel(QAbstractItemModel *source)
{
    bool hadModel = (sourceModel() != 0);

    if (hadModel)
        QObject::disconnect(sourceModel(), 0, this, 0);

    QAbstractProxyModel::setSourceModel(source);

    if (source) {
        //! \todo Optimize updates in the source model
        connect(source, SIGNAL(modelReset()), this, SLOT(invalidate()));
        connect(source, SIGNAL(destroyed()), this, SLOT(modelDestroyed()));
        connect(source, SIGNAL(layoutChanged()), this, SLOT(invalidate()));
        connect(source, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(rowsInserted()));
        connect(source, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(invalidate()));
        connect(source, SIGNAL(columnsInserted(QModelIndex,int,int)), this, SLOT(invalidate()));
        connect(source, SIGNAL(columnsRemoved(QModelIndex,int,int)), this, SLOT(invalidate()));
        connect(source, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(invalidate()));
    }

    invalidate();
}

QModelIndex MvdCompletionModel::mapToSource(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    int row;
    QModelIndex parent = d->engine->currentParent();
    if (!showAll()) {
        if (!d->engine->matchCount())
            return QModelIndex();
        Q_ASSERT(index.row() < d->engine->matchCount());
        MvdMatchData& matchData = d->engine->historyMatch();
        MvdIndexMapper& rootIndices = matchData.indices;
        if (index.row() < rootIndices.count()) {
            row = rootIndices[index.row()];
            parent = QModelIndex();
        } else {
            row = d->engine->currentMatch().indices[index.row() - rootIndices.count()];
        }
    } else {
        row = index.row();
    }

    QAbstractItemModel* sm = sourceModel();
    return sm ? sm->index(row, index.column(), parent) : QModelIndex();
}

QModelIndex MvdCompletionModel::mapFromSource(const QModelIndex& idx) const
{
    if (!idx.isValid())
        return QModelIndex();

    int row = -1;
    if (!showAll()) {
        if (!d->engine->matchCount())
            return QModelIndex();

        MvdIndexMapper& rootIndices = d->engine->historyMatch().indices;
        if (idx.parent().isValid()) {
            if (idx.parent() != d->engine->currentParent())
                return QModelIndex();
        } else {
            row = rootIndices.indexOf(idx.row());
            if (row == -1 && d->engine->currentParent().isValid())
                return QModelIndex(); // source parent and our parent don't match
        }

        if (row == -1) {
            MvdIndexMapper& indices = d->engine->currentMatch().indices;
            d->engine->filterOnDemand(idx.row() - indices.last());
            row = indices.indexOf(idx.row()) + rootIndices.count();
        }

        if (row == -1)
            return QModelIndex();
    } else {
        if (idx.parent() != d->engine->currentParent())
            return QModelIndex();
        row = idx.row();
    }

    return createIndex(row, idx.column());
}

bool MvdCompletionModel::setCurrentRow(int row)
{
    if (row < 0 || !d->engine->matchCount())
        return false;

    if (row >= d->engine->matchCount())
        d->engine->filterOnDemand(row + 1 - d->engine->matchCount());

    if (row >= d->engine->matchCount()) // invalid row
        return false;

    d->engine->setCurrentRow(row);
    return true;
}

QModelIndex MvdCompletionModel::currentIndex(bool sourceIndex) const
{
    if (!d->engine->matchCount())
        return QModelIndex();

    int row = d->engine->currentRow();
    if (showAll())
        row = d->engine->currentMatch().indices[d->engine->currentRow()];

    QModelIndex idx = createIndex(row, d->completer->completionColumn());
    if (!sourceIndex)
        return idx;
    return mapToSource(idx);
}

QModelIndex MvdCompletionModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || column < 0 || column >= columnCount(parent) || parent.isValid())
        return QModelIndex();

    if (!showAll()) {
        if (!d->engine->matchCount())
            return QModelIndex();
        if (row >= d->engine->historyMatch().indices.count()) {
            int want = row + 1 - d->engine->matchCount();
            if (want > 0)
                d->engine->filterOnDemand(want);
            if (row >= d->engine->matchCount())
                return QModelIndex();
        }
    } else {
        QAbstractItemModel* sm = sourceModel();
        if (!sm || row >= sm->rowCount(d->engine->currentParent()))
            return QModelIndex();
    }

    return createIndex(row, column);
}

int MvdCompletionModel::completionCount() const
{
    if (!d->engine->matchCount())
        return 0;

    d->engine->filterOnDemand(INT_MAX);
    return d->engine->matchCount();
}

int MvdCompletionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    if (showAll()) {
        // Show all items below current parent, even if we have no valid matches
        if (d->engine->currentPartsCount() != 1  && !d->engine->matchCount()
            && !d->engine->currentParent().isValid())
            return 0;
        QAbstractItemModel* sm = sourceModel();
        return sm ? sm->rowCount(d->engine->currentParent()) : 0;
    }

    return completionCount();
}

void MvdCompletionModel::setFiltered(bool filtered)
{
    if (d->showAll == !filtered)
        return;
    d->showAll = !filtered;
    resetModel();
}

bool MvdCompletionModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.isValid())
        return false;

    QAbstractItemModel* sm = sourceModel();
    if (!sm)
        return false;

    if (showAll())
        return sm->hasChildren(mapToSource(parent));

    if (!d->engine->matchCount())
        return false;

    return true;
}

QVariant MvdCompletionModel::data(const QModelIndex& index, int role) const
{
    QAbstractItemModel* sm = sourceModel();
    return sm ? sm->data(mapToSource(index), role) : QVariant();
}

void MvdCompletionModel::modelDestroyed()
{
    QAbstractProxyModel::setSourceModel(0); // switch to static empty model
    invalidate();
}

void MvdCompletionModel::rowsInserted()
{
    invalidate();
    emit rowsAdded();
}

void MvdCompletionModel::invalidate()
{
    d->engine->clearCache();
    filter(d->engine->currentParts());
}

void MvdCompletionModel::filter(const QStringList& parts)
{
    d->engine->filter(parts);
    resetModel();

    QAbstractItemModel* sm = sourceModel();
    if (sm && sm->canFetchMore(d->engine->currentParent()))
        sm->fetchMore(d->engine->currentParent());
}

void MvdCompletionModel::resetModel()
{
    if (rowCount() == 0) {
        reset();
        return;
    }

    emit layoutAboutToBeChanged();
    QModelIndexList piList = persistentIndexList();
    QModelIndexList empty;
    for (int i = 0; i < piList.size(); i++)
        empty.append(QModelIndex());
    changePersistentIndexList(piList, empty);
    emit layoutChanged();
}


//////////////////////////////////////////////////////////////////////////////


void MvdCompletionEngine::filter(const QStringList& parts)
{
    const QAbstractItemModel *model = d->completer->completionModel()->sourceModel();
    d->curParts = parts;
    if (d->curParts.isEmpty())
        d->curParts.append(QString());

    d->curRow = -1;
    d->curParent = QModelIndex();
    d->curMatch = MvdMatchData();
    d->historyMatch = filterHistory();

    if (!model)
        return;

    QModelIndex parent;
    for (int i = 0; i < d->curParts.count() - 1; i++) {
        QString part = d->curParts[i];
        int emi = filter(part, parent, -1).exactMatchIndex;
        if (emi == -1)
            return;
        parent = model->index(emi, d->completer->completionColumn(), parent);
    }

    // Note that we set the curParent to a valid parent, even if we have no matches
    // When filtering is disabled, we show all the items under this parent
    d->curParent = parent;
    if (d->curParts.last().isEmpty())
        d->curMatch = MvdMatchData(MvdIndexMapper(0, model->rowCount(d->curParent) - 1), -1, false);
    else
        d->curMatch = filter(d->curParts.last(), d->curParent, 1); // build at least one
    d->curRow = d->curMatch.isValid() ? 0 : -1;
}

MvdMatchData MvdCompletionEngine::filterHistory()
{
    QAbstractItemModel *source = d->completer->completionModel()->sourceModel();
    if (d->curParts.count() <= 1 || d->completer->completionModel()->showAll() || !source)
        return MvdMatchData();
    bool dirModel = false;
#ifndef QT_NO_DIRMODEL
    dirModel = (qobject_cast<QDirModel *>(source) != 0);
#endif
    QVector<int> v;
    MvdIndexMapper im(v);
    MvdMatchData m(im, -1, true);

    for (int i = 0; i < source->rowCount(); i++) {
        QString str = source->index(i, d->completer->completionColumn()).data().toString();
        if (str.startsWith(d->completer->completionPrefix(), d->completer->caseSensitivity())
#if !defined(Q_OS_WIN) || defined(Q_OS_WINCE)
            && (!dirModel || QDir::toNativeSeparators(str) != QDir::separator())
#endif
            )
            m.indices.append(i);
    }
    return m;
}

// Returns a match hint from the cache by chopping the search string
bool MvdCompletionEngine::matchHint(QString part, const QModelIndex& parent, MvdMatchData *hint)
{
    if (d->completer->caseSensitivity() == Qt::CaseInsensitive)
        part = part.toLower();

    const CacheItem& map = d->cache[parent];

    QString key = part;
    while (!key.isEmpty()) {
        key.chop(1);
        if (map.contains(key)) {
            *hint = map[key];
            return true;
        }
    }

    return false;
}

bool MvdCompletionEngine::lookupCache(QString part, const QModelIndex& parent, MvdMatchData *m)
{
   if (d->completer->caseSensitivity() == Qt::CaseInsensitive)
        part = part.toLower();
   const CacheItem& map = d->cache[parent];
   if (!map.contains(part))
       return false;
   *m = map[part];
   return true;
}

// When the cache size exceeds 1MB, it clears out about 1/2 of the cache.
void MvdCompletionEngine::saveInCache(QString part, const QModelIndex& parent, const MvdMatchData& m)
{
    MvdMatchData old = d->cache[parent].take(part);
    d->cost = d->cost + m.indices.cost() - old.indices.cost();
    if (d->cost * sizeof(int) > 1024 * 1024) {
        QMap<QModelIndex, CacheItem>::iterator it1 ;
        for (it1 = d->cache.begin(); it1 != d->cache.end(); ++it1) {
            CacheItem& ci = it1.value();
            int sz = ci.count()/2;
            QMap<QString, MvdMatchData>::iterator it2 = ci.begin();
            for (int i = 0; it2 != ci.end() && i < sz; i++, ++it2) {
                d->cost -= it2.value().indices.cost();
                ci.erase(it2);
            }
            if (ci.count() == 0)
                d->cache.erase(it1);
        }
    }

    if (d->completer->caseSensitivity() == Qt::CaseInsensitive)
        part = part.toLower();
    d->cache[parent][part] = m;
}

MvdCompletionEngine::Cache& MvdCompletionEngine::cache() const
{
    return d->cache;
}


///////////////////////////////////////////////////////////////////////////////////


MvdIndexMapper MvdSortedModelEngine::indexHint(QString part, const QModelIndex& parent, Qt::SortOrder order)
{
    MvdCompleter *c = completer();
    const QAbstractItemModel *model = c->completionModel()->sourceModel();

    if (c->caseSensitivity() == Qt::CaseInsensitive)
        part = part.toLower();

    const CacheItem& map = cache()[parent];

    // Try to find a lower and upper bound for the search from previous results
    int to = model->rowCount(parent) - 1;
    int from = 0;
    const CacheItem::const_iterator it = map.lowerBound(part);

    // look backward for first valid hint
    for(CacheItem::const_iterator it1 = it; it1-- != map.constBegin();) {
        const MvdMatchData& value = it1.value();
        if (value.isValid()) {
            if (order == Qt::AscendingOrder) {
                from = value.indices.last() + 1;
            } else {
                to = value.indices.first() - 1;
            }
            break;
        }
    }

    // look forward for first valid hint
    for(CacheItem::const_iterator it2 = it; it2 != map.constEnd(); ++it2) {
        const MvdMatchData& value = it2.value();
        if (value.isValid() && !it2.key().startsWith(part)) {
            if (order == Qt::AscendingOrder) {
                to = value.indices.first() - 1;
            } else {
                from = value.indices.first() + 1;
            }
            break;
        }
    }

    return MvdIndexMapper(from, to);
}

Qt::SortOrder MvdSortedModelEngine::sortOrder(const QModelIndex &parent) const
{
    MvdCompleter *c = completer();
    const QAbstractItemModel *model = c->completionModel()->sourceModel();

    int rowCount = model->rowCount(parent);
    if (rowCount < 2)
        return Qt::AscendingOrder;
    QString first = model->data(model->index(0, c->completionColumn(), parent), c->completionRole()).toString();
    QString last = model->data(model->index(rowCount - 1, c->completionColumn(), parent), c->completionRole()).toString();
    return QString::compare(first, last, c->caseSensitivity()) <= 0 ? Qt::AscendingOrder : Qt::DescendingOrder;
}

MvdMatchData MvdSortedModelEngine::filter(const QString& part, const QModelIndex& parent, int)
{
    MvdCompleter *c = completer();
    const QAbstractItemModel *model = c->completionModel()->sourceModel();

    MvdMatchData hint;
    if (lookupCache(part, parent, &hint))
        return hint;

    MvdIndexMapper indices;
    Qt::SortOrder order = sortOrder(parent);

    if (matchHint(part, parent, &hint)) {
        if (!hint.isValid())
            return MvdMatchData();
        indices = hint.indices;
    } else {
        indices = indexHint(part, parent, order);
    }

    // binary search the model within 'indices' for 'part' under 'parent'
    int high = indices.to() + 1;
    int low = indices.from() - 1;
    int probe;
    QModelIndex probeIndex;
    QString probeData;

    while (high - low > 1)
    {
        probe = (high + low) / 2;
        probeIndex = model->index(probe, c->completionColumn(), parent);
        probeData = model->data(probeIndex, c->completionRole()).toString();
        const int cmp = QString::compare(probeData, part, c->caseSensitivity());
        if ((order == Qt::AscendingOrder && cmp >= 0)
            || (order == Qt::DescendingOrder && cmp < 0)) {
            high = probe;
        } else {
            low = probe;
        }
    }

    if ((order == Qt::AscendingOrder && low == indices.to())
        || (order == Qt::DescendingOrder && high == indices.from())) { // not found
        saveInCache(part, parent, MvdMatchData());
        return MvdMatchData();
    }

    probeIndex = model->index(order == Qt::AscendingOrder ? low+1 : high-1, c->completionColumn(), parent);
    probeData = model->data(probeIndex, c->completionRole()).toString();
    if (!probeData.startsWith(part, c->caseSensitivity())) {
        saveInCache(part, parent, MvdMatchData());
        return MvdMatchData();
    }

    const bool exactMatch = QString::compare(probeData, part, c->caseSensitivity()) == 0;
    int emi =  exactMatch ? (order == Qt::AscendingOrder ? low+1 : high-1) : -1;

    int from = 0;
    int to = 0;
    if (order == Qt::AscendingOrder) {
        from = low + 1;
        high = indices.to() + 1;
        low = from;
    } else {
        to = high - 1;
        low = indices.from() - 1;
        high = to;
    }

    while (high - low > 1)
    {
        probe = (high + low) / 2;
        probeIndex = model->index(probe, c->completionColumn(), parent);
        probeData = model->data(probeIndex, c->completionRole()).toString();
        const bool startsWith = probeData.startsWith(part, c->caseSensitivity());
        if ((order == Qt::AscendingOrder && startsWith)
            || (order == Qt::DescendingOrder && !startsWith)) {
            low = probe;
        } else {
            high = probe;
        }
    }

    MvdMatchData m(order == Qt::AscendingOrder ? MvdIndexMapper(from, high - 1) : MvdIndexMapper(low+1, to), emi, false);
    saveInCache(part, parent, m);
    return m;
}

////////////////////////////////////////////////////////////////////////////////////////


int MvdUnsortedModelEngine::buildIndices(const QString& str, const QModelIndex& parent, int n,
    const MvdIndexMapper& indices, MvdMatchData* m)
{
    Q_ASSERT(m->partial);
    Q_ASSERT(n != -1 || m->exactMatchIndex == -1);

    MvdCompleter *c = completer();
    const QAbstractItemModel *model = c->completionModel()->sourceModel();
    int i, count = 0;

    for (i = 0; i < indices.count() && count != n; ++i) {
        QModelIndex idx = model->index(indices[i], c->completionColumn(), parent);
        QString data = model->data(idx, c->completionRole()).toString();
        if (!data.startsWith(str, c->caseSensitivity()) || !(model->flags(idx) & Qt::ItemIsSelectable))
            continue;
        m->indices.append(indices[i]);
        ++count;
        if (m->exactMatchIndex == -1 && QString::compare(data, str, c->caseSensitivity()) == 0) {
            m->exactMatchIndex = indices[i];
            if (n == -1)
                return indices[i];
        }
    }
    return indices[i-1];
}

void MvdUnsortedModelEngine::filterOnDemand(int n)
{
    Q_ASSERT(matchCount());
    if (!currentMatch().partial)
        return;
    MvdCompleter *c = completer();

    Q_ASSERT(n >= -1);
    const QAbstractItemModel *model = c->completionModel()->sourceModel();
    int lastRow = model->rowCount(currentParent()) - 1;
    MvdIndexMapper im(currentMatch().indices.last() + 1, lastRow);
    int lastIndex = buildIndices(currentParts().last(), currentParent(), n, im, &currentMatch());
    currentMatch().partial = (lastRow != lastIndex);
    saveInCache(currentParts().last(), currentParent(), currentMatch());
}

MvdMatchData MvdUnsortedModelEngine::filter(const QString& part, const QModelIndex& parent, int n)
{
    MvdMatchData hint;

    QVector<int> v;
    MvdIndexMapper im(v);
    MvdMatchData m(im, -1, true);

    MvdCompleter *c = completer();
    const QAbstractItemModel *model = c->completionModel()->sourceModel();
    bool foundInCache = lookupCache(part, parent, &m);

    if (!foundInCache) {
        if (matchHint(part, parent, &hint) && !hint.isValid())
            return MvdMatchData();
    }

    if (!foundInCache && !hint.isValid()) {
        const int lastRow = model->rowCount(parent) - 1;
        MvdIndexMapper all(0, lastRow);
        int lastIndex = buildIndices(part, parent, n, all, &m);
        m.partial = (lastIndex != lastRow);
    } else {
        if (!foundInCache) { // build from hint as much as we can
            buildIndices(part, parent, INT_MAX, hint.indices, &m);
            m.partial = hint.partial;
        }
        if (m.partial && ((n == -1 && m.exactMatchIndex == -1) || (m.indices.count() < n))) {
            // need more and have more
            const int lastRow = model->rowCount(parent) - 1;
            MvdIndexMapper rest(hint.indices.last() + 1, lastRow);
            int want = n == -1 ? -1 : n - m.indices.count();
            int lastIndex = buildIndices(part, parent, want, rest, &m);
            m.partial = (lastRow != lastIndex);
        }
    }

    saveInCache(part, parent, m);
    return m;
}


///////////////////////////////////////////////////////////////////////////////


MvdCompleterItemDelegate::MvdCompleterItemDelegate(QAbstractItemView *view) :
    QItemDelegate(view), mView(view)
{ }

void MvdCompleterItemDelegate::paint(QPainter *p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const {
    QStyleOptionViewItem optCopy = opt;
    optCopy.showDecorationSelected = true;
    if (mView->currentIndex() == idx)
        optCopy.state |= QStyle::State_HasFocus;
    QItemDelegate::paint(p, optCopy, idx);
}
