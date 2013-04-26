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
#include "ldtypes.h"
#include "file.h"
#include "misc.h"
#include "gui.h"

char const* g_saObjTypeNames[] = {
	"subfile",
	"radial",
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
	"radial",
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
	qObjListEntry = null;
	parent = null;
}

LDGibberish::LDGibberish () {
	dColor = -1;
}

LDGibberish::LDGibberish (str _zContent, str _zReason) {
	zContents = _zContent;
	zReason = _zReason;
	dColor = -1;
}

LDEmpty::LDEmpty () {
	dColor = -1;
}

LDComment::LDComment () {
	dColor = -1;
}

LDSubfile::LDSubfile () {
	
}

LDLine::LDLine () {
	
}

LDTriangle::LDTriangle () {
	
}

LDQuad::LDQuad () {
	
}

LDCondLine::LDCondLine () {
	
}

LDVertex::LDVertex () {
	
}

LDBFC::LDBFC () {
	
}

// =============================================================================
str LDComment::getContents () {
	return format ("0 %s", zText.chars ());
}

str LDSubfile::getContents () {
	str val = format ("1 %d %s ", dColor, vPosition.stringRep (false).chars ());
	val += mMatrix.stringRep ();
	val += ' ';
	val += zFileName;
	return val;
}

str LDLine::getContents () {
	str val = format ("2 %d", dColor);
	
	for (ushort i = 0; i < 2; ++i)
		val.appendformat (" %s", vaCoords[i].stringRep (false).chars ());
	
	return val;
}

str LDTriangle::getContents () {
	str val = format ("3 %d", dColor);
	
	for (ushort i = 0; i < 3; ++i)
		val.appendformat (" %s", vaCoords[i].stringRep (false).chars ());
	
	return val;
}

str LDQuad::getContents () {
	str val = format ("4 %d", dColor);
	
	for (ushort i = 0; i < 4; ++i)
		val.appendformat (" %s", vaCoords[i].stringRep (false).chars ());
	
	return val;
}

str LDCondLine::getContents () {
	str val = format ("5 %d", dColor);
	
	// Add the coordinates
	for (ushort i = 0; i < 4; ++i)
		val.appendformat (" %s", vaCoords[i].stringRep (false).chars ());
	
	return val;
}

str LDGibberish::getContents () {
	return zContents;
}

str LDVertex::getContents () {
	return format ("0 !LDFORGE VERTEX %d %s", dColor, vPosition.stringRep (false).chars());
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
	return format ("0 BFC %s", LDBFC::saStatements[eStatement]);
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
	vaCoords[0] = v1;
	vaCoords[1] = v2;
}

LDObject::~LDObject () {
	// Remove this object from the selection array if it is there.
	for (ulong i = 0; i < g_ForgeWindow->sel.size(); ++i)
		if (g_ForgeWindow->sel[i] == this)
			g_ForgeWindow->sel.erase (g_ForgeWindow->sel.begin() + i);
}

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
LDRadial::~LDRadial () {}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void transformObject (LDObject* obj, matrix transform, vertex pos, short parentcolor) {
	switch (obj->getType()) {
	case OBJ_Line:
	case OBJ_CondLine:
	case OBJ_Triangle:
	case OBJ_Quad:
		for (short i = 0; i < obj->vertices (); ++i)
			obj->vaCoords[i].transform (transform, pos);
		break;
	
	case OBJ_Subfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			
			matrix mNewMatrix = transform * ref->mMatrix;
			ref->vPosition.transform (transform, pos);
			ref->mMatrix = mNewMatrix;
		}
		break;
	
	default:
		break;
	}
	
	if (obj->dColor == dMainColor)
		obj->dColor = parentcolor;
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
				if (static_cast<LDBFC*> (obj)->eStatement != LDBFC::InvertNext)
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
	for (LDObject* obj : objs) {
		// Set the parent now so we know what inlined this.
		obj->parent = this;
		
		transformObject (obj, mMatrix, vPosition, dColor);
	}
	
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
str LDObject::objectListContents (const std::vector<LDObject*>& objs) {
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
			
			str zNoun = format ("%s%s", g_saObjTypeNames[eType], PLURAL (ulCount));
			
			// Plural of "vertex" is "vertices". Stupid English.
			if (eType == OBJ_Vertex && ulCount != 1)
				zNoun = "vertices";
			
			zText.appendformat ("%lu %s", ulCount, zNoun.chars ());
			bFirstDetails = false;
		}
	}
	
	return zText;
}

