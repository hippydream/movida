/****************************************************************************
** Filename: logger.cpp
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

#include <QPixmap>
#include <QBitmap>
#include <QIcon>
#include <QCursor>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QShortcut>
#include <QMessageBox>
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QFileDialog>

#include <QVariant>
#include <QList>
#include <QFile>
#include <QTime>
#include <QTextStream>

#include "logger.h"
#include "global.h"
#include "columnids.h"

#define LOGGER_PRIORITY Qt::UserRole + 11
#define LOGGER_TYPE Qt::UserRole + 12

/*!
	\class Logger logger.h

	\brief Log messages dialog.
*/

//! \internal
Logger* Logger::mInstance = 0;

/*!
	Returns the unique logger instance.
*/
Logger* Logger::instance()
{
	if (mInstance == 0)
	{
		mInstance = new Logger();
	}

	return mInstance;
}


/*!
	Private constructor.
 */
Logger::Logger()
: QWidget(0, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint)
{
	setupUi(this);

	_WINDOW_ICON_
	_WINDOW_CAPTION_("Log messages")

	mTRW_Messages->setObjectName("log_messages");

	QList<DataView::ColumnDescriptor> cols;
	DataView::ColumnDescriptor cd;

	cd.cid = MovidaCID::LogMessage;
	cols << cd;

	mTRW_Messages->setColumns(cols);

	mTRW_Messages->setSortingEnabled(false);
	mTRW_Messages->setRootIsDecorated(false);
	mTRW_Messages->setAlternatingRowColors(true);
	mTRW_Messages->setSelectionBehavior(QAbstractItemView::SelectRows);
	mTRW_Messages->header()->setSortIndicatorShown(false);
	mTRW_Messages->header()->setStretchLastSection(true);

	mPB_Save->setEnabled(false);
	mPB_Clear->setEnabled(false);

	setFocusProxy(mPB_Hide);

	mMask = new QBitmap(QPixmap(":/images/misc/mask.png").createHeuristicMask());
	int maskX = mMask->width();
	int maskY = mMask->height();

	mColN = new QColor(102,255,153);
	mPixN = new QPixmap(maskX, maskY);
	mPixN->fill(*mColN);
	mPixN->setMask(*mMask);

	mColW = new QColor(255,204,51);
	mPixW = new QPixmap(maskX, maskY);
	mPixW->fill(*mColW);
	mPixW->setMask(*mMask);

	mColE = new QColor(204,0,51);
	mPixE = new QPixmap(maskX, maskY);
	mPixE->fill(*mColE);
	mPixE->setMask(*mMask);

	mCounter = 0;

	mPriority = Low;
	mFilterN = mFilterW = mFilterE = false;

	connect ( mPB_Hide, SIGNAL( clicked() ), this, SLOT( hide() ) );
	connect ( mPB_Clear, SIGNAL( clicked() ), this, SLOT( clear() ) );
	connect ( mPB_Save, SIGNAL( clicked() ), this, SLOT( saveLog() ) );

	new QShortcut(Qt::Key_Escape, this, SLOT(hide()));


	mLastDir = 0;
}

/*!
	Releases used resources.
*/
Logger::~Logger()
{
	if (this == mInstance)
	{
		delete mPixN;
		delete mPixW;
		delete mPixE;
		delete mMask;
		delete mLastDir;
	}
	else delete mInstance;
}

/*!
	Appends a notification message to the log.
 */
void Logger::notify(const QString& msg, MsgPriority priority)
{
	if (mPriority == DisableLogging) return;
	addMessage(msg, Notification, priority);
}


/*!
	Appends a warning message to the log.
 */
void Logger::warning(const QString& msg, MsgPriority priority)
{
	if (mPriority == DisableLogging) return;
	addMessage(msg, Warning, priority);
}


/*!
	Appends an error message to the log.
 */
void Logger::error(const QString& msg, MsgPriority priority)
{
	if (mPriority == DisableLogging) return;
	addMessage(msg, Error, priority);
}


/*!
	Appends a message to the log.
 */
