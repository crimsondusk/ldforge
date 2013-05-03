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

#include <qgridlayout.h>
#include "zz_newPartDialog.h"
#include "file.h"

// -------------------------------------
enum {
	LICENSE_CCAL,
	LICENSE_NonCA,
	LICENSE_None
};

// -------------------------------------
enum {
	BFCBOX_CCW,
	BFCBOX_CW,
	BFCBOX_None,
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
NewPartDialog::NewPartDialog (QWidget* parent, Qt::WindowFlags f) : QDialog (parent, f) {
	lb_brickIcon = new QLabel;
	lb_brickIcon->setPixmap (QPixmap ("icons/brick.png"));
	
	lb_name = new QLabel ("Name:");
	le_name = new QLineEdit;
	le_name->setMinimumWidth (320);
	
	lb_author = new QLabel ("Author:");
	le_author = new QLineEdit;
	
	rb_license = new RadioBox ("License", {
		"CCAL Redistributable",
		"Non-redistributable",
		"Don't append a license",
	}, LICENSE_CCAL);
	
	rb_BFC = new RadioBox ("BFC Winding", {
		"CCW",
		"CW",
		"No winding"
	}, BFCBOX_CCW);
	
	QHBoxLayout* boxes = new QHBoxLayout;
	boxes->addWidget (rb_license);
	boxes->addWidget (rb_BFC);
	
	IMPLEMENT_DIALOG_BUTTONS
	
	QGridLayout* layout = new QGridLayout;
	layout->addWidget (lb_brickIcon, 0, 0);
	layout->addWidget (lb_name, 0, 1);
	layout->addWidget (le_name, 0, 2);
	layout->addWidget (lb_author, 1, 1);
	layout->addWidget (le_author, 1, 2);
	layout->addLayout (boxes, 2, 1, 1, 2);
	layout->addWidget (bbx_buttons, 3, 2);
	
	setLayout (layout);
	setWindowIcon (QIcon ("icons/brick.png"));
	setWindowTitle (APPNAME_DISPLAY " - new part");
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void NewPartDialog::StaticDialog () {
	NewPartDialog dlg (g_ForgeWindow);
	if (dlg.exec ()) {
		newFile ();
		
		short idx;
		str zAuthor = dlg.le_author->text ();
		vector<LDObject*>& objs = g_CurrentFile->objects;
		
		idx = dlg.rb_BFC->value ();
		const LDBFC::Type eBFCType =
			(idx == BFCBOX_CCW) ? LDBFC::CertifyCCW :
			(idx == BFCBOX_CW) ? LDBFC::CertifyCW :
			LDBFC::NoCertify;
		
		idx = dlg.rb_license->value ();
		const char* sLicense =
			(idx == LICENSE_CCAL) ? "Redistributable under CCAL version 2.0 : see CAreadme.txt" :
			(idx == LICENSE_NonCA) ? "Not redistributable : see NonCAreadme.txt" :
			null;
		
		objs.push_back (new LDComment (dlg.le_name->text ()));
		objs.push_back (new LDComment ("Name: <untitled>.dat"));
		objs.push_back (new LDComment (format ("Author: %s", zAuthor.chars())));
		objs.push_back (new LDComment (format ("!LDRAW_ORG Unofficial_Part")));
		
		if (sLicense != null)
			objs.push_back (new LDComment (format ("!LICENSE %s", sLicense)));
		
		objs.push_back (new LDEmpty);
		objs.push_back (new LDBFC (eBFCType));
		objs.push_back (new LDEmpty);
		
		g_ForgeWindow->refresh ();
	}
}