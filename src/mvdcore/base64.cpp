/**************************************************************************
** Filename: base64.cpp
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

#include "base64.h"

#include <QtCore/QBitArray>
#include <QtCore/QByteArray>
#include <QtCore/QString>

/*!
    \class MvdBase64 base64.h
    \ingroup MvdCore

    \brief Base 64 encoding and decoding.
*/


//! \internal
namespace {

static const char encodingTable[64] =
{
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
    0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5A, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x78, 0x79, 0x7A, 0x30, 0x31, 0x32, 0x33,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2B, 0x2F
};

static const char decodingTable[128] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x3F,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
    0x3C, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
    0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
    0x17, 0x18, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
    0x31, 0x32, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00
};

} // anonymous namespace


/************************************************************************
    Base64
 *************************************************************************/

//! Decodes a base 64 encoded string.
QByteArray MvdBase64::decode(const QString &encoded)
{
    if (encoded.isEmpty())
        return QByteArray();

    quint32 length = encoded.length();
    QByteArray in = encoded.toAscii();
    const char *data = in.constData();

    // Final length does not count invalid chars (RFC 2045)
    quint32 finalLength = 0;

    for (quint32 i = 0; i < length; ++i) {
        quint8 ch = data[i];
        if (ch < 123 && (ch == 65 || ::decodingTable[ch]))
            in.data()[finalLength++] = ::decodingTable[ch];
    }

    if (finalLength < 1)
        return QByteArray();

    // Base64 uses 4-byte blocks, but input may have exceeding bytes!
    quint32 rest = finalLength % 4;
    finalLength -= rest;
    length = finalLength / 4;

    switch (rest) {
        // We can get another byte out of 2 6-bit chars
        case 2:
            finalLength = length * 3 + 1; break;

            // We can get 2 bytes out of 3 6-bit chars
        case 3:
            finalLength = length * 3 + 2; break;

        default:
            finalLength = length * 3;
    }

    QByteArray out(finalLength, '\0');

    quint32 outIdx = 0; // Index for the next decoded byte
    quint32 inIdx = 0; // Index for the next encoded byte

    // 4-byte to 3-byte conversion
    for (quint32 i = 0; i < length; ++i) {
        out.data()[outIdx++] =
            (((in.at(inIdx) << 2) & 252) | ((in.at(inIdx + 1) >> 4) & 003));
        out.data()[outIdx++] =
            (((in.at(inIdx + 1) << 4) & 240) | ((in.at(inIdx + 2) >> 2) & 017));
        out.data()[outIdx++] =
            (((in.at(inIdx + 2) << 6) & 192) | (in.at(inIdx + 3) & 077));
        inIdx += 4;
    }

    switch (rest) {
        case 2:
            // Get one byte out of two 6-bit chars
            out.data()[outIdx] =
                ((in.at(inIdx) << 2) & 252) | ((in.at(inIdx + 1) >> 4) & 03);
            break;

        case 3:
            // Get one byte out of two 6-bit chars
            out.data()[outIdx++] =
                ((in.at(inIdx) << 2) & 252) | ((in.at(inIdx + 1) >> 4) & 03);
            // Get next byte
            out.data()[outIdx] =
                ((in.at(inIdx + 1) << 4) & 240) | ((in.at(inIdx + 2) >> 2) & 017);
    }

    return out;
}

/*!
    Convenience method.
    Decodes a base64 encoded string to a bit array of given size.
    If size is bigger than the decoded array's size, only decoded bytes are
    returned.
*/
QBitArray MvdBase64::decode(const QString &encoded, quint32 size)
{
    if (size == 0)
        return QBitArray();

    QByteArray ba = decode(encoded);

    quint32 sz = ba.size() * 8;

    if (sz == 0)
        return QBitArray();
    if (size < sz)
        sz = size;

    QBitArray bits(sz);

    quint32 idx = 0, currentByte = 0;

    for (quint32 i = 0; i < sz; ++i) {
        if (idx == 8) {
            idx = 0;
            currentByte++;
        }

        bits.setBit(i, ba.at(currentByte) & 1 << (7 - idx));
        idx++;
    }

    return bits;
}

