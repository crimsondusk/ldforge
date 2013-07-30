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

#ifndef WIDGETS_H
#define WIDGETS_H

#include <QGroupBox>
#include <QSpinBox>
#include <map>
#include "common.h"
#include "types.h"

class QCheckBox;
class QButtonGroup;
class QBoxLayout;
class QRadioButton;

// =============================================================================
// RadioBox
//
// Convenience widget - is a groupbox of radio buttons.
// =============================================================================
class RadioBox : public QGroupBox {
	Q_OBJECT
	
public:
	typedef List<QRadioButton*>::it it;
	
	explicit RadioBox (QWidget* parent = null) : QGroupBox (parent) { init (Qt::Vertical); }
	explicit RadioBox () { init (Qt::Vertical); }
	explicit RadioBox (const QString& title, QWidget* parent = null);
	explicit RadioBox (const QString& title, initlist<char const*> entries, int const defaultId,
		const Qt::Orientation orient = Qt::Vertical, QWidget* parent = null);
	
	void			addButton		(const char* entry);
	void			addButton		(QRadioButton* button);
	it				begin			();
	it				end				();
	void			init			(Qt::Orientation orient);
	bool			isChecked		(int n) const;
	void			rowBreak		();
	void			setCurrentRow	(uint row);
	void			setValue		(int val);
	int				value			() const;
	
	QRadioButton*	operator[]		(uint n) const;
	RadioBox&		operator<<		(QRadioButton* button);
	RadioBox&		operator<<		(const char* entry);

signals:
	void buttonPressed (int btn);
	void buttonReleased (int btn);
	void valueChanged (int val);

private:
	List<QRadioButton*> m_objects;
	List<QBoxLayout*> m_layouts;
	QBoxLayout* m_coreLayout;
	QBoxLayout* m_currentLayout;
	bool m_vert;
	int m_curId, m_defId, m_oldId;
	QButtonGroup* m_buttonGroup;
	
	Q_DISABLE_COPY (RadioBox)

private slots:
	void slot_buttonPressed (int btn);
	void slot_buttonReleased (int btn);
};

#endif // WIDGETS_H