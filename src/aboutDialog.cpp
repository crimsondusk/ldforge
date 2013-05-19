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

#include <QLabel>
#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QDesktopServices>
#include <QPushButton>
#include <QTextEdit>
#include <QUrl>
#include "common.h"
#include "aboutDialog.h"

AboutDialog::AboutDialog (QWidget* parent, Qt::WindowFlags f) : QDialog (parent, f) {
	// Application icon - in full 64 x 64 glory.
	QLabel* icon = new QLabel;
	icon->setPixmap (getIcon ("ldforge"));
	
	// Heading - application label and copyright information
	QLabel* title = new QLabel (fmt ("<b>" APPNAME " %s</b><br />"
		"Copyright (C) 2013 Santeri Piippo",
		fullVersionString ().chars ()));
	
	// Body text
	QLabel* info = new QLabel (
		"<p>This software is intended for usage as a parts<br />"
		"authoring tool for the <a href=\"http://ldraw.org/\">LDraw</a> parts library.</p>"
		
		"<p>" APPNAME " is free software, and you are welcome<br />"
		"to redistribute it under the terms of GPL v3. See the<br />"
		"LICENSE text file for details. If the license text is not<br />"
		"available for some reason, see<br />"
		"<a href=\"http://www.gnu.org/licenses/\">http://www.gnu.org/licenses/</a> for the license terms.</p>"
		
		"<p>The application icon is derived from "
		"<a href=\"http://en.wikipedia.org/wiki/File:Anvil,_labelled_en.svg\">this image</a>.</p>"
	);
	
	// Rest in peace, James.
	QLabel* memorial = new QLabel ("In living memory of James Jessiman.");
	
	// Align everything to the center.
	for (QLabel* label : vector<QLabel*> ({icon, title, info, memorial}))
		label->setAlignment (Qt::AlignCenter);
	
	QDialogButtonBox* buttons = new QDialogButtonBox (QDialogButtonBox::Close);
	QPushButton* helpButton = new QPushButton;
	
	helpButton->setText ("Mail Author");
	helpButton->setIcon (getIcon ("mail"));
	buttons->addButton (static_cast<QAbstractButton*> (helpButton), QDialogButtonBox::HelpRole);
	connect (buttons, SIGNAL (helpRequested ()), this, SLOT (slot_mail ()));
	connect (buttons, SIGNAL (rejected ()), this, SLOT (reject ()));
	
	QVBoxLayout* layout = new QVBoxLayout (this);
	layout->addWidget (icon);
	layout->addWidget (title);
	layout->addWidget (info);
	layout->addWidget (memorial);
	layout->addWidget (buttons);
	
	setWindowTitle ("About " APPNAME);
}

void AboutDialog::slot_mail () {
	QDesktopServices::openUrl (QUrl ("mailto:Santeri Piippo <arezey@gmail.com>?subject=LDForge"));
}