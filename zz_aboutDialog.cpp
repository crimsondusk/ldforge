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

#include <stdlib.h>
#include <qlabel.h>
#include <qboxlayout.h>
#include <qdialogbuttonbox.h>
#include <qdesktopservices.h>
#include <qurl.h>
#include "common.h"
#include "zz_aboutDialog.h"

AboutDialog::AboutDialog (QWidget* parent, Qt::WindowFlags f) : QDialog (parent, f) {
	QWidget* mainTab, *licenseTab;
	QTabWidget* tabs = new QTabWidget;
	
	{
		mainTab = new QWidget;
		
		// Application icon - in full 64x64 glory.
		QLabel* icon = new QLabel;
		icon->setPixmap (QPixmap ("icons/ldforge.png"));
		
		// Heading - application label and copyright information
		QLabel* title = new QLabel (format ("<b>" APPNAME_DISPLAY " v%d.%d</b><br />"
			"Copyright (C) 2013 Santeri Piippo",
			VERSION_MAJOR, VERSION_MINOR));
		
		// Body text
		QLabel* info = new QLabel (
			"<p>This software is intended for usage as a parts<br />"
			"authoring tool for the <a href=\"http://ldraw.org/\">LDraw</a> parts library.</p>"
			
			"<p>" APPNAME_DISPLAY " is free software, and you are welcome<br />"
			"to redistribute it under the terms of GPL v3. See the LICENSE<br />"
			"text file or the license tab in this dialog for details. If the<br />"
			"license text is not available for some reason, see<br />"
			"<a href=\"http://www.gnu.org/licenses/\">http://www.gnu.org/licenses/</a>"
			"for the license terms.</p>"
			
			"<p>The application icon is derived from "
			"<a href=\"http://en.wikipedia.org/wiki/File:Anvil,_labelled_en.svg\">this image</a>.</p>"
		);
		
		// Rest in peace, James.
		QLabel* memorial = new QLabel ("In living memory of James Jessiman.");
		
		QVBoxLayout* layout = new QVBoxLayout;
		layout->addWidget (icon);
		layout->addWidget (title);
		layout->addWidget (info);
		layout->addWidget (memorial);
		
		// Align everything to the center.
		for (QLabel* label : {icon, title, info, memorial})
			label->setAlignment (Qt::AlignCenter);
		
		mainTab->setLayout (layout);
		tabs->addTab (mainTab, "About " APPNAME_DISPLAY);
	}
	
	{
		licenseTab = new QWidget;
		
		QTextEdit* license = new QTextEdit;
		license->setReadOnly (true);
		
		QFont font ("Monospace");
		font.setStyleHint (QFont::TypeWriter);
		font.setPixelSize (10);
		
		license->setFont (font);
		
		// Make the text view wide enough to display the license text.
		// Why isn't 80 sufficient here?
		license->setMinimumWidth (license->fontMetrics ().width ('a') * 85);
		
		// Try open the license text
		FILE* fp = fopen ("LICENSE", "r");
		
		if (fp == null) {
			// Failed; tell the user how to get the license text instead.
			setlocale (LC_ALL, "C");
			char const* fmt = "Couldn't open LICENSE: %s.<br />"
				"See <a href=\"http://www.gnu.org/licenses/\">http://www.gnu.org/licenses/</a> for the GPLv3 text.";
			
			license->setHtml (format (fmt, strerror (errno)));
		} else {
			// Figure out file size
			fseek (fp, 0, SEEK_END);
			const size_t length = ftell (fp);
			rewind (fp);
			
			// Init text buffer and write pointer
			char* licenseText = new char[length];
			char* writePtr = &licenseText[0];
			
			// Read in the license text
			while (true) {
				*writePtr = fgetc (fp);
				
				if (feof (fp))
					break;
				
				writePtr++;
			}
			
			// Add terminating null character and add the license text to the
			// license dialog text view.
			*writePtr = '\0';
			license->setText (licenseText);
			
			// And dump the trash on the way out.
			delete[] licenseText;
		}
		
		QVBoxLayout* layout = new QVBoxLayout;
		layout->addWidget (license);
		licenseTab->setLayout (layout);
		tabs->addTab (licenseTab, "License");
	}
	
	QDialogButtonBox* buttons = new QDialogButtonBox (QDialogButtonBox::Close);
	
	QPushButton* helpButton = new QPushButton;
	helpButton->setText ("Mail Author");
	helpButton->setIcon (getIcon ("mail"));
	
	buttons->addButton (dynamic_cast<QAbstractButton*> (helpButton), QDialogButtonBox::HelpRole);
	connect (buttons, SIGNAL (helpRequested ()), this, SLOT (slot_mail ()));
	connect (buttons, SIGNAL (rejected ()), this, SLOT (reject ()));
	
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget (tabs);
	layout->addWidget (buttons);
	setLayout (layout);
	setWindowTitle ("About " APPNAME_DISPLAY);
}

void AboutDialog::slot_mail () {
	QDesktopServices::openUrl (QUrl ("mailto:Santeri Piippo <arezey@gmail.com>?subject=LDForge"));
}