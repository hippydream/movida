/**************************************************************************
** Filename: iconengine.h
**
** Copyright (C) 2007-2008 Angius Fabrizio. All rights reserved.
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

#ifndef MVD_SVGZ_ICONENGINE_H
#define MVD_SVGZ_ICONENGINE_H

#include <QIconEngineV2>
#include <QSharedData>

class MvdSvgzIconEnginePrivate;

class MvdSvgzIconEngine : public QIconEngineV2
{
public:
	MvdSvgzIconEngine();
	MvdSvgzIconEngine(const MvdSvgzIconEngine& other);
	virtual ~MvdSvgzIconEngine();

	void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state);
	QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state);
	QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state);

	void addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state);
	void addFile(const QString &fileName, const QSize &size, QIcon::Mode mode, QIcon::State state);

	QString key() const;
	QIconEngineV2 *clone() const;
	bool read(QDataStream &in);
	bool write(QDataStream &out) const;

private:
	QSharedDataPointer<MvdSvgzIconEnginePrivate> d;

};

#endif // MVD_SVGZ_ICONENGINE_H
