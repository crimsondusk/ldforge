/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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
#include "ldObject.h"
#include "ldDocument.h"
#include "miscallenous.h"
#include "mainWindow.h"
#include "editHistory.h"
#include "glRenderer.h"
#include "colors.h"
#include "glCompiler.h"

cfg (String, ld_defaultname, "");
cfg (String, ld_defaultuser, "");
cfg (Int, ld_defaultlicense, 0);

// List of all LDObjects
static LDObjectList g_LDObjects;

// =============================================================================
// LDObject constructors
//
LDObject::LDObject() :
	m_isHidden (false),
	m_isSelected (false),
	m_parent (null),
	m_document (null),
	m_isGLInit (false),
	qObjListEntry (null)
{
	memset (m_coords, 0, sizeof m_coords);
	chooseID();
	g_LDObjects << this;
	setRandomColor (QColor::fromRgb (rand()));
}

// =============================================================================
//
void LDObject::chooseID()
{
	int32 id = 1; // 0 shalt be null

	for (LDObject* obj : g_LDObjects)
	{
		assert (obj != this);

		if (obj->id() >= id)
			id = obj->id() + 1;
	}

	setID (id);
}

// =============================================================================
//
void LDObject::setVertexCoord (int i, Axis ax, double value)
{
	Vertex v = vertex (i);
	v.setCoordinate (ax, value);
	setVertex (i, v);
}

LDError::LDError() {}

// =============================================================================
//
String LDComment::asText() const
{
	return format ("0 %1", text());
}

// =============================================================================
//
String LDSubfile::asText() const
{
	String val = format ("1 %1 %2 ", color(), position());
	val += transform().toString();
	val += ' ';
	val += fileInfo()->name();
	return val;
}

// =============================================================================
//
String LDLine::asText() const
{
	String val = format ("2 %1", color());

	for (int i = 0; i < 2; ++i)
		val += format (" %1", vertex (i));

	return val;
}

// =============================================================================
//
String LDTriangle::asText() const
{
	String val = format ("3 %1", color());

	for (int i = 0; i < 3; ++i)
		val += format (" %1", vertex (i));

	return val;
}

// =============================================================================
//
String LDQuad::asText() const
{
	String val = format ("4 %1", color());

	for (int i = 0; i < 4; ++i)
		val += format (" %1", vertex (i));

	return val;
}

// =============================================================================
//
String LDCondLine::asText() const
{
	String val = format ("5 %1", color());

	// Add the coordinates
	for (int i = 0; i < 4; ++i)
		val += format (" %1", vertex (i));

	return val;
}

// =============================================================================
//
String LDError::asText() const
{
	return contents();
}

// =============================================================================
//
String LDVertex::asText() const
{
	return format ("0 !LDFORGE VERTEX %1 %2", color(), pos);
}

// =============================================================================
//
String LDEmpty::asText() const
{
	return "";
}

