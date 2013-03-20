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
#include <qdialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qpushbutton.h>
#include <qcheckbox.h>

class ConfigDialog : public QDialog {
	Q_OBJECT
	
public:
	QLabel* qLDrawPathLabel;
	QLabel* qGLBackgroundLabel, *qGLForegroundLabel, *qGLLineThicknessLabel;
	QLineEdit* qLDrawPath;
	QPushButton* qLDrawPathFindButton;
	QPushButton* qGLBackgroundButton, *qGLForegroundButton;
	QCheckBox* qLVColorize, *qGLColorBFC;
	QSlider* qGLLineThickness;
	
	QDialogButtonBox* qButtons;
	
	ConfigDialog (ForgeWindow* parent);
	~ConfigDialog ();
	static void staticDialog (ForgeWindow* window);
	
private:
    void setButtonBackground (QPushButton* qButton, str zValue);
    void pickColor (strconfig& cfg, QPushButton* qButton);
	
private slots:
	void slot_findLDrawPath ();
	void slot_setGLBackground ();
	void slot_setGLForeground ();
};