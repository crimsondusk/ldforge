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

#include <qgridlayout.h>
#include "gui.h"
#include "zz_addObjectDialog.h"
#include "file.h"
#include "colors.h"
#include "zz_colorSelectDialog.h"
#include "history.h"

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
	str zIconName = format ("icons/add-%s.png", g_saObjTypeIcons[type]);
	
	qTypeIcon = new QLabel;
	qTypeIcon->setPixmap (QPixmap (zIconName.chars ()));
	
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
	
	case OBJ_Radial:
		dCoordCount = 3;
		
		qRadialTypeLabel = new QLabel ("Type:");
		qRadialResolutionLabel = new QLabel ("Resolution:");
		qRadialSegmentsLabel = new QLabel ("Segments:");
		qRadialRingNumLabel = new QLabel ("Ring number:");
		
		qRadialType = new QComboBox;
		
		for (int i = 0; i < LDRadial::NumTypes; ++i)
			qRadialType->addItem (LDRadial::radialTypeName ((LDRadial::Type) i));
		
		connect (qRadialType, SIGNAL (currentIndexChanged (int)), this, SLOT (slot_radialTypeChanged (int)));
		
		qRadialResolution = new QComboBox;
		qRadialResolution->addItems ({"Normal (16)", "Hi-Res (48)"});
		
		qRadialSegments = new QSpinBox;
		qRadialSegments->setMinimum (1);
		
		qRadialRingNum = new QSpinBox;
		qRadialRingNum->setEnabled (false);
		break;
	
	default:
		break;
	}
	
	// Show a color edit dialog for the types that actually use the color
	bool bUsesColor = false;
	switch (type) {
	case OBJ_CondLine:
	case OBJ_Line:
	case OBJ_Quad:
	case OBJ_Triangle:
	case OBJ_Vertex:
	case OBJ_Subfile:
	case OBJ_Radial:
		bUsesColor = true;
		break;
	default:
		break;
	}
	
	if (bUsesColor) {
		dColor = (type == OBJ_CondLine || type == OBJ_Line) ? dEdgeColor : dMainColor;
		
		qColorButton = new QPushButton;
		setButtonBackground (qColorButton, dColor);
		connect (qColorButton, SIGNAL (clicked ()), this, SLOT (slot_colorButtonClicked ()));
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
	
	case OBJ_Radial:
		qLayout->addWidget (qRadialTypeLabel, 1, 1);
		qLayout->addWidget (qRadialType, 1, 2);
		qLayout->addWidget (qRadialResolutionLabel, 2, 1);
		qLayout->addWidget (qRadialResolution, 2, 2);
		qLayout->addWidget (qRadialSegmentsLabel, 3, 1);
		qLayout->addWidget (qRadialSegments, 3, 2);
		qLayout->addWidget (qRadialRingNumLabel, 4, 1);
		qLayout->addWidget (qRadialRingNum, 4, 2);
		break;
	
	default:
		break;
	}
	
	if (bUsesColor)
		qLayout->addWidget (qColorButton, 1, 0);
	
	if (dCoordCount > 0) {
		QGridLayout* const qCoordLayout = new QGridLayout;
		
		for (short i = 0; i < dCoordCount; ++i)
			qCoordLayout->addWidget (qaCoordinates[i], (i / 3), (i % 3));
		
		qLayout->addLayout (qCoordLayout, 0, 1, 2, 2);
	}
	
	qLayout->addWidget (qButtons, 5, 1);
	setLayout (qLayout);
	setWindowTitle (format (APPNAME_DISPLAY " - new %s",
		g_saObjTypeNames[type]).chars());
	
	setWindowIcon (QIcon (zIconName.chars ()));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void AddObjectDialog::setButtonBackground (QPushButton* qButton, short dColor) {
	qButton->setIcon (QIcon ("icons/palette.png"));
	qButton->setAutoFillBackground (true);
	qButton->setStyleSheet (
		format ("background-color: %s", getColor (dColor)->zColorString.chars()).chars()
	);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void AddObjectDialog::slot_colorButtonClicked () {
	ColorSelectDialog::staticDialog (dColor, dColor, this);
	setButtonBackground (qColorButton, dColor);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void AddObjectDialog::slot_radialTypeChanged (int dType) {
	LDRadial::Type eType = (LDRadial::Type) dType;
	qRadialRingNum->setEnabled (eType == LDRadial::Ring || eType == LDRadial::Cone);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void AddObjectDialog::staticDialog (const LDObjectType_e type, ForgeWindow* window) {
	AddObjectDialog dlg (type, window);
	LDObject* obj = null;
	
	if (dlg.exec ()) {
		switch (type) {
		case OBJ_Comment:
			obj = new LDComment (dlg.qCommentLine->text ());
			break;
		
		case OBJ_Line:
			{
				LDLine* line = new LDLine;
				line->dColor = dlg.dColor;
				APPLY_COORDS (line, 2)
				obj = line;
			}
			break;
		
		case OBJ_Triangle:
			{
				LDTriangle* tri = new LDTriangle;
				tri->dColor = dlg.dColor;
				APPLY_COORDS (tri, 3)
				obj = tri;
			}
			break;
		
		case OBJ_Quad:
			{
				LDQuad* quad = new LDQuad;
				quad->dColor = dlg.dColor;
				APPLY_COORDS (quad, 4)
				obj = quad;
			}
			break;
		
		case OBJ_CondLine:
			{
				LDCondLine* line = new LDCondLine;
				line->dColor = dlg.dColor;
				APPLY_COORDS (line, 4)
				obj = line;
			}
			break;
		
		case OBJ_Vertex:
			{
				LDVertex* vert = new LDVertex;
				vert->dColor = dlg.dColor;
				vert->vPosition.x = dlg.qaCoordinates[0]->value ();
				vert->vPosition.y = dlg.qaCoordinates[1]->value ();
				vert->vPosition.z = dlg.qaCoordinates[2]->value ();
				obj = vert;
			}
			break;
		
		case OBJ_Radial:
			{
				LDRadial* pRad = new LDRadial;
				pRad->dColor = dlg.dColor;
				pRad->vPosition.x = dlg.qaCoordinates[0]->value ();
				pRad->vPosition.y = dlg.qaCoordinates[1]->value ();
				pRad->vPosition.z = dlg.qaCoordinates[2]->value ();
				pRad->dDivisions = (dlg.qRadialResolution->currentIndex () == 0) ? 16 : 48;
				pRad->dSegments = min<short> (dlg.qRadialSegments->value (), pRad->dDivisions);
				pRad->eRadialType = (LDRadial::Type) dlg.qRadialType->currentIndex ();
				pRad->dRingNum = dlg.qRadialRingNum->value ();
				pRad->mMatrix = g_mIdentity;
				
				obj = pRad;
			}
			break;
		
		default:
			break;
		}
		
		ulong idx = g_CurrentFile->addObject (obj);
		History::addEntry (new AddHistory ({idx}, {obj->clone ()}));
		window->refresh ();
	}
}