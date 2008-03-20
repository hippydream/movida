/**************************************************************************
** Filename: smartviewdelegate.cpp
**
** Copyright (C) 2007-2008 Angius Fabrizio. All rights reserved.
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
#include "mainwindow.h"
#include "mvdcore/core.h"
#include "mvdcore/moviecollection.h"
#include "mvdcore/movie.h"
#include <QApplication>
#include <QFile>
#include <QFontMetrics>
#include <QListView>
#include <QModelIndex>
#include <QPainter>
#include <QPixmapCache>
#include <QStatusBar>
#include <QtDebug>
#include <math.h>

/*!
	\class MvdSmartViewDelegate smartviewdelegate.h

	\brief Draws icons with a quick description of the movie on the right 
	side of a pixmap (possibly containing a movie poster).
*/

//! Margin around the item - replaces QListView item to fix a layout bug
const int MvdSmartViewDelegate::Margin = 10;
//! Spacing between item border and contents
const int MvdSmartViewDelegate::Padding = 4;
//! Focus indicator corner
const int MvdSmartViewDelegate::CornerWidth = 8;
//! Height/width for a single control
const int MvdSmartViewDelegate::ControlSize = 12;
//! Item and icon border
const int MvdSmartViewDelegate::BorderWidth = 1;
//! Alternative spacing used in some cases (e.g. overlay icon margin)
const int MvdSmartViewDelegate::HalfPadding = 1;
//! Spacing between icon area and text area
const int MvdSmartViewDelegate::IconMarginRight = 6;
//! Spacing between actual icon pixmap and its border
const int MvdSmartViewDelegate::IconPadding = 1;

const float MvdSmartViewDelegate::ItemAspectRatio = 2.7f;
const float MvdSmartViewDelegate::IconAspectRatio = 0.66f;

const bool MvdSmartViewDelegate::UseTitleSeparator = true;

const QColor MvdSmartViewDelegate::BorderColor = QColor(164, 164, 164);
const QColor MvdSmartViewDelegate::SelectionColor = QColor(112, 142, 194);
const QColor MvdSmartViewDelegate::InactiveSelectionColor = QColor(173, 190, 220);

const Qt::Alignment MvdSmartViewDelegate::IconAlignment = Qt::AlignHCenter | Qt::AlignVCenter;


/*!
	Creates a new Smart View delegate.
	The delegate attempts to override some parameters of the attached view,
	like spacing and ViewMode if the view is a QListView.
	Please change these parameters after setting the delegate if you need to.
*/
MvdSmartViewDelegate::MvdSmartViewDelegate(QObject* parent)
: QItemDelegate(parent), mItemSize(InvalidItemSize)
{
	mView = qobject_cast<QListView*>(parent);
	Q_ASSERT_X(mView, "MvdSmartViewDelegate constructor", "MvdSmartViewDelegate must be used on a QListView or on a QListView subclass.");
	
	QPalette p = mView->palette();
	p.setBrush(QPalette::Normal, QPalette::Highlight, QBrush(SelectionColor));
	p.setBrush(QPalette::Inactive, QPalette::Highlight, QBrush(InactiveSelectionColor));
	mView->setPalette(p);

	mView->setViewMode(QListView::IconMode);
	mView->setWrapping(true);

	setItemSize(MediumItemSize);
	mRatingIcon = QIcon(QLatin1String(":/images/rating.svgz"));
	mSpecialIcon = QIcon(QLatin1String(":/images/special.svgz"));
	mLoanedIcon = QIcon(QLatin1String(":/images/loaned-overlay.svgz"));
	mSeenIcon = QIcon(QLatin1String(":/images/seen.svgz"));
}

