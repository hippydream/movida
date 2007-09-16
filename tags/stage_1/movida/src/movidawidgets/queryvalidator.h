/**************************************************************************
** Filename: queryvalidator.h
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

#ifndef MVDW_QUERYVALIDATOR_H
#define MVDW_QUERYVALIDATOR_H

#include <QString>
#include <QValidator>
#include <QRegExp>

class MvdwQueryValidator : public QValidator
{
public:
	//! Creates a new validator that only accepts non empty strings (i.e. strings that contain at least one non white space character).
	MvdwQueryValidator(QObject* parent = 0)
	: QValidator(parent)
	{}

	//! Creates a new validator for the specified regular expression.
	MvdwQueryValidator(const QString& pattern, QObject* parent = 0)
	: QValidator(parent)
	{
		rx.setPattern(pattern);
	}

	//! Creates a new validator for the specified regular expression.
	MvdwQueryValidator(const QRegExp& regExp, QObject* parent = 0) 
	: QValidator(parent), rx(regExp)
	{ }

	virtual void fixup(QString& input) const
	{
		input = input.trimmed();
	}

	virtual ~MvdwQueryValidator()
	{}

	virtual QValidator::State validate(QString& input, int& pos) const
	{
		if (input.isEmpty())
			return QValidator::Intermediate;

		if (input.trimmed().isEmpty())
			return QValidator::Intermediate;

		if (!rx.pattern().isEmpty() && rx.isValid())
			return rx.exactMatch(input) ? QValidator::Acceptable : QValidator::Invalid;
		
		return QValidator::Acceptable;
	}

	QRegExp rx;
};

#endif // MVDW_QUERYVALIDATOR_H

