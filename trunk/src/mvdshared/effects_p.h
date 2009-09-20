/**************************************************************************
** Filename: effects_p.h
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the MvdShared API.  It exists for the convenience
// of Movida.  This header file may change from version to version without notice,
// or even be removed.
//
// We mean it.
//

#ifndef MVD_EFFECTS_P_H
#define MVD_EFFECTS_P_H

#include "effects.h"

#include <QtCore/QDateTime>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtCore/QtDebug>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QImage>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>


/*!
    Internal class to get access to protected QWidget-members
*/
class MvdAccessWidget : public QWidget
{
    friend class MvdAlphaWidget;
    friend class MvdRollEffect;

public:
    MvdAccessWidget(QWidget* parent=0, Qt::WindowFlags f = 0) :
        QWidget(parent, f)
        {
        }
};

//////////////////////////////////////////////////////////////////////////

/*!
    Internal class MvdAlphaWidget.

    The MvdAlphaWidget is shown while the animation lasts
    and displays the pixmap resulting from the alpha blending.
*/

class MvdAlphaWidget : public QWidget, private MvdEffects
{
    Q_OBJECT

public:
    MvdAlphaWidget(QWidget* w, Qt::WindowFlags f = 0, bool fadeOut = false);
    ~MvdAlphaWidget();

    void run(int time);

protected:
    void paintEvent(QPaintEvent* e);
    void closeEvent(QCloseEvent*);
    void alphaBlend();
    bool eventFilter(QObject *, QEvent *);

protected slots:
    void render();

private:
    QPixmap pm;
    double alpha;
    QImage backImage;
    QImage frontImage;
    QImage mixedImage;
    QPointer<MvdAccessWidget> widget;
    int duration;
    int elapsed;
    bool showWidget;
    QTimer anim;
    QTime checkTime;
    double windowOpacity;
    bool fadeOut;
};

//////////////////////////////////////////////////////////////////////////

/*!
Internal class MvdRollEffect

The MvdRollEffect widget is shown while the animation lasts
and displays a scrolling pixmap.
*/

class MvdRollEffect : public QWidget, private MvdEffects
{
    Q_OBJECT

public:
    MvdRollEffect(QWidget* w, Qt::WindowFlags f, DirFlags orient);

    void run(int time);

protected:
    void paintEvent(QPaintEvent*);
    void closeEvent(QCloseEvent*);

private slots:
    void scroll();

private:
    QPointer<MvdAccessWidget> widget;

    int currentHeight;
    int currentWidth;
    int totalHeight;
    int totalWidth;

    int duration;
    int elapsed;
    bool done;
    bool showWidget;
    int orientation;

    QTimer anim;
    QTime checkTime;

    QPixmap pm;
};

#endif // MVD_EFFECTS_P_H
