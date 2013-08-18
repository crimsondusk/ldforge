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

#include <QDesktopServices>
#include <QPushButton>
#include <QUrl>
#include "build/moc_aboutDialog.cpp"
#include "aboutDialog.h"
#include "ui_about.h"
#include "gui.h"

// =============================================================================
// -----------------------------------------------------------------------------
AboutDialog::AboutDialog (QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f) {
	
	Ui::AboutUI ui;
	ui.setupUi (this);
	ui.versionInfo->setText (fmt (tr ("LDForge %1"), fullVersionString()));
	
	QPushButton* mailButton = new QPushButton;
	mailButton->setText ("Contact");
	mailButton->setIcon (getIcon ("mail"));
	ui.buttonBox->addButton (static_cast<QAbstractButton*> (mailButton), QDialogButtonBox::HelpRole);
	connect (ui.buttonBox, SIGNAL (helpRequested()), this, SLOT (slot_mail()));
	
	setWindowTitle ("About " APPNAME);
}

// =============================================================================
// -----------------------------------------------------------------------------
void AboutDialog::slot_mail() {
	QDesktopServices::openUrl (QUrl ("mailto:Santeri Piippo <arezey@gmail.com>?subject=LDForge"));
}