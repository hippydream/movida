/**************************************************************************
** Filename: smartviewdelegate.cpp
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

#include "smartviewdelegate.h"
#include "core.h"

#include <QPainter>
#include <QModelIndex>
#include <QFile>
#include <QPixmapCache>
#include <QListView>
#include <QApplication>
#include <QtDebug>
#include <math.h>

/*!
	\class MvdSmartViewDelegate smartviewdelegate.h

	\brief Draws icons with a quick description of the movie on the right 
	side of a pixmap (possibly containing a movie poster).
*/

/*!
	Creates a new Smart View delegate.
	The delegate attempts to override some parameters of the attached view,
	like spacing and ViewMode if the view is a QListView.
	Please change these parameters after setting the delegate if you need to.
*/
MvdSmartViewDelegate::MvdSmartViewDelegate(QObject* parent)
: QItemDelegate(parent),
iconAspectRatio(0.7), borderColor(164, 164, 164), 
selectionColor(112, 142, 194), inactiveSelectionColor(173, 190, 220),
shadowColor(127, 127, 127), currentItemSize(300, 110), 
iconBorderWidth(7), innerIconBorderWidth(2), borderWidth(1), shadowWidth(4), roundLevel(0),
firstLineSpacing(3), textAlignment(Qt::AlignLeft | Qt::AlignTop),
view(0),showHeaderData(true), useRichText(false)
{
	// Override some parameters of the view
	QWidget* w = qobject_cast<QWidget*>(parent);
	if (w != 0)
	{
		QPalette p = w->palette();
		p.setBrush(QPalette::Normal, QPalette::Highlight, QBrush(selectionColor));
		p.setBrush(QPalette::Inactive, QPalette::Highlight, QBrush(inactiveSelectionColor));
		w->setPalette(p);
	}

	view = qobject_cast<QAbstractItemView*>(parent);
	if (view != 0)
	{

	}

	QListView* lv = qobject_cast<QListView*>(parent);
	if (lv != 0)
	{
		lv->setSpacing(10);
		lv->setViewMode(QListView::IconMode);
		lv->setWrapping(true);
		QSize viewIconSize = lv->iconSize();
		if (viewIconSize.isValid())
			currentItemSize = viewIconSize;
	}

	rebuildDefaultIcon();
}

/*!
	Adds additional columns to this view.
	The delegate should be attached to one dimensional views, possibly a
	QListView, or a separate item will be drawn for every cell of the model.
	The view's selected column does not need to be in the list as it would be
	ignored. That column is always drawn at the top of the item's text part.
*/
void MvdSmartViewDelegate::setAdditionalColumns(const QList<int>& c)
{
	columns = c;
}

//! Sets the alignment flags for the item's text. Default is Top Left.
void MvdSmartViewDelegate::setTextAlignment(Qt::Alignment a)
{
	textAlignment = a;
}

//! Sets the size of a tile. \p sz must be a valid size.
void MvdSmartViewDelegate::setItemSize(const QSize& sz)
{
	if (sz.isValid())
		currentItemSize = sz;

	rebuildDefaultIcon();

	// This will update the view automatically.
	if (view)
		view->setIconSize(sz);
}

//! Returns the current item size.
QSize MvdSmartViewDelegate::itemSize() const
{
	return currentItemSize;
}

//! Returns true if header data is being displayed on the tiles.
bool MvdSmartViewDelegate::isShowingHeaderData() const
{
	return showHeaderData;
}

void MvdSmartViewDelegate::setShowHeaderData(bool show)
{
	if (show != showHeaderData)
	{
		showHeaderData = show;

		// Force an update of the view
		if (view)
			view->doItemsLayout();
	}
}

