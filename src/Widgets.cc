/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// I still find the radio group useful... find a way to use this in Designer.
// I probably need to look into how to make Designer plugins.
// TODO: try make this usable in Designer

#include <QBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <map>

#include "Widgets.h"
#include "moc_Widgets.cpp"

// =============================================================================
// -----------------------------------------------------------------------------
RadioGroup::RadioGroup (const QString& title, QWidget* parent) : QGroupBox (title, parent)
{
	init (Qt::Vertical);
}

// =============================================================================
// -----------------------------------------------------------------------------
QBoxLayout::Direction makeDirection (Qt::Orientation orient, bool invert = false)
{
	return (orient == (invert ? Qt::Vertical : Qt::Horizontal)) ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom;
}

// =============================================================================
// -----------------------------------------------------------------------------
bool RadioGroup::isChecked (int n) const
{
	return m_buttonGroup->checkedId() == n;
}

// =============================================================================
// -----------------------------------------------------------------------------
void RadioGroup::init (Qt::Orientation orient)
{
	m_vert = orient == Qt::Vertical;

	m_buttonGroup = new QButtonGroup;
	m_oldId = m_curId = 0;
	m_coreLayout = null;

	m_coreLayout = new QBoxLayout ( (orient == Qt::Vertical) ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
	setLayout (m_coreLayout);

	// Init the first row with a break
	rowBreak();

	connect (m_buttonGroup, SIGNAL (buttonPressed (int)), this, SLOT (slot_buttonPressed (int)));
	connect (m_buttonGroup, SIGNAL (buttonReleased (int)), this, SLOT (slot_buttonReleased (int)));
}

// =============================================================================
// -----------------------------------------------------------------------------
RadioGroup::RadioGroup (const QString& title, initlist<char const*> entries, int const defaultId, const Qt::Orientation orient, QWidget* parent) :
		QGroupBox (title, parent),
		m_defId (defaultId)
{
	init (orient);
	m_oldId = m_defId;

	for (const char* entry : entries)
		addButton (entry);
}

// =============================================================================
// -----------------------------------------------------------------------------
void RadioGroup::rowBreak()
{
	QBoxLayout* newLayout = new QBoxLayout (m_vert ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
	m_currentLayout = newLayout;
	m_layouts << newLayout;

	m_coreLayout->addLayout (newLayout);
}

// =============================================================================
// -----------------------------------------------------------------------------
void RadioGroup::addButton (const char* entry)
{
	QRadioButton* button = new QRadioButton (entry);
	addButton (button);
}

// =============================================================================
// -----------------------------------------------------------------------------
void RadioGroup::addButton (QRadioButton* button)
{
	bool const selectThis = (m_curId == m_defId);

	m_objects << button;
	m_buttonGroup->addButton (button, m_curId++);
	m_currentLayout->addWidget (button);

	if (selectThis)
		button->setChecked (true);
}

// =============================================================================
// -----------------------------------------------------------------------------
RadioGroup& RadioGroup::operator<< (QRadioButton* button)
{
	addButton (button);
	return *this;
}

// =============================================================================
// -----------------------------------------------------------------------------
RadioGroup& RadioGroup::operator<< (const char* entry)
{
	addButton (entry);
	return *this;
}

// =============================================================================
// -----------------------------------------------------------------------------
void RadioGroup::setCurrentRow (int row)
{
	m_currentLayout = m_layouts[row];
}

// =============================================================================
// -----------------------------------------------------------------------------
int RadioGroup::value() const
{
	return m_buttonGroup->checkedId();
}

// =============================================================================
// -----------------------------------------------------------------------------
void RadioGroup::setValue (int val)
{
	m_buttonGroup->button (val)->setChecked (true);
}

// =============================================================================
// -----------------------------------------------------------------------------
QRadioButton* RadioGroup::operator[] (int n) const
{
	return m_objects[n];
}

// =============================================================================
// -----------------------------------------------------------------------------
void RadioGroup::slot_buttonPressed (int btn)
{
	emit buttonPressed (btn);

	m_oldId = m_buttonGroup->checkedId();
}

// =============================================================================
// -----------------------------------------------------------------------------
void RadioGroup::slot_buttonReleased (int btn)
{
	emit buttonReleased (btn);
	int newid = m_buttonGroup->checkedId();

	if (m_oldId != newid)
		emit valueChanged (newid);
}

// =============================================================================
// -----------------------------------------------------------------------------
RadioGroup::Iterator RadioGroup::begin()
{
	return m_objects.begin();
}

// =============================================================================
// -----------------------------------------------------------------------------
RadioGroup::Iterator RadioGroup::end()
{
	return m_objects.end();
}
