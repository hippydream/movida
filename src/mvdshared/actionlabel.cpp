/**************************************************************************
** Filename: actionlabel.cpp
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

#include "actionlabel.h"
#include "actionlabel_p.h"
#include "core.h"

/*! 
	\class MvdActionLabel actionlabel.h
	\ingroup MovidaShared

	\brief QLabel subclass to be used for application internal commands.
	
	Usage example:
	
	\verbatim
	label = new MvdActionLabel(this);
	resetId = label->addControl(tr("Reset page"));
	connect(label, SIGNAL(controlTriggered(int)), this, SLOT(controlTriggered(int)));
	
	// ...

	void MyClass::controlTriggered(int id) {
		if (id == resetId) {
			resetPage();
			label->setControlEnabled(resetId, false);
		}
	}
	\endverbatim
*/

//! \internal
 MvdActionLabel_P::MvdActionLabel_P(MvdActionLabel* label)
 : QObject(label), ids(0), q(label)
 {
 }

//! \internal
void MvdActionLabel_P::linkActivated(const QString& url)
{
	MvdCore::LabelAction a = MvdCore::parseLabelAction(url);
	if (!a.isValid())
		return;

	QRegExp rx("^advanced-control\\?id=([0-9])*$");
	if (rx.indexIn(a.action) >= 0) {
		int id = rx.cap(1).toInt();
		Q_ASSERT_X(controls.contains(AdvancedControl(id)), "MvdActionLabel_P::linkActivated()", "Internal error.");
		q->emit_controlTriggered(id);
	}
}

//! \internal
void MvdActionLabel_P::layoutAdvancedControls()
{
	QString disabledColor = q->palette().color(QPalette::Disabled, QPalette::Text).name();
	QString txt;

	for (int i = 0; i < controls.size(); ++i) {
		const AdvancedControl& ac = controls.at(i);

		QString c = ac.label;
		if (ac.enabled) {
			c.prepend(QString("<a href='movida://advanced-control?id=%1'>").arg(ac.id));
			c.append("</a>");
		} else {
			c.prepend(QString("<span style='color:%1;'>").arg(disabledColor));
			c.append("</span>");
		}

		txt.append(c);
		if (i != controls.size() - 1)
			txt.append(" - ");
	}

	q->setText(txt);
}

/*! Creates a new enabled label with no controls.
	Use addControl() to add controls. Do no use QLabel::setText() as it will be 
	overwritten by the controls.
*/
MvdActionLabel::MvdActionLabel(QWidget* parent)
: QLabel(parent), d(new MvdActionLabel_P(this))
{
	setContextMenuPolicy(Qt::NoContextMenu);
	connect(this, SIGNAL(linkActivated(QString)), d, SLOT(linkActivated(QString)));
}

//!
MvdActionLabel::~MvdActionLabel()
{
}

//! Adds a new control to the label and returns its identifier.
int MvdActionLabel::addControl(const QString& text, bool enabled)
{
	MvdActionLabel_P::AdvancedControl ac(d->ids++, text.trimmed(), enabled);
	d->controls.append(ac);
	d->layoutAdvancedControls();
	return ac.id;
}

//! Enables or disables a control.
void MvdActionLabel::setControlEnabled(int control, bool enabled)
{
	int i = d->controls.indexOf(MvdActionLabel_P::AdvancedControl(control));
	if (i < 0) {
		qDebug("MvdActionLabel::setControlEnabled: invalid control id %d.", control);
		return;
	}

	MvdActionLabel_P::AdvancedControl& ac = d->controls[i];
	if (ac.enabled == enabled)
		return;

	ac.enabled = enabled;
	d->layoutAdvancedControls();
}

//! Returns true if the given control is enabled.
bool MvdActionLabel::controlEnabled(int control)
{
	int i = d->controls.indexOf(MvdActionLabel_P::AdvancedControl(control));
	if (i < 0)
		return false;

	return d->controls[i].enabled;
}
