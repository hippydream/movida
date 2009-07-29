/****************************************************************************
** Filename: utils.cpp
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
**********************************************************************/

#include "utils.h"
#include "base64.h"
#include <QPoint>
#include <QRect>
#include <QStringList>

namespace Movida {

//! From QSettingsPrivate::splitArgs()
QStringList splitArgs(const QString &s, int idx)
{
    int l = s.length();
    Q_ASSERT(l > 0);
    Q_ASSERT(s.at(idx) == QLatin1Char('('));
    Q_ASSERT(s.at(l - 1) == QLatin1Char(')'));

    QStringList result;
    QString item;

    for (++idx; idx < l; ++idx) {
        QChar c = s.at(idx);
        if (c == QLatin1Char(')')) {
            Q_ASSERT(idx == l - 1);
            result.append(item);
        } else if (c == QLatin1Char(' ')) {
            result.append(item);
            item.clear();
        } else {
            item.append(c);
        }
    }

    return result;
}

/*! \internal The code is a slightly modified version of QSettingsPrivate::stringToVariant(),
    with base 64 encoding of binary data.
*/
QVariant stringToVariant(const QString& s)
{
    if (s.startsWith(QLatin1Char('@'))) {
        if (s.endsWith(QLatin1Char(')'))) {
            if (s.startsWith(QLatin1String("@ByteArray("))) {
                return QVariant(MvdBase64::decode(s.toLatin1().mid(11, s.size() - 12)));
            } else if (s.startsWith(QLatin1String("@Variant("))) {
                //QByteArray a(s.toLatin1().mid(9));
                QByteArray a = MvdBase64::decode(s.toLatin1().mid(9));
                QDataStream stream(&a, QIODevice::ReadOnly);
                stream.setVersion(QDataStream::Qt_4_0);
                QVariant result;
                stream >> result;
                return result;
            } else if (s.startsWith(QLatin1String("@Rect("))) {
                QStringList args = splitArgs(s, 5);
                if (args.size() == 4)
                    return QVariant(QRect(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt()));
            } else if (s.startsWith(QLatin1String("@Size("))) {
                QStringList args = splitArgs(s, 5);
                if (args.size() == 2)
                    return QVariant(QSize(args[0].toInt(), args[1].toInt()));
            } else if (s.startsWith(QLatin1String("@Point("))) {
                QStringList args = splitArgs(s, 6);
                if (args.size() == 2)
                    return QVariant(QPoint(args[0].toInt(), args[1].toInt()));
            } else if (s == QLatin1String("@Invalid()")) {
                return QVariant();
            }
        }
        if (s.startsWith(QLatin1String("@@")))
            return QVariant(s.mid(1));
    }

    return QVariant(s);
}

/*! \internal The code is a slightly modified version of QSettingsPrivate::variantToString(),
    with base 64 encoding of binary data.
*/
QString variantToString(const QVariant &v)
{
    QString result;

    switch (v.type()) {
    case QVariant::Invalid:
        result = QLatin1String("@Invalid()");
        break;

    case QVariant::ByteArray:
    {
        QByteArray a = v.toByteArray();
        result = QLatin1String("@ByteArray(");
        result += MvdBase64::encode(a);
        // result += QString::fromLatin1(a.constData(), a.size());
        result += QLatin1Char(')');
        break;
    }

    case QVariant::String:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::Bool:
    case QVariant::Double:
    case QVariant::KeySequence:
    {
        result = v.toString();
        if (result.startsWith(QLatin1Char('@')))
            result.prepend(QLatin1Char('@'));
        break;
    }

    case QVariant::Rect:
    {
        QRect r = qvariant_cast<QRect>(v);
        result += QLatin1String("@Rect(");
        result += QString::number(r.x());
        result += QLatin1Char(' ');
        result += QString::number(r.y());
        result += QLatin1Char(' ');
        result += QString::number(r.width());
        result += QLatin1Char(' ');
        result += QString::number(r.height());
        result += QLatin1Char(')');
        break;
    }

    case QVariant::Size: 
    {
        QSize s = qvariant_cast<QSize>(v);
        result += QLatin1String("@Size(");
        result += QString::number(s.width());
        result += QLatin1Char(' ');
        result += QString::number(s.height());
        result += QLatin1Char(')');
        break;
    }

    case QVariant::Point:
    {
        QPoint p = qvariant_cast<QPoint>(v);
        result += QLatin1String("@Point(");
        result += QString::number(p.x());
        result += QLatin1Char(' ');
        result += QString::number(p.y());
        result += QLatin1Char(')');
        break;
    }

    default:
    {
        QByteArray a;
        {
            QDataStream s(&a, QIODevice::WriteOnly);
            s.setVersion(QDataStream::Qt_4_0);
            s << v;
        }
        result = QLatin1String("@Variant(");
        // result += QString::fromLatin1(a.constData(), a.size());
        result += MvdBase64::encode(a);
        result += QLatin1Char(')');
        break;
    }
    }

    return result;
}

} // namespace Movida
