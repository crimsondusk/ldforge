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

#include "common.h"
#include "zz_configDialog.h"
#include "file.h"
#include <qgridlayout.h>
#include <qfiledialog.h>
#include <qcolordialog.h>

ConfigDialog* g_ConfigDialog = nullptr;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ConfigDialog::ConfigDialog (ForgeWindow* parent) : QDialog (parent) {
	g_ConfigDialog = this;
	
	qLDrawPath = new QLineEdit;
	qLDrawPath->setText (io_ldpath.value.chars());
	
	qLDrawPathLabel = new QLabel ("LDraw path:");
	
	qLDrawPathFindButton = new QPushButton;
	qLDrawPathFindButton->setIcon (QIcon ("icons/folder.png"));
	connect (qLDrawPathFindButton, SIGNAL (clicked ()),
		this, SLOT (slot_findLDrawPath ()));
	
	qGLBackgroundLabel = new QLabel ("Background color:");
	qGLBackgroundButton = new QPushButton;
	qGLBackgroundButton->setIcon (QIcon ("icons/colorselect.png"));
	qGLBackgroundButton->setAutoFillBackground (true);
	setButtonBackground (qGLBackgroundButton, gl_bgcolor.value);
	connect (qGLBackgroundButton, SIGNAL (clicked()),
		this, SLOT (slot_setGLBackground ()));
	
	qButtons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect (qButtons, SIGNAL (accepted ()), this, SLOT (accept ()));
	connect (qButtons, SIGNAL (rejected ()), this, SLOT (reject ()));
	
	QGridLayout* layout = new QGridLayout;
	layout->addWidget (qLDrawPathLabel, 0, 0);
	layout->addWidget (qLDrawPath, 0, 1);
	layout->addWidget (qLDrawPathFindButton, 0, 2);
	
	layout->addWidget (qGLBackgroundLabel, 1, 0);
	layout->addWidget (qGLBackgroundButton, 1, 1);
	
	layout->addWidget (qButtons, 2, 1, 1, 2);
	setLayout (layout);
	
	setWindowTitle (APPNAME_DISPLAY " - editing settings");
	setWindowIcon (QIcon ("icons/settings.png"));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ConfigDialog::~ConfigDialog() {
	g_ConfigDialog = nullptr;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::slot_findLDrawPath () {
	str zDir = QFileDialog::getExistingDirectory (this, "Choose LDraw directory",
		qLDrawPath->text());
	
	if (~zDir)
		qLDrawPath->setText (zDir.chars());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::slot_setGLBackground () {
	QColorDialog dlg (QColor (gl_bgcolor.value.chars()));
	dlg.setWindowIcon (QIcon ("icons/colorselect.png"));
	
	if (dlg.exec ()) {
		uchar r = dlg.currentColor ().red (),
			g = dlg.currentColor ().green (),
			b = dlg.currentColor ().blue ();
		gl_bgcolor.value.format ("#%.2X%.2X%.2X", r, g, b);
		setButtonBackground (qGLBackgroundButton, gl_bgcolor.value);
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::setButtonBackground (QPushButton* qButton, str zValue) {
	qButton->setStyleSheet (
		str::mkfmt ("background-color: %s", zValue.chars()).chars()
	);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::staticDialog (ForgeWindow* window) {
	ConfigDialog dlg (window);
	
	if (dlg.exec ()) {
		io_ldpath = dlg.qLDrawPath->text();
		
		// Save the config
		config::save ();
		
		// Reload all subfiles
		reloadAllSubfiles ();
		
		window->R->setBackground ();
	}
}