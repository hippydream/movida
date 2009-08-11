/**************************************************************************
** Filename: ratingwidget.h
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

#ifndef MVD_RATINGWIDGET_H
#define MVD_RATINGWIDGET_H

#include <QtGui/QPixmap>
#include <QtGui/QWidget>

class QMouseEvent;
class QPaintEvent;

class MvdRatingWidget : public QWidget
{
    Q_OBJECT

public:
    MvdRatingWidget(QWidget *parent = 0);
    virtual ~MvdRatingWidget();

    int value() const;
    int minimum() const;
    int maximum() const;

    QIcon icon() const;
    void setIcon(const QIcon &i, QSize size = QSize());

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void leaveEvent(QEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);

public slots:
    void setValue(int value);
    void setMinimum(int value);
    void setMaximum(int value);

signals:
    void valueChanged(int value);
    //! Emitted when a pixmap is hovered or un-hovered (-1).
    void hovered(int value);

private:
    class Private;
    Private *d;
};

#endif // MVD_RATINGWIDGET_H
