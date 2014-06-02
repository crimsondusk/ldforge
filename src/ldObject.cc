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
QMap<long, LDObjectWeakPtr>	g_allObjects;
static int32						g_idcursor = 1; // 0 shalt be null
static constexpr int32				g_maxID = (1 << 24);

#define LDOBJ_DEFAULT_CTOR(T,BASE) \
	T :: T (LDObjectPtr* selfptr) : \
		BASE (selfptr) {}

// =============================================================================
// LDObject constructors
//
LDObject::LDObject (LDObjectPtr* selfptr) :
	m_isHidden (false),
	m_isSelected (false),
	m_isDestructed (false),
	qObjListEntry (null)
{
	*selfptr = LDObjectPtr (this, [](LDObject* obj){ obj->finalDelete(); });
	memset (m_coords, 0, sizeof m_coords);
	m_self = selfptr->toWeakRef();
	chooseID();
	g_allObjects[id()] = self();
	setRandomColor (QColor::fromHsv (rand() % 360, rand() % 256, rand() % 96 + 128));
}

LDSubfile::LDSubfile (LDObjectPtr* selfptr) :
	LDObject (selfptr)
{
	setLinkPointer (self());
}

LDOBJ_DEFAULT_CTOR (LDEmpty, LDObject)
LDOBJ_DEFAULT_CTOR (LDError, LDObject)
LDOBJ_DEFAULT_CTOR (LDLine, LDObject)
LDOBJ_DEFAULT_CTOR (LDTriangle, LDObject)
LDOBJ_DEFAULT_CTOR (LDCondLine, LDLine)
LDOBJ_DEFAULT_CTOR (LDQuad, LDObject)
LDOBJ_DEFAULT_CTOR (LDVertex, LDObject)
LDOBJ_DEFAULT_CTOR (LDOverlay, LDObject)
LDOBJ_DEFAULT_CTOR (LDBFC, LDObject)
LDOBJ_DEFAULT_CTOR (LDComment, LDObject)

// =============================================================================
//
void LDObject::chooseID()
{
	// If this is the first pass we can just use a global ID counter for each
	// unique object. Let's hope that nobody goes to create 17 million objects
	// anytime soon.
	if (g_idcursor < g_maxID)
	{
		setID (g_idcursor++);
		return;
	}

	// In case someone does, we cannot really continue execution. We must abort,
	// give the user a chance to save their documents though.
	critical ("Created too many objects. Execution cannot continue. You have a "
		"chance to save any changes to documents, then restart.");
	(void) safeToCloseAll();
	exit (0);
}

// =============================================================================
//
void LDObject::setVertexCoord (int i, Axis ax, double value)
{
	Vertex v = vertex (i);
	v.setCoordinate (ax, value);
	setVertex (i, v);
}

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
	document().toStrongRef()->setObject (idx, other);

	// Remove the old object
	destroy();
}

// =============================================================================
//
void LDObject::swap (LDObjectPtr other)
{
	assert (document() == other->document());
	document().toStrongRef()->swapObjects (self(), other);
}

// =============================================================================
//
LDLine::LDLine (LDObjectPtr* selfptr, Vertex v1, Vertex v2) :
	LDObject (selfptr)
{
	setVertex (0, v1);
	setVertex (1, v2);
}

// =============================================================================
//
LDQuad::LDQuad (LDObjectPtr* selfptr, const Vertex& v0, const Vertex& v1,
				const Vertex& v2, const Vertex& v3) :
	LDObject (selfptr)
{
	setVertex (0, v0);
	setVertex (1, v1);
	setVertex (2, v2);
	setVertex (3, v3);
}

// =============================================================================
//
LDCondLine::LDCondLine (LDObjectPtr* selfptr, const Vertex& v0, const Vertex& v1,
						const Vertex& v2, const Vertex& v3) :
	LDLine (selfptr)
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
void LDObject::destroy()
{
	// If this object was selected, unselect it now
	if (isSelected())
		deselect();

	// If this object was associated to a file, remove it off it now
	if (document() != null)
		document().toStrongRef()->forgetObject (self());

	// Delete the GL lists
	g_win->R()->forgetObject (self());

	// Remove this object from the list of LDObjects
	g_allObjects.erase (g_allObjects.find (id()));
	setDestructed (true);
}

