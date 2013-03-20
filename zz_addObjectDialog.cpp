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

#define APPLY_COORDS(OBJ, N) \
	for (short i = 0; i < N; ++i) { \
		OBJ->vaCoords[i].x = dlg.qaCoordinates[(i * 3) + 0]->value (); \
		OBJ->vaCoords[i].y = dlg.qaCoordinates[(i * 3) + 1]->value (); \
		OBJ->vaCoords[i].z = dlg.qaCoordinates[(i * 3) + 2]->value (); \
	}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
AddObjectDialog::AddObjectDialog (const LDObjectType_e type, QWidget* parent) :
	QDialog (parent)
{
	short dCoordCount = 0;
	
	qTypeIcon = new QLabel;
	qTypeIcon->setPixmap (QPixmap (g_saObjTypeIcons[type]));
	
	switch (type) {
	case OBJ_Comment:
		qCommentLine = new QLineEdit;
		break;
	case OBJ_Line:
		dCoordCount = 6;
		break;
	case OBJ_Triangle:
		dCoordCount = 9;
		break;
	case OBJ_Quad:
	case OBJ_CondLine:
		dCoordCount = 12;
		break;
	case OBJ_Vertex:
		dCoordCount = 3;
	default:
		break;
	}
	
	for (short i = 0; i < dCoordCount; ++i) {
		qaCoordinates[i] = new QDoubleSpinBox;
		qaCoordinates[i]->setMaximumWidth (96);
		qaCoordinates[i]->setMinimum (-fMaxCoord);
		qaCoordinates[i]->setMaximum (fMaxCoord);
	}
	
	IMPLEMENT_DIALOG_BUTTONS
	
	QGridLayout* const qLayout = new QGridLayout;
	qLayout->addWidget (qTypeIcon, 0, 0);
	
	switch (type) {
	case OBJ_Comment:
		qLayout->addWidget (qCommentLine, 0, 1);
		break;
	default:
		break;
	}
	
	if (dCoordCount > 0) {
		QGridLayout* const qCoordLayout = new QGridLayout;
		
		for (short i = 0; i < dCoordCount; ++i)
			qCoordLayout->addWidget (qaCoordinates[i], (i / 3), (i % 3));
		
		qLayout->addLayout (qCoordLayout, 0, 1);
	}
	
	qLayout->addWidget (qButtons, 1, 1);
	setLayout (qLayout);
	setWindowTitle (str::mkfmt (APPNAME_DISPLAY " - new %s",
		g_saObjTypeNames[type]).chars());
	
	setWindowIcon (QIcon (str::mkfmt ("icons/add-%s.png", g_saObjTypeIcons[type]).chars ()));
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
			}
			break;
		
		case OBJ_Line:
			{
				LDLine* line = new LDLine;
				line->dColor = dEdgeColor;
				
				APPLY_COORDS (line, 2)
				
				g_CurrentFile->addObject (line);
				window->refresh ();
			}
			break;
		
		case OBJ_Triangle:
			{
				LDTriangle* tri = new LDTriangle;
				tri->dColor = dMainColor;
				
				APPLY_COORDS (tri, 3)
				
				g_CurrentFile->addObject (tri);
				window->refresh ();
			}
			break;
		
		case OBJ_Quad:
			{
				LDQuad* quad = new LDQuad;
				quad->dColor = dMainColor;
				
				APPLY_COORDS (quad, 4)
				
				g_CurrentFile->addObject (quad);
				window->refresh ();
			}
			break;
		
		case OBJ_CondLine:
			{
				LDCondLine* line = new LDCondLine;
				line->dColor = dEdgeColor;
				
				APPLY_COORDS (line, 4)
				
				g_CurrentFile->addObject (line);
				window->refresh ();
			}
			break;
		
		case OBJ_Vertex:
			{
				LDVertex* vert = new LDVertex;
				vert->dColor = dMainColor;
				vert->vPosition.x = dlg.qaCoordinates[0]->value ();
				vert->vPosition.y = dlg.qaCoordinates[1]->value ();
				vert->vPosition.z = dlg.qaCoordinates[2]->value ();
				
				g_CurrentFile->addObject (vert);
				window->refresh ();
			}
			break;
		
		default:
			break;
		}
	}
}