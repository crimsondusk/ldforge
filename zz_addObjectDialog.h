/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri `arezey` Piippo
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

#ifndef __ZZ_ADDOBJECTDIALOG_H__
#define __ZZ_ADDOBJECTDIALOG_H__

#include "gui.h"
#include <qdialog.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qlabel.h>
#include <qspinbox.h>

class AddObjectDialog : public QDialog {
public:
    AddObjectDialog (const LDObjectType_e type, QWidget* parent = nullptr);
	static void staticDialog (const LDObjectType_e type, ForgeWindow* window);
	
	QLabel* qTypeIcon;
	
	// -- COMMENT --
	QLineEdit* qCommentLine;
	
	// Coordinate edits for.. anything with coordinates, really.
	QDoubleSpinBox* qaCoordinates[12];
	
	QDialogButtonBox* qButtons;
};

#endif // __ZZ_ADDOBJECTDIALOG_H__