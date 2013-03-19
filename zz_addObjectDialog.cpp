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

#include <qgridlayout.h>
#include "gui.h"
#include "zz_addObjectDialog.h"
#include "file.h"

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
AddObjectDialog::AddObjectDialog (const LDObjectType_e type, QWidget* parent) :
	QDialog (parent)
{
	qTypeIcon = new QLabel;
	qTypeIcon->setPixmap (QPixmap (g_saObjTypeIcons[type]));
	
	qCommentLine = new QLineEdit;
	
	IMPLEMENT_DIALOG_BUTTONS
	
	QGridLayout* layout = new QGridLayout;
	layout->addWidget (qTypeIcon, 0, 0);
	layout->addWidget (qCommentLine, 0, 1);
	layout->addWidget (qButtons, 1, 1);
	
	setLayout (layout);
	setWindowTitle (str::mkfmt (APPNAME_DISPLAY " - new %s",
		g_saObjTypeNames[type]).chars());
	
	setWindowIcon (QIcon (g_saObjTypeIcons[type]));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void AddObjectDialog::staticDialog (const LDObjectType_e type, ForgeWindow* window) {
	AddObjectDialog dlg (type, window);
	
	if (dlg.exec ()) {
		switch (type) {
		case OBJ_Comment:
			{
				LDComment* comm = new LDComment;
				comm->zText = dlg.qCommentLine->text ();
				g_CurrentFile->addObject (comm);
				window->refresh ();
				break;
			}
		default:
			break;
		}
	}
}