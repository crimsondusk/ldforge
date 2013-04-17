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
	qLB_Icon = new QLabel;
	qLB_Icon->setPixmap (QPixmap ("icons/brick.png"));
	
	qLB_NameLabel = new QLabel ("Name:");
	qLE_Name = new QLineEdit;
	qLE_Name->setMinimumWidth (320);
	
	qLB_AuthorLabel = new QLabel ("Author:");
	qLE_Author = new QLineEdit;
	
	qLB_LicenseLabel = new QLabel ("License:");
	qCB_LicenseBox = new QComboBox;
	qCB_LicenseBox->addItems ({
		"CCAL Redistributable",
		"Non-redistributable",
		"[none]",
	});
	
	qLB_BFCLabel = new QLabel ("BFC:");
	qCB_BFCBox = new QComboBox;
	qCB_BFCBox->addItems ({
		"CCW",
		"CW",
		"No winding"
	});
	
	IMPLEMENT_DIALOG_BUTTONS
	
	QGridLayout* layout = new QGridLayout;
	layout->addWidget (qLB_Icon, 0, 0);
	layout->addWidget (qLB_NameLabel, 0, 1);
	layout->addWidget (qLE_Name, 0, 2);
	layout->addWidget (qLB_AuthorLabel, 1, 1);
	layout->addWidget (qLE_Author, 1, 2);
	layout->addWidget (qLB_LicenseLabel, 2, 1);
	layout->addWidget (qCB_LicenseBox, 2, 2);
	layout->addWidget (qLB_BFCLabel, 3, 1);
	layout->addWidget (qCB_BFCBox, 3, 2);
	layout->addWidget (qButtons, 4, 2);
	
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
		str zAuthor = dlg.qLE_Author->text ();
		vector<LDObject*>& objs = g_CurrentFile->objects;
		
		idx = dlg.qCB_BFCBox->currentIndex ();
		const LDBFC::Type eBFCType =
			(idx == BFCBOX_CCW) ? LDBFC::CertifyCCW :
			(idx == BFCBOX_CW) ? LDBFC::CertifyCW :
			LDBFC::NoCertify;
		
		idx = dlg.qCB_LicenseBox->currentIndex ();
		const char* sLicense =
			(idx == LICENSE_CCAL) ? "Redistributable under CCAL version 2.0 : see CAreadme.txt" :
			(idx == LICENSE_NonCA) ? "Not redistributable : see NonCAreadme.txt" :
			null;
		
		objs.push_back (new LDComment (dlg.qLE_Name->text ()));
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