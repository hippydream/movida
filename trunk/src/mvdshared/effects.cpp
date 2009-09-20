/**************************************************************************
** Filename: effects.cpp
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

#include "effects.h"
#include "effects_p.h"

#include <QtGui/QFrame>
#include <QtGui/QStyleOptionFrame>

static MvdAlphaWidget* mvd_blend = 0;

/*!
    Constructs a MvdAlphaWidget.
*/
MvdAlphaWidget::MvdAlphaWidget(QWidget* w, Qt::WindowFlags f, bool o) :
    QWidget(QApplication::desktop()->screen(QApplication::desktop()->screenNumber(w)), f)
{
#ifndef Q_WS_WIN
    setEnabled(false);
#endif
    setAttribute(Qt::WA_NoSystemBackground, true);
    widget = (MvdAccessWidget*)w;
    windowOpacity = w->windowOpacity();
    alpha = o ? windowOpacity : 0;
    fadeOut = o;
}

MvdAlphaWidget::~MvdAlphaWidget()
{
#ifdef Q_WS_WIN
    // Restore user-defined opacity value
    if (widget && QSysInfo::WindowsVersion >= QSysInfo::WV_2000 && QSysInfo::WindowsVersion < QSysInfo::WV_NT_based)
        widget->setWindowOpacity(windowOpacity);
#endif
}

void MvdAlphaWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.drawPixmap(0, 0, pm);
}

/*
    Starts the alphablending animation.
    The animation will take about \a time ms
*/
void MvdAlphaWidget::run(int time)
{
    duration = time;

    if (duration < 0)
        duration = 150;

    if (!widget)
        return;

    elapsed = 0;
    checkTime.start();

    showWidget = true;
#if defined(Q_OS_WIN)
    if (QSysInfo::WindowsVersion >= QSysInfo::WV_2000 && QSysInfo::WindowsVersion < QSysInfo::WV_NT_based) {
        qApp->installEventFilter(this);
        if (fadeOut) {
            widget->setWindowOpacity(1.0);
        } else {
            widget->setWindowOpacity(0.0);
        }
        widget->show();
        connect(&anim, SIGNAL(timeout()), this, SLOT(render()));
        anim.start(1);
    } else
#endif
    {
        //This is roughly equivalent to calling setVisible(true) without actually showing the widget
        /*widget->setAttribute(Qt::WA_WState_ExplicitShowHide, true);
        widget->setAttribute(Qt::WA_WState_Hidden, false);*/

        qApp->installEventFilter(this);

        QRect r = widget->geometry();
        r.moveTo(widget->mapToGlobal(r.topLeft()));
        move(r.x(), r.y());
        resize(r.width(), r.height());

        frontImage = QPixmap::grabWidget(widget).toImage();
        backImage = QPixmap::grabWindow(QApplication::desktop()->winId(),
            widget->geometry().x(), widget->geometry().y(),
            widget->geometry().width(), widget->geometry().height()).toImage();

        if (!backImage.isNull() && checkTime.elapsed() < duration / 2) {
            mixedImage = backImage.copy();
            pm = QPixmap::fromImage(mixedImage);
            show();
            setEnabled(false);

            connect(&anim, SIGNAL(timeout()), this, SLOT(render()));
            anim.start(1);
        } else {
            duration = 0;
            render();
        }
    }
}

bool MvdAlphaWidget::eventFilter(QObject *o, QEvent *e)
{
    switch (e->type()) {
    case QEvent::Move:
        if (o != widget)
            break;
        move(widget->geometry().x(),widget->geometry().y());
        update();
        break;

    case QEvent::Hide:
    case QEvent::Close:
        if (o != widget)
            break;

    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
        showWidget = false;
        render();
        break;

    case QEvent::KeyPress:
    {
        QKeyEvent *ke = (QKeyEvent*)e;
        if (ke->key() == Qt::Key_Escape) {
            showWidget = false;
        } else {
            duration = 0;
        }
        render();
        break;
    }

    default:
        break;
    }

    return QWidget::eventFilter(o, e);
}

void MvdAlphaWidget::closeEvent(QCloseEvent *e)
{
    e->accept();
    if (!mvd_blend)
        return;

    showWidget = false;
    render();

    QWidget::closeEvent(e);
}

