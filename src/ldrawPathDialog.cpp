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

#include <qlineedit.h>
#include <qpushbutton.h>
#include <qdialogbuttonbox.h>
#include <QFileDialog>
#include <QBoxLayout>
#include <QLabel>
#include "ldrawPathDialog.h"
#include "gui.h"
#include "file.h"

extern_cfg (str, io_ldpath);

// ========================================================================================================================================
LDrawPathDialog::LDrawPathDialog (const bool validDefault, QWidget* parent, Qt::WindowFlags f)
	: QDialog (parent, f), m_validDefault (validDefault)
{
	QLabel* lb_description = null;
	lb_resolution = new QLabel ("---");
	
	if (validDefault == false)
		lb_description = new QLabel ("Please input your LDraw directory");
	
	QLabel* lb_path = new QLabel ("LDraw path:");
	le_path = new QLineEdit;
	btn_findPath = new QPushButton;
	btn_findPath->setIcon (getIcon ("folder"));
	
	btn_tryConfigure = new QPushButton ("Configure");
	btn_tryConfigure->setIcon (getIcon ("settings"));
	
	btn_cancel = new QPushButton;
	
	if (validDefault == false) {
		btn_cancel->setText ("Exit");
		btn_cancel->setIcon (getIcon ("exit"));
	} else {
		btn_cancel->setText ("Cancel");
		btn_cancel->setIcon (getIcon ("cancel"));
	}
	
	dbb_buttons = new QDialogButtonBox (QDialogButtonBox::Ok);
	dbb_buttons->addButton (btn_tryConfigure, QDialogButtonBox::ActionRole);
	dbb_buttons->addButton (btn_cancel, QDialogButtonBox::RejectRole);
	okButton ()->setEnabled (false);
	
	QHBoxLayout* inputLayout = new QHBoxLayout;
	inputLayout->addWidget (lb_path);
	inputLayout->addWidget (le_path);
	inputLayout->addWidget (btn_findPath);
	
	QVBoxLayout* mainLayout = new QVBoxLayout;
	
	if (validDefault == false)
		mainLayout->addWidget (lb_description);
	
	mainLayout->addLayout (inputLayout);
	mainLayout->addWidget (lb_resolution);
	mainLayout->addWidget (dbb_buttons);
	setLayout (mainLayout);
	
	connect (le_path, SIGNAL (textEdited ()), this, SLOT (slot_tryConfigure ()));
	connect (btn_findPath, SIGNAL (clicked ()), this, SLOT (slot_findPath ()));
	connect (btn_tryConfigure, SIGNAL (clicked ()), this, SLOT (slot_tryConfigure ()));
	connect (dbb_buttons, SIGNAL (accepted ()), this, SLOT (accept ()));
	connect (dbb_buttons, SIGNAL (rejected ()), this, (validDefault) ? SLOT (reject ()) : SLOT (slot_exit ()));
	
	setPath (io_ldpath);
	if (validDefault)
		slot_tryConfigure ();
}

// ========================================================================================================================================
QPushButton* LDrawPathDialog::okButton () {
	return dbb_buttons->button (QDialogButtonBox::Ok);
}

// ========================================================================================================================================
void LDrawPathDialog::setPath (str path) {
	le_path->setText (path);
}

// ========================================================================================================================================
str LDrawPathDialog::path () const {
	return le_path->text ();
}

// ========================================================================================================================================
void LDrawPathDialog::slot_findPath () {
	str newpath = QFileDialog::getExistingDirectory (this, "Find LDraw Path");
	
	if (~newpath > 0 && newpath != path ()) {
		setPath (newpath);
		slot_tryConfigure ();
	}
}


// ========================================================================================================================================
void LDrawPathDialog::slot_exit () {
	exit (1);
}

// ========================================================================================================================================
void LDrawPathDialog::slot_tryConfigure () {
	if (LDPaths::tryConfigure (path ()) == false) {
		lb_resolution->setText (fmt ("<span style=\"color:red; font-weight: bold;\">%s</span>", LDPaths::getError().chars ()));
		okButton ()->setEnabled (false);
		return;
	}
	
	lb_resolution->setText ("<span style=\"color: #7A0; font-weight: bold;\">OK!</span>");
	okButton ()->setEnabled (true);
}