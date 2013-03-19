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

#include <QAbstractButton>
#include <qboxlayout.h>
#include "zz_setContentsDialog.h"
#include "file.h"
#include "gui.h"

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
SetContentsDialog::SetContentsDialog (LDObject* obj, QWidget* parent) : QDialog(parent) {
	setWindowTitle (APPNAME_DISPLAY ": Set Contents");
	
	qContentsLabel = new QLabel ("Set contents:", parent);
	
	qContents = new QLineEdit (parent);
	qContents->setText (obj->getContents ().chars());
	qContents->setWhatsThis ("The LDraw code of this object. The code written "
		"here is expected to be valid LDraw code, invalid code here results "
		"the object being turned into an error object. Please do refer to the "
		"<a href=\"http://www.ldraw.org/article/218.html\">official file format "
		"standard</a> for further information.");
	qContents->setMinimumWidth (384);
	
	qOKCancel = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
		Qt::Horizontal, parent);
	
	connect (qOKCancel, SIGNAL (accepted ()), this, SLOT (accept ()));
	connect (qOKCancel, SIGNAL (rejected ()), this, SLOT (reject ()));
	/*
	connect (qOKCancel, SIGNAL (clicked (QAbstractButton*)), this, SLOT (slot_handleButtons (QAbstractButton*)));
	*/
	
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget (qContentsLabel);
	layout->addWidget (qContents);
	layout->addWidget (qOKCancel);
	setLayout (layout);
	
	setWindowTitle (APPNAME_DISPLAY " - setting contents");
	setWindowIcon (QIcon ("icons/set-contents.png"));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void SetContentsDialog::slot_handleButtons (QAbstractButton* qButton) {
	qButton = qButton;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void SetContentsDialog::staticDialog (LDObject* obj, ForgeWindow* parent) {
	if (!obj)
		return;
	
	SetContentsDialog dlg (obj, parent);
	if (dlg.exec ()) {
		LDObject* oldobj = obj;
		
		// Reinterpret it from the text of the input field
		obj = parseLine (dlg.qContents->text ().toStdString ().c_str ());
		
		oldobj->replace (obj);
		
		// Rebuild stuff after this
		parent->buildObjList ();
		parent->R->hardRefresh ();
	}
}