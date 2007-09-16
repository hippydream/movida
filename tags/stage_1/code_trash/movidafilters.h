/****************************************************************************
** Filename: movidafilters.h
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** MvdMovie-related filters used in the whole application.
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

#ifndef MOVIDA_MOVIDAFILTERS__H
#define MOVIDA_MOVIDAFILTERS__H

#include <QtCore/QString>

enum FilterCompare { FC_MATCH, FC_NOTMATCH, FC_CONTAIN, FC_NOTCONTAIN, FC_EMPTY, FC_NOTEMPTY, FC_GREATER, FC_LOWER, FC_EQUAL };

struct PersonFilter
{
	QString fname;
	QString lname;
	int links;

	FilterCompare fnameC;
	FilterCompare lnameC;
	FilterCompare linksC;

	bool matchAll; // true == AND, false == OR

	PersonFilter()
	{
		links = -1;
		matchAll = true;
	}

	PersonFilter(const PersonFilter &f)
	{
		fname = f.fname;
		fnameC = f.fnameC;
		lname = f.lname;
		lnameC = f.lnameC;
		links = f.links;
		linksC = f.linksC;
		matchAll = f.matchAll;
	}
};

struct SimpleFilter
{
	QString value;
	int links;

	FilterCompare valueC;
	FilterCompare linksC;

	bool matchAll; // true == AND, false == OR

	SimpleFilter()
	{
		links = -1;
		matchAll = true;
	}

	SimpleFilter(const SimpleFilter &f)
	{
		value = f.value;
		valueC = f.valueC;
		links = f.links;
		linksC = f.linksC;
		matchAll = f.matchAll;
	}
};

#endif // MOVIDA_MOVIDAFILTERS__H