/*!
    Render alphablending for the time elapsed.

    Show the blended widget and free all allocated source
    if the blending is finished.
*/
void MvdAlphaWidget::render()
{
    int tempel = checkTime.elapsed();
    if (elapsed >= tempel)
        elapsed++;
    else
        elapsed = tempel;

    if (duration != 0)
        alpha = fadeOut ? (1.0f - tempel / double(duration)) : (tempel / double(duration));
    else
        alpha = fadeOut ? 0.0 : 1.0;

#if defined(Q_OS_WIN)
    if (QSysInfo::WindowsVersion >= QSysInfo::WV_2000 && QSysInfo::WindowsVersion < QSysInfo::WV_NT_based) {
        if (alpha >= windowOpacity || !showWidget) {
            anim.stop();
            qApp->removeEventFilter(this);
            widget->setWindowOpacity(windowOpacity);
            mvd_blend = 0;
            deleteLater();
        } else {
            widget->setWindowOpacity(alpha);
        }
    } else
#endif
        if ((fadeOut ? alpha <= 0 : alpha >= 1) || !showWidget) {
            anim.stop();
            qApp->removeEventFilter(this);

            if (widget) {
                if (!showWidget) {
#ifdef Q_WS_WIN
                    setEnabled(true);
                    setFocus();
#endif
                    widget->hide();
                } else {
                    //Since we are faking the visibility of the widget 
                    //we need to unset the hidden state on it before calling show
                    widget->setAttribute(Qt::WA_WState_Hidden, true);
                    if (fadeOut)
                        widget->hide();
                    else widget->show();
                    lower();
                }
            }
            mvd_blend = 0;
            deleteLater();
        } else {
            alphaBlend();
            pm = QPixmap::fromImage(mixedImage);
            repaint();
        }
}

/*!
    Calculate an alphablended image.
*/
void MvdAlphaWidget::alphaBlend()
{
    const int a = qRound(alpha*256);
    const int ia = 256 - a;

    const int sw = frontImage.width();
    const int sh = frontImage.height();
    const int bpl = frontImage.bytesPerLine();
    switch(frontImage.depth()) {
    case 32:
    {
        uchar *mixed_data = mixedImage.bits();
        const uchar *back_data = backImage.bits();
        const uchar *front_data = frontImage.bits();

        for (int sy = 0; sy < sh; sy++) {
            quint32* mixed = (quint32*)mixed_data;
            const quint32* back = (const quint32*)back_data;
            const quint32* front = (const quint32*)front_data;
            for (int sx = 0; sx < sw; sx++) {
                quint32 bp = back[sx];
                quint32 fp = front[sx];

                mixed[sx] =  qRgb((qRed(bp)*ia + qRed(fp)*a)>>8,
                    (qGreen(bp)*ia + qGreen(fp)*a)>>8,
                    (qBlue(bp)*ia + qBlue(fp)*a)>>8);
            }
            mixed_data += bpl;
            back_data += bpl;
            front_data += bpl;
        }
    }
    default:
    break;
    }
}


//////////////////////////////////////////////////////////////////////////


static MvdRollEffect* mvd_roll = 0;

/*
    Construct a MvdRollEffect widget.
*/
MvdRollEffect::MvdRollEffect(QWidget* w, Qt::WindowFlags f, DirFlags orient) :
    QWidget(0, f), orientation(orient)
{
#ifndef Q_WS_WIN
    setEnabled(false);
#endif

    widget = (MvdAccessWidget*) w;
    Q_ASSERT(widget);

    setAttribute(Qt::WA_NoSystemBackground, true);

    if (widget->testAttribute(Qt::WA_Resized)) {
        totalWidth = widget->width();
        totalHeight = widget->height();
    } else {
        totalWidth = widget->sizeHint().width();
        totalHeight = widget->sizeHint().height();
    }

    currentHeight = totalHeight;
    currentWidth = totalWidth;

    if (orientation & (RightScroll|LeftScroll))
        currentWidth = 0;
    if (orientation & (DownScroll|UpScroll))
        currentHeight = 0;

    pm = QPixmap::grabWidget(widget);
}

void MvdRollEffect::paintEvent(QPaintEvent*)
{
    int x = orientation & RightScroll ? qMin(0, currentWidth - totalWidth) : 0;
    int y = orientation & DownScroll ? qMin(0, currentHeight - totalHeight) : 0;

    QPainter p(this);
    p.drawPixmap(x, y, pm);
}

void MvdRollEffect::closeEvent(QCloseEvent *e)
{
    e->accept();
    if (done)
        return;

    showWidget = false;
    done = true;
    scroll();

    QWidget::closeEvent(e);
}

