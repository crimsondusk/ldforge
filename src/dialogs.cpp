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

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>

#include "dialogs.h"
#include "radiobox.h"
#include "gui.h"
#include "gldraw.h"
#include "docs.h"
#include "dialogs.h"

// =============================================================================
OverlayDialog::OverlayDialog (QWidget* parent, Qt::WindowFlags f) : QDialog (parent, f) {
	rb_camera = new RadioBox ("Camera", {}, 0, Qt::Horizontal, this);
	
	for (int i = 0; i < 6; ++i) {
		if (i == 3)
			rb_camera->rowBreak ();
		
		rb_camera->addButton (g_CameraNames[i]);
	}
	
	GL::Camera cam = g_win->R ()->camera ();
	if (cam != GL::Free)
		rb_camera->setValue ((int) cam);
	
	QGroupBox* gb_image = new QGroupBox ("Image", this);
	
	QLabel* lb_fpath = new QLabel ("File:");
	le_fpath = new QLineEdit;
	le_fpath->setFocus ();
	
	QLabel* lb_ofs = new QLabel ("Origin:");
	btn_fpath = new QPushButton;
	btn_fpath->setIcon (getIcon ("folder"));
	connect (btn_fpath, SIGNAL (clicked ()), this, SLOT (slot_fpath ()));
	
	sb_ofsx = new QSpinBox;
	sb_ofsy = new QSpinBox;
	sb_ofsx->setRange (0, 10000);
	sb_ofsy->setRange (0, 10000);
	sb_ofsx->setSuffix (" px");
	sb_ofsy->setSuffix (" px");
	
	QLabel* lb_dimens = new QLabel ("Dimensions:");
	dsb_lwidth = new QDoubleSpinBox;
	dsb_lheight = new QDoubleSpinBox;
	dsb_lwidth->setRange (0.0f, 10000.0f);
	dsb_lheight->setRange (0.0f, 10000.0f);
	dsb_lwidth->setSuffix (" LDU");
	dsb_lheight->setSuffix (" LDU");
	dsb_lwidth->setSpecialValueText ("Automatic");
	dsb_lheight->setSpecialValueText ("Automatic");
	
	dbb_buttons = makeButtonBox (*this);
	dbb_buttons->addButton (QDialogButtonBox::Help);
	connect (dbb_buttons, SIGNAL (helpRequested ()), this, SLOT (slot_help()));
	
	QHBoxLayout* fpathlayout = new QHBoxLayout;
	fpathlayout->addWidget (lb_fpath);
	fpathlayout->addWidget (le_fpath);
	fpathlayout->addWidget (btn_fpath);
	
	QGridLayout* metalayout = new QGridLayout;
	metalayout->addWidget (lb_ofs,			0, 0);
	metalayout->addWidget (sb_ofsx,		0, 1);
	metalayout->addWidget (sb_ofsy,		0, 2);
	metalayout->addWidget (lb_dimens,		1, 0);
	metalayout->addWidget (dsb_lwidth,	1, 1);
	metalayout->addWidget (dsb_lheight,	1, 2);
	
	QVBoxLayout* imagelayout = new QVBoxLayout (gb_image);
	imagelayout->addLayout (fpathlayout);
	imagelayout->addLayout (metalayout);
	
	QVBoxLayout* layout = new QVBoxLayout (this);
	layout->addWidget (rb_camera);
	layout->addWidget (gb_image);
	layout->addWidget (dbb_buttons);
	
	connect (dsb_lwidth, SIGNAL (valueChanged (double)), this, SLOT (slot_dimensionsChanged ()));
	connect (dsb_lheight, SIGNAL (valueChanged (double)), this, SLOT (slot_dimensionsChanged ()));
	connect (rb_camera, SIGNAL (valueChanged (int)), this, SLOT (fillDefaults (int)));
	
	slot_dimensionsChanged ();
	fillDefaults (cam);
}

void OverlayDialog::fillDefaults (int newcam) {
	overlayMeta& info = g_overlays[newcam];
	
	if (info.img != null) {
		le_fpath->setText (info.fname);
		sb_ofsx->setValue (info.ox);
		sb_ofsy->setValue (info.oy);
		dsb_lwidth->setValue (info.lw);
		dsb_lheight->setValue (info.lh);
	} else {
		le_fpath->setText ("");
		sb_ofsx->setValue (0);
		sb_ofsy->setValue (0);
		dsb_lwidth->setValue (0.0f);
		dsb_lheight->setValue (0.0f);
	} 
}

str		OverlayDialog::fpath		() const { return le_fpath->text (); }
ushort	OverlayDialog::ofsx		() const { return sb_ofsx->value (); }
ushort	OverlayDialog::ofsy		() const { return sb_ofsy->value (); }
double	OverlayDialog::lwidth		() const { return dsb_lwidth->value (); }
double	OverlayDialog::lheight		() const { return dsb_lheight->value (); }
int		OverlayDialog::camera		() const { return rb_camera->value (); }

void OverlayDialog::slot_fpath () {
	le_fpath->setText (QFileDialog::getOpenFileName (null, "Overlay image"));
}

void OverlayDialog::slot_help () {
	showDocumentation (g_docs_overlays);
}

void OverlayDialog::slot_dimensionsChanged () {
	bool enable = (dsb_lwidth->value () != 0) || (dsb_lheight->value () != 0);
	dbb_buttons->button (QDialogButtonBox::Ok)->setEnabled (enable);
}