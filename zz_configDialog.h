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

#include "gui.h"
#include <QDialog>
#include <qlabel.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qpushbutton.h>

class ConfigDialog : public QDialog {
	Q_OBJECT
	
public:
	QLabel* qLDrawPathLabel;
	QLineEdit* qLDrawPath;
	QPushButton* qLDrawPathFindButton;
	
	QDialogButtonBox* qButtons;
	
	ConfigDialog (ForgeWindow* parent);
	static void staticDialog (ForgeWindow* window);
	
private slots:
	void slot_findLDrawPath ();
};