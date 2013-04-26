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

#ifndef BUTTONBOX_H
#define BUTTONBOX_H

#include "common.h"
#include <qwidget.h>
#include <qbuttongroup.h>
#include <qgroupbox.h>
#include <qboxlayout.h>

// =============================================================================
// ButtonBox<R>
//
// Convenience widget - is a groupbox of buttons. Mainly useful for quick creation
// of radio button groups.
// =============================================================================
template<class R> class ButtonBox : public QGroupBox {
private:
	std::vector<R*> objects;
	std::vector<QBoxLayout*> layouts;
	QBoxLayout* coreLayout;
	QBoxLayout* currentLayout;
	QBoxLayout::Direction dir;
	int currentId;
	int defaultId;
	
public:
	QButtonGroup* buttonGroup;
	
	QBoxLayout::Direction makeDirection (Qt::Orientation orient, bool invert = false) {
		return (orient == (invert ? Qt::Vertical : Qt::Horizontal)) ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom;
	}
	
	void init (Qt::Orientation orient) {
		dir = makeDirection (orient);
		
		buttonGroup = new QButtonGroup;
		currentId = 0;
		coreLayout = null;
		
		// Ensure we have buttons and not lists or timers or cows or
		// anything like that.
		R* test = new R;
		assert (test->inherits ("QAbstractButton"));
		delete test;
		
		coreLayout = new QBoxLayout (makeDirection (orient, true));
		setLayout (coreLayout);
		
		// Init the first row with a break
		rowBreak ();
	}
	
	explicit ButtonBox (QWidget* parent = null) : QGroupBox (parent) {
		init (Qt::Vertical);
	}
	
	explicit ButtonBox (const QString& title, QWidget* parent = null) : QGroupBox (title, parent) {
		init (Qt::Vertical);
	}
	
	explicit ButtonBox (const QGroupBox& box) : QGroupBox (box) {
		init (Qt::Vertical);
	}
	
	explicit ButtonBox (const QString& title, initlist<char const*> entries, int const defaultId,
		const Qt::Orientation orient = Qt::Vertical, QWidget* parent = null) :
		QGroupBox (title, parent), defaultId (defaultId)
	{
		init (orient);
		
		for (char const* entry : entries) {
			addButton (entry);
		}
	}
	
	void rowBreak () {
		QBoxLayout* newLayout = new QBoxLayout (dir);
		currentLayout = newLayout;
		layouts.push_back (newLayout);
		
		coreLayout->addLayout (newLayout);
	}
	
	void setCurrentRow (uint row) {
		currentLayout = layouts[row];
	}
	
	void addButton (const char* entry) {
		R* button = new R (entry);
		addButton (button);
	}
	
	void addButton (R* button) {
		bool const selectThis = (currentId == defaultId);
		
		objects.push_back (button);
		buttonGroup->addButton (button, currentId++);
		currentLayout->addWidget (button);
		
		if (selectThis)
			button->setChecked (true);
	}
	
	ButtonBox<R>& operator<< (R* button) {
		addButton (button);
		return *this;
	}
	
	ButtonBox<R>& operator<< (const char* entry) {
		addButton (entry);
		return *this;
	}
	
	int value () {
		return buttonGroup->checkedId ();
	}
	
	void setValue (int val) {
		static_cast<R*> (buttonGroup->button (val))->setChecked (true);
	}
	
	R* const& begin () {
		return objects.begin ();
	}
	
	R* const& end () {
		return objects.end ();
	}
	
	R* operator[] (uint n) const {
		return objects[n];
	}
	
	bool exclusive () const {
		return buttonGroup->exclusive ();
	}
	
	void setExclusive (bool val) {
		buttonGroup->setExclusive (val);
	}
	
	bool isChecked (uint n) {
		return objects[n]->checked ();
	}
};

#endif // BUTTONBOX_H