/*!
    Start the animation.

    The animation will take about \a time ms, or is
    calculated if \a time is negative
*/
void MvdRollEffect::run(int time)
{
    if (!widget)
        return;

    duration  = time;
    elapsed = 0;

    if (duration < 0) {
        int dist = 0;
        if (orientation & (RightScroll|LeftScroll))
            dist += totalWidth - currentWidth;
        if (orientation & (DownScroll|UpScroll))
            dist += totalHeight - currentHeight;
        duration = qMin(qMax(dist/3, 50), 120);
    }

    connect(&anim, SIGNAL(timeout()), this, SLOT(scroll()));

    QWidget *p = widget->parentWidget();
    QFrame *pframe = qobject_cast<QFrame *>(p);

    int fw = 0;
    if (pframe) {
        fw = pframe->frameWidth();
    }

    QRect r = widget->geometry();
    r.moveTo(widget->mapToGlobal(r.topLeft()) - QPoint(fw, fw));
    move(r.x(), r.y());
    resize(qMin(currentWidth, totalWidth), qMin(currentHeight, totalHeight));

    //This is roughly equivalent to calling setVisible(true) without actually showing the widget
    //widget->setAttribute(Qt::WA_WState_ExplicitShowHide, true);
    //widget->setAttribute(Qt::WA_WState_Hidden, false);

    show();
    setEnabled(false);

    qApp->installEventFilter(this);

    showWidget = true;
    done = false;
    anim.start(1);
    checkTime.start();
}

/*!
    Roll according to the time elapsed.
*/
void MvdRollEffect::scroll()
{
    if (!done && widget) {
        int tempel = checkTime.elapsed();
        if (elapsed >= tempel)
            elapsed++;
        else
            elapsed = tempel;

        if (currentWidth != totalWidth) {
            currentWidth = totalWidth * (elapsed/duration)
                + (2 * totalWidth * (elapsed%duration) + duration)
                / (2 * duration);
            // equiv. to int((totalWidth*elapsed) / duration + 0.5)
            done = (currentWidth >= totalWidth);
        }
        if (currentHeight != totalHeight) {
            currentHeight = totalHeight * (elapsed/duration)
                + (2 * totalHeight * (elapsed%duration) + duration)
                / (2 * duration);
            // equiv. to int((totalHeight*elapsed) / duration + 0.5)
            done = (currentHeight >= totalHeight);
        }
        done = (currentHeight >= totalHeight) &&
            (currentWidth >= totalWidth);

        int w = totalWidth;
        int h = totalHeight;
        int x = widget->geometry().x();
        int y = widget->geometry().y();

        if (orientation & RightScroll || orientation & LeftScroll)
            w = qMin(currentWidth, totalWidth);
        if (orientation & DownScroll || orientation & UpScroll)
            h = qMin(currentHeight, totalHeight);

        setUpdatesEnabled(false);
        if (orientation & UpScroll)
            y = widget->geometry().y() + qMax(0, totalHeight - currentHeight);
        if (orientation & LeftScroll)
            x = widget->geometry().x() + qMax(0, totalWidth - currentWidth);
        if (orientation & UpScroll || orientation & LeftScroll)
            move(x, y);

        resize(w, h);
        setUpdatesEnabled(true);
        repaint();
    }
    if (done) {
        anim.stop();
        qApp->removeEventFilter(this);
        if (widget) {
            if (!showWidget) {
#ifdef Q_WS_WIN
                setEnabled(true);
                setFocus();
#endif
                widget->hide();
            } else {
                //Since we are faking the visibility of the widget 
                //we need to unset the hidden state on it before calling show
                widget->setAttribute(Qt::WA_WState_Hidden, true);
                widget->show();
                lower();
            }
        }
        mvd_roll = 0;
        deleteLater();
    }
}


//////////////////////////////////////////////////////////////////////////


/*!
    Scroll widget \a w in \a time ms. \a orient may be 1 (vertical), 2
    (horizontal) or 3 (diagonal).
*/
void Movida::scrollEffect(QWidget* w, MvdEffects::DirFlags orient, int time)
{
    if (mvd_roll) {
        mvd_roll->deleteLater();
        mvd_roll = 0;
    }

    if (!w)
        return;

    qApp->sendPostedEvents(w, QEvent::Move);
    qApp->sendPostedEvents(w, QEvent::Resize);
    Qt::WindowFlags flags = Qt::ToolTip;

    // those can be popups - they would steal the focus, but are disabled
    mvd_roll = new MvdRollEffect(w, flags, orient);
    mvd_roll->run(time);
}

/*!
    Fade in widget \a w in \a time ms.
*/
void Movida::fadeEffect(QWidget* w, int time, bool fadeOut)
{
    if (mvd_blend) {
        mvd_blend->deleteLater();
        mvd_blend = 0;
    }

    if (!w)
        return;

    qApp->sendPostedEvents(w, QEvent::Move);
    qApp->sendPostedEvents(w, QEvent::Resize);

    Qt::WindowFlags flags = Qt::ToolTip;

    // those can be popups - they would steal the focus, but are disabled
    mvd_blend = new MvdAlphaWidget(w, flags, fadeOut);

    mvd_blend->run(time);
}
