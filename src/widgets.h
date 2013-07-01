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
	typedef vector<QRadioButton*>::it it;
	
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
	vector<QRadioButton*> m_objects;
	vector<QBoxLayout*> m_layouts;
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

// =============================================================================
// CheckBoxGroup
// =============================================================================
class CheckBoxGroup : public QGroupBox {
	Q_OBJECT
	
public:
	CheckBoxGroup (const char* label, Qt::Orientation orient = Qt::Horizontal, QWidget* parent = null);
	
	void			addCheckBox		(const char* label, int key, bool checked = false);
	vector<int>	checkedValues		() const;
	QCheckBox*		getCheckBox		(int key);
	bool			buttonChecked		(int key);
	
signals:
	void selectionChanged	();
	
private:
	QBoxLayout* m_layout;
	std::map<int, QCheckBox*> m_vals;
	
private slots:
	void buttonChanged		();
};

// =============================================================================
// SpinBox
// =============================================================================
class SpinBox : public QSpinBox
{
	Q_OBJECT
public:
	explicit SpinBox( QWidget* parent = 0 );
	SpinBox();
	
public slots:
	void enable();
	void disable();
	
private:
	Q_DISABLE_COPY( SpinBox )
};

#endif // WIDGETS_H