#include <QAbstractButton>
#include <qboxlayout.h>
#include "zz_setContentsDialog.h"
#include "io.h"
#include "gui.h"

// =============================================================================
// Dialog_SetContents (LDObject*) [constructor]
//
// Initializes the Set Contents dialog for the given LDObject
// =============================================================================
Dialog_SetContents::Dialog_SetContents (LDObject* obj, QWidget* parent) : QDialog(parent) {
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
}

// =============================================================================
// void slot_handleButtons (QAbstractButton*)
//
// Handles a button.. this is used to reset the input field
// =============================================================================
void Dialog_SetContents::slot_handleButtons (QAbstractButton* qButton) {
	qButton = qButton;
}

// =============================================================================
// void staticDialog (LDObject*) [static method]
//
// Performs the Set Contents dialog on the given LDObject. Object's contents
// are exposed to the user and is reinterpreted if the user accepts the new
// contents.
// =============================================================================
void Dialog_SetContents::staticDialog (LDObject* obj, LDForgeWindow* parent) {
	if (!obj)
		return;
	
	Dialog_SetContents dlg (obj, parent);
	if (dlg.exec ()) {
		LDObject* oldobj = obj;
		
		// Reinterpret it from the text of the input field
		obj = ParseLine (dlg.qContents->text ().toStdString ().c_str ());
		
		// Remove the old object
		delete oldobj;
		
		// Replace all instances of the old object with the new object
		objPointer::replacePointers (oldobj, obj);
		
		// Rebuild stuff after this
		parent->buildObjList ();
		parent->R->hardRefresh ();
	}
}