// =============================================================================
LDObject* LDObject::topLevelParent () {
	if (!parent)
		return null;
	
	LDObject* it = this;
	
	while (it->parent)
		it = it->parent;
	
	return it;
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

void LDRadial::move (vertex vVector) {
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

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
LDRadial::LDRadial () {
	
}

static char const* g_saRadialTypeNames[] = {
	"Circle",
	"Cylinder",
	"Disc",
	"Disc Negative",
	"Ring",
	"Cone",
	null
};

char const* LDRadial::radialTypeName () {
	return g_saRadialTypeNames[eRadialType];
}

char const* LDRadial::radialTypeName (const LDRadial::Type eType) {
	return g_saRadialTypeNames[eType];
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
std::vector<LDObject*> LDRadial::decompose (bool bTransform) {
	std::vector<LDObject*> paObjects;
	
	for (short i = 0; i < dSegments; ++i) {
		double x0 = cos ((i * 2 * pi) / dDivisions),
			x1 = cos (((i + 1) * 2 * pi) / dDivisions),
			z0 = sin ((i * 2 * pi) / dDivisions),
			z1 = sin (((i + 1) * 2 * pi) / dDivisions);
		
		switch (eRadialType) {
		case LDRadial::Circle:
			{
				vertex v0 (x0, 0.0f, z0),
					v1 (x1, 0.0f, z1);
				
				if (bTransform) {
					v0.transform (mMatrix, vPosition);
					v1.transform (mMatrix, vPosition);
				}
				
				LDLine* pLine = new LDLine;
				pLine->vaCoords[0] = v0;
				pLine->vaCoords[1] = v1;
				pLine->dColor = dEdgeColor;
				pLine->parent = this;
				
				paObjects.push_back (pLine);
			}
			break;
		
		case LDRadial::Cylinder:
		case LDRadial::Ring:
		case LDRadial::Cone:
			{
				double x2, x3, z2, z3;
				double y0, y1, y2, y3;
				
				if (eRadialType == LDRadial::Cylinder) {
					x2 = x1;
					x3 = x0;
					z2 = z1;
					z3 = z0;
					
					y0 = y1 = 0.0f;
					y2 = y3 = 1.0f;
				} else {
					x2 = x1 * (dRingNum + 1);
					x3 = x0 * (dRingNum + 1);
					z2 = z1 * (dRingNum + 1);
					z3 = z0 * (dRingNum + 1);
					
					x0 *= dRingNum;
					x1 *= dRingNum;
					z0 *= dRingNum;
					z1 *= dRingNum;
					
					if (eRadialType == LDRadial::Ring) {
						y0 = y1 = y2 = y3 = 0.0f;
					} else {
						y0 = y1 = 1.0f;
						y2 = y3 = 0.0f;
					} 
				}
				
				vertex v0 (x0, y0, z0),
					v1 (x1, y1, z1),
					v2 (x2, y2, z2),
					v3 (x3, y3, z3);
				
				if (bTransform) {
					v0.transform (mMatrix, vPosition);
					v1.transform (mMatrix, vPosition);
					v2.transform (mMatrix, vPosition);
					v3.transform (mMatrix, vPosition);
				}
				
				LDQuad* pQuad = new LDQuad;
				pQuad->vaCoords[0] = v0;
				pQuad->vaCoords[1] = v1;
				pQuad->vaCoords[2] = v2;
				pQuad->vaCoords[3] = v3;
				pQuad->dColor = dColor;
				pQuad->parent = this;
				
				paObjects.push_back (pQuad);
			}
			break;
		
		case LDRadial::Disc:
		case LDRadial::DiscNeg:
			{
				double x2, z2;
				
				if (eRadialType == LDRadial::Disc) {
					x2 = z2 = 0.0f;
				} else {
					x2 = (x0 >= 0.0f) ? 1.0f : -1.0f;
					z2 = (z0 >= 0.0f) ? 1.0f : -1.0f;
				}
				
				vertex v0 (x0, 0.0f, z0),
					v1 (x1, 0.0f, z1),
					v2 (x2, 0.0f, z2);
				
				if (bTransform) {
					v0.transform (mMatrix, vPosition);
					v1.transform (mMatrix, vPosition);
					v2.transform (mMatrix, vPosition);
				}
				
				LDTriangle* pSeg = new LDTriangle;
				pSeg->vaCoords[0] = v0;
				pSeg->vaCoords[1] = v1;
				pSeg->vaCoords[2] = v2;
				pSeg->dColor = dColor;
				pSeg->parent = this;
				
				paObjects.push_back (pSeg);
			}
			break;
		
		default:
			break;
		}
	}
	
	return paObjects;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str LDRadial::getContents () {
	return format ("0 !LDFORGE RADIAL %s %d %d %d %d %s %s",
		str (radialTypeName()).toupper ().strip (' ').chars (),
		dColor, dSegments, dDivisions, dRingNum,
		vPosition.stringRep (false).chars(), mMatrix.stringRep().chars());
}

char const* g_saRadialNameRoots[] = {
	"edge",
	"cyli",
	"disc",
	"ndis",
	"ring",
	"cone",
	null
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str LDRadial::makeFileName () {
	short dNumerator = dSegments,
		dDenominator = dDivisions;
	
	// Simplify the fractional part, but the denominator is at least 4.
	simplify (dNumerator, dDenominator);
	
	if (dDenominator < 4) {
		const short dFactor = (4 / dDenominator);
		
		dNumerator *= dFactor;
		dDenominator *= dFactor;
	}
	
	// Compose some general information: prefix, fraction, root, ring number
	str zPrefix = (dDivisions == 16) ? "" : format ("%d/", dDivisions);
	str zFrac = format ("%d-%d", dNumerator, dDenominator);
	str zRoot = g_saRadialNameRoots[eRadialType];
	str zRingNum = (eRadialType == Ring || eRadialType == Cone) ? format ("%d", dRingNum) : "";
	
	// Truncate the root if necessary (7-16rin4.dat for instance).
	// However, always keep the root at least 2 characters.
	short dExtra = (~zFrac + ~zRingNum + ~zRoot) - 8;
	zRoot -= min<short> (max<short> (dExtra, 0), 2);
	
	// Stick them all together and return the result.
	return format ("%s%s%s%s", zPrefix.chars(), zFrac.chars (), zRoot.chars (), zRingNum.chars ());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
#define CHECK_FOR_OBJ(N) \
	if (type == OBJ_##N) \
		return new LD##N;
LDObject* LDObject::getDefault (const LDObjectType_e type) {
	CHECK_FOR_OBJ (Comment)
	CHECK_FOR_OBJ (BFC)
	CHECK_FOR_OBJ (Line)
	CHECK_FOR_OBJ (CondLine)
	CHECK_FOR_OBJ (Radial)
	CHECK_FOR_OBJ (Subfile)
	CHECK_FOR_OBJ (Triangle)
	CHECK_FOR_OBJ (Quad)
	CHECK_FOR_OBJ (Empty)
	CHECK_FOR_OBJ (BFC)
	CHECK_FOR_OBJ (Gibberish)
	CHECK_FOR_OBJ (Vertex)
	return null;
}