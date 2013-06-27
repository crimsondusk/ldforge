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

// List of all LDObjects
vector<LDObject*> g_LDObjects;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// LDObject constructors
LDObject::LDObject () {
	qObjListEntry = null;
	setParent (null);
	m_hidden = false;
	m_selected = false;
	m_glinit = false;
	
	// Determine ID
	int id = 1; // 0 is invalid
	for( LDObject* obj : g_LDObjects )
		if( obj->id() >= id )
			id = obj->id() + 1;
	
	setID( id );
}

LDGibberish::LDGibberish () {}
LDGibberish::LDGibberish (str contents, str reason) : contents (contents), reason (reason) {}

// =============================================================================
str LDComment::raw () {
	return fmt ("0 %1", text);
}

str LDSubfile::raw () {
	str val = fmt ("1 %1 %2 ", color (), position ());
	val += transform ().stringRep ();
	val += ' ';
	val += fileInfo ()->name ();
	return val;
}

str LDLine::raw () {
	str val = fmt ("2 %1", color ());
	
	for (ushort i = 0; i < 2; ++i)
		val += fmt (" %1", getVertex (i));
	
	return val;
}

str LDTriangle::raw () {
	str val = fmt ("3 %1", color ());
	
	for (ushort i = 0; i < 3; ++i)
		val += fmt  (" %1", getVertex (i));
	
	return val;
}

str LDQuad::raw () {
	str val = fmt ("4 %1", color ());
	
	for (ushort i = 0; i < 4; ++i)
		val += fmt  (" %1", getVertex (i));
	
	return val;
}

str LDCondLine::raw () {
	str val = fmt ("5 %1", color ());
	
	// Add the coordinates
	for (ushort i = 0; i < 4; ++i)
		val += fmt  (" %1", getVertex (i));
	
	return val;
}

str LDGibberish::raw () {
	return contents;
}

str LDVertex::raw () {
	return fmt ("0 !LDFORGE VERTEX %1 %2", color (), pos);
}

