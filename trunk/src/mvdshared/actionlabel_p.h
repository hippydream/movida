/**************************************************************************
** Filename: actionlabel_p.h
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the MvdShared API.  It exists for the convenience
// of Movida.  This header file may change from version to version without notice, 
// or even be removed.
//
// We mean it.
//

#ifndef MVD_ACTIONLABEL_P_H
#define MVD_ACTIONLABEL_P_H

#include <QObject>
#include <QList>

class MvdActionLabel;

class MvdActionLabel_P : public QObject
{
	Q_OBJECT
	
public:
	struct AdvancedControl
	{
		AdvancedControl(int aid = -1) : id(aid) {}
		AdvancedControl(int aid, const QString& alabel, bool aenabled = true)
			: id(aid), label(alabel), enabled(aenabled) {}

		bool operator==(const AdvancedControl& o) const { return id == o.id; };

		int id;
		QString label;
		bool enabled;
	};
	
	inline MvdActionLabel_P(MvdActionLabel* label);
	inline void layoutAdvancedControls();
	
public slots:
	void linkActivated(const QString& url);

public:
	QList<AdvancedControl> controls;
	int ids;
	
private:
	MvdActionLabel* q;
};

#endif // MVD_ACTIONLABEL_P_H