// =============================================================================
//
const char* LDBFC::k_statementStrings[] =
{
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

String LDBFC::asText() const
{
	return format ("0 BFC %1", LDBFC::k_statementStrings[m_statement]);
}

// =============================================================================
//
QList<LDTriangle*> LDQuad::splitToTriangles()
{
	// Create the two triangles based on this quadrilateral:
	// 0---3       0---3    3
	// |   |       |  /    /|
	// |   |  ==>  | /    / |
	// |   |       |/    /  |
	// 1---2       1    1---2
	LDTriangle* tri1 = new LDTriangle (vertex (0), vertex (1), vertex (3));
	LDTriangle* tri2 = new LDTriangle (vertex (1), vertex (2), vertex (3));

	// The triangles also inherit the quad's color
	tri1->setColor (color());
	tri2->setColor (color());

	QList<LDTriangle*> triangles;
	triangles << tri1;
	triangles << tri2;
	return triangles;
}

// =============================================================================
//
void LDObject::replace (LDObject* other)
{
	long idx = lineNumber();
	assert (idx != -1);

	// Replace the instance of the old object with the new object
	document()->setObject (idx, other);

	// Remove the old object
	destroy();
}

// =============================================================================
//
void LDObject::swap (LDObject* other)
{
	assert (document() == other->document());
	document()->swapObjects (this, other);
}

// =============================================================================
//
LDLine::LDLine (Vertex v1, Vertex v2)
{
	setVertex (0, v1);
	setVertex (1, v2);
}

// =============================================================================
//
LDQuad::LDQuad (const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3)
{
	setVertex (0, v0);
	setVertex (1, v1);
	setVertex (2, v2);
	setVertex (3, v3);
}

// =============================================================================
//
LDObject::~LDObject() {}

// =============================================================================
//
LDSubfile::~LDSubfile() {}

// =============================================================================
//
void LDObject::destroy()
{
	// If this object was selected, unselect it now
	if (isSelected())
		unselect();

	// If this object was associated to a file, remove it off it now
	if (document())
		document()->forgetObject (this);

	// Delete the GL lists
	g_win->R()->forgetObject (this);

	// Remove this object from the list of LDObjects
	g_LDObjects.removeOne (this);

	delete this;
}

// =============================================================================
//
static void transformObject (LDObject* obj, Matrix transform, Vertex pos, int parentcolor)
{
	switch (obj->type())
	{
		case LDObject::ELine:
		case LDObject::ECondLine:
		case LDObject::ETriangle:
		case LDObject::EQuad:

			for (int i = 0; i < obj->vertices(); ++i)
			{
				Vertex v = obj->vertex (i);
				v.transform (transform, pos);
				obj->setVertex (i, v);
			}

			break;

		case LDObject::ESubfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			Matrix newMatrix = transform * ref->transform();
			Vertex newpos = ref->position();

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
// -----------------------------------------------------------------------------
LDObjectList LDSubfile::inlineContents (bool deep, bool render)
{
	LDObjectList objs = fileInfo()->inlineContents (deep, render);

	// Transform the objects
	for (LDObject* obj : objs)
	{
		// Set the parent now so we know what inlined the object.
		obj->setParent (this);
		transformObject (obj, transform(), position(), color());
	}

	return objs;
}

// =============================================================================
//
LDPolygon* LDObject::getPolygon()
{
	Type ot = type();
	int num =
		(ot == LDObject::ELine)		?	2 :
		(ot == LDObject::ETriangle)	?	3 :
		(ot == LDObject::EQuad)		?	4 :
		(ot == LDObject::ECondLine)	?	5 :
										0;
	if (num == 0)
		return null;

	LDPolygon* data = new LDPolygon;
	data->id = id();
	data->num = num;
	data->color = color();

	for (int i = 0; i < data->numVertices(); ++i)
		data->vertices[i] = vertex (i);

	return data;
}

// =============================================================================
//
QList<LDPolygon> LDSubfile::inlinePolygons()
{
	QList<LDPolygon> data = fileInfo()->inlinePolygons();

	for (LDPolygon& entry : data)
		for (int i = 0; i < entry.numVertices(); ++i)
			entry.vertices[i].transform (transform(), position());

	return data;
}

// =============================================================================
// -----------------------------------------------------------------------------
long LDObject::lineNumber() const
{
	assert (document() != null);

	for (int i = 0; i < document()->getObjectCount(); ++i)
		if (document()->getObject (i) == this)
			return i;

	return -1;
}

// =============================================================================
//
void LDObject::moveObjects (LDObjectList objs, const bool up)
{
	if (objs.isEmpty())
		return;

	// If we move down, we need to iterate the array in reverse order.
	const long start = up ? 0 : (objs.size() - 1);
	const long end = up ? objs.size() : -1;
	const long incr = up ? 1 : -1;
	LDObjectList objsToCompile;
	LDDocument* file = objs[0]->document();

	for (long i = start; i != end; i += incr)
	{
		LDObject* obj = objs[i];

		const long idx = obj->lineNumber(),
				   target = idx + (up ? -1 : 1);

		if ((up && idx == 0) || (not up && idx == (long) file->objects().size() - 1l))
		{
			// One of the objects hit the extrema. If this happens, this should be the first
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
//
String LDObject::typeName (LDObject::Type type)
{
	LDObject* obj = LDObject::getDefault (type);
	String name = obj->typeName();
	obj->destroy();
	return name;
}

// =============================================================================
//
String LDObject::describeObjects (const LDObjectList& objs)
{
	bool firstDetails = true;
	String text = "";

	if (objs.isEmpty())
		return "nothing"; // :)

	for (long i = 0; i < ENumTypes; ++i)
	{
		Type objType = (Type) i;
		int count = 0;

		for (LDObject * obj : objs)
			if (obj->type() == objType)
				count++;

		if (count == 0)
			continue;

		if (not firstDetails)
			text += ", ";

		String noun = format ("%1%2", typeName (objType), plural (count));

		// Plural of "vertex" is "vertices", correct that
		if (objType == EVertex && count != 1)
			noun = "vertices";

		text += format ("%1 %2", count, noun);
		firstDetails = false;
	}

	return text;
}

// =============================================================================
//
LDObject* LDObject::topLevelParent()
{
	if (parent() == null)
		return this;

	LDObject* it = this;

	while (it->parent() != null)
		it = it->parent();

	return it;
}

// =============================================================================
//
LDObject* LDObject::next() const
{
	long idx = lineNumber();
	assert (idx != -1);

	if (idx == (long) document()->getObjectCount() - 1)
		return null;

	return document()->getObject (idx + 1);
}

// =============================================================================
//
LDObject* LDObject::previous() const
{
	long idx = lineNumber();
	assert (idx != -1);

	if (idx == 0)
		return null;

	return document()->getObject (idx - 1);
}

// =============================================================================
//
void LDObject::move (Vertex vect)
{
	if (hasMatrix())
	{
		LDMatrixObject* mo = dynamic_cast<LDMatrixObject*> (this);
		mo->setPosition (mo->position() + vect);
	}
	elif (type() == LDObject::EVertex)
	{
		// ugh
		static_cast<LDVertex*> (this)->pos += vect;
	}
	else
	{
		for (int i = 0; i < vertices(); ++i)
			setVertex (i, vertex (i) + vect);
	}
}

// =============================================================================
//
#define CHECK_FOR_OBJ(N) \
	if (type == LDObject::E##N) \
		return new LD##N;

LDObject* LDObject::getDefault (const LDObject::Type type)
{
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
//
void LDObject::invert() {}
void LDBFC::invert() {}
void LDEmpty::invert() {}
void LDComment::invert() {}
void LDError::invert() {}

// =============================================================================
//
void LDTriangle::invert()
{
	// Triangle goes 0 -> 1 -> 2, reversed: 0 -> 2 -> 1.
	// Thus, we swap 1 and 2.
	Vertex tmp = vertex (1);
	setVertex (1, vertex (2));
	setVertex (2, tmp);

	return;
}

// =============================================================================
//
void LDQuad::invert()
{
	// Quad: 0 -> 1 -> 2 -> 3
	// rev:  0 -> 3 -> 2 -> 1
	// Thus, we swap 1 and 3.
	Vertex tmp = vertex (1);
	setVertex (1, vertex (3));
	setVertex (3, tmp);
}

// =============================================================================
//
void LDSubfile::invert()
{
	// Subfiles are inverted when they're prefixed with
	// a BFC INVERTNEXT statement. Thus we need to toggle this status.
	// For flat primitives it's sufficient that the determinant is
	// flipped but I don't have a method for checking flatness yet.
	// Food for thought...

	int idx = lineNumber();

	if (idx > 0)
	{
		LDBFC* bfc = dynamic_cast<LDBFC*> (previous());

		if (bfc && bfc->statement() == LDBFC::InvertNext)
		{
			// This is prefixed with an invertnext, thus remove it.
			bfc->destroy();
			return;
		}
	}

	// Not inverted, thus prefix it with a new invertnext.
	LDBFC* bfc = new LDBFC (LDBFC::InvertNext);
	document()->insertObj (idx, bfc);
}

// =============================================================================
//
static void invertLine (LDObject* line)
{
	// For lines, we swap the vertices. I don't think that a
	// cond-line's control points need to be swapped, do they?
	Vertex tmp = line->vertex (0);
	line->setVertex (0, line->vertex (1));
	line->setVertex (1, tmp);
}

void LDLine::invert()
{
	invertLine (this);
}

void LDCondLine::invert()
{
	invertLine (this);
}

void LDVertex::invert() {}

// =============================================================================
//
LDLine* LDCondLine::demote()
{
	LDLine* repl = new LDLine;

	for (int i = 0; i < repl->vertices(); ++i)
		repl->setVertex (i, vertex (i));

	repl->setColor (color());

	replace (repl);
	return repl;
}

// =============================================================================
//
LDObject* LDObject::fromID (int id)
{
	for (LDObject* obj : g_LDObjects)
		if (obj->id() == id)
			return obj;

	return null;
}

// =============================================================================
//
String LDOverlay::asText() const
{
	return format ("0 !LDFORGE OVERLAY %1 %2 %3 %4 %5 %6",
		fileName(), camera(), x(), y(), width(), height());
}

void LDOverlay::invert() {}

// =============================================================================
//
// Hook the set accessors of certain properties to this changeProperty function.
// It takes care of history management so we can capture low-level changes, this
// makes history stuff work out of the box.
//
template<class T> static void changeProperty (LDObject* obj, T* ptr, const T& val)
{
	long idx;

	if (*ptr == val)
		return;

	if (obj->document() != null && (idx = obj->lineNumber()) != -1)
	{
		String before = obj->asText();
		*ptr = val;
		String after = obj->asText();

		if (before != after)
		{
			obj->document()->addToHistory (new EditHistory (idx, before, after));
			g_win->R()->compileObject (obj);
		}
	}
	else
		*ptr = val;
}

// =============================================================================
//
void LDObject::setColor (const int& val)
{
	changeProperty (this, &m_color, val);
}

// =============================================================================
//
const Vertex& LDObject::vertex (int i) const
{
	return m_coords[i]->data();
}

// =============================================================================
//
void LDObject::setVertex (int i, const Vertex& vert)
{
	if (document() != null)
		document()->vertexChanged (*m_coords[i], vert);

	changeProperty (this, &m_coords[i], LDSharedVertex::getSharedVertex (vert));
}

// =============================================================================
//
void LDMatrixObject::setPosition (const Vertex& a)
{
	if (linkPointer()->document() != null)
		linkPointer()->document()->removeKnownVerticesOf (linkPointer());

	changeProperty (linkPointer(), &m_position, LDSharedVertex::getSharedVertex (a));

	if (linkPointer()->document() != null)
		linkPointer()->document()->addKnownVerticesOf (linkPointer());
}

// =============================================================================
//
void LDMatrixObject::setTransform (const Matrix& val)
{
	if (linkPointer()->document() != null)
		linkPointer()->document()->removeKnownVerticesOf (linkPointer());

	changeProperty (linkPointer(), &m_transform, val);

	if (linkPointer()->document() != null)
		linkPointer()->document()->addKnownVerticesOf (linkPointer());
}

// =============================================================================
//
static QMap<Vertex, LDSharedVertex*> g_sharedVerts;

LDSharedVertex* LDSharedVertex::getSharedVertex (const Vertex& a)
{
	auto it = g_sharedVerts.find (a);

	if (it == g_sharedVerts.end())
	{
		LDSharedVertex* v = new LDSharedVertex (a);
		g_sharedVerts[a] = v;
		return v;
	}

	return *it;
}

// =============================================================================
//
void LDSharedVertex::addRef (LDObject* a)
{
	m_refs << a;
}

// =============================================================================
//
void LDSharedVertex::delRef (LDObject* a)
{
	m_refs.removeOne (a);

	if (m_refs.empty())
	{
		g_sharedVerts.remove (m_data);
		delete this;
	}
}

// =============================================================================
//
void LDObject::select()
{
	assert (document() != null);
	document()->addToSelection (this);
}

// =============================================================================
//
void LDObject::unselect()
{
	assert (document() != null);
	document()->removeFromSelection (this);
}

// =============================================================================
//
String getLicenseText (int id)
{
	switch (id)
	{
		case 0:
			return g_CALicense;

		case 1:
			return g_nonCALicense;

		case 2:
			return "";
	}

	assert (false);
	return "";
}

// =============================================================================
//
LDObject* LDObject::createCopy() const
{
	/*
	LDObject* copy = clone();
	copy->setFile (null);
	copy->setGLInit (false);
	copy->chooseID();
	copy->setSelected (false);
	*/

	/*
	LDObject* copy = getDefault (getType());
	copy->setColor (color());

	if (hasMatrix())
	{
		LDMatrixObject* copyMo = static_cast<LDMatrixObject*> (copy);
		const LDMatrixObject* mo = static_cast<const LDMatrixObject*> (this);
		copyMo->setPosition (mo->getPosition());
		copyMo->setTransform (mo->transform());
	}
	else
	{
		for (int i = 0; i < vertices(); ++i)
			copy->setVertex (getVertex (i));
	}

	switch (getType())
	{
		case Subfile:
		{
			LDSubfile* copyRef = static_cast<LDSubfile*> (copy);
			const LDSubfile* ref = static_cast<const LDSubfile*> (this);

			copyRef->setFileInfo (ref->fileInfo());
		}
	}
	*/

	LDObject* copy = parseLine (asText());
	return copy;
}

// =============================================================================
//
void LDSubfile::setFileInfo (const LDDocumentPointer& a)
{
	if (document() != null)
		document()->removeKnownVerticesOf (this);

	m_fileInfo = a;

	// If it's an immediate subfile reference (i.e. this subfile belongs in an
	// explicit file), we need to pre-compile the GL polygons for the document
	// if they don't exist already.
	if (a != null &&
		a->isImplicit() == false &&
		a->polygonData().isEmpty())
	{
		a->initializeCachedData();
	}

	if (document() != null)
		document()->addKnownVerticesOf (this);
};