str LDEmpty::raw () {
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

str LDBFC::raw () {
	return fmt ("0 BFC %1", LDBFC::statements[type]);
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
	LDTriangle* tri1 = new LDTriangle (getVertex (0), getVertex (1), getVertex (3));
	LDTriangle* tri2 = new LDTriangle (getVertex (1), getVertex (2), getVertex (3));
	
	// The triangles also inherit the quad's color
	tri1->setColor (color ());
	tri2->setColor (color ());
	
	vector<LDTriangle*> triangles;
	triangles << tri1;
	triangles << tri2;
	return triangles;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::replace (LDObject* replacement) {
	long idx = getIndex (g_curfile);
	assert (idx != -1);
	
	// Replace the instance of the old object with the new object
	g_curfile->setObject (idx, replacement);
	
	// Remove the old object
	delete this;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::swap (LDObject* other) {
	for (LDObject*& obj : *g_curfile) {
		if (obj == this)
			obj = other;
		else if (obj == other)
			obj = this;
	}
}

LDLine::LDLine (vertex v1, vertex v2) {
	setVertex (0, v1);
	setVertex (1, v2);
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
		for (short i = 0; i < obj->vertices (); ++i) {
			vertex v = obj->getVertex (i);
			v.transform (transform, pos);
			obj->setVertex (i, v);
		}
		break;
	
	case LDObject::Subfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			matrix newMatrix = transform * ref->transform ();
			vertex newpos = ref->position ();
			
			newpos.transform (transform, pos);
			ref->setPosition (newpos);
			ref->setTransform (newMatrix);
		}
		break;
	
	default:
		break;
	}
	
	if (obj->color () == maincolor)
		obj->setColor (parentcolor);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
vector<LDObject*> LDSubfile::inlineContents (bool deep, bool cache) {
	vector<LDObject*> objs, objcache;
	
	// If we have this cached, just clone that
	if (deep && fileInfo ()->cache ().size ()) {
		for (LDObject* obj : fileInfo ()->cache ())
			objs << obj->clone ();
	} else {
		if (!deep)
			cache = false;
		
		for (LDObject* obj : *fileInfo ()) {
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
			fileInfo ()->setCache (objcache);
	}
	
	// Transform the objects
	for (LDObject* obj : objs) {
		// Set the parent now so we know what inlined this.
		obj->setParent (this);
		
		transformObject (obj, transform (), position (), color ());
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
		
		str noun = fmt ("%1%2", g_saObjTypeNames[objType], plural (objCount));
		
		// Plural of "vertex" is "vertices". Stupid English.
		if (objType == LDObject::Vertex && objCount != 1)
			noun = "vertices";
		
		text += fmt  ("%1 %2", objCount, noun);
		firstDetails = false;
	}
	
	return text;
}

// =============================================================================
LDObject* LDObject::topLevelParent () {
	if (!parent ())
		return this;
	
	LDObject* it = this;
	
	while (it->parent ())
		it = it->parent ();
	
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
	setPosition (position () + vect);
}

void LDRadial::move (vertex vect) {
	setPosition (position () + vect);
}

void LDLine::move (vertex vect) {
	for (short i = 0; i < 2; ++i)
		setVertex (i, getVertex (i) + vect);
}

void LDTriangle::move (vertex vect) {
	for (short i = 0; i < 3; ++i)
		setVertex (i, getVertex (i) + vect);
}

void LDQuad::move (vertex vect) {
	for (short i = 0; i < 4; ++i)
		setVertex (i, getVertex (i) + vect);
}

void LDCondLine::move (vertex vect) {
	for (short i = 0; i < 4; ++i)
		setVertex (i, getVertex (i) + vect);
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
	return g_saRadialTypeNames[type ()];
}

char const* LDRadial::radialTypeName (const LDRadial::Type typeval) {
	return g_saRadialTypeNames[typeval];
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
vector<LDObject*> LDRadial::decompose (bool applyTransform) {
	vector<LDObject*> objs;
	
	for (short i = 0; i < segments (); ++i) {
		double x0 = cos ((i * 2 * pi) / divisions ()),
			x1 = cos (((i + 1) * 2 * pi) / divisions ()),
			z0 = sin ((i * 2 * pi) / divisions ()),
			z1 = sin (((i + 1) * 2 * pi) / divisions ());
		
		LDObject* obj = null;
		
		switch (type ()) {
		case LDRadial::Circle:
			{
				vertex v0 (x0, 0.0f, z0),
					v1 (x1, 0.0f, z1);
				
				LDLine* line = new LDLine;
				line->setVertex (0, v0);
				line->setVertex (1, v1);
				line->setColor (edgecolor);
				line->setParent (this);
				
				obj = line;
			}
			break;
		
		case LDRadial::Cylinder:
		case LDRadial::Ring:
		case LDRadial::Cone:
			{
				double x2, x3, z2, z3;
				double y0, y1, y2, y3;
				
				if (type () == LDRadial::Cylinder) {
					x2 = x1;
					x3 = x0;
					z2 = z1;
					z3 = z0;
					
					y0 = y1 = 0.0f;
					y2 = y3 = 1.0f;
				} else {
					x2 = x1 * (number () + 1);
					x3 = x0 * (number () + 1);
					z2 = z1 * (number () + 1);
					z3 = z0 * (number () + 1);
					
					x0 *= number ();
					x1 *= number ();
					z0 *= number ();
					z1 *= number ();
					
					if (type () == LDRadial::Ring) {
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
				
				LDQuad* quad = new LDQuad;
				quad->setVertex (0, v0);
				quad->setVertex (1, v1);
				quad->setVertex (2, v2);
				quad->setVertex (3, v3);
				
				quad->setColor (color ());
				quad->setParent (this);
				
				obj = quad;
			}
			break;
		
		case LDRadial::Disc:
		case LDRadial::DiscNeg:
			{
				double x2, z2;
				
				if (type () == LDRadial::Disc) {
					x2 = z2 = 0.0f;
				} else {
					x2 = (x0 >= 0.0f) ? 1.0f : -1.0f;
					z2 = (z0 >= 0.0f) ? 1.0f : -1.0f;
				}
				
				vertex v0 (x0, 0.0f, z0),
					v1 (x1, 0.0f, z1),
					v2 (x2, 0.0f, z2);
				
				LDTriangle* seg = new LDTriangle;
				seg->setVertex (0, v0);
				seg->setVertex (1, v1);
				seg->setVertex (2, v2);
				seg->setColor (color ());
				seg->setParent (this);
				
				if (applyTransform) {
					for (int i = 0; i < 3; ++i) {
						vertex v = seg->getVertex (i);
						v.transform (transform (), position ());
						seg->setVertex (i, v);
					}
				}
				
				obj = seg;
			}
			break;
		
		default:
			break;
		}
		
		if (obj) {
			if (applyTransform) {
				for (int i = 0; i < obj->vertices (); ++i) {
					vertex v = obj->getVertex (i);
					v.transform (transform (), position ());
					obj->setVertex (i, v);
				}
			}
			
			objs << obj;
		}
	}
	
	return objs;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str LDRadial::raw () {
	return fmt ("0 !LDFORGE RADIAL %1 %2 %3 %4 %5 %6 %7",
		str (radialTypeName ()).toUpper ().remove (' '),
		color (), segments (), divisions (), number (),
		position ().stringRep (false), transform ().stringRep());
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
	short numer = segments (),
		denom = divisions ();
	
	// Simplify the fractional part, but the denominator must be at least 4.
	simplify (numer, denom);
	
	if (denom < 4) {
		const short factor = (4 / denom);
		
		numer *= factor;
		denom *= factor;
	}
	
	// Compose some general information: prefix, fraction, root, ring number
	str prefix = (divisions () == lores) ? "" : fmt ("%1/", divisions ());
	str frac = fmt ("%1-%2", numer, denom);
	str root = g_radialNameRoots[type ()];
	str num = (type () == Ring || type () == Cone) ? fmt ("%1", number ()) : "";
	
	// Truncate the root if necessary (7-16rin4.dat for instance).
	// However, always keep the root at least 2 characters.
	short extra = (frac.length () + num.length () + root.length ()) - 8;
	root.chop (min<short> (max<short> (extra, 0), 2));
	
	// Stick them all together and return the result.
	return (prefix + frac + root + num + ".dat");
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
void LDObject::invert () { }
void LDBFC::invert () { }
void LDEmpty::invert () { }
void LDComment::invert () { }
void LDGibberish::invert () { }

void LDTriangle::invert () {
	// Triangle goes 0 -> 1 -> 2, reversed: 0 -> 2 -> 1.
	// Thus, we swap 1 and 2.
	vertex tmp = getVertex (1);
	setVertex (1, getVertex (2));
	setVertex (2, tmp);
	
	return;
}

void LDQuad::invert () {
	// Quad: 0 -> 1 -> 2 -> 3
	// rev:  0 -> 3 -> 2 -> 1
	// Thus, we swap 1 and 3.
	vertex tmp = getVertex (1);
	setVertex (1, getVertex (3));
	setVertex (3, tmp);
}

static void invertSubfile (LDObject* obj) {
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
			g_curfile->forgetObject (bfc);
			delete bfc;
			return;
		}
	}
	
	// Not inverted, thus prefix it with a new invertnext.
	LDBFC* bfc = new LDBFC (LDBFC::InvertNext);
	g_curfile->insertObj (idx, bfc);
}

void LDSubfile::invert () {
	invertSubfile (this);
}

void LDRadial::invert () {
	invertSubfile (this);
}

static void invertLine (LDObject* line) {
	// For lines, we swap the vertices. I don't think that a
	// cond-line's control points need to be swapped, do they?
	vertex tmp = line->getVertex (0);
	line->setVertex (0, line->getVertex (1));
	line->setVertex (1, tmp);
}

void LDLine::invert () {
	invertLine (this);
}

void LDCondLine::invert () {
	invertLine (this);
}

void LDVertex::invert () { }

// =============================================================================
LDLine* LDCondLine::demote () {
	LDLine* repl = new LDLine;
	
	for (int i = 0; i < repl->vertices (); ++i)
		repl->setVertex (i, getVertex (i));
	
	repl->setColor (color ());
	
	replace (repl);
	return repl;
}

// =============================================================================
template<class T> void changeProperty (LDObject* obj, T* ptr, const T& val) {
	long idx;
	if ((idx = obj->getIndex (g_curfile)) != -1) {
		str before = obj->raw ();
		*ptr = val;
		str after = obj->raw ();
		
		g_curfile->addToHistory (new EditHistory (idx, before, after));
	} else
		*ptr = val;
}

READ_ACCESSOR (short, LDObject::color) { return m_color; }
SET_ACCESSOR (short, LDObject::setColor) {
	changeProperty (this, &m_color, val);
}

const vertex& LDObject::getVertex (int i) const {
	return m_coords[i];
}

void LDObject::setVertex (int i, const vertex& vert) {
	changeProperty (this, &m_coords[i], vert);
}

READ_ACCESSOR (vertex, LDMatrixObject::position) { return m_position; }
SET_ACCESSOR (vertex, LDMatrixObject::setPosition) {
	changeProperty (linkPointer (), &m_position, val);
}

READ_ACCESSOR (matrix, LDMatrixObject::transform) { return m_transform; }
SET_ACCESSOR (matrix, LDMatrixObject::setTransform) {
	changeProperty (linkPointer (), &m_transform, val);
}