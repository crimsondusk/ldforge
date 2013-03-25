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

#define INIT_CHECKBOX(BOX, CFG) \
	BOX->setCheckState (CFG ? Qt::Checked : Qt::Unchecked);

#define APPLY_CHECKBOX(BTN, CFG) \
	CFG = BTN->checkState() == Qt::Checked;

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
	setButtonBackground (qGLBackgroundButton, gl_bgcolor.value);
	connect (qGLBackgroundButton, SIGNAL (clicked ()),
		this, SLOT (slot_setGLBackground ()));
	
	qGLForegroundLabel = new QLabel ("Foreground color:");
	qGLForegroundButton = new QPushButton;
	setButtonBackground (qGLForegroundButton, gl_maincolor.value);
	connect (qGLForegroundButton, SIGNAL (clicked ()),
		this, SLOT (slot_setGLForeground ()));
	
	qGLForegroundAlphaLabel = new QLabel ("Translucency:");
	qGLForegroundAlpha = new QSlider (Qt::Horizontal);
	qGLForegroundAlpha->setRange (1, 10);
	qGLForegroundAlpha->setTickInterval (1);
	qGLForegroundAlpha->setSliderPosition (gl_maincolor_alpha * 10.0f);
	qGLForegroundAlpha->setTickPosition (QSlider::TicksAbove);
	
	qGLLineThicknessLabel = new QLabel ("Line thickness:");
	qGLLineThickness = new QSlider (Qt::Horizontal);
	qGLLineThickness->setRange (1, 8);
	qGLLineThickness->setSliderPosition (gl_linethickness);
	qGLLineThickness->setTickPosition (QSlider::TicksAbove);
	qGLLineThickness->setTickInterval (1);
	
	qLVColorize = new QCheckBox ("Colorize polygons in list view");
	INIT_CHECKBOX (qLVColorize, lv_colorize)
	
	qGLColorBFC = new QCheckBox ("Red/green BFC view");
	INIT_CHECKBOX (qGLColorBFC, gl_colorbfc)
	
	IMPLEMENT_DIALOG_BUTTONS
	
	QGridLayout* layout = new QGridLayout;
	layout->addWidget (qLDrawPathLabel, 0, 0);
	layout->addWidget (qLDrawPath, 0, 1, 1, 2);
	layout->addWidget (qLDrawPathFindButton, 0, 3);
	
	layout->addWidget (qGLBackgroundLabel, 1, 0);
	layout->addWidget (qGLBackgroundButton, 1, 1);
	layout->addWidget (qGLForegroundLabel, 1, 2);
	layout->addWidget (qGLForegroundButton, 1, 3);
	
	layout->addWidget (qGLLineThicknessLabel, 2, 0);
	layout->addWidget (qGLLineThickness, 2, 1);
	layout->addWidget (qGLForegroundAlphaLabel, 2, 2);
	layout->addWidget (qGLForegroundAlpha, 2, 3);
	
	layout->addWidget (qLVColorize, 3, 0, 1, 2);
	layout->addWidget (qGLColorBFC, 3, 2, 1, 2);
	
	layout->addWidget (qButtons, 4, 2, 1, 2);
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
void ConfigDialog::pickColor (strconfig& cfg, QPushButton* qButton) {
	QColorDialog dlg (QColor (cfg.value.chars()));
	dlg.setWindowIcon (QIcon ("icons/colorselect.png"));
	
	if (dlg.exec ()) {
		uchar r = dlg.currentColor ().red (),
			g = dlg.currentColor ().green (),
			b = dlg.currentColor ().blue ();
		cfg.value.format ("#%.2X%.2X%.2X", r, g, b);
		setButtonBackground (qButton, cfg.value);
	}
}

void ConfigDialog::slot_setGLBackground () {
	pickColor (gl_bgcolor, qGLBackgroundButton);
}

void ConfigDialog::slot_setGLForeground () {
	pickColor (gl_maincolor, qGLForegroundButton);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::setButtonBackground (QPushButton* qButton, str zValue) {
	qButton->setIcon (QIcon ("icons/colorselect.png"));
	qButton->setAutoFillBackground (true);
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
		
		APPLY_CHECKBOX (dlg.qLVColorize, lv_colorize)
		APPLY_CHECKBOX (dlg.qGLColorBFC, gl_colorbfc)
		
		gl_maincolor_alpha = ((double)dlg.qGLForegroundAlpha->value ()) / 10.0f;
		gl_linethickness = dlg.qGLLineThickness->value ();
		
		// Save the config
		config::save ();
		
		// Reload all subfiles
		reloadAllSubfiles ();
		
		window->R->setBackground ();
		window->refresh ();
	}
}