/*!
    Encodes a byte array. Adds a CRLF pair at column 76 if the BreakLongLines
    option is set.
*/
QString MvdBase64::encode(const QByteArray &decoded, EncodingOptions options)
{
    quint32 len = decoded.size();

    if (len == 0)
        return QString();

    quint32 idxIn = 0; // Index of next decoded byte
    quint32 idxOut = 0; // Index for next encoded byte
    const char *data = decoded.data(); // Convenience pointer

    const quint32 rest = len % 3;
    // Final output size
    quint32 final = (len < 3) ? 4 : rest ? len / 3 * 4 + 4 : len / 3 * 4;

    bool limitLines = options.testFlag(BreakLongLines);
    if (limitLines)
        final += len % 76 ? final / 76 * 2 : (final / 76 - 1) * 2;

    QByteArray out(final, '\0');

    len = (len - rest) / 3;
    quint32 charsOnLine = 0;

    // Decoding + 3-byte to 4-byte conversion
    for (quint32 i = 0; i < len; ++i) {
        if (limitLines && charsOnLine > 0 && !(charsOnLine % 76)) {
            out.data()[idxOut++] = '\r';
            out.data()[idxOut++] = '\n';
            charsOnLine = 0;
        }

        out.data()[idxOut++] =
                ::encodingTable[(data[idxIn] >> 2) & 077];
        out.data()[idxOut++] =
                ::encodingTable[((data[idxIn + 1] >> 4) & 017) |
                                ((data[idxIn] << 4) & 077)];
        out.data()[idxOut++] =
                ::encodingTable[((data[idxIn + 2] >> 6) & 003) |
                                ((data[idxIn + 1] << 2) & 077)];
        out.data()[idxOut++] =
                ::encodingTable[data[idxIn + 2] & 077];
        idxIn += 3;
        charsOnLine += 4;
    }

    switch (rest) {
        case 1:
            // Need to encode one last byte
            // Add 4 0-bits to the right, encode two chars and append two '='
            // padding chars to output
            if (limitLines && !(charsOnLine % 76)) {
                out.data()[idxOut++] = '\r';
                out.data()[idxOut++] = '\n';
            }

            // Encode first 6 bits
            out.data()[idxOut++] =
                    ::encodingTable[(data[idxIn] >> 2) & 077];
            // Encode next 6 bits (2 significant + 4 0-bits!)
            out.data()[idxOut++] =
                    ::encodingTable[(data[idxIn] << 4) & 077];
            // Padding
            out.data()[idxOut++] = '=';
            out.data()[idxOut] = '=';
            break;

        case 2:
            // Need to encode two last bytes
            // Add 2 0-bits to the right, encode three chars and append one '='
            // padding char to output
            if (limitLines && !(charsOnLine % 76)) {
                out.data()[idxOut++] = '\r';
                out.data()[idxOut++] = '\n';
            }

            // Encode first 6 bits
            out.data()[idxOut++] =
                    ::encodingTable[(data[idxIn] >> 2) & 077];
            // Encode next 6 bits
            out.data()[idxOut++] =
                    ::encodingTable[((data[idxIn + 1] >> 4) & 017) |
                                    ((data[idxIn] << 4) & 077)];
            // Last 6 bits (4 significant + 2 0-bits)
            out.data()[idxOut++] =
                    ::encodingTable[(data[idxIn + 1] << 2) & 077];
            // Padding
            out.data()[idxOut] = '=';
    }

    return QString(out);
}

/*!
    Encodes a bit array. Adds a CRLF pair at column 76 if the BreakLongLines
    option is set.
*/
QString MvdBase64::encode(const QBitArray &decoded, EncodingOptions options)
{
    quint32 sz = decoded.size();
    quint32 rest = sz % 8;
    quint32 bsz = rest ? sz / 8 + 1 : sz / 8;
    quint32 currByte = 0;

    QByteArray ba(bsz, '\0');

    for (quint32 i = 0; i < sz; i += 8) {
        ba.data()[currByte] = 0;

        for (quint32 j = 0; j < 8; ++j) {
            if (i + j == sz)
                break;

            if (decoded.at(i + j))
                ba.data()[currByte] |= 1 << (7 - j);
        }

        currByte++;
    }

    return encode(ba, options);
}