//
// Deletes the object. Only the shared pointer is to call this!
//
void LDObject::finalDelete()
{
	if (not isDestructed())
		destroy();

	delete this;
}

// =============================================================================
//
static void transformObject (LDObjectPtr obj, Matrix transform, Vertex pos, int parentcolor)
{
	switch (obj->type())
	{
		case OBJ_Line:
		case OBJ_CondLine:
		case OBJ_Triangle:
		case OBJ_Quad:

			for (int i = 0; i < obj->numVertices(); ++i)
			{
				Vertex v = obj->vertex (i);
				v.transform (transform, pos);
				obj->setVertex (i, v);
			}

			break;

		case OBJ_Subfile:
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
		obj->setParent (self());
		transformObject (obj, transform(), position(), color());
	}

	return objs;
}

// =============================================================================
//
LDPolygon* LDObject::getPolygon()
{
	LDObjectType ot = type();
	int num =
		(ot == OBJ_Line)		?	2 :
		(ot == OBJ_Triangle)	?	3 :
		(ot == OBJ_Quad)		?	4 :
		(ot == OBJ_CondLine)	?	5 :
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

	for (int i = 0; i < document().toStrongRef()->getObjectCount(); ++i)
		if (document().toStrongRef()->getObject (i) == this)
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
	LDDocumentPtr file = objs[0]->document();

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
String LDObject::typeName (LDObjectType type)
{
	return LDObject::getDefault (type)->typeName();
}

// =============================================================================
//
String LDObject::describeObjects (const LDObjectList& objs)
{
	String text;

	if (objs.isEmpty())
		return "nothing"; // :)

	for (LDObjectType objType = OBJ_FirstType; objType < OBJ_NumTypes; ++objType)
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
		if (objType == OBJ_Vertex && count != 1)
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
		return self();

	LDObjectWeakPtr it (self());

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

	if (idx == (long) document().toStrongRef()->getObjectCount() - 1)
		return LDObjectPtr();

	return document().toStrongRef()->getObject (idx + 1);
}

// =============================================================================
//
LDObjectPtr LDObject::previous() const
{
	long idx = lineNumber();
	assert (idx != -1);

	if (idx == 0)
		return LDObjectPtr();

	return document().toStrongRef()->getObject (idx - 1);
}

// =============================================================================
//
bool LDObject::previousIsInvertnext (LDBFCPtr& ptr)
{
	LDObjectPtr prev (previous());

	if (prev != null && prev->type() == OBJ_BFC && prev.staticCast<LDBFC>()->statement() == LDBFC::InvertNext)
	{
		ptr = prev.staticCast<LDBFC>();
		return true;
	}

	return false;
}

// =============================================================================
//
void LDObject::move (Vertex vect)
{
	if (hasMatrix())
	{
		LDMatrixObjectPtr mo = self().toStrongRef().dynamicCast<LDMatrixObject>();
		mo->setPosition (mo->position() + vect);
	}
	elif (type() == OBJ_Vertex)
	{
		// ugh
		self().toStrongRef().staticCast<LDVertex>()->pos += vect;
	}
	else
	{
		for (int i = 0; i < numVertices(); ++i)
			setVertex (i, vertex (i) + vect);
	}
}

// =============================================================================
//
LDObjectPtr LDObject::getDefault (const LDObjectType type)
{
	switch (type)
	{
		case OBJ_Comment:		return spawn<LDComment>();
		case OBJ_BFC:			return spawn<LDBFC>();
		case OBJ_Line:			return spawn<LDLine>();
		case OBJ_CondLine:		return spawn<LDCondLine>();
		case OBJ_Subfile:		return spawn<LDSubfile>();
		case OBJ_Triangle:		return spawn<LDTriangle>();
		case OBJ_Quad:			return spawn<LDQuad>();
		case OBJ_Empty:		return spawn<LDEmpty>();
		case OBJ_Error:		return spawn<LDError>();
		case OBJ_Vertex:		return spawn<LDVertex>();
		case OBJ_Overlay:		return spawn<LDOverlay>();
		case OBJ_Unknown:	assert (false);
		case OBJ_NumTypes:		assert (false);
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
	if (document() == null)
		return;

	// Check whether subfile is flat
	int axisSet = (1 << X) | (1 << Y) | (1 << Z);
	LDObjectList objs = inlineContents (true, false);

	for (LDObjectPtr obj : objs)
	{
		for (int i = 0; i < obj->numVertices(); ++i)
		{
			Vertex const& vrt = obj->vertex (i);

			if (axisSet & (1 << X) && vrt.x() != 0.0)
				axisSet &= ~(1 << X);

			if (axisSet & (1 << Y) && vrt.y() != 0.0)
				axisSet &= ~(1 << Y);

			if (axisSet & (1 << Z) && vrt.z() != 0.0)
				axisSet &= ~(1 << Z);
		}

		if (axisSet == 0)
			break;
	}

	if (axisSet != 0)
	{
		// Subfile has all vertices zero on one specific plane, so it is flat.
		// Let's flip it.
		Matrix matrixModifier = g_identity;

		if (axisSet & (1 << X))
			matrixModifier[0] = -1;

		if (axisSet & (1 << Y))
			matrixModifier[4] = -1;

		if (axisSet & (1 << Z))
			matrixModifier[8] = -1;

		setTransform (transform() * matrixModifier);
		return;
	}

	// Subfile is not flat. Resort to invertnext.
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
	document().toStrongRef()->insertObj (idx, spawn<LDBFC> (LDBFC::InvertNext));
}

// =============================================================================
//
void LDLine::invert()
{
	// For lines, we swap the vertices.
	Vertex tmp = vertex (0);
	setVertex (0, vertex (1));
	setVertex (1, tmp);
}

// =============================================================================
//
void LDCondLine::invert()
{
	// I don't think that a conditional line's control points need to be
	// swapped, do they?
	Vertex tmp = vertex (0);
	setVertex (0, vertex (1));
	setVertex (1, tmp);
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
	auto it = g_allObjects.find (id);

	if (it != g_allObjects.end())
		return *it;

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
			obj->document().toStrongRef()->addToHistory (new EditHistory (idx, before, after));
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
	changeProperty (self(), &m_color, val);
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
		document().toStrongRef()->vertexChanged (*m_coords[i], vert);

	changeProperty (self(), &m_coords[i], LDSharedVertex::getSharedVertex (vert));
}

// =============================================================================
//
void LDMatrixObject::setPosition (const Vertex& a)
{
	LDObjectPtr ref = linkPointer().toStrongRef();

	if (ref->document() != null)
		ref->document().toStrongRef()->removeKnownVerticesOf (ref);

	changeProperty (ref, &m_position, LDSharedVertex::getSharedVertex (a));

	if (ref->document() != null)
		ref->document().toStrongRef()->addKnownVerticesOf (ref);
}

// =============================================================================
//
void LDMatrixObject::setTransform (const Matrix& val)
{
	LDObjectPtr ref = linkPointer().toStrongRef();

	if (ref->document() != null)
		ref->document().toStrongRef()->removeKnownVerticesOf (ref);

	changeProperty (ref, &m_transform, val);

	if (ref->document() != null)
		ref->document().toStrongRef()->addKnownVerticesOf (ref);
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
	document().toStrongRef()->addToSelection (self());

	// If this object is inverted with INVERTNEXT, pick the INVERTNEXT as well.
	LDBFCPtr invertnext;

	if (previousIsInvertnext (invertnext))
		invertnext->select();
}

// =============================================================================
//
void LDObject::deselect()
{
	assert (document() != null);
	document().toStrongRef()->removeFromSelection (self());

	// If this object is inverted with INVERTNEXT, deselect the INVERTNEXT as well.
	LDBFCPtr invertnext;

	if (previousIsInvertnext (invertnext))
		invertnext->deselect();
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
void LDSubfile::setFileInfo (const LDDocumentPtr& a)
{
	if (document() != null)
		document().toStrongRef()->removeKnownVerticesOf (self());

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
		document().toStrongRef()->addKnownVerticesOf (self());
};
