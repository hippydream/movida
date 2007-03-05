/**************************************************************************
** Filename: smartviewdelegate.h
** Revision: 1
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

#ifndef MVD_SMARTVIEWDELEGATE_H
#define MVD_SMARTVIEWDELEGATE_H

#include <QItemDelegate>
#include <QTextLayout>
#include <QTextOption>

class QAbstractItemView;

class MvdSmartViewDelegate : public QItemDelegate
{
	Q_OBJECT

public:
	MvdSmartViewDelegate(QObject* parent = 0);

	void setAdditionalColumns(const QList<int>& c);
	void setTextAlignment(Qt::Alignment a);

	void setItemSize(const QSize& sz);
	QSize itemSize() const;

	bool isShowingHeaderData() const;

public slots:
	void setShowHeaderData(bool show);

protected:
	virtual void paint(QPainter* painter,
		const QStyleOptionViewItem& option,
		const QModelIndex& index) const;

	virtual QSize sizeHint(const QStyleOptionViewItem& option,
		const QModelIndex& index) const;

	virtual QSize maximumIconSize() const;

private:
	void drawItemText(QPainter* painter, const QStyleOptionViewItem& option,
		const QRect& rect, const QModelIndex& index) const;
	QSizeF doTextLayout(int lineWidth, int maxHeight, int* lineCount = 0) const;
	QString prepareItemText(const QModelIndex& index, int* maxCharsOnLine = 0) const;
	QList<QTextLayout::FormatRange> setTextFormatting(const QString& text) const;
	void rebuildDefaultIcon();

	const qreal iconAspectRatio;

	QColor borderColor;
	QColor selectionColor;
	QColor inactiveSelectionColor;
	QColor shadowColor;

	QSize currentItemSize;

	int iconBorderWidth;
	int innerIconBorderWidth;
	int borderWidth;
	int shadowWidth;
	int roundLevel;
	int firstLineSpacing;

	Qt::Alignment textAlignment;

	QPixmap defaultPixmap;

	mutable QTextLayout textLayout;
	mutable QTextOption textOption;

	QAbstractItemView* view;
	
	QList<int> columns;

	bool showHeaderData;

	//! Buggy. We need a way to elide formatted text with respect to a width boundary.
	bool useRichText;
};

#endif // MVD_SMARTVIEWDELEGATE_H
