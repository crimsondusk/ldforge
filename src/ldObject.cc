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

CFGENTRY (String, defaultName, "");
CFGENTRY (String, defaultUser, "");
CFGENTRY (Int, defaultLicense, 0);

// List of all LDObjects
static LDObjectWeakList g_LDObjects;

// =============================================================================
// LDObject constructors
//
LDObject::LDObject() :
	m_isHidden (false),
	m_isSelected (false),
	m_document (null),
	qObjListEntry (null)
{
	memset (m_coords, 0, sizeof m_coords);
	chooseID();
	g_LDObjects << LDObjectWeakPtr (thisptr());
	setRandomColor (QColor::fromHsv (rand() % 360, rand() % 256, rand() % 96 + 128));
}

// =============================================================================
//
void LDObject::chooseID()
{
	int32 id = 1; // 0 shalt be null

	for (LDObjectWeakPtr obj : g_LDObjects)
	{
		LDObjectPtr obj2 = obj.toStrongRef();
		assert (obj2 != this);

		if (obj2->id() >= id)
			id = obj2->id() + 1;
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
QList<LDTrianglePtr> LDQuad::splitToTriangles()
{
	// Create the two triangles based on this quadrilateral:
	// 0---3       0---3    3
	// |   |       |  /    /|
	// |   |  ==>  | /    / |
	// |   |       |/    /  |
	// 1---2       1    1---2
	LDTrianglePtr tri1 (spawn<LDTriangle> (vertex (0), vertex (1), vertex (3)));
	LDTrianglePtr tri2 (spawn<LDTriangle> (vertex (1), vertex (2), vertex (3)));

	// The triangles also inherit the quad's color
	tri1->setColor (color());
	tri2->setColor (color());

	return {tri1, tri2};
}

// =============================================================================
//
void LDObject::replace (LDObjectPtr other)
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
void LDObject::swap (LDObjectPtr other)
{
	assert (document() == other->document());
	document()->swapObjects (thisptr(), other);
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
LDCondLine::LDCondLine (const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3)
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
		deselect();

	// If this object was associated to a file, remove it off it now
	if (document())
		document()->forgetObject (thisptr());

	// Delete the GL lists
	g_win->R()->forgetObject (thisptr());

	// Remove this object from the list of LDObjects
	g_LDObjects.removeOne (LDObjectWeakPtr (thisptr()));
	delete this;
}

// =============================================================================
//
static void transformObject (LDObjectPtr obj, Matrix transform, Vertex pos, int parentcolor)
{
	switch (obj->type())
	{
		case LDObject::ELine:
		case LDObject::ECondLine:
		case LDObject::ETriangle:
		case LDObject::EQuad:

			for (int i = 0; i < obj->numVertices(); ++i)
			{
				Vertex v = obj->vertex (i);
				v.transform (transform, pos);
				obj->setVertex (i, v);
			}

			break;

		case LDObject::ESubfile:
		{
			LDSubfilePtr ref = qSharedPointerCast<LDSubfile> (obj);
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
	for (LDObjectPtr obj : objs)
	{
		// Set the parent now so we know what inlined the object.
		obj->setParent (LDObjectWeakPtr (thisptr()));
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
		LDObjectPtr obj = objs[i];

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
	for (LDObjectPtr obj : objsToCompile)
		g_win->R()->compileObject (obj);
}

// =============================================================================
//
String LDObject::typeName (LDObject::Type type)
{
	LDObjectPtr obj = LDObject::getDefault (type);
	String name = obj->typeName();
	obj->destroy();
	return name;
}

// =============================================================================
//
String LDObject::describeObjects (const LDObjectList& objs)
{
	String text;

	if (objs.isEmpty())
		return "nothing"; // :)

	for (Type objType = EFirstType; objType < ENumTypes; ++objType)
	{
		int count = 0;

		for (LDObjectPtr obj : objs)
			if (obj->type() == objType)
				count++;

		if (count == 0)
			continue;

		if (not text.isEmpty())
			text += ", ";

		String noun = format ("%1%2", typeName (objType), plural (count));

		// Plural of "vertex" is "vertices", correct that
		if (objType == EVertex && count != 1)
			noun = "vertices";

		text += format ("%1 %2", count, noun);
	}

	return text;
}

// =============================================================================
//
LDObjectPtr LDObject::topLevelParent()
{
	if (parent() == null)
		return thisptr();

	LDObjectWeakPtr it (thisptr());

	while (it.toStrongRef()->parent() != null)
		it = it.toStrongRef()->parent();

	return it.toStrongRef();
}

// =============================================================================
//
LDObjectPtr LDObject::next() const
{
	long idx = lineNumber();
	assert (idx != -1);

	if (idx == (long) document()->getObjectCount() - 1)
		return LDObjectPtr();

	return document()->getObject (idx + 1);
}

// =============================================================================
//
LDObjectPtr LDObject::previous() const
{
	long idx = lineNumber();
	assert (idx != -1);

	if (idx == 0)
		return LDObjectPtr();

	return document()->getObject (idx - 1);
}

// =============================================================================
//
void LDObject::move (Vertex vect)
{
	if (hasMatrix())
	{
		LDMatrixObjectPtr mo = thisptr().dynamicCast<LDMatrixObject>();
		mo->setPosition (mo->position() + vect);
	}
	elif (type() == LDObject::EVertex)
	{
		// ugh
		thisptr().staticCast<LDVertex>()->pos += vect;
	}
	else
	{
		for (int i = 0; i < numVertices(); ++i)
			setVertex (i, vertex (i) + vect);
	}
}

// =============================================================================
//
LDObjectPtr LDObject::getDefault (const LDObject::Type type)
{
	switch (type)
	{
		case EComment:		return spawn<LDComment>();
		case EBFC:			return spawn<LDBFC>();
		case ELine:			return spawn<LDLine>();
		case ECondLine:		return spawn<LDCondLine>();
		case ESubfile:		return spawn<LDSubfile>();
		case ETriangle:		return spawn<LDTriangle>();
		case EQuad:			return spawn<LDQuad>();
		case EEmpty:		return spawn<LDEmpty>();
		case EError:		return spawn<LDError>();
		case EVertex:		return spawn<LDVertex>();
		case EOverlay:		return spawn<LDOverlay>();
		case EUnidentified:	assert (false);
		case ENumTypes:		assert (false);
	}
	return LDObjectPtr();
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
		LDBFCPtr bfc = previous().dynamicCast<LDBFC>();

		if (not bfc.isNull() && bfc->statement() == LDBFC::InvertNext)
		{
			// This is prefixed with an invertnext, thus remove it.
			bfc->destroy();
			return;
		}
	}

	// Not inverted, thus prefix it with a new invertnext.
	document()->insertObj (idx, spawn<LDBFC> (LDBFC::InvertNext));
}

// =============================================================================
//
void LDLine::invert()
{
	// For lines, we swap the vertices. I don't think that a
	// cond-line's control points need to be swapped, do they?
	Vertex tmp = vertex (0);
	setVertex (0, vertex (1));
	setVertex (1, tmp);
}

void LDCondLine::invert()
{
	static_cast<LDLine*> (this)->invert();
}

void LDVertex::invert() {}

// =============================================================================
//
LDLinePtr LDCondLine::demote()
{
	LDLinePtr replacement (spawn<LDLine>());

	for (int i = 0; i < replacement->numVertices(); ++i)
		replacement->setVertex (i, vertex (i));

	replacement->setColor (color());

	replace (replacement);
	return replacement;
}

// =============================================================================
//
LDObjectPtr LDObject::fromID (int id)
{
	for (LDObjectWeakPtr obj : g_LDObjects)
	{
		LDObjectPtr obj2 = obj.toStrongRef();

		if (obj2->id() == id)
			return obj;
	}

	return LDObjectPtr();
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
template<typename T>
static void changeProperty (LDObjectPtr obj, T* ptr, const T& val)
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
	changeProperty (thisptr(), &m_color, val);
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

	changeProperty (thisptr(), &m_coords[i], LDSharedVertex::getSharedVertex (vert));
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
void LDSharedVertex::addRef (LDObjectPtr a)
{
	m_refs << a;
}

// =============================================================================
//
void LDSharedVertex::delRef (LDObjectPtr a)
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
	document()->addToSelection (thisptr());
}

// =============================================================================
//
void LDObject::deselect()
{
	assert (document() != null);
	document()->removeFromSelection (thisptr());
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
LDObjectPtr LDObject::createCopy() const
{
	LDObjectPtr copy = parseLine (asText());
	return copy;
}

// =============================================================================
//
void LDSubfile::setFileInfo (const LDDocumentPointer& a)
{
	if (document() != null)
		document()->removeKnownVerticesOf (thisptr());

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
		document()->addKnownVerticesOf (thisptr());
};
