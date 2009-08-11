/**************************************************************************
** Filename: richtexteditor_p.h
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

#ifndef MVD_RICHTEXTEDITOR_P_H
#define MVD_RICHTEXTEDITOR_P_H

#include "richtexteditor.h"

#include <QtCore/QObject>
#include <QtGui/QComboBox>
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtGui/QTextEdit>

//! \internal
class MvdRichTextEditor::Private : public QObject
{
    Q_OBJECT

public :
    Private(MvdRichTextEditor::ControlsPosition pos, MvdRichTextEditor *parent) :
        QObject(parent),
        position(pos),
        q(parent)
{ }

    struct ColorWrapper {
        ColorWrapper() { }

        ColorWrapper(const QString &s) :
            name(s),
            color(QColor(s)) { }

        inline bool operator<(const ColorWrapper &o) const
        {
#if defined (MVD_USE_COLOR_SORT)
            if (color.red() != o.color.red())
                return color.red() < o.color.red();
            if (color.green() != o.color.green())
                return color.green() < o.color.green();
            return color.blue() < o.color.blue();
#else
            return name < o.name;
#endif
        }

        QString name;
        QColor color;
    };

    QTextEdit *editor;
    QPushButton *bold;
    QPushButton *italic;
    QPushButton *underline;
    QComboBox *fontSize;
    QComboBox *fontColor;
    MvdRichTextEditor::ControlsPosition position;

    QIcon colorIcon(const QColor &color)
    {
        QPixmap pm(12, 12);
        QPainter painter(&pm);

        painter.setPen(Qt::black);
        painter.setBrush(color);
        painter.drawRect(0, 0, pm.width() - 1, pm.height() - 1);
        painter.end();
        return QIcon(pm);
    }

public slots:
    void setFontBold(bool enable)
    {
        if (!editor)
            return;

        if (enable)
            editor->setFontWeight(QFont::Bold);
        else editor->setFontWeight(QFont::Normal);
        editor->setFocus(Qt::ShortcutFocusReason);
    }

    void setFontItalic(bool enable)
    {
        if (!editor)
            return;

        editor->setFontItalic(enable);
        editor->setFocus(Qt::ShortcutFocusReason);
    }

    void setFontUnderline(bool enable)
    {
        if (!editor)
            return;

        editor->setFontUnderline(enable);
        editor->setFocus(Qt::ShortcutFocusReason);
    }

    void fontSizeChanged(const QString &s)
    {
        if (!editor)
            return;

        editor->setFontPointSize(s.toInt());
        editor->setFocus(Qt::ShortcutFocusReason);
    }

    void fontColorChanged(const QString &s)
    {
        if (!editor)
            return;

        QColor color(s);
        if (!color.isValid())
            return;

        editor->setTextColor(color);
        editor->setFocus(Qt::ShortcutFocusReason);
    }

    void refreshControls()
    {
        if (!editor)
            return;

        QTextCursor cursor = editor->textCursor();
        QTextCharFormat charFormat = cursor.charFormat();

        bold->setChecked(charFormat.fontWeight() == QFont::Bold);
        italic->setChecked(charFormat.fontItalic());
        underline->setChecked(charFormat.fontUnderline());

        int size = (int)charFormat.fontPointSize();
        if (size == 0) // workaround for a bug in QTextEdit
            size = (int)editor->document()->defaultFont().pointSize();
        int idx = fontSize->findText(QString::number(size));
        if (idx != -1)
            fontSize->setCurrentIndex(idx);
        QColor color = editor->textColor();
        idx = fontColor->findData(color);
        if (idx < 0) {
            fontColor->addItem(colorIcon(color), color.name().toUpper(), color);
            idx = fontColor->count() - 1;
        }
        fontColor->setCurrentIndex(idx);
    }

private:
    MvdRichTextEditor* q;
};

#endif // MVD_RICHTEXTEDITOR_P_H