void Logger::addMessage(const QString& msg, MsgType type, MsgPriority priority)
{
	if (mPriority > priority) return;

	if (msg.isEmpty()) return;

	if (mCounter == 0)
	{
		mPB_Clear->setDisabled(false);
		mPB_Save->setDisabled(false);
	}

	QTreeWidgetItem* item = new QTreeWidgetItem(
 		mTRW_Messages, mTRW_Messages->topLevelItem(mTRW_Messages->topLevelItemCount()-1));

    // store message type (notify, warning, error...)
	item->setData(0, LOGGER_TYPE, QVariant( (int)type ) );
	// store message priority
	item->setData(0, LOGGER_PRIORITY, QVariant( (int)priority ) );
    // store id to sort items
	item->setData(0, DATAVIEW_ID, QVariant(mCounter++) );


    // set icon and evtl. filter out new message
	switch (type)
	{
	case Warning: item->setIcon(0,QIcon(*mPixW)); mTRW_Messages->setItemHidden(item, mFilterW); break;
	case Error: item->setIcon(0,QIcon(*mPixE)); mTRW_Messages->setItemHidden(item, mFilterE); break;
	case Notification: item->setIcon(0,QIcon(*mPixN)); mTRW_Messages->setItemHidden(item, mFilterN);
	default:
		;
	}

	QTime now = QTime::currentTime();

	item->setText(0,QString("[%1] %2").arg(now.toString("hh:mm:ss")).arg(msg));

	mTRW_Messages->scrollToItem(item);
	//mTRW_Messages->resizeColumnToContents(0);
}

/*!
	Hides the dialog and emits a visibilityChanged() signal.
*/
void Logger::closeEvent(QCloseEvent*)
{
	hide();
	emit visibilityChanged(false);
}

/*!
	Hides the dialog and emits a visibilityChanged() signal.
*/
void Logger::hideEvent(QHideEvent* e)
{
	QWidget::hideEvent(e);
	emit visibilityChanged(false);
}

/*!
	Shows the dialog and emits a visibilityChanged() signal.
*/
void Logger::show()
{
	emit visibilityChanged(true);
	QWidget::show();
}

/*!
	Clears the log.
*/
void Logger::clear()
{
	mTRW_Messages->clear();
	mCounter = 0;

	mPB_Clear->setDisabled(true);
	mPB_Save->setDisabled(true);
}

/*!
	Shows a context menu for the selected log message.
 */
void Logger::contextMenuEvent(QContextMenuEvent*)
{
 	QList<QTreeWidgetItem*> list = mTRW_Messages->selectedItems();

	QTreeWidgetItem* item = (list.count() != 1) ? 0 : list[0];

	QMenu context(this);
	bool addSeparator = false;

	if (item != 0)
	{
		// get cursor position relative to viewport
		QPoint itemP = mTRW_Messages->viewport()->mapFromGlobal(QCursor::pos());
		// check if curor is on some item
		// QTreeWidget's itemAt() does not consider the whole row!
		if (itemP.y() < mTRW_Messages->visualItemRect(item).bottom())
		{
            context.addAction(tr("&Copy to clipboard"), this, SLOT(copyMessage()), Qt::CTRL + Qt::Key_C);
            addSeparator = true;
		}
	}

	QAction* _showN = 0;
	QAction* _showW = 0;
	QAction* _showE = 0;

	if (mCounter != 0)
	{
		addSeparator = true;
		context.addAction(tr("C&lear log window"), this, SLOT(clear()), Qt::CTRL + Qt::Key_L);
	}

	if (addSeparator)
		context.addSeparator();

	if (mFilterN)
		_showN = context.addAction(QIcon(*mPixN), tr("Show notification messages"));
	else _showN = context.addAction(QIcon(*mPixN), tr("Hide notification messages"));

	if (mFilterW)
		_showW = context.addAction(QIcon(*mPixW), tr("Show warning messages"));
	else _showW = context.addAction(QIcon(*mPixW), tr("Hide warning messages"));

	if (mFilterE)
		_showE = context.addAction(QIcon(*mPixE), tr("Show error messages"));
	else _showE = context.addAction(QIcon(*mPixE), tr("Hide error messages"));

	QAction* _res = context.exec(QCursor::pos());

	if (_res == 0)
	   return;

	if (_res == _showN)
	{
		mFilterN = !mFilterN;
		applyFilter();
	}
	else if (_res == _showW)
	{
		mFilterW = !mFilterW;
		applyFilter();
	}
	else if (_res == _showE)
	{
		mFilterE = !mFilterE;
		applyFilter();
	}
}

/*!
	Copies the text of the selected log message to the clipboard.
 */
void Logger::copyMessage()
{
	QList<QTreeWidgetItem*> list = mTRW_Messages->selectedItems();
	if (list.count() != 1)
	   return;

	QTreeWidgetItem* item = list[0];

	QString msg = item->text(0);

	// remove [hh:mm:ss] part
	msg = msg.right(msg.length() - 11);

	QClipboard* cb = QApplication::clipboard();
	cb->setText(msg);
}

/*!
	Saves the log messages to a text file.
 */
