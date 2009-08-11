/**************************************************************************
** Filename: richtexteditor.cpp
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

#include "richtexteditor.h"
#include "richtexteditor_p.h"

#include <QtGui/QFontDatabase>
#include <QtGui/QHBoxLayout>
#include <QtGui/QSizePolicy>
#include <QtGui/QVBoxLayout>

/*!
    \class MvdRichTextEditor richtexteditor.h
    \ingroup MovidaShared

    \brief A trivial widget with a QTextEdit and a bunch of rich text handling controls.
*/

MvdRichTextEditor::MvdRichTextEditor(QWidget *parent) :
    QWidget(parent),
    d(new Private(Bottom, this))
{
    init();
}

MvdRichTextEditor::MvdRichTextEditor(ControlsPosition pos, QWidget *parent) :
    QWidget(parent),
    d(new Private(pos, this))
{
    init();
}

MvdRichTextEditor::~MvdRichTextEditor()
{
    delete d;
}

MvdRichTextEditor::ControlsPosition MvdRichTextEditor::position() const
{
    return d->position;
}

QTextEdit *MvdRichTextEditor::editor() const
{
    return d->editor;
}

void MvdRichTextEditor::init()
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    vl->setMargin(0);

    QHBoxLayout *hl = new QHBoxLayout;
    hl->setMargin(0);

    vl->addLayout(hl);

    d->editor = new QTextEdit;
    vl->addWidget(d->editor);

    d->bold = new QPushButton(this);
    d->bold->setFlat(true);
    d->bold->setToolTip(tr("Bold (Ctrl+B)"));
    d->bold->setCheckable(true);
    d->bold->setIcon(QIcon(":/images/bold.svgz"));
    d->bold->setShortcut(tr("CTRL+B"));
    connect(d->bold, SIGNAL(toggled(bool)), d, SLOT(setFontBold(bool)));
    hl->addWidget(d->bold);

    d->italic = new QPushButton(this);
    d->italic->setFlat(true);
    d->italic->setToolTip(tr("Italic (Ctrl+I)"));
    d->italic->setCheckable(true);
    d->italic->setIcon(QIcon(":/images/italic.svgz"));
    d->italic->setShortcut(tr("CTRL+I"));
    connect(d->italic, SIGNAL(toggled(bool)), d, SLOT(setFontItalic(bool)));
    hl->addWidget(d->italic);

    d->underline = new QPushButton(this);
    d->underline->setFlat(true);
    d->underline->setToolTip(tr("Underline (Ctrl+U)"));
    d->underline->setCheckable(true);
    d->underline->setIcon(QIcon(":/images/underline.svgz"));
    d->underline->setShortcut(tr("CTRL+U"));
    connect(d->underline, SIGNAL(toggled(bool)), d, SLOT(setFontUnderline(bool)));
    hl->addWidget(d->underline);

    d->fontSize = new QComboBox;
    d->fontSize->setToolTip(tr("Font size"));
    d->fontSize->setEditable(false);
    QList<int> fontSizes = QFontDatabase::standardSizes();
    for (int i = 0; i < fontSizes.size(); ++i)
        d->fontSize->addItem(QString::number(fontSizes.at(i)));
    connect(d->fontSize, SIGNAL(activated(QString)), d, SLOT(fontSizeChanged(QString)));
    hl->addWidget(d->fontSize);

    d->fontColor = new QComboBox;
    d->fontColor->setToolTip(tr("Font color"));
    d->fontColor->setEditable(false);
    QStringList colorNames = QColor::colorNames();
    colorNames.removeAll(QLatin1String("transparent"));
    QVector<Private::ColorWrapper> colorWrappers(colorNames.size());
    for (int i = 0; i < colorNames.size(); ++i)
        colorWrappers[i] = Private::ColorWrapper(colorNames.at(i));
    qSort(colorWrappers);
    for (int i = 0; i < colorWrappers.size(); ++i) {
        const Private::ColorWrapper &cw = colorWrappers.at(i);
        const QString &colorName = cw.name;
        d->fontColor->addItem(d->colorIcon(cw.color), colorName, cw.color);
    }
    connect(d->fontColor, SIGNAL(activated(QString)), d, SLOT(fontColorChanged(QString)));
    hl->addWidget(d->fontColor);

    hl->addStretch();

    d->refreshControls();
    connect(d->editor, SIGNAL(textChanged()), d, SLOT(refreshControls()));
    connect(d->editor, SIGNAL(selectionChanged()), d, SLOT(refreshControls()));

    // Ensure the two combos have the same height or it looks awful!
    int h = qMax(d->fontSize->sizeHint().height(), d->fontColor->sizeHint().height());
    d->fontSize->setFixedHeight(h);
    d->fontColor->setFixedHeight(h);
}
