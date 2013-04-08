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

#ifndef ZZ_NEWPARTDIALOG_H
#define ZZ_NEWPARTDIALOG_H

#include "gui.h"
#include <qdialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qdialogbuttonbox.h>

class NewPartDialog : public QDialog {
public:
	explicit NewPartDialog (QWidget* parent = nullptr, Qt::WindowFlags f = 0);
	static void StaticDialog ();
	
	QLabel* qLB_Icon, *qLB_NameLabel, *qLB_AuthorLabel, *qLB_LicenseLabel, *qLB_BFCLabel;
	QLineEdit* qLE_Name, *qLE_Author;
	QComboBox* qCB_LicenseBox, *qCB_BFCBox;
	QDialogButtonBox* qButtons;
};

#endif // ZZ_NEWPARTDIALOG_H