void Logger::saveLog()
{
	if (mTRW_Messages->topLevelItemCount() == 0)
	   return;

	QString filename = QFileDialog::getSaveFileName(
		this,
		_CAPTION_,
		mLastDir == 0 ? QString() : *mLastDir);

	if (filename.isNull()) return;

	int sep = filename.lastIndexOf("/");
	if (sep >= 0)
	{
		delete mLastDir;
		mLastDir = new QString(filename.left(sep));
	}

	QFile file(filename);

	if (file.exists())
	{
		if (QMessageBox::question(this, _CAPTION_, tr("File already exists. Overwrite?"),
				QMessageBox::Yes, QMessageBox::No|QMessageBox::Escape) != QMessageBox::Yes)
			return;
	}

	// write log messages to file
	if ( !file.open( QIODevice::WriteOnly ) )
	{
		QMessageBox::warning(this, _CAPTION_, tr("Could not create file"));
		return;
	}

	QTextStream ts(&file);

	QTreeWidgetItem* item = mTRW_Messages->topLevelItem(0);
	int count = mTRW_Messages->topLevelItemCount();

	QString title = tr("## %1 Log Messages ##").arg(_APP_NAME_);

	QString fill;
	fill.fill('#', title.length());

	ts << fill << "\r\n" << title << "\r\n" << fill << "\r\n\r\n";

	for (int i=1; i<=count && item != 0; ++i)
	{
		ts << item->text(0) << "\r\n";
		item = mTRW_Messages->topLevelItem(i);
	}
}

/*!
	Returns the color used for messages of given type.
 */
const QColor& Logger::color(MsgType type) const
{
	switch (type)
	{
	case Notification: return *mColN;
	case Warning: return *mColW;
	case Error: return *mColE;
	}

	return *mColN;
}



/*!
	Sets the color used for messages of given type.
 */
void Logger::setColor(MsgType type, const QColor& col)
{
	if (!col.isValid())
	{
		error(tr("Attempting to set an invalid color"), Low);
		return;
	}

	switch (type)
	{
	case Notification:
		delete mColN;
		mColN = new QColor(col);
		mPixN->fill(col);
		break;
	case Warning:
		delete mColW;
		mColW = new QColor(col);
		mPixW->fill(col);
		break;
	case Error:
		delete mColE;
		mColE = new QColor(col);
		mPixE->fill(col);
	}

	mPixN->setMask(*mMask);
	mPixW->setMask(*mMask);
	mPixE->setMask(*mMask);
}

/*!
	Returns the current priority.
	Messages with lower priority won't be added.
*/
Logger::MsgPriority Logger::priority() const
{
	return mPriority;
}

/*!
	Sets the priority level.
	Messages with lower priority won't be added.
	Changes only affect new messages.
*/
void Logger::setPriority(MsgPriority priority)
{
	mPriority = priority;
}

/*!
	Returns true if messages with given type are hidden.
*/
bool Logger::filter(MsgType type) const
{
	switch (type)
	{
	case Notification: return mFilterN;
	case Warning: return mFilterW;
	case Error: return mFilterE;
	}

	return false;
}

/*!
	Sets whether messages of given type should be hidden or not.
	Also affects old messages.
*/
void Logger::setFilter(MsgType type, bool enable)
{
	switch (type)
	{
	case Notification: if (mFilterN == enable) return; mFilterN = enable; applyFilter(); break;
	case Warning: if (mFilterW == enable) return; mFilterW = enable; applyFilter(); break;
	case Error: if (mFilterE == enable) return; mFilterE = enable; applyFilter(); break;
	}
}

/*!
	Applies the current log message filter.
*/
void Logger::applyFilter()
{
	QTreeWidgetItem* item = mTRW_Messages->topLevelItem(0);
	int count = mTRW_Messages->topLevelItemCount();

	for (int i=1; i<=count && item != 0; ++i)
	{
		switch ((MsgType)(item->data(0, LOGGER_TYPE)).toInt())
		{
		case Notification: mTRW_Messages->setItemHidden(item, mFilterN); break;
		case Warning: mTRW_Messages->setItemHidden(item, mFilterW); break;
		case Error: mTRW_Messages->setItemHidden(item, mFilterE);
		}

		item = mTRW_Messages->topLevelItem(i);
	}
}

/*!
	Sets the initial directory for the save log dialog.
*/
void Logger::setLastDir(const QString& dir)
{
	delete mLastDir;
	mLastDir = new QString(dir);
}

/*!
	Returns the initial directory for the save log dialog.
*/
QString Logger::lastDir() const
{
	return mLastDir == 0 ? QString() : QString(*mLastDir);
}
