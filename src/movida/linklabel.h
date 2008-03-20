/**************************************************************************
** Filename: linklabel.h
**
** Copyright (C) 2007 Angius Fabrizio. All rights reserved.
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

#ifndef MVD_LINKLABEL_H
#define MVD_LINKLABEL_H

#include <QLabel>
#include <QFileInfo>
#include <QUrl>
#include <QMouseEvent>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>

/*!
	\class MvdLinkLabel linklabel.h
	\ingroup Movida

	\brief QLabel subclass with D&D handling.
*/

class MvdLinkLabel : public QLabel
{
	Q_OBJECT

public:
	MvdLinkLabel(QWidget* parent = 0)
		: QLabel(parent) { init(); }
	MvdLinkLabel(const QString& text, QWidget* parent = 0)
		: QLabel(text, parent) { init(); }

	//! The activa area rectangle can be set to limit the area where a clicked() event is valid.
	void setActiveAreaRect(const QRect& rect) { activeRect = rect; }
	//! Returns the rectangle containing the current active area or the whole widget area if none has been set.
	QRect activeAreaRect() const { return activeRect.isNull() ? rect() : activeRect; }

	/*!
		Register a QObject as receiver of drag and drop actions.
		\p dragEnter is called when the mouse (dragging something!) enters the widget.
		The attached slot should return a bool and take a const reference to a QMimeData object as
		unique parameter. The method should also return true if it wants to handle the dragged data
		(it won't receive any drop event if it returns false).
		The \p drop slot is called as soon as a drop occurs. The slot signature is the same as for
		the drag enter event.
		The last slot will be called as soon as the mouse leaves the widget with no drop occurred.
		This might be useful to reset any visual elements that have been changed after accepting the
		drag.

		Example usage:

		\verbatim
		MyClass {
			MvdLinkLabel* poster = new MvdLinkLabel;
			poster->setDragEnterFilter(this, "posterDragEntered");
			poster->setDropFilter(this, "posterDropped");
			poster->setDragLeaveFilter(this, "resetPosterStatus");
		}

		// MyClass slots (access rights do not matter):
		bool MyClass::posterDragEntered(const QMimeData& d) {
			if (checkIfMimeDataContainsAPicture(d)) {
				setStatus("Drop here to set the picture as a poster for this movie.")
				return true;
			}
			else return false; // Reject dragged data
		}

		bool MyClass::posterDropped(const QMimeData& d) {
			setMoviePoster(extractImageFileFromMimeData(d));
			return true;
		}

		void MyClass::resetPosterStatus() {
			resetStatus(); // Clear the previously set "Drop here to..." status
		}
		\endverbatim
	*/
	void setDragAndDropHandler(QObject* obj, const char* dragEnter, const char* drop, const char* dragLeave = 0)
	{
		dndHandler = obj;

		if (! (dragEnter && drop && dragLeave) )
			dndHandler = 0;

		dragEnterMember = (obj && dragEnter) ? QString(dragEnter) : QString();
		dropMember = (obj && drop) ? QString(drop) : QString();
		dragLeaveMember = (obj && dragLeave) ? QString(dragLeave) : QString();
	}

protected:
	virtual void mouseReleaseEvent(QMouseEvent* event)
	{
		if (event->button() == Qt::LeftButton)
		{
			QRect r = activeAreaRect();
			if (r.contains(event->pos()))
				emit clicked();
		}
	}

	virtual void mouseMoveEvent(QMouseEvent* event)
	{
		QRect r = activeAreaRect();
		setCursor(r.contains(event->pos()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
	}

	void dragEnterEvent(QDragEnterEvent* event)
	{
		const QMimeData* mimeData = event->mimeData();
		bool accept = false;

		if (dndHandler)
		{
			QMetaObject::invokeMethod(dndHandler, dragEnterMember.toAscii().constData(),
				Qt::DirectConnection,
				Q_RETURN_ARG(bool, accept),
				Q_ARG(QMimeData, *mimeData));
		}

		if (accept)
			event->acceptProposedAction();
	}

	void dragLeaveEvent(QDragLeaveEvent*)
	{
		if (dndHandler)
		{
			QMetaObject::invokeMethod(dndHandler, dragLeaveMember.toAscii().constData(),
				Qt::DirectConnection);
		}
	}

	virtual void dropEvent(QDropEvent* event)
	{
		const QMimeData* mimeData = event->mimeData();
		bool accept = false;

		if (dndHandler)
		{
			QMetaObject::invokeMethod(dndHandler, dropMember.toAscii().constData(),
				Qt::DirectConnection,
				Q_RETURN_ARG(bool, accept),
				Q_ARG(QMimeData, *mimeData));
		}

		if (accept)
			event->acceptProposedAction();
	}

private:
	void init()
	{
		dndHandler = 0;
		setAcceptDrops(true);
		setMouseTracking(true);
	}

	QObject* dndHandler;
	QString dragEnterMember;
	QString dropMember;
	QString dragLeaveMember;
	QRect activeRect;

signals:
	void clicked();
};

#endif // MVD_LINKLABEL_H
