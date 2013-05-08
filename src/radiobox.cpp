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

#include <qboxlayout.h>
#include <qradiobutton.h>
#include "radiobox.h"

static QBoxLayout::Direction makeDirection (Qt::Orientation orient, bool invert = false) {
	return (orient == (invert ? Qt::Vertical : Qt::Horizontal)) ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom;
}

void RadioBox::init (Qt::Orientation orient) {
	dir = makeDirection (orient);
	
	buttonGroup = new QButtonGroup;
	currentId = 0;
	coreLayout = null;
	
	coreLayout = new QBoxLayout (makeDirection (orient, true));
	setLayout (coreLayout);
	
	// Init the first row with a break
	rowBreak ();
	
	connect (buttonGroup, SIGNAL (buttonPressed (QAbstractButton*)), this, SLOT (slot_buttonPressed (QAbstractButton*)));
	connect (buttonGroup, SIGNAL (buttonPressed (int)), this, SLOT (slot_buttonPressed (int)));
}

RadioBox::RadioBox (const QString& title, initlist<char const*> entries, int const defaultId,
	const Qt::Orientation orient, QWidget* parent) : QGroupBox (title, parent), defaultId (defaultId)
{
	init (orient);
	
	for (char const* entry : entries)
		addButton (entry);
}

void RadioBox::rowBreak () {
	QBoxLayout* newLayout = new QBoxLayout (dir);
	currentLayout = newLayout;
	layouts.push_back (newLayout);
	
	coreLayout->addLayout (newLayout);
}

void RadioBox::addButton (const char* entry) {
	QRadioButton* button = new QRadioButton (entry);
	addButton (button);
}

void RadioBox::addButton (QRadioButton* button) {
	bool const selectThis = (currentId == defaultId);
	
	objects.push_back (button);
	buttonGroup->addButton (button, currentId++);
	currentLayout->addWidget (button);
	
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
	currentLayout = layouts[row];
}

void RadioBox::slot_buttonPressed (int btn) {
	emit sig_buttonPressed (btn);
}

void RadioBox::slot_buttonPressed (QAbstractButton* btn) {
	emit sig_buttonPressed (btn);
}