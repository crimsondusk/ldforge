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

#include <qdialog.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qdialogbuttonbox.h>
#include "common.h"

// =============================================================================
// SetContentsDialog
//
// Performs the Set Contents dialog on the given LDObject. Object's contents
// are exposed to the user and is reinterpreted if the user accepts the new
// contents.
// =============================================================================
class SetContentsDialog : public QDialog {
public:
	QLabel* qContentsLabel, *qErrorLabel;
	QLineEdit* qContents;
	QDialogButtonBox* qButtons;
	
	SetContentsDialog (LDObject* obj, QWidget* parent = nullptr);
	static void staticDialog (LDObject* obj, ForgeWindow* parent);
	
private slots:
	void slot_handleButtons (QAbstractButton* qButton);
};