//! \internal
void MvdSmartViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	bool isSelected =  option.state & QStyle::State_Selected;
	bool hasFocus =  option.state & QStyle::State_HasFocus;

	QBrush defaultBrush = painter->brush();

	painter->save();

	// Init painter
	QPen pen = painter->pen();
	pen.setColor(borderColor);
	pen.setStyle((hasFocus && !isSelected) ? Qt::DashLine : Qt::SolidLine);
	pen.setWidth(borderWidth);
	painter->setPen(pen);

	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setRenderHint(QPainter::TextAntialiasing, true);

	// Draw possibly highlighted background
	QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
		? QPalette::Normal : QPalette::Disabled;
	if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
		cg = QPalette::Inactive;

	painter->fillRect(option.rect, 
		option.palette.brush(cg, isSelected ? QPalette::Highlight : QPalette::Base));

	// Draw border
	painter->drawRoundRect(option.rect, roundLevel, roundLevel);

	painter->setBrush(defaultBrush);

	QRect dataRect = option.rect.adjusted(iconBorderWidth, iconBorderWidth, -iconBorderWidth, -iconBorderWidth);

	// Draw icon if we have enough space
	QSize iconSize = maximumIconSize();

	if (currentItemSize.width() > iconSize.width() + iconBorderWidth * 2 &&
		currentItemSize.height() > iconSize.height() + iconBorderWidth * 2)
	{
		QRect iconRect(iconBorderWidth, iconBorderWidth, iconSize.width(), iconSize.height());
		iconRect.translate(option.rect.topLeft());

		// Adjust data area
		dataRect.adjust(iconRect.width() + iconBorderWidth * 2, iconBorderWidth, -iconBorderWidth, -iconBorderWidth);

		// Load pixmap so we can adjust the icon border later.
		// Default pixmap is stretched so we don't care.
		QRect pixmapRect = iconRect.adjusted(innerIconBorderWidth, innerIconBorderWidth, -innerIconBorderWidth, -innerIconBorderWidth);
		
		QPixmap pixmap;
		QString customPixmapPath = index.data(Qt::DecorationRole).toString();

		if (!customPixmapPath.isEmpty())
		{
			QString pixmapKey = QString("%1x%2/%3")
				.arg(pixmapRect.width()).arg(pixmapRect.height())
				.arg(customPixmapPath);

			if (!QPixmapCache::find(pixmapKey, pixmap))
			{
				pixmap = QPixmap(customPixmapPath);
				if (!pixmap.isNull())
				{
					pixmap = pixmap.scaled(pixmapRect.size(),
						Qt::KeepAspectRatio, Qt::SmoothTransformation);
					QPixmapCache::insert(pixmapKey, pixmap);
				}
			}

			if (!pixmap.isNull())
			{
				// Center pixmap
				int dx = (pixmapRect.width() - pixmap.width()) / 2;
				int dy = (pixmapRect.height() - pixmap.height()) / 2;
				pixmapRect.adjust(dx, dy, -dx, -dy);
			}
		}

		// Adjust actual icon area to respect the pixmap's AR.
		iconRect = pixmapRect.adjusted( -innerIconBorderWidth, -innerIconBorderWidth, innerIconBorderWidth, innerIconBorderWidth );

		// Icon border
		pen.setStyle(Qt::SolidLine);
		painter->setPen(pen);		
		painter->drawRect(iconRect);

		// Icon shadow
		painter->setPen(shadowColor);

		qreal opacityDelta = qreal(1) / qreal(shadowWidth);
		QRect shadowRect = iconRect;

		for (int i = 1; i <= shadowWidth; ++i)
		{
			shadowRect.translate(1, 1);
			painter->setOpacity(1 - opacityDelta * (i - 1));
			painter->drawLine(shadowRect.topRight(), shadowRect.bottomRight());
			painter->drawLine(shadowRect.bottomLeft(), shadowRect.bottomRight());
		}

		painter->setOpacity(1);

		// Draw pixmap
		painter->drawPixmap(pixmapRect, pixmap.isNull() ? defaultPixmap : pixmap);
	}

	painter->restore();

	drawItemText(painter, option, dataRect, index);
}

void MvdSmartViewDelegate::drawItemText(QPainter* painter, const QStyleOptionViewItem& option,
	const QRect& rect, const QModelIndex& index) const
{	
	// Extract text from the model.
	QString text = prepareItemText(index);

	if (text.isEmpty())
		return;

	QPen pen = painter->pen();

	QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
		? QPalette::Normal : QPalette::Disabled;

	if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
		cg = QPalette::Inactive;

	painter->setPen(option.palette.color(cg, option.state & QStyle::State_Selected ?
		QPalette::HighlightedText : QPalette::Text));

	const QStyleOptionViewItemV2 opt = option;
	const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
	QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0); // remove width padding
	
	textOption.setWrapMode(QTextOption::ManualWrap);
	textOption.setTextDirection(option.direction);
	textOption.setAlignment(QStyle::visualAlignment(option.direction, textAlignment));
	textLayout.setTextOption(textOption);
	textLayout.setFont(option.font);

	textLayout.setText(text);

	if (useRichText)
		textLayout.setAdditionalFormats(setTextFormatting(text));

	QSizeF textLayoutSize = doTextLayout(textRect.width(), textRect.height());

	bool widthViolation = textRect.width() < textLayoutSize.width();
	bool heightViolation = textRect.height() < textLayoutSize.height();
	if (widthViolation || heightViolation)
	{

		// Text does not fit and must be elided (or lines must be discarded)
		QString elided;
		QStringList lines = text.split(QChar::LineSeparator);

		int lineCount = lines.size();
		qreal avgLineHeight = textLayoutSize.height() / qreal(lineCount);
		int maxLines = int(floor(textRect.height() / avgLineHeight));
	
		if (!widthViolation)
		{
			int exceedingLines = lineCount - maxLines;
			for (int i = 0; i < exceedingLines; ++i)
				lines.removeLast();
			textLayout.setText(lines.join(QString(QChar::LineSeparator)));
		}
		else
		{
			for (int i = 0; i < lines.size() && i < maxLines; ++i)
			{
				if (!lines.at(i).isEmpty())
				{
					elided.append( option.fontMetrics.elidedText(lines.at(i),
						option.textElideMode, textRect.width()) );
				}
				elided.append( QChar::LineSeparator );
			}
			textLayout.setText(elided);
		}
		
		if (useRichText)
			textLayout.setAdditionalFormats(setTextFormatting(elided));
		textLayoutSize = doTextLayout(textRect.width(), textRect.height());
	}

	// Adjust vertical text alignment
	if (textAlignment & Qt::AlignVCenter)
		textRect.setTop(textRect.top() + (textRect.height()/2) - (textLayoutSize.toSize().height()/2));
	else if (textAlignment & Qt::AlignBottom)
		textRect.setTop(textRect.top() + textRect.height() - textLayoutSize.toSize().height());

	textLayout.draw(painter, textRect.topLeft(), QVector<QTextLayout::FormatRange>(), textRect);
}

