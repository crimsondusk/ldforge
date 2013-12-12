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

#include "main.h"
#include "ldtypes.h"
#include "document.h"
#include "misc.h"
#include "gui.h"
#include "history.h"
#include "gldraw.h"
#include "colors.h"

cfg (String, ld_defaultname, "");
cfg (String, ld_defaultuser, "");
cfg (Int, ld_defaultlicense, 0);

// List of all LDObjects
QList<LDObject*> g_LDObjects;

// =============================================================================
// LDObject constructors
// -----------------------------------------------------------------------------
LDObject::LDObject() :
	m_Hidden (false),
	m_Selected (false),
	m_Parent (null),
	m_File (null),
	qObjListEntry (null),
	m_glinit (false)
{
	memset (m_coords, 0, sizeof m_coords);

	// Determine ID
	int32 id = 1; // 0 is invalid

	for (LDObject* obj : g_LDObjects)
		if (obj->getID() >= id)
			id = obj->getID() + 1;

	setID (id);
	g_LDObjects << this;
}

// =============================================================================
// Default implementations for LDObject's virtual methods. These should never be
// actually called, for a subclass-less LDObject should never come into existance.
// These exist only to satisfy the linker.
// -----------------------------------------------------------------------------
LDObject::Type LDObject::getType() const
{	return LDObject::Unidentified;
}

bool LDObject::hasMatrix() const
{	return false;
}

bool LDObject::isColored() const
{	return false;
}

bool LDObject::isScemantic() const
{	return false;
}

str LDObject::getTypeName() const
{	return "";
}

