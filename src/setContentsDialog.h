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

#ifndef SETCONTENTSDIALOG_H

#include <QDialog>
#include "common.h"

class QAbstractButton;
class QLabel;
class QLineEdit;

// =============================================================================
// SetContentsDialog
//
// Performs the Set Contents dialog on the given LDObject. Object's contents
// are exposed to the user and is reinterpreted if the user accepts the new
// contents.
// =============================================================================
class SetContentsDialog : public QDialog {
public:
	QLabel* lb_contents, *lb_errorIcon, *lb_error;
	QLineEdit* le_contents;
	
	SetContentsDialog (LDObject* obj, QWidget* parent = null);
	static void staticDialog (LDObject* obj);
	
private slots:
	void slot_handleButtons (QAbstractButton* btn);
};

#endif // SETCONTENTSDIALOG_H