/*!
	Sets the size of a tile. MediumItemSize is set if size is InvalidItemSize.

	<i>SmallItemSize</i>: 4 lines of text
	<i>MediumItemSize</i>: 6 lines of text
	<i>LargeItemSize</i>: 8 lines of text
*/
void MvdSmartViewDelegate::setItemSize(ItemSize size)
{
	if (mItemSize == size)
		return;

	if (!size)
		size = MediumItemSize;
	mItemSize = size;

	// Compute new metrics
	QFontMetrics fontMetrics = mView->fontMetrics();
	int lineCount = size == SmallItemSize ? 4 : size == MediumItemSize ? 6 : 8;

	// Text width depends on item aspect ratio, and thus on the icon size
	int textHeight = fontMetrics.height() * lineCount + 3 * Padding; // Add some extra space
	if (UseTitleSeparator) {
		textHeight += HalfPadding + Padding + BorderWidth; // Separator after title
		textHeight += fontMetrics.leading() * (lineCount - 2);
	} else textHeight += fontMetrics.leading() * (lineCount - 1);

	// Icon size depends on text height and controls height.
	int iconHeight = textHeight + HalfPadding + ControlSize;
	int iconWidth = int(IconAspectRatio * iconHeight);
	
	textHeight -= (2 * BorderWidth + 2 * HalfPadding); // Leave more space

	// We have the icon width and we can compute the text width now
	int itemHeight = BorderWidth + Padding + iconHeight + Padding + BorderWidth;
	int itemWidth = itemHeight * ItemAspectRatio;
	int textWidth = itemWidth - BorderWidth - Padding - iconWidth - IconMarginRight - Padding - BorderWidth;

	mSize = QSize(itemWidth, itemHeight);
	mIconSize = QSize(iconWidth, iconHeight);
	mControlsSize = QSize(textWidth, ControlSize);
	mTextSize = QSize(textWidth, textHeight);
	
	// This will update the view automatically.
	mView->setIconSize(mSize);
	rebuildDefaultIcon();
}

//! Returns the current item size.
MvdSmartViewDelegate::ItemSize MvdSmartViewDelegate::itemSize() const
{
	return mItemSize;
}

//! Forces a metrics update after a style or font change.
void MvdSmartViewDelegate::forcedUpdate()
{
	ItemSize sz = mItemSize;
	mItemSize = InvalidItemSize;
	setItemSize(sz);
}

//! \internal
void MvdSmartViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if (!index.isValid())
		return;

	bool isSelected =  option.state & QStyle::State_Selected;
	bool hasFocus =  option.state & QStyle::State_HasFocus;
	bool isEnabled = option.state & QStyle::State_Enabled;
	bool isActive = option.state & QStyle::State_Active;
	int ctrlIndex = -1;
	Control hoveredCtrl = hoveredControl(option.rect, &ctrlIndex);

	QRect rItem = option.rect;
	QRect rIconArea(rItem.x() + Padding, rItem.y() + Padding, mIconSize.width(), mIconSize.height());
	QRect rText(rIconArea.x() + rIconArea.width() + IconMarginRight, rIconArea.y() + BorderWidth + HalfPadding, mTextSize.width(), mTextSize.height());
	QRect rControls(rText.x(), rText.y() + rText.height() + HalfPadding, mControlsSize.width(), mControlsSize.height());

	QPen pen;
	QBrush brush;

	QAbstractItemModel* model = mView->model();
	
	bool isLoaned = false;
	bool isSpecial = false;
	bool isSeen = false;

	if (model) {
		QModelIndex columnIndex = model->index(index.row(), int(Movida::SpecialAttribute), index.parent());
		isSpecial = columnIndex.data(Movida::SmartViewDisplayRole).toBool();
		columnIndex = model->index(index.row(), int(Movida::SeenAttribute), index.parent());
		isSeen = columnIndex.data(Movida::SmartViewDisplayRole).toBool();
		columnIndex = model->index(index.row(), int(Movida::LoanedAttribute), index.parent());
		isLoaned = columnIndex.data(Movida::SmartViewDisplayRole).toBool();
	}
	
	painter->save();

	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setRenderHint(QPainter::TextAntialiasing, true);

	QColor borderColor = isSelected ? BorderColor.darker() : BorderColor;

	//////////////////////////////////////////////////////////////////////////
	// Draw item border & background
	//////////////////////////////////////////////////////////////////////////
	painter->save();

	pen = painter->pen();
	pen.setColor(borderColor);
	pen.setStyle(hasFocus ? Qt::DashLine : Qt::SolidLine);
	pen.setWidth(BorderWidth);
	painter->setPen(pen);

	QPalette::ColorGroup cg = isEnabled ? isActive ? QPalette::Normal : QPalette::Inactive : QPalette::Disabled;
	brush = option.palette.brush(cg, isSelected ? QPalette::Highlight : QPalette::Base);
	if (hasFocus)
		brush.setColor(brush.color().lighter(110));
	painter->fillRect(option.rect, brush);
	
	// Disable AA to fix a rendering issue with the dashed focus rectangle
	painter->setRenderHint(QPainter::Antialiasing, false);
	painter->drawRect(rItem.adjusted(0, 0, -BorderWidth, -BorderWidth));
	if (hasFocus) {
		// Draw an additional focus marker to enhance visibility
		QRect r = option.rect.adjusted(0, 0, -BorderWidth, -BorderWidth);
		QPainterPath path(QPoint(r.x() + r.width() - CornerWidth, r.y()));
		path.lineTo(r.x() + r.width(), r.y());
		path.lineTo(r.x() + r.width(), r.y() + CornerWidth);
		painter->setBrush(borderColor);
		painter->drawPath(path);
	}

	painter->restore();
	//////////////////////////////////////////////////////////////////////////