int LDObject::vertices() const
{	return 0;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDObject::setVertexCoord (int i, Axis ax, double value)
{	vertex v = getVertex (i);
	v[ax] = value;
	setVertex (i, v);
}

LDError::LDError() {}

// =============================================================================
// -----------------------------------------------------------------------------
str LDComment::raw()
{	return fmt ("0 %1", text);
}

// =============================================================================
// -----------------------------------------------------------------------------
str LDSubfile::raw()
{	str val = fmt ("1 %1 %2 ", getColor(), getPosition());
	val += getTransform().stringRep();
	val += ' ';
	val += getFileInfo()->getName();
	return val;
}

// =============================================================================
// -----------------------------------------------------------------------------
str LDLine::raw()
{	str val = fmt ("2 %1", getColor());

	for (int i = 0; i < 2; ++i)
		val += fmt (" %1", getVertex (i));

	return val;
}

// =============================================================================
// -----------------------------------------------------------------------------
str LDTriangle::raw()
{	str val = fmt ("3 %1", getColor());

	for (int i = 0; i < 3; ++i)
		val += fmt (" %1", getVertex (i));

	return val;
}

// =============================================================================
// -----------------------------------------------------------------------------
str LDQuad::raw()
{	str val = fmt ("4 %1", getColor());

	for (int i = 0; i < 4; ++i)
		val += fmt (" %1", getVertex (i));

	return val;
}

// =============================================================================
// -----------------------------------------------------------------------------
str LDCondLine::raw()
{	str val = fmt ("5 %1", getColor());

	// Add the coordinates
	for (int i = 0; i < 4; ++i)
		val += fmt (" %1", getVertex (i));

	return val;
}

// =============================================================================
// -----------------------------------------------------------------------------
str LDError::raw()
{	return contents;
}

// =============================================================================
// -----------------------------------------------------------------------------
str LDVertex::raw()
{	return fmt ("0 !LDFORGE VERTEX %1 %2", getColor(), pos);
}

// =============================================================================
// -----------------------------------------------------------------------------
str LDEmpty::raw()
{	return "";
}

// =============================================================================
// -----------------------------------------------------------------------------
const char* LDBFC::statements[] =
{	"CERTIFY CCW",
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

str LDBFC::raw()
{	return fmt ("0 BFC %1", LDBFC::statements[type]);
}

// =============================================================================
// -----------------------------------------------------------------------------
QList<LDTriangle*> LDQuad::splitToTriangles()
{	// Create the two triangles based on this quadrilateral:
	// 0---3       0---3    3
	// |   |       |  /    /|
	// |   |  ==>  | /    / |
	// |   |       |/    /  |
	// 1---2       1    1---2
	LDTriangle* tri1 = new LDTriangle (getVertex (0), getVertex (1), getVertex (3));
	LDTriangle* tri2 = new LDTriangle (getVertex (1), getVertex (2), getVertex (3));

	// The triangles also inherit the quad's color
	tri1->setColor (getColor());
	tri2->setColor (getColor());

	QList<LDTriangle*> triangles;
	triangles << tri1;
	triangles << tri2;
	return triangles;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDObject::replace (LDObject* other)
{	long idx = getIndex();
	assert (idx != -1);

	// Replace the instance of the old object with the new object
	getFile()->setObject (idx, other);

	// Remove the old object
	delete this;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDObject::swap (LDObject* other)
{	int i = 0;

	for (LDObject* obj : getFile()->getObjects())
	{	if (obj == this)
			getFile()->setObject (i, other);
		elif (obj == other)
			getFile()->setObject (i, this);

		++i;
	}

	getFile()->addToHistory (new SwapHistory (getID(), other->getID()));
}

// =============================================================================
// -----------------------------------------------------------------------------
LDLine::LDLine (vertex v1, vertex v2)
{	setVertex (0, v1);
	setVertex (1, v2);
}

// =============================================================================
// -----------------------------------------------------------------------------
LDQuad::LDQuad (const vertex& v0, const vertex& v1, const vertex& v2, const vertex& v3)
{	setVertex (0, v0);
	setVertex (1, v1);
	setVertex (2, v2);
	setVertex (3, v3);
}

// =============================================================================
// -----------------------------------------------------------------------------
LDObject::~LDObject()
{	// If this object was selected, unselect it now
	if (isSelected())
		unselect();

	// If this object was associated to a file, remove it off it now
	if (getFile())
		getFile()->forgetObject (this);

	// Delete the GL lists
	GL::deleteLists (this);

	// Remove this object from the list of LDObjects
	g_LDObjects.removeOne (this);
}

// =============================================================================
// -----------------------------------------------------------------------------
static void transformObject (LDObject* obj, matrix transform, vertex pos, int parentcolor)
{	switch (obj->getType())
	{	case LDObject::Line:
		case LDObject::CondLine:
		case LDObject::Triangle:
		case LDObject::Quad:

			for (int i = 0; i < obj->vertices(); ++i)
			{	vertex v = obj->getVertex (i);
				v.transform (transform, pos);
				obj->setVertex (i, v);
			}

			break;

		case LDObject::Subfile:
		{	LDSubfile* ref = static_cast<LDSubfile*> (obj);
			matrix newMatrix = transform * ref->getTransform();
			vertex newpos = ref->getPosition();

			newpos.transform (transform, pos);
			ref->setPosition (newpos);
			ref->setTransform (newMatrix);
		}
		break;

		default:
			break;
	}

	if (obj->getColor() == maincolor)
		obj->setColor (parentcolor);
}

// =============================================================================
// -----------------------------------------------------------------------------
QList<LDObject*> LDSubfile::inlineContents (InlineFlags flags)
{	QList<LDObject*> objs = getFileInfo()->inlineContents (flags);

	// Transform the objects
for (LDObject * obj : objs)
	{	// Set the parent now so we know what inlined this.
		obj->setParent (this);
		transformObject (obj, getTransform(), getPosition(), getColor());
	}

	return objs;
}

// =============================================================================
// -----------------------------------------------------------------------------
long LDObject::getIndex() const
{
#ifndef RELEASE
	assert (getFile() != null);
#endif

	for (int i = 0; i < getFile()->getObjectCount(); ++i)
		if (getFile()->getObject (i) == this)
			return i;

	return -1;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDObject::moveObjects (QList<LDObject*> objs, const bool up)
{	if (objs.isEmpty())
		return;

	// If we move down, we need to iterate the array in reverse order.
	const long start = up ? 0 : (objs.size() - 1);
	const long end = up ? objs.size() : -1;
	const long incr = up ? 1 : -1;
	QList<LDObject*> objsToCompile;
	LDDocument* file = objs[0]->getFile();

	for (long i = start; i != end; i += incr)
	{	LDObject* obj = objs[i];

		const long idx = obj->getIndex(),
				   target = idx + (up ? -1 : 1);

		if ( (up && idx == 0) || (!up && idx == (long) (file->getObjects().size() - 1)))
		{	// One of the objects hit the extrema. If this happens, this should be the first
			// object to be iterated on. Thus, nothing has changed yet and it's safe to just
			// abort the entire operation.
			assert (i == start);
			return;
		}

		objsToCompile << obj;
		objsToCompile << file->getObject (target);

		obj->swap (file->getObject (target));
	}

	removeDuplicates (objsToCompile);

	// The objects need to be recompiled, otherwise their pick lists are left with
	// the wrong index colors which messes up selection.
	for (LDObject* obj : objsToCompile)
		g_win->R()->compileObject (obj);
}

// =============================================================================
// -----------------------------------------------------------------------------
str LDObject::typeName (LDObject::Type type)
{	LDObject* obj = LDObject::getDefault (type);
	str name = obj->getTypeName();
	delete obj;
	return name;
}

// =============================================================================
// -----------------------------------------------------------------------------
str LDObject::describeObjects (const QList<LDObject*>& objs)
{	bool firstDetails = true;
	str text = "";

	if (objs.isEmpty())
		return "nothing"; // :)

	for (long i = 0; i < LDObject::NumTypes; ++i)
	{	LDObject::Type objType = (LDObject::Type) i;
		int count = 0;

		for (LDObject * obj : objs)
			if (obj->getType() == objType)
				count++;

		if (count == 0)
			continue;

		if (!firstDetails)
			text += ", ";

		str noun = fmt ("%1%2", typeName (objType), plural (count));

		// Plural of "vertex" is "vertices", correct that
		
		if (objType == LDObject::Vertex && count != 1)
			noun = "vertices";

		text += fmt ("%1 %2", count, noun);
		firstDetails = false;
	}

	return text;
}

// =============================================================================
// -----------------------------------------------------------------------------
LDObject* LDObject::topLevelParent()
{	if (!getParent())
		return this;

	LDObject* it = this;

	while (it->getParent())
		it = it->getParent();

	return it;
}

// =============================================================================
// -----------------------------------------------------------------------------
LDObject* LDObject::next() const
{	long idx = getIndex();
	assert (idx != -1);

	if (idx == (long) getFile()->getObjectCount() - 1)
		return null;

	return getFile()->getObject (idx + 1);
}

// =============================================================================
// -----------------------------------------------------------------------------
LDObject* LDObject::prev() const
{	long idx = getIndex();
	assert (idx != -1);

	if (idx == 0)
		return null;

	return getFile()->getObject (idx - 1);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDObject::move (vertex vect)
{	(void) vect;
}
void LDEmpty::move (vertex vect)
{	(void) vect;
}
void LDBFC::move (vertex vect)
{	(void) vect;
}
void LDComment::move (vertex vect)
{	(void) vect;
}
void LDError::move (vertex vect)
{	(void) vect;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDVertex::move (vertex vect)
{	pos += vect;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDSubfile::move (vertex vect)
{	setPosition (getPosition() + vect);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDLine::move (vertex vect)
{	for (int i = 0; i < 2; ++i)
		setVertex (i, getVertex (i) + vect);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDTriangle::move (vertex vect)
{	for (int i = 0; i < 3; ++i)
		setVertex (i, getVertex (i) + vect);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDQuad::move (vertex vect)
{	for (int i = 0; i < 4; ++i)
		setVertex (i, getVertex (i) + vect);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDCondLine::move (vertex vect)
{	for (int i = 0; i < 4; ++i)
		setVertex (i, getVertex (i) + vect);
}

// =============================================================================
// -----------------------------------------------------------------------------
#define CHECK_FOR_OBJ(N) \
	if (type == LDObject::N) \
		return new LD##N;

LDObject* LDObject::getDefault (const LDObject::Type type)
{	CHECK_FOR_OBJ (Comment)
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
// -----------------------------------------------------------------------------
void LDObject::invert() {}
void LDBFC::invert() {}
void LDEmpty::invert() {}
void LDComment::invert() {}
void LDError::invert() {}

// =============================================================================
// -----------------------------------------------------------------------------
void LDTriangle::invert()
{	// Triangle goes 0 -> 1 -> 2, reversed: 0 -> 2 -> 1.
	// Thus, we swap 1 and 2.
	vertex tmp = getVertex (1);
	setVertex (1, getVertex (2));
	setVertex (2, tmp);

	return;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDQuad::invert()
{	// Quad: 0 -> 1 -> 2 -> 3
	// rev:  0 -> 3 -> 2 -> 1
	// Thus, we swap 1 and 3.
	vertex tmp = getVertex (1);
	setVertex (1, getVertex (3));
	setVertex (3, tmp);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDSubfile::invert()
{	// Subfiles are inverted when they're prefixed with
	// a BFC INVERTNEXT statement. Thus we need to toggle this status.
	// For flat primitives it's sufficient that the determinant is
	// flipped but I don't have a method for checking flatness yet.
	// Food for thought...

	int idx = getIndex();

	if (idx > 0)
	{	LDBFC* bfc = dynamic_cast<LDBFC*> (prev());

		if (bfc && bfc->type == LDBFC::InvertNext)
		{	// This is prefixed with an invertnext, thus remove it.
			getFile()->forgetObject (bfc);
			delete bfc;
			return;
		}
	}

	// Not inverted, thus prefix it with a new invertnext.
	LDBFC* bfc = new LDBFC (LDBFC::InvertNext);
	getFile()->insertObj (idx, bfc);
}

// =============================================================================
// -----------------------------------------------------------------------------
static void invertLine (LDObject* line)
{	// For lines, we swap the vertices. I don't think that a
	// cond-line's control points need to be swapped, do they?
	vertex tmp = line->getVertex (0);
	line->setVertex (0, line->getVertex (1));
	line->setVertex (1, tmp);
}

void LDLine::invert()
{	invertLine (this);
}

void LDCondLine::invert()
{	invertLine (this);
}

void LDVertex::invert() {}

// =============================================================================
// -----------------------------------------------------------------------------
LDLine* LDCondLine::demote()
{	LDLine* repl = new LDLine;

	for (int i = 0; i < repl->vertices(); ++i)
		repl->setVertex (i, getVertex (i));

	repl->setColor (getColor());

	replace (repl);
	return repl;
}

// =============================================================================
// -----------------------------------------------------------------------------
LDObject* LDObject::fromID (int id)
{	for (LDObject * obj : g_LDObjects)
		if (obj->getID() == id)
			return obj;

	return null;
}

// =============================================================================
// -----------------------------------------------------------------------------
str LDOverlay::raw()
{	return fmt ("0 !LDFORGE OVERLAY %1 %2 %3 %4 %5 %6",
		getFileName(), getCamera(), getX(), getY(), getWidth(), getHeight());
}

void LDOverlay::move (vertex vect)
{	Q_UNUSED (vect)
}

void LDOverlay::invert() {}

// =============================================================================
// Hook the set accessors of certain properties to this changeProperty function.
// It takes care of history management so we can capture low-level changes, this
// makes history stuff work out of the box.
// -----------------------------------------------------------------------------
template<class T> static void changeProperty (LDObject* obj, T* ptr, const T& val)
{	long idx;

	if (*ptr == val)
		return;

	if (obj->getFile() && (idx = obj->getIndex()) != -1)
	{	str before = obj->raw();
		*ptr = val;
		str after = obj->raw();

		obj->getFile()->addToHistory (new EditHistory (idx, before, after));
	}
	else
		*ptr = val;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDObject::setColor (const int& val)
{	changeProperty (this, &m_Color, val);
}

// =============================================================================
// -----------------------------------------------------------------------------
const vertex& LDObject::getVertex (int i) const
{	return m_coords[i]->data();
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDObject::setVertex (int i, const vertex& vert)
{	changeProperty (this, &m_coords[i], LDSharedVertex::getSharedVertex (vert));
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDMatrixObject::setPosition (const vertex& a)
{	changeProperty (getLinkPointer(), &m_Position, LDSharedVertex::getSharedVertex (a));
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDMatrixObject::setTransform (const matrix& val)
{	changeProperty (getLinkPointer(), &m_Transform, val);
}

// =============================================================================
// -----------------------------------------------------------------------------
static QMap<vertex, LDSharedVertex*> g_sharedVerts;

LDSharedVertex* LDSharedVertex::getSharedVertex (const vertex& a)
{	auto it = g_sharedVerts.find (a);

	if (it == g_sharedVerts.end())
	{	LDSharedVertex* v = new LDSharedVertex (a);
		g_sharedVerts[a] = v;
		return v;
	}

	return *it;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDSharedVertex::addRef (LDObject* a)
{	m_refs << a;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDSharedVertex::delRef (LDObject* a)
{	m_refs.removeOne (a);

	if (m_refs.empty())
	{	g_sharedVerts.remove (m_data);
		delete this;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDObject::select()
{	if (!getFile())
	{	log ("Warning: Object #%1 cannot be selected as it is not assigned a file!\n", getID());
		return;
	}

	getFile()->addToSelection (this);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDObject::unselect()
{	if (!getFile())
	{	log ("Warning: Object #%1 cannot be unselected as it is not assigned a file!\n", getID());
		return;
	}

	getFile()->removeFromSelection (this);
}

// =============================================================================
// -----------------------------------------------------------------------------
str getLicenseText (int id)
{	switch (id)
	{	case 0:
			return CALicense;

		case 1:
			return NonCALicense;

		case 2:
			return "";
	}

	assert (false);
	return "";
}