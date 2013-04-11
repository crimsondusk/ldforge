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
	"subfile",
	"quadrilateral",
	"triangle",
	"line",
	"condline",
	"vertex",
	"bfc",
	"comment",
	"unknown",
	"empty",
	"unidentified",
};

// Should probably get rid of this array sometime
char const* g_saObjTypeIcons[] = {
	"subfile",
	"quad",
	"triangle",
	"line",
	"condline",
	"vertex",
	"bfc",
	"comment",
	"error",
	"empty",
	"error",
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

LDBFC::LDBFC () {
	commonInit ();
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
const char* LDBFC::saStatements[] = {
	"CERTIFY CCW",
	"CCW",
	"CERTIFY CW",
	"CW",
	"NOCERTIFY",
	"INVERTNEXT",
};

str LDBFC::getContents () {
	return str::mkfmt ("0 BFC %s", LDBFC::saStatements[dStatement]);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
vector<LDTriangle*> LDQuad::splitToTriangles () {
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
	
	vector<LDTriangle*> triangles;
	triangles.push_back (tri1);
	triangles.push_back (tri2);
	return triangles;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::replace (LDObject* replacement) {
	// Replace all instances of the old object with the new object
	for (LDObject*& obj : g_CurrentFile->objects)
		if (obj == this)
			obj = replacement;
	
	// Remove the old object
	delete this;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::swap (LDObject* other) {
	for (LDObject*& obj : g_CurrentFile->objects) {
		if (obj == this)
			obj = other;
		else if (obj == other)
			obj = this;
	}
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
LDBFC::~LDBFC () {}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
template<class T> static void transformSubObject (LDObject* obj, matrix mMatrix,
	vertex vPos, short dColor)
{
	T* newobj = static_cast<T*> (obj);
	for (short i = 0; i < (short)(sizeof newobj->vaCoords / sizeof *newobj->vaCoords); ++i)
		newobj->vaCoords[i].transform (mMatrix, vPos);
	
	if (newobj->dColor == dMainColor)
		newobj->dColor = dColor;
}

// -----------------------------------------------------------------------------
static void transformObject (LDObject* obj, matrix mMatrix, vertex vPos,
	short dColor)
{
	switch (obj->getType()) {
	case OBJ_Line:
		transformSubObject<LDLine> (obj, mMatrix, vPos, dColor);
		break;
	
	case OBJ_CondLine:
		transformSubObject<LDCondLine> (obj, mMatrix, vPos, dColor);
		break;
	
	case OBJ_Triangle:
		transformSubObject<LDTriangle> (obj, mMatrix, vPos, dColor);
		break;
	
	case OBJ_Quad:
		transformSubObject<LDQuad> (obj, mMatrix, vPos, dColor);
		break;
	
	case OBJ_Subfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			
			matrix mNewMatrix = mMatrix * ref->mMatrix;
			ref->vPosition.transform (mMatrix, vPos);
			ref->mMatrix = mNewMatrix;
		}
		
		break;
	
	default:
		break;
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
vector<LDObject*> LDSubfile::inlineContents (bool bDeepInline, bool bCache) {
	vector<LDObject*> objs, cache;
	
	// If we have this cached, just clone that
	if (bDeepInline && pFile->objCache.size ()) {
		for (LDObject* obj : pFile->objCache)
			objs.push_back (obj->clone ());
	} else {
		if (!bDeepInline)
			bCache = false;
		
		for (LDObject* obj : pFile->objects) {
			// Skip those without schemantic meaning
			switch (obj->getType ()) {
			case OBJ_Comment:
			case OBJ_Empty:
			case OBJ_Gibberish:
			case OBJ_Unidentified:
			case OBJ_Vertex:
				continue;
			
			case OBJ_BFC:
				// Filter non-INVERTNEXT statements
				if (static_cast<LDBFC*> (obj)->dStatement != BFC_InvertNext)
					continue;
				break;
			
			default:
				break;
			}
			
			// Got another sub-file reference, inline it if we're deep-inlining. If not,
			// just add it into the objects normally. Also, we only cache immediate
			// subfiles and this is not one. Yay, recursion!
			if (bDeepInline && obj->getType() == OBJ_Subfile) {
				LDSubfile* ref = static_cast<LDSubfile*> (obj);
				
				vector<LDObject*> otherobjs = ref->inlineContents (true, false);
				
				for (LDObject* otherobj : otherobjs) {
					// Cache this object, if desired
					if (bCache)
						cache.push_back (otherobj->clone ());
					
					objs.push_back (otherobj);
				}
			} else {
				if (bCache)
					cache.push_back (obj->clone ());
				
				objs.push_back (obj->clone ());
			}
		}
		
		if (bCache)
			pFile->objCache = cache;
	}
	
	// Transform the objects
	for (LDObject* obj : objs)
		transformObject (obj, mMatrix, vPosition, dColor);
	
	return objs;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
long LDObject::getIndex (OpenFile* pFile) {
	for (ulong i = 0; i < pFile->objects.size(); ++i)
		if (pFile->objects[i] == this)
			return i;
	
	return -1;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::moveObjects (std::vector<LDObject*> objs, const bool bUp) {
	// If we move down, we need to iterate the array in reverse order.
	const long start = bUp ? 0 : (objs.size() - 1);
	const long end = bUp ? objs.size() : -1;
	const long incr = bUp ? 1 : -1;
	
	for (long i = start; i != end; i += incr) {
		LDObject* obj = objs[i];
		
		const long lIndex = obj->getIndex (g_CurrentFile),
			lTarget = lIndex + (bUp ? -1 : 1);
		
		if ((bUp == true and lIndex == 0) or
			(bUp == false and lIndex == (long)(g_CurrentFile->objects.size() - 1)))
		{
			// One of the objects hit the extrema. If this happens, this should be the first
			// object to be iterated on. Thus, nothing has changed yet and it's safe to just
			// abort the entire operation.
			assert (i == start);
			return;
		}
		
		obj->swap (g_CurrentFile->objects[lTarget]);
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str LDObject::objectListContents (std::vector<LDObject*>& objs) {
	bool bFirstDetails = true;
	str zText = "";
	
	if (objs.size() == 0)
		return "nothing"; // :)
	
	for (long i = 0; i < NUM_ObjectTypes; ++i) {
		LDObjectType_e eType = (LDObjectType_e) i;
		ulong ulCount = 0;
		
		for (LDObject* obj : objs)
			if (obj->getType() == eType)
				ulCount++;
		
		if (ulCount > 0) {
			if (!bFirstDetails)
				zText += ", ";
			
			zText.appendformat ("%lu %s%s", ulCount, g_saObjTypeNames[eType], PLURAL (ulCount));
			bFirstDetails = false;
		}
	}
	
	return zText;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::move (vertex vVector) { vVector = vVector; /* to shut up GCC */ }
void LDEmpty::move (vertex vVector) { vVector = vVector; }
void LDBFC::move (vertex vVector) { vVector = vVector; }
void LDComment::move (vertex vVector) { vVector = vVector; }
void LDGibberish::move (vertex vVector) { vVector = vVector; }

void LDVertex::move (vertex vVector) {
	vPosition += vVector;
}

void LDSubfile::move (vertex vVector) {
	vPosition += vVector;
}

void LDLine::move (vertex vVector) {
	for (short i = 0; i < 2; ++i)
		vaCoords[i] += vVector;
}

void LDTriangle::move (vertex vVector) {
	for (short i = 0; i < 3; ++i)
		vaCoords[i] += vVector;
}

void LDQuad::move (vertex vVector) {
	for (short i = 0; i < 4; ++i)
		vaCoords[i] += vVector;
}

void LDCondLine::move (vertex vVector) {
	for (short i = 0; i < 4; ++i)
		vaCoords[i] += vVector;
}