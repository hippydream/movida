/****************************************************************************
** Filename: logger.h
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** Log messages dialog.
**
** Copyright (C) 2006 Angius Fabrizio. All rights reserved.
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
**********************************************************************/

#ifndef MOVIDA_LOGGER__H
#define MOVIDA_LOGGER__H

#include <QWidget>
#include <QTreeWidgetItem>

#include "ui_logger.h"

class QContextMenuEvent;

class Logger : public QWidget, private Ui::Logger
{
	Q_OBJECT

public:
	// only messages with priority bigger than or equal to mPriority are displayed
	// -> Low = very verbose, High = only high priority messages allowed

	enum MsgPriority { Low, Medium, High, DisableLogging };
	enum MsgType { Notification, Warning, Error };

	static Logger* instance();
	virtual ~Logger();

	MsgPriority priority() const;
	void setPriority(MsgPriority priority);

	const QColor& color(MsgType type) const;
	void setColor(MsgType type, const QColor& col);

	bool filter(MsgType type) const;
	void setFilter(MsgType type, bool enable);

	void notify(const QString& msg, MsgPriority priority = Medium);
	void warning(const QString& msg, MsgPriority priority = Medium);
	void error(const QString& msg, MsgPriority priority = Medium);

	void setLastDir(const QString& dir);
	QString lastDir() const;

protected:
  	void contextMenuEvent(QContextMenuEvent* e);

public slots:
	virtual void show();
	virtual void clear();

protected slots:
	virtual void closeEvent(QCloseEvent* e);
	virtual void hideEvent(QHideEvent* e);

signals:
	void visibilityChanged(bool visible);

private:
	static Logger* mInstance;

	QPixmap* mPixN;
	QPixmap* mPixW;
	QPixmap* mPixE;

	QColor* mColN;
	QColor* mColW;
	QColor* mColE;

	QBitmap* mMask;

	int mCounter;

	MsgPriority mPriority;

	bool mFilterN;
	bool mFilterW;
	bool mFilterE;

	QString* mLastDir;


	// private constructor
	Logger();

	void addMessage(const QString& msg, MsgType type, MsgPriority priority);


private slots:
	void copyMessage();
	void saveLog();
	void applyFilter();
};

#endif // MOVIDA_LOGGER__H
