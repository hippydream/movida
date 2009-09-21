/**************************************************************************
** Filename: naturalcompare.cpp
**
** Copyright (C) 2007-2009 Angius Fabrizio. All rights reserved.
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

/*
    This code is based on the C++ implementation of the "Alphanum" algorithm
    by Dirk Jagdmann.

    The code has been rewritten in order to use Qt's locale and string
    classes. As with the original implementation, this software is 
    provided 'as-is', without any express or implied warranty.

    Original license terms found below apply to this implementation as well.
    
    ---- ORIGINAL DESCRIPTION AND LICENSE ----

    The Alphanum Algorithm is an improved sorting algorithm for strings 
    containing numbers. Instead of sorting numbers in ASCII order like a 
    standard sort, this algorithm sorts numbers in numeric order. The 
    Alphanum Algorithm is discussed at http://www.DaveKoelle.com 
    
    This implementation is Copyright (c) 2008 Dirk Jagdmann . 

    It is a cleanroom implementation of the algorithm and not derived by 
    other's works. In contrast to the versions written by Dave Koelle this 
    source code is distributed with the libpng/zlib license. 
    This software is provided 'as-is', without any express or implied warranty. 
    In no event will the authors be held liable for any damages arising from 
    the use of this software. Permission is granted to anyone to use this 
    software for any purpose, including commercial applications, and to alter 
    it and redistribute it freely, subject to the following restrictions: 
    
    1. The origin of this software must not be misrepresented; 
       you must not claim that you wrote the original software. 
       If you use this software in a product, an acknowledgment in the 
       product documentation would be appreciated but is not required. 
    2. Altered source versions must be plainly marked as such, and must 
       not be misrepresented as being the original software. 
    3. This notice may not be removed or altered from any source distribution.
*/
#include "naturalcompare.h"

#include <QtCore/QLocale>
#include <QtCore/QVarLengthArray>

//! In qlocale.cpp
extern qlonglong qstrtoll(const char *nptr, const char **endptr, register int base, bool *ok);

namespace {
typedef QVarLengthArray<char, 256> CharBuff;

/*! \internal Converts a Unicode QChar digit using C locale 
    (i.e. 0 becomes the '0' ascii char).
*/
char digitToCLocale(const QChar &in)
{
    const QChar _zero = QLocale::system().zeroDigit();
    const ushort zeroUnicode = _zero.unicode();
    const ushort tenUnicode = zeroUnicode + 10;

    if (in.unicode() >= zeroUnicode && in.unicode() < tenUnicode)
        return '0' + in.unicode() - zeroUnicode;
    return '\0';
}

/*! \internal Extracts a number from the string at \p a_begin.
    \p a_end must point to the end of the string. The function returns a pointer
    to the first non-number char (or \p a_end if the end of the string has been reached).
    The digits are converted to C locale (i.e. 0 is the '0' ascii char).
*/
const QChar* extractNumber(const QChar* a_begin, const QChar* a_end, CharBuff& buff)
{
    while (a_begin != a_end) {
        if (a_begin->isNumber())
            buff.append(digitToCLocale(*a_begin));
        else break;
        ++a_begin;
    }
    return a_begin;
}

} // Anonymous namespace


//////////////////////////////////////////////////////////////////////////

/*! Compares a string using a natural comparison algorithm. In other words,
    it sorts the numbers in value order and the remaining characters in 
    ASCII order.

    The code is based on the LGPL C++ implementation found at 
    http://www.davekoelle.com/alphanum.html

    See this header file for license details.
*/
int Movida::naturalCompare(const QString &string_a, const QString &string_b, Qt::CaseSensitivity cs)
{
    if (string_a.unicode() == string_b.unicode())
        return 0;
    if (string_a.isEmpty())
        return -1;
    if (string_b.isEmpty())
        return +1;

    const bool _cs = cs == Qt::CaseSensitive;

    enum Mode { String, Number } mode = String;

    const QChar* a_begin = string_a.unicode();
    const QChar* a_end = a_begin + string_a.length();
    const QChar* a = a_begin;
    
    const QChar* b_begin = string_b.unicode();
    const QChar* b_end = b_begin + string_b.length();
    const QChar* b = b_begin;

    bool mode_change = false;
    int last_num_compare = 0;
    while (a != a_end && b != b_end) {
        last_num_compare = 0;
        switch (mode) {
        case String:
        {
            while (a != a_end && b != b_end)
	        {
                // Check if this are digit characters
                const bool a_digit = a->isDigit();
                const bool b_digit = b->isDigit();

                // If both characters are digits, we continue in Number mode
                if (a_digit && b_digit) {
                    mode = Number;
                    mode_change = true;
                    break;
                }

                // If only the left character is a digit, we have a result
                if (a_digit)
                    return -1;

                // If only the right character is a digit, we have a result
                if (b_digit)
                    return +1;

                // Compute the difference of both characters
                const quint16 u_a = a->unicode();
                const quint16 u_b = b->unicode();
                const quint16 diff = _cs ? (u_a - u_b) : (QChar::toCaseFolded(u_a) - QChar::toCaseFolded(u_b));
                // If they differ we have a result
                if (diff != 0)
                    return diff;

                // Otherwise process the next characters
                ++a;
                ++b;
            }
        }
        break;

        case Number:
        {
            // Get the left number
            CharBuff a_buff;
            qMemSet(a_buff.data(), 0, a_buff.capacity());
            a = extractNumber(a, a_end, a_buff);
            quint64 a_num = qstrtoll(a_buff.constData(), 0, 10, 0);

            // Get the right number
            CharBuff b_buff;
            qMemSet(b_buff.data(), 0, b_buff.capacity());
            b = extractNumber(b, b_end, b_buff);
            quint64 b_num = qstrtoll(b_buff.constData(), 0, 10, 0);

            // If the difference is not equal to zero, we have a comparison result
            const long diff = a_num - b_num;
            if (diff != 0)
                return diff;

            // If the strings differ only by the number of padding zeros
            // in the current number, we should take it into account and
            // simulate a lexical comparison by comparing the number of 
            // digits.
            last_num_compare = a_buff.size() - b_buff.size();

            // Otherwise we process the next substring in String mode
            mode = String;
        }
        break;
        }

        if (!mode_change && a != a_end && b != b_end) {
            ++a;
            ++b;
        } else mode_change = false;
    }

    if (a == a_end && b == b_end)
        return last_num_compare;
    if (a == a_end)
        return -1;
    if (b == b_end)
        return +1;
    return last_num_compare;
}
