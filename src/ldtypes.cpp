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
#include "history.h"
#include "gldraw.h"

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
	m_hidden = false;
	m_selected = false;
	m_glinit = false;
}

LDGibberish::LDGibberish (str _zContent, str _zReason) {
	contents = _zContent;
	reason = _zReason;
}

// =============================================================================
str LDComment::getContents () {
	return fmt ("0 %s", text.chars ());
}

str LDSubfile::getContents () {
	str val = fmt ("1 %d %s ", color, pos.stringRep (false).chars ());
	val += transform.stringRep ();
	val += ' ';
	val += fileName;
	return val;
}

str LDLine::getContents () {
	str val = fmt ("2 %d", color);
	
	for (ushort i = 0; i < 2; ++i)
		val += fmt  (" %s", coords[i].stringRep (false).chars ());
	
	return val;
}

str LDTriangle::getContents () {
	str val = fmt ("3 %d", color);
	
	for (ushort i = 0; i < 3; ++i)
		val += fmt  (" %s", coords[i].stringRep (false).chars ());
	
	return val;
}

str LDQuad::getContents () {
	str val = fmt ("4 %d", color);
	
	for (ushort i = 0; i < 4; ++i)
		val += fmt  (" %s", coords[i].stringRep (false).chars ());
	
	return val;
}

str LDCondLine::getContents () {
	str val = fmt ("5 %d", color);
	
	// Add the coordinates
	for (ushort i = 0; i < 4; ++i)
		val += fmt  (" %s", coords[i].stringRep (false).chars ());
	
	return val;
}

str LDGibberish::getContents () {
	return contents;
}

str LDVertex::getContents () {
	return fmt ("0 !LDFORGE VERTEX %d %s", color, pos.stringRep (false).chars());
}

