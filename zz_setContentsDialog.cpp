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

#include <QAbstractButton>
#include <qboxlayout.h>
#include "zz_setContentsDialog.h"
#include "file.h"
#include "gui.h"
#include "history.h"

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
	
	if (obj->getType() == OBJ_Gibberish) {
		qErrorLabel = new QLabel;
		qErrorLabel->setText (format ("<span style=\"color: #900\">%s</span>",
			static_cast<LDGibberish*> (obj)->zReason.chars()));
		
		QPixmap qErrorPixmap = QPixmap ("icons/error.png").scaledToHeight (16);
		
		qErrorIcon = new QLabel;
		qErrorIcon->setPixmap (qErrorPixmap);
	}
	
	IMPLEMENT_DIALOG_BUTTONS
	
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget (qContentsLabel);
	layout->addWidget (qContents);
	
	QHBoxLayout* layout2 = new QHBoxLayout;
	
	if (obj->getType() == OBJ_Gibberish) {
		layout2->addWidget (qErrorIcon);
		layout2->addWidget (qErrorLabel);
	}
	
	layout2->addWidget (qButtons);
	layout->addLayout (layout2);
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
void SetContentsDialog::staticDialog (LDObject* obj) {
	if (!obj)
		return;
	
	SetContentsDialog dlg (obj, g_ForgeWindow);
	if (dlg.exec () == false)
		return;
	
	LDObject* oldobj = obj;
	
	// Reinterpret it from the text of the input field
	obj = parseLine (dlg.qContents->text ().toStdString ().c_str ());
	
	// Mark down the history now before we perform the replacement (which
	// destroys the old object)
	History::addEntry (new EditHistory ({(ulong) oldobj->getIndex (g_CurrentFile)},
		{oldobj->clone ()}, {obj->clone ()}));
	
	oldobj->replace (obj);
	
	// Rebuild stuff after this
	g_ForgeWindow->refresh ();
}