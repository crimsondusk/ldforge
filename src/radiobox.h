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
		return m_buttonGroup->checkedId ();
	}
	
	void setValue (int val) {
		m_buttonGroup->button (val)->setChecked (true);
	}
	
	std::vector<QRadioButton*>::iterator begin () {
		return m_objects.begin ();
	}
	
	std::vector<QRadioButton*>::iterator end () {
		return m_objects.end ();
	}
	
	QRadioButton* operator[] (uint n) const {
		return m_objects[n];
	}
	
	bool exclusive () const {
		return m_buttonGroup->exclusive ();
	}
	
	void setExclusive (bool val) {
		m_buttonGroup->setExclusive (val);
	}
	
	bool isChecked (int n) const {
		return m_buttonGroup->checkedId () == n;
	}

signals:
	void sig_buttonPressed (int btn);
	void buttonReleased (int btn);
	void valueChanged (int val);

private:
	std::vector<QRadioButton*> m_objects;
	std::vector<QBoxLayout*> m_layouts;
	QBoxLayout* m_coreLayout;
	QBoxLayout* m_currentLayout;
	QBoxLayout::Direction m_dir;
	int m_curId, m_defId, m_oldId;
	QButtonGroup* m_buttonGroup;
	
	Q_DISABLE_COPY (RadioBox)

private slots:
	void slot_buttonPressed (int btn);
	void slot_buttonReleased (int btn);
};

#endif // RADIOBOX_H