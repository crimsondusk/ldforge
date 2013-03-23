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

#include "common.h"
#include "bbox.h"
#include "ldtypes.h"
#include "file.h"

#define CHECK_DIMENSION(V,X) \
	if (V.X < v0.X) v0.X = V.X; \
	if (V.X > v1.X) v1.X = V.X;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void bbox::calculate () {
	reset ();
	
	if (!g_CurrentFile)
		return;
	
	for (uint i = 0; i < g_CurrentFile->objects.size(); i++) {
		LDObject* obj = g_CurrentFile->objects[i];
		switch (obj->getType ()) {
		
		case OBJ_Line:
			{
				LDLine* line = static_cast<LDLine*> (obj);
				for (short i = 0; i < 2; ++i)
					checkVertex (line->vaCoords[i]);
			}
			break;
		
		case OBJ_Triangle:
			{
				LDTriangle* tri = static_cast<LDTriangle*> (obj);
				for (short i = 0; i < 3; ++i)
					checkVertex (tri->vaCoords[i]);
			}
			break;
		
		case OBJ_Quad:
			{
				LDQuad* quad = static_cast<LDQuad*> (obj);
				for (short i = 0; i < 4; ++i)
					checkVertex (quad->vaCoords[i]);
			}
			break;
		
		case OBJ_CondLine:
			{
				LDCondLine* line = static_cast<LDCondLine*> (obj);
				for (short i = 0; i < 4; ++i)
					checkVertex (line->vaCoords[i]);
			}
			break;
		
		default:
			break;
		}
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void bbox::checkVertex (vertex v) {
	CHECK_DIMENSION (v, x)
	CHECK_DIMENSION (v, y)
	CHECK_DIMENSION (v, z)
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bbox::bbox () {
	reset ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void bbox::reset () {
	memset (&v0, 0, sizeof v0);
	memset (&v1, 0, sizeof v1);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
double bbox::calcSize () {
	double fXScale = (v0.x - v1.x);
	double fYScale = (v0.y - v1.y);
	double fZScale = (v0.z - v1.z);
	double fSize = fZScale;
	
	if (fXScale > fYScale) {
		if (fXScale > fZScale)
			fSize = fXScale;
	} else if (fYScale > fZScale)
		fSize = fYScale;
	
	printf ("fsize: %f\n", fSize);
	if (abs (fSize) >= 2.0f)
		return abs (fSize / 2);
	return 1.0f;
}