str LDEmpty::getContents () {
	return str ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
const char* LDBFC::statements[] = {
	"CERTIFY CCW",
	"CCW",
	"CERTIFY CW",
	"CW",
	"NOCERTIFY",
	"INVERTNEXT",
};

str LDBFC::getContents () {
	return fmt ("0 BFC %s", LDBFC::statements[type]);
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
	tri1->coords[0] = coords[0];
	tri1->coords[1] = coords[1];
	tri1->coords[2] = coords[3];
	
	LDTriangle* tri2 = new LDTriangle;
	tri2->coords[0] = coords[1];
	tri2->coords[1] = coords[2];
	tri2->coords[2] = coords[3];
	
	// The triangles also inherit the quad's color
	tri1->color = tri2->color = color;
	
	vector<LDTriangle*> triangles;
	triangles << tri1;
	triangles << tri2;
	return triangles;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::replace (LDObject* replacement) {
	// Replace the instance of the old object with the new object
	for (LDObject*& obj : g_curfile->objs ()) {
		if (obj == this) {
			obj = replacement;
			break;
		}
	}
	
	// Remove the old object
	delete this;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::swap (LDObject* other) {
	for (LDObject*& obj : g_curfile->objs ()) {
		if (obj == this)
			obj = other;
		else if (obj == other)
			obj = this;
	}
}

LDLine::LDLine (vertex v1, vertex v2) {
	coords[0] = v1;
	coords[1] = v2;
}

LDObject::~LDObject () {
	// Remove this object from the selection array if it is there.
	for (ulong i = 0; i < g_win->sel ().size(); ++i)
		if (g_win->sel ()[i] == this)
			g_win->sel ().erase (i);
	
	// Delete the GL lists
	GL::deleteLists (this);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void transformObject (LDObject* obj, matrix transform, vertex pos, short parentcolor) {
	switch (obj->getType()) {
	case LDObject::Line:
	case LDObject::CondLine:
	case LDObject::Triangle:
	case LDObject::Quad:
		for (short i = 0; i < obj->vertices (); ++i)
			obj->coords[i].transform (transform, pos);
		break;
	
	case LDObject::Subfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			
			matrix newMatrix = transform * ref->transform;
			ref->pos.transform (transform, pos);
			ref->transform = newMatrix;
		}
		break;
	
	default:
		break;
	}
	
	if (obj->color == maincolor)
		obj->color = parentcolor;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
vector<LDObject*> LDSubfile::inlineContents (bool deep, bool cache) {
	vector<LDObject*> objs, objcache;
	
	// If we have this cached, just clone that
	if (deep && fileInfo->cache ().size ()) {
		for (LDObject* obj : fileInfo->cache ())
			objs << obj->clone ();
	} else {
		if (!deep)
			cache = false;
		
		for (LDObject* obj : fileInfo->objs ()) {
			// Skip those without schemantic meaning
			switch (obj->getType ()) {
			case LDObject::Comment:
			case LDObject::Empty:
			case LDObject::Gibberish:
			case LDObject::Unidentified:
			case LDObject::Vertex:
				continue;
			
			case LDObject::BFC:
				// Filter non-INVERTNEXT statements
				if (static_cast<LDBFC*> (obj)->type != LDBFC::InvertNext)
					continue;
				break;
			
			default:
				break;
			}
			
			// Got another sub-file reference, inline it if we're deep-inlining. If not,
			// just add it into the objects normally. Also, we only cache immediate
			// subfiles and this is not one. Yay, recursion!
			if (deep && obj->getType() == LDObject::Subfile) {
				LDSubfile* ref = static_cast<LDSubfile*> (obj);
				
				vector<LDObject*> otherobjs = ref->inlineContents (true, false);
				
				for (LDObject* otherobj : otherobjs) {
					// Cache this object, if desired
					if (cache)
						objcache << otherobj->clone ();
					
					objs << otherobj;
				}
			} else {
				if (cache)
					objcache << obj->clone ();
				
				objs << obj->clone ();
			}
		}
		
		if (cache)
			fileInfo->setCache (objcache);
	}
	
	// Transform the objects
	for (LDObject* obj : objs) {
		// Set the parent now so we know what inlined this.
		obj->parent = this;
		
		transformObject (obj, transform, pos, color);
	}
	
	return objs;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
long LDObject::getIndex (LDOpenFile* file) const {
	for (ulong i = 0; i < file->numObjs (); ++i)
		if (file->obj (i) == this)
			return i;
	
	return -1;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::moveObjects (vector<LDObject*> objs, const bool bUp) {
	// If we move down, we need to iterate the array in reverse order.
	const long start = bUp ? 0 : (objs.size() - 1);
	const long end = bUp ? objs.size() : -1;
	const long incr = bUp ? 1 : -1;
	
	vector<LDObject*> objsToCompile;
	
	for (long i = start; i != end; i += incr) {
		LDObject* obj = objs[i];
		
		const long idx = obj->getIndex (g_curfile),
			target = idx + (bUp ? -1 : 1);
		
		if ((bUp == true and idx == 0) or
			(bUp == false and idx == (long)(g_curfile->objs ().size() - 1)))
		{
			// One of the objects hit the extrema. If this happens, this should be the first
			// object to be iterated on. Thus, nothing has changed yet and it's safe to just
			// abort the entire operation.
			assert (i == start);
			return;
		}
		
		objsToCompile << obj;
		objsToCompile << g_curfile->obj (target);
		
		obj->swap (g_curfile->obj (target));
	}
	
	// The objects need to be recompiled, otherwise their pick lists are left with
	// the wrong index colors which messes up selection.
	for (LDObject* obj : objsToCompile)
		g_win->R ()->compileObject (obj);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str LDObject::objectListContents (const vector<LDObject*>& objs) {
	bool firstDetails = true;
	str text = "";
	
	if (objs.size() == 0)
		return "nothing"; // :)
	
	for (long i = 0; i < LDObject::NumTypes; ++i) {
		LDObject::Type objType = (LDObject::Type) i;
		ulong objCount = 0;
		
		for (LDObject* obj : objs)
			if (obj->getType() == objType)
				objCount++;
		
		if (objCount == 0)
			continue;
		
		if (!firstDetails)
			text += ", ";
		
		str noun = fmt ("%s%s", g_saObjTypeNames[objType], plural (objCount));
		
		// Plural of "vertex" is "vertices". Stupid English.
		if (objType == LDObject::Vertex && objCount != 1)
			noun = "vertices";
		
		text += fmt  ("%lu %s", objCount, noun.chars ());
		firstDetails = false;
	}
	
	return text;
}

// =============================================================================
LDObject* LDObject::topLevelParent () {
	if (!parent)
		return this;
	
	LDObject* it = this;
	
	while (it->parent)
		it = it->parent;
	
	return it;
}

// =============================================================================
LDObject* LDObject::next () const {
	long idx = getIndex (g_curfile);
	assert (idx != -1);
	
	if (idx == (long) g_curfile->numObjs () - 1)
		return null;
	
	return g_curfile->obj (idx + 1);
}

// =============================================================================
LDObject* LDObject::prev () const {
	long idx = getIndex (g_curfile);
	assert (idx != -1);
	
	if (idx == 0)
		return null;
	
	return g_curfile->obj (idx - 1);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::move (vertex vect) { (void) vect; }
void LDEmpty::move (vertex vect) { (void) vect; }
void LDBFC::move (vertex vect) { (void) vect; }
void LDComment::move (vertex vect) { (void) vect; }
void LDGibberish::move (vertex vect) { (void) vect; }

void LDVertex::move (vertex vect) {
	pos += vect;
}

void LDSubfile::move (vertex vect) {
	pos += vect;
}

void LDRadial::move (vertex vect) {
	pos += vect;
}

void LDLine::move (vertex vect) {
	for (short i = 0; i < 2; ++i)
		coords[i] += vect;
}

void LDTriangle::move (vertex vect) {
	for (short i = 0; i < 3; ++i)
		coords[i] += vect;
}

void LDQuad::move (vertex vect) {
	for (short i = 0; i < 4; ++i)
		coords[i] += vect;
}

void LDCondLine::move (vertex vect) {
	for (short i = 0; i < 4; ++i)
		coords[i] += vect;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
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
	return g_saRadialTypeNames[radType];
}

char const* LDRadial::radialTypeName (const LDRadial::Type eType) {
	return g_saRadialTypeNames[eType];
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
vector<LDObject*> LDRadial::decompose (bool applyTransform) {
	vector<LDObject*> paObjects;
	
	for (short i = 0; i < segs; ++i) {
		double x0 = cos ((i * 2 * pi) / divs),
			x1 = cos (((i + 1) * 2 * pi) / divs),
			z0 = sin ((i * 2 * pi) / divs),
			z1 = sin (((i + 1) * 2 * pi) / divs);
		
		switch (radType) {
		case LDRadial::Circle:
			{
				vertex v0 (x0, 0.0f, z0),
					v1 (x1, 0.0f, z1);
				
				if (applyTransform) {
					v0.transform (transform, pos);
					v1.transform (transform, pos);
				}
				
				LDLine* pLine = new LDLine;
				pLine->coords[0] = v0;
				pLine->coords[1] = v1;
				pLine->color = edgecolor;
				pLine->parent = this;
				
				paObjects << pLine;
			}
			break;
		
		case LDRadial::Cylinder:
		case LDRadial::Ring:
		case LDRadial::Cone:
			{
				double x2, x3, z2, z3;
				double y0, y1, y2, y3;
				
				if (radType == LDRadial::Cylinder) {
					x2 = x1;
					x3 = x0;
					z2 = z1;
					z3 = z0;
					
					y0 = y1 = 0.0f;
					y2 = y3 = 1.0f;
				} else {
					x2 = x1 * (ringNum + 1);
					x3 = x0 * (ringNum + 1);
					z2 = z1 * (ringNum + 1);
					z3 = z0 * (ringNum + 1);
					
					x0 *= ringNum;
					x1 *= ringNum;
					z0 *= ringNum;
					z1 *= ringNum;
					
					if (radType == LDRadial::Ring) {
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
				
				if (applyTransform) {
					v0.transform (transform, pos);
					v1.transform (transform, pos);
					v2.transform (transform, pos);
					v3.transform (transform, pos);
				}
				
				LDQuad* pQuad = new LDQuad;
				pQuad->coords[0] = v0;
				pQuad->coords[1] = v1;
				pQuad->coords[2] = v2;
				pQuad->coords[3] = v3;
				pQuad->color = color;
				pQuad->parent = this;
				
				paObjects << pQuad;
			}
			break;
		
		case LDRadial::Disc:
		case LDRadial::DiscNeg:
			{
				double x2, z2;
				
				if (radType == LDRadial::Disc) {
					x2 = z2 = 0.0f;
				} else {
					x2 = (x0 >= 0.0f) ? 1.0f : -1.0f;
					z2 = (z0 >= 0.0f) ? 1.0f : -1.0f;
				}
				
				vertex v0 (x0, 0.0f, z0),
					v1 (x1, 0.0f, z1),
					v2 (x2, 0.0f, z2);
				
				if (applyTransform) {
					v0.transform (transform, pos);
					v1.transform (transform, pos);
					v2.transform (transform, pos);
				}
				
				LDTriangle* pSeg = new LDTriangle;
				pSeg->coords[0] = v0;
				pSeg->coords[1] = v1;
				pSeg->coords[2] = v2;
				pSeg->color = color;
				pSeg->parent = this;
				
				paObjects << pSeg;
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
	return fmt ("0 !LDFORGE RADIAL %s %d %d %d %d %s %s",
		str (radialTypeName()).upper ().strip (' ').c (),
		color, segs, divs, ringNum,
		pos.stringRep (false).chars(), transform.stringRep().chars());
}

char const* g_radialNameRoots[] = {
	"edge",
	"cyli",
	"disc",
	"ndis",
	"ring",
	"con",
	null
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str LDRadial::makeFileName () {
	short numer = segs,
		denom = divs;
	
	// Simplify the fractional part, but the denominator must be at least 4.
	simplify (numer, denom);
	
	if (denom < 4) {
		const short factor = (4 / denom);
		
		numer *= factor;
		denom *= factor;
	}
	
	// Compose some general information: prefix, fraction, root, ring number
	str prefix = (divs == 16) ? "" : fmt ("%d/", divs);
	str frac = fmt ("%d-%d", numer, denom);
	str root = g_radialNameRoots[radType];
	str num = (radType == Ring || radType == Cone) ? fmt ("%d", ringNum) : "";
	
	// Truncate the root if necessary (7-16rin4.dat for instance).
	// However, always keep the root at least 2 characters.
	short extra = (~frac + ~num + ~root) - 8;
	root -= min<short> (max<short> (extra, 0), 2);
	
	// Stick them all together and return the result.
	return fmt ("%s%s%s%s.dat", prefix.chars(), frac.chars (), root.chars (), num.chars ());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
#define CHECK_FOR_OBJ(N) \
	if (type == LDObject::N) \
		return new LD##N;
LDObject* LDObject::getDefault (const LDObject::Type type) {
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

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
HistoryEntry* LDObject::invert () { return null; }
HistoryEntry* LDBFC::invert () { return null; }
HistoryEntry* LDEmpty::invert () { return null; }
HistoryEntry* LDComment::invert () { return null; }
HistoryEntry* LDGibberish::invert () { return null; }

HistoryEntry* LDTriangle::invert () {
	// Triangle goes 0 -> 1 -> 2, reversed: 0 -> 2 -> 1.
	// Thus, we swap 1 and 2.
	vertex tmp = coords[1];
	
	LDObject* oldCopy = clone ();
	coords[1] = coords[2];
	coords[2] = tmp;
	
	return new EditHistory ({(ulong) getIndex (g_curfile)}, {oldCopy}, {clone ()});
}

HistoryEntry* LDQuad::invert () {
	// Quad: 0 -> 1 -> 2 -> 3
	// rev:  0 -> 3 -> 2 -> 1
	// Thus, we swap 1 and 3.
	vertex tmp = coords[1];
	LDObject* oldCopy = clone ();
	
	coords[1] = coords[3];
	coords[3] = tmp;
	
	return new EditHistory ({(ulong) getIndex (g_curfile)}, {oldCopy}, {clone ()});
}

static HistoryEntry* invertSubfile (LDObject* obj) {
	// Subfiles and radials are inverted when they're prefixed with
	// a BFC INVERTNEXT statement. Thus we need to toggle this status.
	// For flat primitives it's sufficient that the determinant is
	// flipped but I don't have a method for checking flatness yet.
	// Food for thought...
	
	ulong idx = obj->getIndex (g_curfile);
	
	if (idx > 0) {
		LDBFC* bfc = dynamic_cast<LDBFC*> (obj->prev ());
		
		if (bfc && bfc->type == LDBFC::InvertNext) {
			// Object is prefixed with an invertnext, thus remove it.
			HistoryEntry* history = new DelHistory ({idx - 1}, {bfc->clone ()});
			
			g_curfile->forgetObject (bfc);
			delete bfc;
			return history;
		}
	}
	
	// Not inverted, thus prefix it with a new invertnext.
	LDBFC* bfc = new LDBFC (LDBFC::InvertNext);
	g_curfile->insertObj (idx, bfc);
	
	return new AddHistory ({idx}, {bfc->clone ()});
}

HistoryEntry* LDSubfile::invert () {
	return invertSubfile (this);
}

HistoryEntry* LDRadial::invert () {
	return invertSubfile (this);
}

static HistoryEntry* invertLine (LDObject* line) {
	// For lines, we swap the vertices. I don't think that a
	// cond-line's control points need to be swapped, do they?
	LDObject* oldCopy = line->clone ();
	vertex tmp = line->coords[0];
	
	oldCopy = line->clone ();
	line->coords[0] = line->coords[1];
	line->coords[1] = tmp;
	
	return new EditHistory ({(ulong) line->getIndex (g_curfile)}, {oldCopy}, {line->clone ()});
}

HistoryEntry* LDLine::invert () {
	return invertLine (this);
}

HistoryEntry* LDCondLine::invert () {
	return invertLine (this);
}

HistoryEntry* LDVertex::invert () { return null; }

// =============================================================================
LDLine* LDCondLine::demote () {
	LDLine* repl = new LDLine;
	memcpy (repl->coords, coords, sizeof coords);
	repl->color = color;
	
	replace (repl);
	return repl;
}