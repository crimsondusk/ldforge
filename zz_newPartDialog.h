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

#ifndef NEWPARTDIALOG_H
#define NEWPARTDIALOG_H

#include <qdialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qdialogbuttonbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include "gui.h"
#include "radiobox.h"

class NewPartDialog : public QDialog {
public:
	explicit NewPartDialog (QWidget* parent = null, Qt::WindowFlags f = 0);
	static void StaticDialog ();
	
	QLabel* lb_brickIcon, *lb_name, *lb_author, *lb_license, *lb_BFC;
	QLineEdit* le_name, *le_author;
	
	RadioBox* rb_license, *rb_BFC;
	QDialogButtonBox* bbx_buttons;
};

#endif // NEWPARTDIALOG_H