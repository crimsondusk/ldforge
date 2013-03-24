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
#include "ldtypes.h"
#include "file.h"
#include "misc.h"

char const* g_saObjTypeNames[] = {
	"unidentified",
	"unknown",
	"empty",
	"comment",
	"subfile",
	"line",
	"triangle",
	"quadrilateral",
	"condline",
	"vertex",
};

char const* g_saObjTypeIcons[] = {
	"error",
	"error",
	"empty",
	"comment",
	"subfile",
	"line",
	"triangle",
	"quad",
	"condline",
	"vertex"
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// LDObject constructors
LDObject::LDObject () {
	commonInit ();
}

void LDObject::commonInit () {
	qObjListEntry = nullptr;
}

LDGibberish::LDGibberish () {
	commonInit ();
	dColor = -1;
}

LDGibberish::LDGibberish (str _zContent, str _zReason) {
	zContents = _zContent;
	zReason = _zReason;
	dColor = -1;
	
	commonInit ();
}

LDEmpty::LDEmpty () {
	commonInit ();
	dColor = -1;
}

LDComment::LDComment () {
	commonInit ();
	dColor = -1;
}

LDSubfile::LDSubfile () {
	commonInit ();
}

LDLine::LDLine () {
	commonInit ();
}

LDTriangle::LDTriangle () {
	commonInit ();
}

LDQuad::LDQuad () {
	commonInit ();
}

LDCondLine::LDCondLine () {
	commonInit ();
}

LDVertex::LDVertex () {
	commonInit ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ulong LDObject::getIndex () {
	if (!g_CurrentFile)
		return -1u;
	
	// TODO: shouldn't rely on g_CurrentFile
	for (ulong i = 0; i < g_CurrentFile->objects.size(); ++i) {
		if (g_CurrentFile->objects[i] == this)
			return i;
	}
	
	return -1u;
}

// =============================================================================
str LDComment::getContents () {
	return str::mkfmt ("0 %s", zText.chars ());
}

str LDSubfile::getContents () {
	str val = str::mkfmt ("1 %d %s ", dColor, vPosition.getStringRep (false).chars ());
	
	for (short i = 0; i < 9; ++i)
		val.appendformat ("%s ", ftoa (mMatrix[i]).chars());
	
	val += zFileName;
	return val;
}

str LDLine::getContents () {
	str val = str::mkfmt ("2 %d", dColor);
	
	for (ushort i = 0; i < 2; ++i)
		val.appendformat (" %s", vaCoords[i].getStringRep (false).chars ());
	
	return val;
}

str LDTriangle::getContents () {
	str val = str::mkfmt ("3 %d", dColor);
	
	for (ushort i = 0; i < 3; ++i)
		val.appendformat (" %s", vaCoords[i].getStringRep (false).chars ());
	
	return val;
}

str LDQuad::getContents () {
	str val = str::mkfmt ("4 %d", dColor);
	
	for (ushort i = 0; i < 4; ++i)
		val.appendformat (" %s", vaCoords[i].getStringRep (false).chars ());
	
	return val;
}

str LDCondLine::getContents () {
	str val = str::mkfmt ("5 %d", dColor);
	
	// Add the coordinates
	for (ushort i = 0; i < 4; ++i)
		val.appendformat (" %s", vaCoords[i].getStringRep (false).chars ());
	
	return val;
}

str LDGibberish::getContents () {
	return zContents;
}

str LDVertex::getContents () {
	return str::mkfmt ("0 !LDFORGE VERTEX %d %s", dColor, vPosition.getStringRep (false).chars());
}

str LDEmpty::getContents () {
	return str ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDQuad::splitToTriangles () {
	// Find the index of this quad
	long lIndex = getIndex (g_CurrentFile);
	
	if (lIndex == -1) {
		// couldn't find it?
		logf (LOG_Error, "LDQuad::splitToTriangles: Couldn't find quad %p in "
			"current object list!!\n", this);
		return;
	}
	
	// Create the two triangles based on this quadrilateral:
	// 0---3     0---3    3
	// |   |     |  /    /|
	// |   |  =  | /    / |
	// |   |     |/    /  |
	// 1---2     1    1---2
	LDTriangle* tri1 = new LDTriangle;
	tri1->vaCoords[0] = vaCoords[0];
	tri1->vaCoords[1] = vaCoords[1];
	tri1->vaCoords[2] = vaCoords[3];
	
	LDTriangle* tri2 = new LDTriangle;
	tri2->vaCoords[0] = vaCoords[1];
	tri2->vaCoords[1] = vaCoords[2];
	tri2->vaCoords[2] = vaCoords[3];
	
	// The triangles also inherit the quad's color
	tri1->dColor = tri2->dColor = dColor;
	
	// Replace the quad with the first triangle and add the second triangle
	// after the first one.
	g_CurrentFile->objects[lIndex] = tri1;
	g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + lIndex + 1, tri2);
	
	// Delete this quad now, it has been split.
	delete this;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::replace (LDObject* replacement) {
	// Replace all instances of the old object with the new object
	for (ulong i = 0; i < g_CurrentFile->objects.size(); ++i) {
		if (g_CurrentFile->objects[i] == this)
			g_CurrentFile->objects[i] = replacement;
	}
	
	// Remove the old object
	delete this;
}

LDLine::LDLine (vertex v1, vertex v2) {
	commonInit ();
	
	vaCoords[0] = v1;
	vaCoords[1] = v2;
}

LDObject::~LDObject () {}
LDComment::~LDComment () {}
LDCondLine::~LDCondLine () {}
LDEmpty::~LDEmpty () {}
LDGibberish::~LDGibberish () {}
LDLine::~LDLine () {}
LDQuad::~LDQuad () {}
LDSubfile::~LDSubfile () {}
LDTriangle::~LDTriangle () {}
LDVertex::~LDVertex () {}

#define ADD_TYPE(T,N) \
	case OBJ_##T: \
			{ \
				LD##T* newobj = static_cast<LD##T*> (obj)->makeClone (); \
				for (short i = 0; i < N; ++i) \
					newobj->vaCoords[i].transform (mMatrix, pos); \
				\
				objs.push_back (newobj); \
			} \
			break;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static uint g_uTabs = 0;
vector<LDObject*> LDSubfile::inlineContents (bool bDeepInline, matrix mMatrix, vertex pos, bool bCache) {
	// If we have this cached, just return that.
	if (bDeepInline && objCache.size ())
		return objCache;
	
	vector<LDObject*> objs;
	
	FOREACH (LDObject, *, obj, pFile->objects) {
		switch (obj->getType()) {
		case OBJ_Comment:
		case OBJ_Empty:
		case OBJ_Gibberish:
		case OBJ_Unidentified:
		case OBJ_Vertex:
			break; // Skip non-essentials
		
		ADD_TYPE (Line, 2)
		ADD_TYPE (Triangle, 3)
		ADD_TYPE (Quad, 4)
		ADD_TYPE (CondLine, 4)
		
		case OBJ_Subfile:
			{
				LDSubfile* ref = static_cast<LDSubfile*> (obj);
				
				// Got another sub-file reference, inline it if we're deep-inlining. If not,
				// just add it into the objects normally.
				if (bDeepInline) {
					matrix mNewMatrix = mMatrix * ref->mMatrix;
					vertex vNewPos = ref->vPosition;
					vNewPos.transform (mMatrix, pos);
					
					// Only cache immediate subfiles, this is not one. Yay recursion!
					g_uTabs++;
					vector<LDObject*> otherobjs = ref->inlineContents (true, mNewMatrix, vNewPos, false);
					g_uTabs--;
					
					for (ulong i = 0; i < otherobjs.size(); ++i)
						objs.push_back (otherobjs[i]);
				} else {
					LDSubfile* clone = ref->makeClone ();
					clone->vPosition.transform (mMatrix, pos);
					clone->mMatrix *= mMatrix;
					
					objs.push_back (clone);
				}
			}
			
			break;
		}
	}
	
	// If we cache this stuff, keep it around
	if (bCache)
		objCache = objs;
	
	return objs;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
long LDObject::getIndex (OpenFile* pFile) {
	long lIndex;
	
	for (lIndex = 0; lIndex < (long)pFile->objects.size(); ++lIndex)
		if (pFile->objects[lIndex] == this)
			return lIndex;
	
	return -1;
}