//! \internal
QString MvdSmartViewDelegate::prepareItemText(const QModelIndex& index, int* maxCharsOnLine) const
{
	QString text;
	QString data;
	QString label;

	if (maxCharsOnLine)
		*maxCharsOnLine = 0;

	// Do one more step!
	for (int i = -1; i < columns.size(); ++i)
	{
		if (i >= 0 && columns.at(i) == index.column())
			continue;

		QModelIndex colIndex;
		if (i < 0)
			colIndex = index;
		else
		{
			if (!view || !view->model())
				break;

			colIndex = view->model()->index(index.row(), columns.at(i), index.parent());
			if (!colIndex.isValid())
				continue;
		}

		int charsOnLine = 0;

		data = colIndex.data().toString();
		if (showHeaderData && view && view->model())
			label = view->model()->headerData(colIndex.column(), Qt::Horizontal).toString();

		if (!label.isEmpty())
			text.append(label).append(": ");

		if (maxCharsOnLine)
		{
			charsOnLine = label.length();
			if (charsOnLine != 0)
				charsOnLine += 2; // Count for ": "
			charsOnLine += data.length();

			*maxCharsOnLine = qMax(charsOnLine, *maxCharsOnLine);
		}

		text.append(data).append(QChar::LineSeparator);
	}

	return text;
}

//! \internal
QList<QTextLayout::FormatRange> MvdSmartViewDelegate::setTextFormatting(const QString& text) const
{
	QList<QTextLayout::FormatRange> formats;
	int cursor = 0;

	QStringList lines = text.split(QChar::LineSeparator);
	for (int i = 0; i < lines.size(); ++i)
	{
		QString s = lines.at(i);
		
		if (s.isEmpty())
		{
			// Take the LineSeparator into account!
			cursor++;
			continue;
		}

		int idx = s.indexOf(":");
		QTextLayout::FormatRange fr;
		fr.start = cursor;
		fr.length = idx < 0 ? s.length() : idx + 1;
		fr.format.setFontWeight(QFont::Bold);
		formats << fr;

		if (i == 0)
		{
			// First line is assumed to contain the most relevant attribute
			// so let's put some more emphasis on it
			QTextLayout::FormatRange fr;
			fr.start = idx < 0 ? cursor : cursor + idx + 1;
			fr.length = idx < 0 ? s.length() : s.length() - (idx + 1);
			fr.format.setFontWeight(QFont::DemiBold);
			formats << fr;
		}

		// Do not format more than the first line
		if (!showHeaderData)
			break;

		// Take the LineSeparator into account!
		cursor += s.length() + 1;
	}

	return formats;
}

//! \internal
QSizeF MvdSmartViewDelegate::doTextLayout(int lineWidth, int maxHeight, int* _lineCount) const
{
	Q_UNUSED(maxHeight);

	QFontMetrics fontMetrics(textLayout.font());
	int leading = fontMetrics.leading();
	qreal height = 0;
	qreal widthUsed = 0;
	textLayout.beginLayout();
	
	int lineCount = 0;

	while (true) {
		QTextLine line = textLayout.createLine();

		if (!line.isValid())
			break;

		height += leading;
					
		line.setLineWidth(lineWidth);

		// Second line: add some more spacing so that the first line gets more emphasis
		if (lineCount++ == 1)
			height += firstLineSpacing;

		line.setPosition(QPointF(0, height));
		height += line.height();
		widthUsed = qMax(widthUsed, line.naturalTextWidth());
	}
	textLayout.endLayout();

	if (_lineCount)
		*_lineCount = lineCount;

	return QSizeF(widthUsed, height);
}

//! \internal
QSize MvdSmartViewDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	Q_UNUSED(option);
	Q_UNUSED(index);
	return currentItemSize;
}

//! Returns the current maximum icon size (whatever the AR of the drawn pixmap is).
QSize MvdSmartViewDelegate::maximumIconSize() const
{
	int h = currentItemSize.height() - (borderWidth + iconBorderWidth) * 2;
	int w = int(iconAspectRatio * h);

	return QSize(w, h);
}

//! \internal
void MvdSmartViewDelegate::rebuildDefaultIcon()
{
	QSize iconSize = maximumIconSize();

	defaultPixmap = QPixmap(":/images/misc/default-poster.png")
		.scaled(iconSize.width() - innerIconBorderWidth, iconSize.height() - innerIconBorderWidth,
		Qt::IgnoreAspectRatio, Qt::SmoothTransformation);	
}