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

#include "common.h"
#include "bbox.h"
#include "ldtypes.h"
#include "file.h"

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bbox::bbox () {
	reset ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void bbox::calculate () {
	reset ();
	
	if (!g_curfile)
		return;
	
	for (LDObject* obj : g_curfile->m_objs)
		calcObject (obj);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void bbox::calcObject (LDObject* obj) {
	switch (obj->getType ()) {
	case OBJ_Line:
		{
			LDLine* line = static_cast<LDLine*> (obj);
			for (short i = 0; i < 2; ++i)
				calcVertex (line->vaCoords[i]);
		}
		break;
	
	case OBJ_Triangle:
		{
			LDTriangle* tri = static_cast<LDTriangle*> (obj);
			for (short i = 0; i < 3; ++i)
				calcVertex (tri->vaCoords[i]);
		}
		break;
	
	case OBJ_Quad:
		{
			LDQuad* quad = static_cast<LDQuad*> (obj);
			for (short i = 0; i < 4; ++i)
				calcVertex (quad->vaCoords[i]);
		}
		break;
	
	case OBJ_CondLine:
		{
			LDCondLine* line = static_cast<LDCondLine*> (obj);
			for (short i = 0; i < 4; ++i)
				calcVertex (line->vaCoords[i]);
		}
		break;
	
	case OBJ_Subfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			vector<LDObject*> objs = ref->inlineContents (true, true);
			
			for (LDObject* obj : objs) {
				calcObject (obj);
				delete obj;
			}
		}
		break;
	
	case OBJ_Radial:
		{
			LDRadial* rad = static_cast<LDRadial*> (obj);
			vector<LDObject*> objs = rad->decompose (true);
			
			for (LDObject* obj : objs) {
				calcObject (obj);
				delete obj;
			}
		}
		break;
	
	default:
		break;
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void bbox::calcVertex (vertex v) {
	for (const Axis ax : g_Axes) {
		if (v[ax] < m_v0[ax])
			m_v0[ax] = v[ax];
		
		if (v[ax] > m_v1[ax])
			m_v1[ax] = v[ax];
	}
	
	m_empty = false;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void bbox::reset () {
	m_v0[X] = m_v0[Y] = m_v0[Z] = +0x7FFFFFFF;
	m_v1[X] = m_v1[Y] = m_v1[Z] = -0x7FFFFFFF;
	
	m_empty = true;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
double bbox::size () const {
	double fXScale = (m_v0[X] - m_v1[X]);
	double fYScale = (m_v0[Y] - m_v1[Y]);
	double fZScale = (m_v0[Z] - m_v1[Z]);
	double fSize = fZScale;
	
	if (fXScale > fYScale) {
		if (fXScale > fZScale)
			fSize = fXScale;
	} else if (fYScale > fZScale)
		fSize = fYScale;
	
	if (abs (fSize) >= 2.0f)
		return abs (fSize / 2);
	
	return 1.0f;
}

// =============================================================================
vertex bbox::center () const {
	return vertex (
		(m_v0[X] + m_v1[X]) / 2,
		(m_v0[Y] + m_v1[Y]) / 2,
		(m_v0[Z] + m_v1[Z]) / 2);
}

// =============================================================================
bool bbox::empty() const {
	return m_empty;
}