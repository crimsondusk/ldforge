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
#include "colors.h"

// List of all LDObjects
List<LDObject*> g_LDObjects;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// LDObject constructors
LDObject::LDObject() :
	m_hidden (false),
	m_selected (false),
	m_parent (null),
	m_file (null),
	qObjListEntry (null),
	m_glinit (false) {
	
	// Determine ID
	int32 id = 1; // 0 is invalid
	
	for (LDObject* obj : g_LDObjects)
		if (obj->id() >= id)
			id = obj->id() + 1;
	
	setID (id);
	g_LDObjects << this;
}

// Default implementations for LDObject's virtual methods. These should never be
// actually called, for a subclass-less LDObject should never come into existance.
// These exist only to satisfy the linker.
LDObject::Type LDObject::getType() const {
	return LDObject::Unidentified;
}

bool LDObject::hasMatrix() const {
	return false;
}

bool LDObject::isColored() const {
	return false;
}

bool LDObject::isScemantic() const {
	return false;
}

str LDObject::typeName() const {
	return "";
}

short LDObject::vertices() const {
	return 0;
}

// =============================================================================
void LDObject::setVertexCoord (int i, Axis ax, double value) {
	vertex v = getVertex (i);
	v[ax] = value;
	setVertex (i, v);
}

LDErrorObject::LDErrorObject() {}

// =============================================================================
str LDCommentObject::raw() {
	return fmt ("0 %1", text);
}

str LDSubfileObject::raw() {
	str val = fmt ("1 %1 %2 ", color(), position());
	val += transform().stringRep();
	val += ' ';
	val += fileInfo()->name();
	return val;
}

str LDLineObject::raw() {
	str val = fmt ("2 %1", color());
	
	for (ushort i = 0; i < 2; ++i)
		val += fmt (" %1", getVertex (i));
	
	return val;
}

str LDTriangleObject::raw() {
	str val = fmt ("3 %1", color());
	
	for (ushort i = 0; i < 3; ++i)
		val += fmt (" %1", getVertex (i));
	
	return val;
}

str LDQuadObject::raw() {
	str val = fmt ("4 %1", color());
	
	for (ushort i = 0; i < 4; ++i)
		val += fmt (" %1", getVertex (i));
	
	return val;
}

str LDCondLineObject::raw() {
	str val = fmt ("5 %1", color());
	
	// Add the coordinates
	for (ushort i = 0; i < 4; ++i)
		val += fmt (" %1", getVertex (i));
	
	return val;
}

str LDErrorObject::raw() {
	return contents;
}

str LDVertexObject::raw() {
	return fmt ("0 !LDFORGE VERTEX %1 %2", color(), pos);
}

