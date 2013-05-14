/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
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

#include <QBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <map>

#include "widgets.h"

RadioBox::RadioBox (const QString& title, QWidget* parent) : QGroupBox (title, parent) {
	init (Qt::Vertical);
}

QBoxLayout::Direction makeDirection (Qt::Orientation orient, bool invert = false) {
	return (orient == (invert ? Qt::Vertical : Qt::Horizontal)) ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom;
}

bool RadioBox::isChecked (int n) const {
	return m_buttonGroup->checkedId () == n;
}

void RadioBox::init (Qt::Orientation orient) {
	m_vert = orient == Qt::Vertical;
	
	m_buttonGroup = new QButtonGroup;
	m_oldId = m_curId = 0;
	m_coreLayout = null;
	
	m_coreLayout = new QBoxLayout ((orient == Qt::Vertical) ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
	setLayout (m_coreLayout);
	
	// Init the first row with a break
	rowBreak ();
	
	connect (m_buttonGroup, SIGNAL (buttonPressed (int)), this, SLOT (slot_buttonPressed (int)));
	connect (m_buttonGroup, SIGNAL (buttonReleased (int)), this, SLOT (slot_buttonReleased (int)));
}

RadioBox::RadioBox (const QString& title, initlist<char const*> entries, int const defaultId,
	const Qt::Orientation orient, QWidget* parent) : QGroupBox (title, parent), m_defId (defaultId)
{
	init (orient);
	m_oldId = m_defId;
	
	for (char const* entry : entries)
		addButton (entry);
}

void RadioBox::rowBreak () {
	QBoxLayout* newLayout = new QBoxLayout (m_vert ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
	m_currentLayout = newLayout;
	m_layouts.push_back (newLayout);
	
	m_coreLayout->addLayout (newLayout);
}

void RadioBox::addButton (const char* entry) {
	QRadioButton* button = new QRadioButton (entry);
	addButton (button);
}

void RadioBox::addButton (QRadioButton* button) {
	bool const selectThis = (m_curId == m_defId);
	
	m_objects.push_back (button);
	m_buttonGroup->addButton (button, m_curId++);
	m_currentLayout->addWidget (button);
	
	if (selectThis)
		button->setChecked (true);
}

RadioBox& RadioBox::operator<< (QRadioButton* button) {
	addButton (button);
	return *this;
}

RadioBox& RadioBox::operator<< (const char* entry) {
	addButton (entry);
	return *this;
}

void RadioBox::setCurrentRow (uint row) {
	m_currentLayout = m_layouts[row];
}

int RadioBox::value () const {
	return m_buttonGroup->checkedId ();
}

void RadioBox::setValue (int val) {
	m_buttonGroup->button (val)->setChecked (true);
}

QRadioButton* RadioBox::operator[] (uint n) const {
	return m_objects[n];
}

void RadioBox::slot_buttonPressed (int btn) {
	emit buttonPressed (btn);
	
	m_oldId = m_buttonGroup->checkedId ();
}

void RadioBox::slot_buttonReleased (int btn) {
	emit buttonReleased (btn);
	int newid = m_buttonGroup->checkedId ();
	
	if (m_oldId != newid)
		emit valueChanged (newid);
}

RadioBox::iter RadioBox::begin() {
	 return m_objects.begin ();
}

RadioBox::iter RadioBox::end() {
	return m_objects.end ();
}

CheckBoxGroup::CheckBoxGroup (const char* label, Qt::Orientation orient, QWidget* parent) : QGroupBox (parent) {
	m_layout = new QBoxLayout (makeDirection (orient));
	setTitle (label);
	setLayout (m_layout);
}

void CheckBoxGroup::addCheckBox (const char* label, int key, bool checked) {
	if (m_vals.find (key) != m_vals.end ())
		return;
	
	QCheckBox* box = new QCheckBox (label);
	box->setChecked (checked);
	
	m_vals[key] = box;
	m_layout->addWidget (box);
	
	connect (box, SIGNAL (stateChanged (int)), this, SLOT (buttonChanged ()));
}

std::vector<int> CheckBoxGroup::checkedValues () const {
	std::vector<int> vals;
	
	for (const auto& kv : m_vals)
		if (kv.second->isChecked ())
			vals.push_back (kv.first);
	
	return vals;
}

QCheckBox* CheckBoxGroup::getCheckBox (int key) {
	return m_vals[key];
}

void CheckBoxGroup::buttonChanged () {
	emit selectionChanged ();
}

bool CheckBoxGroup::buttonChecked (int key) {
	return m_vals[key]->isChecked ();
}