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

void bbox::calculate () {
	if (!g_CurrentFile)
		return;
	
	// The bounding box, bbox for short, is the
	// box that encompasses the model we have open.
	// v0 is the minimum vertex, v1 is the maximum vertex.
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

#define CHECK_DIMENSION(V,X) \
	if (V.X < v0.X) v0.X = V.X; \
	if (V.X > v1.X) v1.X = V.X;
void bbox::checkVertex (vertex v) {
	CHECK_DIMENSION (v, x)
	CHECK_DIMENSION (v, y)
	CHECK_DIMENSION (v, z)
}
#undef CHECK_DIMENSION

bbox::bbox () {
	memset (&v0, 0, sizeof v0);
	memset (&v1, 0, sizeof v1);
}