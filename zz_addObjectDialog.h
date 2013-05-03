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

#ifndef ZZ_ADDOBJECTDIALOG_H
#define ZZ_ADDOBJECTDIALOG_H

#include "gui.h"
#include "buttonbox.h"
#include <qdialog.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qlistwidget.h>
#include <qtreewidget.h>

class AddObjectDialog : public QDialog {
	Q_OBJECT
	
public:
    AddObjectDialog (const LDObjectType_e type, LDObject* obj, QWidget* parent = null);
	static void staticDialog (const LDObjectType_e type, LDObject* obj);
	
	QLabel* lb_typeIcon;
	
	// -- COMMENT --
	QLineEdit* le_comment;
	
	// Coordinate edits for.. anything with coordinates, really.
	QDoubleSpinBox* dsb_coords[12];
	
	// Color selection dialog button
	QPushButton* pb_color;
	
	// BFC-related widgets
	ButtonBox<QRadioButton>* bb_bfcType;
	
	// Subfile stuff
	QTreeWidget* tw_subfileList;
	QLineEdit* le_subfileName;
	
	// Radial stuff
	QCheckBox* cb_radHiRes;
	ButtonBox<QRadioButton>* bb_radType;
	QSpinBox* sb_radSegments, *sb_radRingNum;
	QLabel* lb_radType, *lb_radResolution, *lb_radSegments,
		*lb_radRingNum;
	
	QDialogButtonBox* bbx_buttons;
	
private:
	void setButtonBackground (QPushButton* button, short color);
	char* currentSubfileName ();
	
	short dColor;
	
private slots:
	void slot_colorButtonClicked ();
	void slot_radialTypeChanged (int type);
	void slot_subfileTypeChanged ();
};

#endif // ZZ_ADDOBJECTDIALOG_H