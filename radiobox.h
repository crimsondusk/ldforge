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

#ifndef RADIOBOX_H
#define RADIOBOX_H

#include "common.h"
#include <qwidget.h>
#include <QGroupBox>
#include <qradiobutton.h>
#include <qboxlayout.h>
#include <qbuttongroup.h>

// =============================================================================
// RadioBox
//
// Convenience widget - is a groupbox of radio buttons.
// =============================================================================
class RadioBox : public QGroupBox {
	Q_OBJECT
	
public:
	void init (Qt::Orientation orient);
	
	explicit RadioBox (QWidget* parent = null) : QGroupBox (parent) {
		init (Qt::Vertical);
	}
	
	explicit RadioBox (const QString& title, QWidget* parent = null) : QGroupBox (title, parent) {
		init (Qt::Vertical);
	}
	
	explicit RadioBox () {
		init (Qt::Vertical);
	}
	
	explicit RadioBox (const QString& title, initlist<char const*> entries, int const defaultId,
		const Qt::Orientation orient = Qt::Vertical, QWidget* parent = null);
	
	void rowBreak ();
	void setCurrentRow (uint row);
	void addButton (const char* entry);
	void addButton (QRadioButton* button);
	RadioBox& operator<< (QRadioButton* button);
	RadioBox& operator<< (const char* entry);
	
	int value () const {
		return buttonGroup->checkedId ();
	}
	
	void setValue (int val) {
		buttonGroup->button (val)->setChecked (true);
	}
	
	std::vector<QRadioButton*>::iterator begin () {
		return objects.begin ();
	}
	
	std::vector<QRadioButton*>::iterator end () {
		return objects.end ();
	}
	
	QRadioButton* operator[] (uint n) const {
		return objects[n];
	}
	
	bool exclusive () const {
		return buttonGroup->exclusive ();
	}
	
	void setExclusive (bool val) {
		buttonGroup->setExclusive (val);
	}
	
	bool isChecked (int n) const {
		return buttonGroup->checkedId () == n;
	}

signals:
	void sig_buttonPressed (int btn);
	void sig_buttonPressed (QAbstractButton* btn);

private:
	std::vector<QRadioButton*> objects;
	std::vector<QBoxLayout*> layouts;
	QBoxLayout* coreLayout;
	QBoxLayout* currentLayout;
	QBoxLayout::Direction dir;
	int currentId;
	int defaultId;
	QButtonGroup* buttonGroup;
	
	Q_DISABLE_COPY (RadioBox)

private slots:
	void slot_buttonPressed (int btn);
	void slot_buttonPressed (QAbstractButton* btn);
};

#endif // RADIOBOX_H