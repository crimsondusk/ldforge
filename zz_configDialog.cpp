#include "common.h"
#include "zz_configDialog.h"
#include "file.h"
#include <qgridlayout.h>
#include <qfiledialog.h>

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ConfigDialog::ConfigDialog (ForgeWindow* parent) : QDialog () {
	qLDrawPath = new QLineEdit;
	qLDrawPath->setText (io_ldpath.value.chars());
	
	qLDrawPathLabel = new QLabel ("LDraw path:");
	
	qLDrawPathFindButton = new QPushButton;
	qLDrawPathFindButton->setIcon (QIcon ("icons/folder.png"));
	connect (qLDrawPathFindButton, SIGNAL (clicked ()),
		this, SLOT (slot_findLDrawPath ()));
	
	qButtons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect (qButtons, SIGNAL (accepted ()), this, SLOT (accept ()));
	connect (qButtons, SIGNAL (rejected ()), this, SLOT (reject ()));
	
	QGridLayout* layout = new QGridLayout;
	layout->addWidget (qLDrawPathLabel, 0, 0);
	layout->addWidget (qLDrawPath, 0, 1);
	layout->addWidget (qLDrawPathFindButton, 0, 2);
	layout->addWidget (qButtons, 2, 1, 1, 2);
	setLayout (layout);
	
	setWindowTitle (APPNAME_DISPLAY " - editing settings");
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
void ConfigDialog::staticDialog (ForgeWindow* window) {
	ConfigDialog dlg (window);
	ulong ulChange = 0;
	
	if (dlg.exec ()) {
		str zOldLDPath = io_ldpath;
		io_ldpath = dlg.qLDrawPath->text();
		
		if (io_ldpath != zOldLDPath) {
			ulChange |= (1 << 1);
		}
		
		if (ulChange != 0)
			config::save ();
		
		if (ulChange & (1 << 1)) {
			// Reload all subfiles
			reloadAllSubfiles ();
		}
	}
}