#if 0
	if (isHovered) {
		painter->fillRect(rIconArea, QBrush(Qt::yellow));
		painter->fillRect(rControls, QBrush(Qt::yellow));
		painter->fillRect(rText, QBrush(Qt::yellow));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Icon
	//////////////////////////////////////////////////////////////////////////
	painter->save();

	// Load pixmap so we can adjust the icon border later.
	// Default pixmap is stretched so we don't care.
	int iconDecoWidth = IconPadding + BorderWidth;
	QRect rIcon(rIconArea.adjusted(iconDecoWidth, iconDecoWidth, -iconDecoWidth, -iconDecoWidth));

	QPixmap pixmap;
	QString customPixmapPath = index.data(Qt::DecorationRole).toString();

	if (!customPixmapPath.isEmpty()) {
		QString pixmapKey = QString("%1x%2/%3")
			.arg(rIconArea.width()).arg(rIconArea.height())
			.arg(customPixmapPath);

		if (!QPixmapCache::find(pixmapKey, pixmap)) {
			pixmap = QPixmap(customPixmapPath);
			if (!pixmap.isNull()) {
				// Resize and cache poster pixmap
				pixmap = pixmap.scaled(rIcon.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
				QPixmapCache::insert(pixmapKey, pixmap);
			}
		}

		// Align pixmap
		if (!pixmap.isNull()) {
			int dx = (rIcon.width() - pixmap.width()) / 2;
			int dy = (rIcon.height() - pixmap.height()) / 2;
			
			if (IconAlignment.testFlag(Qt::AlignLeft)) {
				dx = rIcon.width() - pixmap.width();
				rIcon.adjust(0, 0, -dx, 0);
			}
			if (IconAlignment.testFlag(Qt::AlignHCenter)) {
				rIcon.adjust(dx, 0, -dx, 0);
			}
			if (IconAlignment.testFlag(Qt::AlignRight)) {
				dx = rIcon.width() - pixmap.width();
				rIcon.adjust(dx, 0, 0, 0);
			}

			if (IconAlignment.testFlag(Qt::AlignTop)) {
				dy = rIcon.height() - pixmap.height();
				rIcon.adjust(0, 0, 0, -dy);
			}
			if (IconAlignment.testFlag(Qt::AlignVCenter)) {
				rIcon.adjust(0, dy, 0, -dy);
			}
			if (IconAlignment.testFlag(Qt::AlignBottom)) {
				dy = rIcon.height() - pixmap.height();
				rIcon.adjust(0, dy, 0, 0);
			}
		}
	}

	// Re-inflate to take border & padding into account
	QRect rIconWithBorder = rIcon.adjusted(-iconDecoWidth, -iconDecoWidth, iconDecoWidth, iconDecoWidth);

	// Internal icon border should use the base color and not the selection color
	if (isSelected) {
		painter->fillRect(rIconWithBorder, option.palette.brush(cg, QPalette::Base));
	}

	// Icon border
	pen = painter->pen();
	pen.setStyle(Qt::SolidLine);
	pen.setColor(borderColor);
	painter->setPen(pen);
	// Disable AA to avoid a rendering bug (or just an ugly "feature" with 1px lines)
	painter->setRenderHint(QPainter::Antialiasing, false);
	painter->drawRect(rIconWithBorder.adjusted(0, 0, -BorderWidth, -BorderWidth));
	painter->setRenderHint(QPainter::Antialiasing, true);

	// And now draw the pixmap! :)
	painter->drawPixmap(rIcon, pixmap.isNull() ? mDefaultPoster : pixmap);

	if (isLoaned) {
		painter->save();
		painter->setOpacity(0.4);
		painter->fillRect(rIcon, QBrush(Qt::black));
		painter->restore();

		// Get a square area in the center of the pixmap
		int overlayWidth = qMin(rIcon.width(), rIcon.height());
		int dx = (rIcon.width() - overlayWidth) / 2  + HalfPadding;
		int dy = (rIcon.height() - overlayWidth) / 2 + HalfPadding;
		QRect rOverlay = rIcon.adjusted(dx, dy, -dx, -dy);
		
		painter->save();
		painter->setOpacity(0.8);
		painter->drawPixmap(rOverlay, mLoanedIcon.pixmap(rOverlay.size(), QIcon::Normal));
		painter->restore();
	}

	painter->restore();
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Text
	//////////////////////////////////////////////////////////////////////////
	painter->save();

	if (model) {
		
		QRect br;
		QRect rCurrentText(rText);
		TextOptions textOptions;

		//////////// Title
		QModelIndex columnIndex = model->index(index.row(), int(Movida::TitleAttribute), index.parent());
		QString text = columnIndex.data(Movida::SmartViewDisplayRole).toString();
		if (text.isEmpty()) {
			columnIndex = model->index(index.row(), int(Movida::OriginalTitleAttribute), index.parent());
			text = columnIndex.data(Movida::SmartViewDisplayRole).toString();
			if (text.isEmpty())
				text = QLatin1String("???");
		}

		QFont font = option.font;
		font.setBold(true);
		painter->setFont(font);
		drawItemText(painter, option, rCurrentText, text, textOptions, &br);
		font.setBold(false);
		painter->setFont(font);

		if (UseTitleSeparator) {
			int h = br.bottom() + HalfPadding;
			painter->save();
			pen = painter->pen();
			pen.setWidth(BorderWidth);
			pen.setColor(borderColor);
			painter->setPen(pen);
			painter->setRenderHint(QPainter::Antialiasing, false);
			painter->drawLine(rText.x(), h, rText.x() + rText.width(), h);
			painter->setRenderHint(QPainter::Antialiasing, true);
			painter->restore();
			br.setHeight(br.height() + Padding + BorderWidth);
		}

		//////////// Year
		textOptions.headingLevel = TextOptions::H2_HeadingLevel;

		columnIndex = model->index(index.row(), int(Movida::ReleaseYearAttribute), index.parent());
		text = columnIndex.data(Movida::SmartViewDisplayRole).toString();
		if (!text.isEmpty()) {
			text.prepend(Movida::movieAttributeString(Movida::ReleaseYearAttribute, Movida::SmartViewContext));
			rCurrentText.setTop(rCurrentText.top() + br.height());
			drawItemText(painter, option, rCurrentText, text, textOptions, &br);
		}

		//////////// Running time
		columnIndex = model->index(index.row(), int(Movida::RunningTimeAttribute), index.parent());
		text = columnIndex.data(Movida::SmartViewDisplayRole).toString();
		if (!text.isEmpty()) {
			text.prepend(Movida::movieAttributeString(Movida::RunningTimeAttribute, Movida::SmartViewContext));
			rCurrentText.setTop(rCurrentText.top() + br.height());
			drawItemText(painter, option, rCurrentText, text, textOptions, &br);
		}

		//////////// Directors
		textOptions.headingLevel = TextOptions::H3_HeadingLevel;

		rCurrentText.setTop(rCurrentText.top() + br.height());
		columnIndex = model->index(index.row(), int(Movida::DirectorsAttribute), index.parent());
		text = columnIndex.data(Movida::SmartViewDisplayRole).toString();
		if (!text.isEmpty()) {
			text.prepend(Movida::movieAttributeString(Movida::DirectorsAttribute, Movida::SmartViewContext));
			drawItemText(painter, option, rCurrentText, text, textOptions, &br, 2);
		}

		QFontMetrics fm(painter->font());
		bool hasFreeSpace = text.isEmpty() || (rText.bottom() - br.bottom()) > fm.lineSpacing();
		
		//////////// Cast
		if (hasFreeSpace) {
			rCurrentText.setTop(rCurrentText.top() + br.height() + HalfPadding);
			columnIndex = model->index(index.row(), int(Movida::CastAttribute), index.parent());
			text = columnIndex.data(Movida::SmartViewDisplayRole).toString();
			if (!text.isEmpty()) {
				text.prepend(Movida::movieAttributeString(Movida::CastAttribute, Movida::SmartViewContext));
				drawItemText(painter, option, rCurrentText, text, textOptions, &br, 2);
			}
		}

		hasFreeSpace = text.isEmpty() || (rText.bottom() - br.bottom()) > fm.lineSpacing();

		//////////// Producers
		if (hasFreeSpace) {
			rCurrentText.setTop(rCurrentText.top() + br.height() + HalfPadding);
			columnIndex = model->index(index.row(), int(Movida::ProducersAttribute), index.parent());
			text = columnIndex.data(Movida::SmartViewDisplayRole).toString();
			if (!text.isEmpty()) {
				text.prepend(Movida::movieAttributeString(Movida::ProducersAttribute, Movida::SmartViewContext));
				drawItemText(painter, option, rCurrentText, text, textOptions, &br, 2);
			}
		}
	}

	painter->restore();
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Controls
	//////////////////////////////////////////////////////////////////////////
	painter->save();

	int rating = 0;
	if (model) {
		QModelIndex columnIndex = model->index(index.row(), int(Movida::RatingAttribute), index.parent());
		rating = columnIndex.data(Movida::SmartViewDisplayRole).toInt();
	}
	int maxRating = MvdCore::parameter("mvdcore/max-rating").toInt();

	// Center rating below icon
	QRect rRating(rControls);
	rRating.setWidth(ControlSize);
	for (int i = 1; i <= maxRating; ++i) {
		if (hoveredCtrl == RatingControl) {
			if (i <= ctrlIndex)
				painter->drawPixmap(rRating, mRatingIcon.pixmap(ControlSize, ControlSize, QIcon::Selected));
			else painter->drawPixmap(rRating, mRatingIcon.pixmap(ControlSize, ControlSize, QIcon::Disabled));
		}
		else {
			if (i <= rating)
				painter->drawPixmap(rRating, mRatingIcon.pixmap(ControlSize, ControlSize, QIcon::Normal));
			else painter->drawPixmap(rRating, mRatingIcon.pixmap(ControlSize, ControlSize, QIcon::Disabled));
		}
		rRating.moveLeft(rRating.left() + ControlSize);
	}

	// Extra controls
	QRect rCurrentControl(rControls);
	rCurrentControl.setLeft(rControls.x() + rControls.width() - rControls.height());
	painter->drawPixmap(rCurrentControl, mSpecialIcon.pixmap(rCurrentControl.size(), 
		hoveredCtrl == SpecialControl ? QIcon::Selected : isSpecial ? QIcon::Normal : QIcon::Disabled));

	rCurrentControl.moveLeft(rCurrentControl.left() - ControlSize - Padding);
	painter->drawPixmap(rCurrentControl, mSeenIcon.pixmap(rCurrentControl.size(), 
		hoveredCtrl == SeenControl ? QIcon::Selected : isSeen ? QIcon::Normal : QIcon::Disabled));

	painter->restore();
	//////////////////////////////////////////////////////////////////////////

	// Final restore ;-)
	painter->restore();
}

void MvdSmartViewDelegate::drawItemText(QPainter* painter, const QStyleOptionViewItem& option,
	QRect rect, QString text, const TextOptions& textOptions, QRect* boundingRect, int maxLines) const
{	
	// Extract text from the model.
	if (text.isEmpty()) {
		if (boundingRect) boundingRect->setSize(QSize(0, 0));
		return;
	}

	QFontMetrics fm(painter->font());
	QPen pen = painter->pen();

	QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
		? QPalette::Normal : QPalette::Disabled;

	if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
		cg = QPalette::Inactive;

	QColor color = option.palette.color(cg, option.state & QStyle::State_Selected ?
		QPalette::HighlightedText : QPalette::Text);
	if (textOptions.headingLevel > TextOptions::H1_HeadingLevel) {
		// 20% lighter
		color.setHsv(color.hue(), color.saturation(), ((255 * 30) / 100), color.alpha());
	}

	const QStyleOptionViewItemV2 opt = option;
	
	mTextOption.setWrapMode(maxLines == 1 ? QTextOption::ManualWrap : QTextOption::WrapAtWordBoundaryOrAnywhere);
	mTextOption.setTextDirection(option.direction);
	mTextLayout.setTextOption(mTextOption);
	mTextLayout.setFont(painter->font());
	mTextLayout.setText(text);

	int lines = 0;
	QSizeF textLayoutSize = doTextLayout(rect.width(), rect.height(), &lines);

	bool widthViolation = rect.width() < textLayoutSize.width();
	if (maxLines == 1 && widthViolation) {
		// Text does not fit and must be elided
		mTextLayout.setText(fm.elidedText(text, option.textElideMode, rect.width()));
		textLayoutSize = doTextLayout(rect.width(), rect.height());
	}

	if (maxLines > 1) {
		rect.setHeight(qMin(rect.height(), fm.lineSpacing() * maxLines));
	}

	int maxH = rect.height();
	int h = mTextLayout.boundingRect().height();
	while (h > maxH) {
		// Looks awful but I can't see a better way to do it
		QString text = mTextLayout.text();
		int idx = text.lastIndexOf(QRegExp("\\s"));
		if (idx > 0) {
			if (text.at(idx - 1) == ',')
				idx--;
			text.truncate(idx);
			text.append(QLatin1String("..."));
		}
		mTextLayout.setText(text);
		textLayoutSize = doTextLayout(rect.width(), rect.height());
		h = mTextLayout.boundingRect().height();
	}
	painter->save();
	painter->setPen(color);
	painter->setClipRect(rect); // Text layout won't clip if word wrapping is on
	mTextLayout.draw(painter, rect.topLeft(), QVector<QTextLayout::FormatRange>(), rect);
	painter->restore();

	if (boundingRect) {
		boundingRect->setX(rect.x());
		boundingRect->setY(rect.y());
		boundingRect->setSize(QSize(textLayoutSize.width(), textLayoutSize.height()));
	}
}

//! \internal
QSizeF MvdSmartViewDelegate::doTextLayout(int lineWidth, int maxHeight, int* _lineCount) const
{
	Q_UNUSED(maxHeight);

	QFontMetrics fontMetrics(mTextLayout.font());
	int leading = fontMetrics.leading();
	qreal height = 0;
	qreal widthUsed = 0;
	int lineCount = 0;

	mTextLayout.beginLayout();
	while (true) {
		QTextLine line = mTextLayout.createLine();
		if (!line.isValid())
			break;

		line.setLineWidth(lineWidth);
		height += leading;
		line.setPosition(QPointF(0, height));
		height += line.height();
		widthUsed = qMax(widthUsed, line.naturalTextWidth());
		lineCount++;
	}
	mTextLayout.endLayout();

	if (_lineCount)
		*_lineCount = lineCount;

	return QSizeF(widthUsed, height);
}

//! \internal
QSize MvdSmartViewDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	Q_UNUSED(option);
	Q_UNUSED(index);
	return mSize;
}

