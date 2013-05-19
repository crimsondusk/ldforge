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

#ifndef LABELEDWIDGET_H
#define LABELEDWIDGET_H

#include "common.h"
#include <QLabel>
#include <QBoxLayout>

// =============================================================================
// LabeledWidget
//
// Convenience class for a widget with a label beside it.
// =============================================================================
template<class R> class LabeledWidget : public QWidget {
	PROPERTY (QLabel*, label, setLabel)
	PROPERTY (R*, widget, setWidget)
	
public:
	explicit LabeledWidget (const char* labelstr, QWidget* parent = null) : QWidget (parent) {
		m_widget = new R (this);
		commonInit (labelstr);
	}
	
	explicit LabeledWidget (const char* labelstr, R* widget, QWidget* parent = null) :
		QWidget (parent), m_widget (widget)
	{
		commonInit (labelstr);
	}
	
	explicit LabeledWidget (QWidget* parent = 0, Qt::WindowFlags f = 0) {
		m_widget = new R (this);
		commonInit ("");
	}
	
	R* w () const { return m_widget; }
	operator R* () { return m_widget; }
	
private:
	Q_DISABLE_COPY (LabeledWidget<R>)
	
	void commonInit (const char* labelstr) {
		m_label = new QLabel (labelstr, this);
		m_layout = new QHBoxLayout;
		m_layout->addWidget (m_label);
		m_layout->addWidget (m_widget);
		setLayout (m_layout);
	}
	
	QHBoxLayout* m_layout;
};

#endif // LABELEDWIDGET_H