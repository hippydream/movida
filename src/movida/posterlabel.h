/**************************************************************************
** Filename: posterlabel.h
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

#ifndef MVD_POSTERLABEL_H
#define MVD_POSTERLABEL_H

#include "linklabel.h"

class QPaintEvent;

class MvdPosterLabel : public MvdLinkLabel
{
	Q_OBJECT

public:
	MvdPosterLabel(QWidget* parent = 0);
	virtual ~MvdPosterLabel();

	bool setPoster(const QString& path);
	QString poster() const;

protected:
	void paintEvent(QPaintEvent* event);

private:
	QPixmap mPoster;
	QString mPosterPath;

	static const qreal IconAspectRatio;
	static const QColor BorderColor;
	static const QColor ShadowColor;
	static const int InnerIconBorderWidth;
	static const int BorderWidth;
	static const int ShadowWidth;
};

#endif // MVD_POSTERLABEL_H