//! Returns the current maximum icon size (whatever the AR of the drawn pixmap is).
QSize MvdSmartViewDelegate::maximumIconSize() const
{
	int h = mSize.height() - (BorderWidth + Padding) * 2;
	int w = int(IconAspectRatio * h);

	return QSize(w, h);
}

//! \internal
void MvdSmartViewDelegate::rebuildDefaultIcon()
{
	int iconDecoWidth = IconPadding + BorderWidth;
	int w = mIconSize.width() - iconDecoWidth;
	int h = mIconSize.height() - iconDecoWidth;

	//! \todo SVG default poster
	mDefaultPoster = QPixmap(":/images/default-poster.png")
		.scaled(QSize(w, h), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

//! \internal
bool MvdSmartViewDelegate::hasMouseOver(const QRect& itemRect) const
{
	QPoint p = mView->mapFromGlobal(QCursor::pos());
	return itemRect.contains(p);
}

//! \internal
MvdSmartViewDelegate::Control MvdSmartViewDelegate::hoveredControl(const QRect& itemRect, int* index) const
{
	if (QApplication::mouseButtons())
		return NoControl;

	QPoint p = mView->mapFromGlobal(QCursor::pos());
	if (!itemRect.contains(p))
		return NoControl;

	// Compute controls area
	QRect rIconArea(itemRect.x() + Padding, itemRect.y() + Padding, mIconSize.width(), mIconSize.height());
	QRect rText(rIconArea.x() + rIconArea.width() + IconMarginRight, rIconArea.y() + BorderWidth + HalfPadding, mTextSize.width(), mTextSize.height());
	QRect rControls(rText.x(), rText.y() + rText.height() + HalfPadding, mControlsSize.width(), mControlsSize.height());

	if (!rControls.contains(p))
		return NoControl;

	// Rating controls
	int maxRating = MvdCore::parameter("mvdcore/max-rating").toInt();
	QRect r = rControls;
	r.setWidth(ControlSize * maxRating);
	if (r.contains(p)) {
		if (index) {
			int offset = p.x() - rControls.x();
			*index = (int) floor((double)offset / ControlSize) + 1;
		}
		return RatingControl;
	}

	// Right-most control
	r.setX(rControls.right() - ControlSize);
	r.setWidth(ControlSize);
	if (r.contains(p)) {
		return SpecialControl;
	}

	r.moveLeft(r.left() - ControlSize - Padding);
	if (r.contains(p)) {
		return SeenControl;
	}

	return NoControl;
}

void MvdSmartViewDelegate::mousePressed(const QRect& rect, const QModelIndex& index)
{
	int i;
	Control control = hoveredControl(rect, &i);
	if (control == NoControl)
		return;

	Q_ASSERT(Movida::MainWindow);
	MvdMovieCollection* c = Movida::MainWindow->currentCollection();
	mvdid id = index.data(Movida::IdRole).toUInt();
	if (!id)
		return;
	MvdMovie movie = c->movie(id);
	if (!movie.isValid())
		return;
	
	if (control == RatingControl) {
		if (movie.rating() == i || i < 0)
			return;
		movie.setRating(i);
		c->updateMovie(id, movie);

	} else if (control == SpecialControl) {
		
		movie.setSpecialTagEnabled(MvdMovie::SpecialTag, !movie.hasSpecialTagEnabled(MvdMovie::SpecialTag));
		c->updateMovie(id, movie);

	} else if (control == SeenControl) {

		movie.setSpecialTagEnabled(MvdMovie::SeenTag, !movie.hasSpecialTagEnabled(MvdMovie::SeenTag));
		c->updateMovie(id, movie);
	}
}

//!
void MvdSmartViewDelegate::showHoveredControlHint()
{
	QModelIndex index = mView->indexAt(mView->mapFromGlobal(QCursor::pos()));
	QRect rect = mView->visualRect(index);
	int ctrlIndex = -1;
	Control hoveredCtrl = hoveredControl(rect, &ctrlIndex);

	switch (hoveredCtrl) {
	case RatingControl:
		Movida::MainWindow->statusBar()->showMessage(tr("Click to set the rating for this movie to \"%1\".")
			.arg(MvdMovie::ratingTip(quint8(ctrlIndex)).toLower()), 2000);
		break;
	case SeenControl:
		Movida::MainWindow->statusBar()->showMessage(tr("Click to toggle the \"seen\" tag."), 2000);
		break;
	case SpecialControl:
		Movida::MainWindow->statusBar()->showMessage(tr("Click to toggle the \"special\" tag."), 2000);
		break;
	default: ;
	}
}