str LDEmptyObject::raw() {
	return "";
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
const char* LDBFCObject::statements[] = {
	"CERTIFY CCW",
	"CCW",
	"CERTIFY CW",
	"CW",
	"NOCERTIFY",
	"INVERTNEXT",
	"CLIP",
	"CLIP CCW",
	"CLIP CW",
	"NOCLIP",
};

str LDBFCObject::raw() {
	return fmt ("0 BFC %1", LDBFCObject::statements[type]);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
List<LDTriangleObject*> LDQuadObject::splitToTriangles() {
	// Create the two triangles based on this quadrilateral:
	// 0---3       0---3    3
	// |   |       |  /    /|
	// |   |  ==>  | /    / |
	// |   |       |/    /  |
	// 1---2       1    1---2
	LDTriangleObject* tri1 = new LDTriangleObject (getVertex (0), getVertex (1), getVertex (3));
	LDTriangleObject* tri2 = new LDTriangleObject (getVertex (1), getVertex (2), getVertex (3));
	
	// The triangles also inherit the quad's color
	tri1->setColor (color());
	tri2->setColor (color());
	
	List<LDTriangleObject*> triangles;
	triangles << tri1;
	triangles << tri2;
	return triangles;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::replace (LDObject* other) {
	long idx = getIndex();
	assert (idx != -1);
	
	// Replace the instance of the old object with the new object
	file()->setObject (idx, other);
	
	// Remove the old object
	delete this;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::swap (LDObject* other) {
	for (LDObject*& obj : *file()) {
		if (obj == this)
			obj = other;
		elif (obj == other)
			obj = this;
	}

	file()->addToHistory (new SwapHistory (id(), other->id()));
}

LDLineObject::LDLineObject (vertex v1, vertex v2) {
	setVertex (0, v1);
	setVertex (1, v2);
}

LDObject::~LDObject() {
	// Remove this object from the selection array if it is there.
	for (ulong i = 0; i < g_win->sel().size(); ++i)
		if (g_win->sel() [i] == this)
			g_win->sel().erase (i);
	
	// Delete the GL lists
	GL::deleteLists (this);
	
	// Remove this object from the list of LDObjects
	ulong pos = g_LDObjects.find (this);
	
	if (pos < g_LDObjects.size())
		g_LDObjects.erase (pos);
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
		for (short i = 0; i < obj->vertices(); ++i) {
			vertex v = obj->getVertex (i);
			v.transform (transform, pos);
			obj->setVertex (i, v);
		}

		break;

	case LDObject::Subfile:
		{
			LDSubfileObject* ref = static_cast<LDSubfileObject*> (obj);
			matrix newMatrix = transform * ref->transform();
			vertex newpos = ref->position();
			
			newpos.transform (transform, pos);
			ref->setPosition (newpos);
			ref->setTransform (newMatrix);
		}
		break;

	default:
		break;
	}
	
	if (obj->color() == maincolor)
		obj->setColor (parentcolor);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
List<LDObject*> LDSubfileObject::inlineContents (bool deep, bool cache) {
	List<LDObject*> objs, objcache;

	// If we have this cached, just clone that
	if (deep && fileInfo()->cache().size()) {
		for (LDObject* obj : fileInfo()->cache())
			objs << obj->clone();
	} else {
		if (!deep)
			cache = false;
		
		for (LDObject* obj : *fileInfo()) {
			// Skip those without scemantic meaning
			if (!obj->isScemantic())
				continue;
			
			// Got another sub-file reference, inline it if we're deep-inlining. If not,
			// just add it into the objects normally. Also, we only cache immediate
			// subfiles and this is not one. Yay, recursion!
			if (deep && obj->getType() == LDObject::Subfile) {
				LDSubfileObject* ref = static_cast<LDSubfileObject*> (obj);
				
				List<LDObject*> otherobjs = ref->inlineContents (true, false);
				
				for (LDObject* otherobj : otherobjs) {
					// Cache this object, if desired
					if (cache)
						objcache << otherobj->clone();
					
					objs << otherobj;
				}
			} else {
				if (cache)
					objcache << obj->clone();
				
				objs << obj->clone();
			}
		}
		
		if (cache)
			fileInfo()->setCache (objcache);
	}
	
	// Transform the objects
	for (LDObject* obj : objs) {
		// Set the parent now so we know what inlined this.
		obj->setParent (this);
		transformObject (obj, transform(), position(), color());
	}
	
	return objs;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
long LDObject::getIndex() const {
#ifndef RELEASE
	assert (file() != null);
#endif
	
	for (ulong i = 0; i < file()->numObjs(); ++i)
		if (file()->obj (i) == this)
			return i;
	
	return -1;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::moveObjects (List<LDObject*> objs, const bool up) {
	if (objs.size() == 0)
		return;
	
	// If we move down, we need to iterate the array in reverse order.
	const long start = up ? 0 : (objs.size() - 1);
	const long end = up ? objs.size() : -1;
	const long incr = up ? 1 : -1;
	List<LDObject*> objsToCompile;
	LDFile* file = objs[0]->file();
	
	for (long i = start; i != end; i += incr) {
		LDObject* obj = objs[i];
		
		const long idx = obj->getIndex(),
			target = idx + (up ? -1 : 1);
		
		if ((up && idx == 0) || (!up && idx == (long) (file->objs().size() - 1))) {
			// One of the objects hit the extrema. If this happens, this should be the first
			// object to be iterated on. Thus, nothing has changed yet and it's safe to just
			// abort the entire operation.
			assert (i == start);
			return;
		}
		
		objsToCompile << obj;
		objsToCompile << file->obj (target);
		
		obj->swap (file->obj (target));
	}
	
	objsToCompile.makeUnique();
	
	// The objects need to be recompiled, otherwise their pick lists are left with
	// the wrong index colors which messes up selection.
	for (LDObject* obj : objsToCompile)
		g_win->R()->compileObject (obj);
}

str LDObject::typeName (LDObject::Type type) {
	LDObject* obj = LDObject::getDefault (type);
	str name = obj->typeName();
	delete obj;
	return name;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str LDObject::objectListContents (const List<LDObject*>& objs) {
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
		
		str noun = fmt ("%1%2", typeName (objType), plural (objCount));
		
		// Plural of "vertex" is "vertices". Stupid English.
		if (objType == LDObject::Vertex && objCount != 1)
			noun = "vertices";
		
		text += fmt ("%1 %2", objCount, noun);
		firstDetails = false;
	}
	
	return text;
}

// =============================================================================
LDObject* LDObject::topLevelParent() {
	if (!parent())
		return this;
	
	LDObject* it = this;
		
	while (it->parent())
		it = it->parent();

	return it;
}

// =============================================================================
LDObject* LDObject::next() const {
	long idx = getIndex();
	assert (idx != -1);
	
	if (idx == (long) file()->numObjs() - 1)
		return null;
	
	return file()->obj (idx + 1);
}

// =============================================================================
LDObject* LDObject::prev() const {
	long idx = getIndex();
	assert (idx != -1);
	
	if (idx == 0)
		return null;
	
	return file()->obj (idx - 1);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::move (vertex vect)    { (void) vect; }
void LDEmptyObject::move (vertex vect)     { (void) vect; }
void LDBFCObject::move (vertex vect)       { (void) vect; }
void LDCommentObject::move (vertex vect)   { (void) vect; }
void LDErrorObject::move (vertex vect) { (void) vect; }

void LDVertexObject::move (vertex vect) {
	pos += vect;
}

void LDSubfileObject::move (vertex vect) {
	setPosition (position() + vect);
}

void LDLineObject::move (vertex vect) {
	for (short i = 0; i < 2; ++i)
		setVertex (i, getVertex (i) + vect);
}

void LDTriangleObject::move (vertex vect) {
	for (short i = 0; i < 3; ++i)
		setVertex (i, getVertex (i) + vect);
}

void LDQuadObject::move (vertex vect) {
	for (short i = 0; i < 4; ++i)
		setVertex (i, getVertex (i) + vect);
}

void LDCondLineObject::move (vertex vect) {
	for (short i = 0; i < 4; ++i)
		setVertex (i, getVertex (i) + vect);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
#define CHECK_FOR_OBJ(N) \
	if (type == LDObject::N) \
		return new LD##N##Object;

LDObject* LDObject::getDefault (const LDObject::Type type) {
	CHECK_FOR_OBJ (Comment)
	CHECK_FOR_OBJ (BFC)
	CHECK_FOR_OBJ (Line)
	CHECK_FOR_OBJ (CondLine)
	CHECK_FOR_OBJ (Subfile)
	CHECK_FOR_OBJ (Triangle)
	CHECK_FOR_OBJ (Quad)
	CHECK_FOR_OBJ (Empty)
	CHECK_FOR_OBJ (BFC)
	CHECK_FOR_OBJ (Error)
	CHECK_FOR_OBJ (Vertex)
	CHECK_FOR_OBJ (Overlay)
	return null;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDObject::invert() {}
void LDBFCObject::invert() {}
void LDEmptyObject::invert() {}
void LDCommentObject::invert() {}
void LDErrorObject::invert() {}

void LDTriangleObject::invert() {
	// Triangle goes 0 -> 1 -> 2, reversed: 0 -> 2 -> 1.
	// Thus, we swap 1 and 2.
	vertex tmp = getVertex (1);
	setVertex (1, getVertex (2));
	setVertex (2, tmp);

	return;
}

void LDQuadObject::invert() {
	// Quad: 0 -> 1 -> 2 -> 3
	// rev:  0 -> 3 -> 2 -> 1
	// Thus, we swap 1 and 3.
	vertex tmp = getVertex (1);
	setVertex (1, getVertex (3));
	setVertex (3, tmp);
}

void LDSubfileObject::invert() {
	// Subfiles are inverted when they're prefixed with
	// a BFC INVERTNEXT statement. Thus we need to toggle this status.
	// For flat primitives it's sufficient that the determinant is
	// flipped but I don't have a method for checking flatness yet.
	// Food for thought...
	
	ulong idx = getIndex();
	
	if (idx > 0) {
		LDBFCObject* bfc = dynamic_cast<LDBFCObject*> (prev());
		
		if (bfc && bfc->type == LDBFCObject::InvertNext) {
			// This is prefixed with an invertnext, thus remove it.
			file()->forgetObject (bfc);
			delete bfc;
			return;
		}
	}
	
	// Not inverted, thus prefix it with a new invertnext.
	LDBFCObject* bfc = new LDBFCObject (LDBFCObject::InvertNext);
	file()->insertObj (idx, bfc);
}

static void invertLine (LDObject* line) {
	// For lines, we swap the vertices. I don't think that a
	// cond-line's control points need to be swapped, do they?
	vertex tmp = line->getVertex (0);
	line->setVertex (0, line->getVertex (1));
	line->setVertex (1, tmp);
}

void LDLineObject::invert() {
	invertLine (this);
}

void LDCondLineObject::invert() {
	invertLine (this);
}

void LDVertexObject::invert() {}

// =============================================================================
LDLineObject* LDCondLineObject::demote() {
	LDLineObject* repl = new LDLineObject;
	
	for (int i = 0; i < repl->vertices(); ++i)
		repl->setVertex (i, getVertex (i));
	
	repl->setColor (color());
	
	replace (repl);
	return repl;
}

LDObject* LDObject::fromID (int id) {
	for (LDObject* obj : g_LDObjects)
		if (obj->id() == id)
			return obj;
	
	return null;
}

// =============================================================================
str LDOverlayObject::raw() {
	return fmt ("0 !LDFORGE OVERLAY %1 %2 %3 %4 %5 %6",
		filename(), camera(), x(), y(), width(), height());
}

void LDOverlayObject::move (vertex vect) {
	Q_UNUSED (vect)
}

void LDOverlayObject::invert() {}

// =============================================================================
// Hook the set accessors of certain properties to this changeProperty function.
// It takes care of history management so we can capture low-level changes, this
// makes history stuff work out of the box.
template<class T> void changeProperty (LDObject* obj, T* ptr, const T& val) {
	long idx;
	
	if (obj->file() && (idx = obj->getIndex()) != -1) {
		str before = obj->raw();
		*ptr = val;
		str after = obj->raw();
		
		obj->file()->addToHistory (new EditHistory (idx, before, after));
	} else
		*ptr = val;
}

READ_ACCESSOR (short, LDObject::color) {
	return m_color;
}

SET_ACCESSOR (short, LDObject::setColor) {
	changeProperty (this, &m_color, val);
}

const vertex& LDObject::getVertex (int i) const {
	return m_coords[i];
}

void LDObject::setVertex (int i, const vertex& vert) {
	changeProperty (this, &m_coords[i], vert);
}

READ_ACCESSOR (vertex, LDMatrixObject::position) {
	return m_position;
}

SET_ACCESSOR (vertex, LDMatrixObject::setPosition) {
	changeProperty (linkPointer(), &m_position, val);
}

READ_ACCESSOR (matrix, LDMatrixObject::transform) {
	return m_transform;
}

SET_ACCESSOR (matrix, LDMatrixObject::setTransform) {
	changeProperty (linkPointer(), &m_transform, val);
}