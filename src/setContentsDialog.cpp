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
#include "setContentsDialog.h"
#include "file.h"
#include "gui.h"
#include "history.h"

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
SetContentsDialog::SetContentsDialog (LDObject* obj, QWidget* parent) : QDialog(parent) {
	setWindowTitle (APPNAME ": Set Contents");
	
	lb_contents = new QLabel ("Set contents:", parent);
	
	le_contents = new QLineEdit (parent);
	le_contents->setText (obj->getContents ().chars());
	le_contents->setWhatsThis ("The LDraw code of this object. The code written "
		"here is expected to be valid LDraw code, invalid code here results "
		"the object being turned into an error object. Please do refer to the "
		"<a href=\"http://www.ldraw.org/article/218.html\">official file format "
		"standard</a> for further information.");
	le_contents->setMinimumWidth (384);
	
	if (obj->getType() == LDObject::Gibberish) {
		lb_error = new QLabel;
		lb_error->setText (fmt ("<span style=\"color: #900\">%s</span>",
			static_cast<LDGibberish*> (obj)->reason.chars()));
		
		QPixmap qErrorPixmap = getIcon ("error").scaledToHeight (16);
		
		lb_errorIcon = new QLabel;
		lb_errorIcon->setPixmap (qErrorPixmap);
	}
	
	IMPLEMENT_DIALOG_BUTTONS
	
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget (lb_contents);
	layout->addWidget (le_contents);
	
	QHBoxLayout* layout2 = new QHBoxLayout;
	
	if (obj->getType() == LDObject::Gibberish) {
		layout2->addWidget (lb_errorIcon);
		layout2->addWidget (lb_error);
	}
	
	layout2->addWidget (bbx_buttons);
	layout->addLayout (layout2);
	setLayout (layout);
	
	setWindowTitle (APPNAME ": Set Contents");
	setWindowIcon (getIcon ("set-contents"));
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
	
	SetContentsDialog dlg (obj, g_win);
	if (dlg.exec () == false)
		return;
	
	LDObject* oldobj = obj;
	
	// Reinterpret it from the text of the input field
	obj = parseLine (dlg.le_contents->text ().toStdString ().c_str ());
	
	// Mark down the history now before we perform the replacement (which
	// destroys the old object)
	History::addEntry (new EditHistory ({(ulong) oldobj->getIndex (g_curfile)},
		{oldobj->clone ()}, {obj->clone ()}));
	
	oldobj->replace (obj);
	
	// Rebuild stuff after this
	g_win->refresh ();
}