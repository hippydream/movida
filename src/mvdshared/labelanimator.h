/**************************************************************************
** Filename: labelanimator.h
** Revision: 3
**
** Copyright (C) 2007 Angius Fabrizio. All rights reserved.
**
** This file is part of the Movida project (http://movida.sourceforge.net/).
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

#ifndef MVD_LABELANIMATOR_H
#define MVD_LABELANIMATOR_H

#include <QLabel>
#include <QStringList>
#include <QTimer>
#include <QEvent>

/*!
	\class MvdLabelAnimator labelanimator.h
	\ingroup MovidaShared

	\brief A QLabel subclass that displays a list of frame in an animation.
*/

class MvdLabelAnimator : QObject
{
	Q_OBJECT

public:
	MvdLabelAnimator(const QStringList& _frames, QLabel* _label, QObject* parent = 0)
		: QObject(parent), frames(_frames), label(_label), 
		timer(new QTimer(this)), frame(0)
	{
		Q_ASSERT(!frames.isEmpty());

		connect( timer, SIGNAL(timeout()), this, SLOT(timeout()) );

		timer->setInterval(1000/frames.size());

		if (label->isVisible())
			timer->start();

		label->installEventFilter(this);
	}

public slots:
	void setPixmapVisible(bool visible)
	{
		if (!visible)
		{
			if (timer->isActive())
			{
				timer->stop();
				frame = 0;
			}
			label->setPixmap(QPixmap());
		}
		else
		{
			if (!timer->isActive())
			{
				timeout();
				timer->start();
			}
		}
	}

protected:
	bool eventFilter(QObject* sender, QEvent* event)
	{
		if (sender == label)			
		{
			if (event->type() == QEvent::Show && !timer->isActive())
			{
				timeout();
				timer->start();
			}
			else if (event->type() == QEvent::Hide && timer->isActive())
			{
				timer->stop();
				frame = 0;
			}
		}
		return false;
	}

private:
	QStringList frames;
	QLabel* label;
	QTimer* timer;
	int frame;

private slots:
	void timeout()
	{
		Q_ASSERT(label);
		
		if (frame >= frames.size())
			frame = 0;

		label->setPixmap(QPixmap(frames.at(frame++)));
	}
};

#endif // MVD_LABELANIMATOR_H
