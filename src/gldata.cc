#include <cstring>
#include "gldata.h"
#include "ldtypes.h"
#include "colors.h"
#include "document.h"
#include "misc.h"
#include "gldraw.h"
#include <QDate>

#define DEBUG_PRINT(...) fprint (stdout, __VA_ARGS__)

extern_cfg (Bool, gl_blackedges);
static QList<short> g_warnedColors;
VertexCompiler g_vertexCompiler;

// =============================================================================
// -----------------------------------------------------------------------------
VertexCompiler::Array::Array() :
	m_data (null)
{
	clear();
}

// =============================================================================
// -----------------------------------------------------------------------------
VertexCompiler::Array::~Array()
{
	delete[] m_data;
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::Array::clear()
{
	delete[] m_data;

	m_data = new Vertex[64];
	m_size = 64;
	m_ptr = &m_data[0];
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::Array::resizeToFit (Size newSize)
{
	if (allocatedSize() >= newSize)
		return;

	int32 cachedWriteSize = writtenSize();

	// Add some lee-way space to reduce the amount of resizing.
	newSize += 256;

	const Size oldSize = allocatedSize();

	// We need to back up the data first
	Vertex* copy = new Vertex[oldSize];
	memcpy (copy, m_data, oldSize);

	// Re-create the buffer
	delete[] m_data;
	m_data = new Vertex[newSize];
	m_size = newSize;
	m_ptr = &m_data[cachedWriteSize / sizeof (Vertex)];

	// Copy the data back
	memcpy (m_data, copy, oldSize);
	delete[] copy;
}

// =============================================================================
// -----------------------------------------------------------------------------
const VertexCompiler::Vertex* VertexCompiler::Array::data() const
{
	return m_data;
}

// =============================================================================
// -----------------------------------------------------------------------------
const VertexCompiler::Array::Size& VertexCompiler::Array::allocatedSize() const
{
	return m_size;
}

// =============================================================================
// -----------------------------------------------------------------------------
VertexCompiler::Array::Size VertexCompiler::Array::writtenSize() const
{
	return (m_ptr - m_data) * sizeof (Vertex);
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::Array::write (const Vertex& f)
{
	// Ensure there's enoughspace for the new vertex
	resizeToFit (writtenSize() + sizeof f);

	// Write the float in
	*m_ptr++ = f;
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::Array::merge (Array* other)
{
	// Ensure there's room for both buffers
	resizeToFit (writtenSize() + other->writtenSize());

	memcpy (m_ptr, other->data(), other->writtenSize());
	m_ptr += other->writtenSize() / sizeof (Vertex);
}

// =============================================================================
// -----------------------------------------------------------------------------
VertexCompiler::VertexCompiler() :
	m_file (null)
{
	needMerge();
}

VertexCompiler::~VertexCompiler() {}

// =============================================================================
// Note: we use the top level object's color but the draw object's vertices.
// This is so that the index color is generated correctly - it has to reference
// the top level object's ID. This is crucial for picking to work.
// -----------------------------------------------------------------------------
void VertexCompiler::compilePolygon (LDObject* drawobj, LDObject* trueobj, QList< VertexCompiler::CompiledTriangle >& data)
{
	const QColor pickColor = getObjectColor (trueobj, PickColor);
	LDObject::Type type = drawobj->getType();
	LDObjectList objs;

	assert (type != LDObject::ESubfile);

	if (type == LDObject::EQuad)
	{
		for (LDTriangle * t : static_cast<LDQuad*> (drawobj)->splitToTriangles())
			objs << t;
	}
	else
		objs << drawobj;

	for (LDObject* obj : objs)
	{
		const LDObject::Type objtype = obj->getType();
		const bool isline = (objtype == LDObject::ELine || objtype == LDObject::ECondLine);
		const int verts = isline ? 2 : obj->vertices();
		QColor normalColor = getObjectColor (obj, Normal);

		assert (isline || objtype == LDObject::ETriangle);

		CompiledTriangle a;
		a.rgb = normalColor.rgb();
		a.pickrgb = pickColor.rgb();
		a.numVerts = verts;
		a.obj = trueobj;
		a.isCondLine = (objtype == LDObject::ECondLine);

		for (int i = 0; i < verts; ++i)
		{
			a.verts[i] = obj->getVertex (i);
			a.verts[i].y() = -a.verts[i].y();
			a.verts[i].z() = -a.verts[i].z();
		}

		data << a;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::compileObject (LDObject* obj)
{
	initObject (obj);
	QList<CompiledTriangle> data;
	QTime t0;

	for (int i = 0; i < GL::NumArrays; ++i)
		m_objArrays[obj][i].clear();

	t0 = QTime::currentTime();
	compileSubObject (obj, obj, data);
	DEBUG_PRINT ("COMPILATION: %1ms\n", t0.msecsTo (QTime::currentTime()));

	t0 = QTime::currentTime();

	for (int i = 0; i < GL::NumArrays; ++i)
	{
		GL::VAOType type = (GL::VAOType) i;
		const bool islinearray = (type == GL::EdgeArray || type == GL::EdgePickArray);

		for (const CompiledTriangle & poly : data)
		{
			if (poly.isCondLine)
			{
				// Conditional lines go to the edge pick array and the array
				// specifically designated for conditional lines and nowhere else.
				if (type != GL::EdgePickArray && type != GL::CondEdgeArray)
					continue;
			}
			else
			{
				// Lines and only lines go to the line array and only to the line array.
				if ( (poly.numVerts == 2) ^ islinearray)
					continue;

				// Only conditional lines go into the conditional line array
				if (type == GL::CondEdgeArray)
					continue;
			}

			Array* verts = postprocess (poly, type);
			m_objArrays[obj][type].merge (verts);
			delete verts;
		}
	}

	DEBUG_PRINT ("POST-PROCESS: %1ms\n", t0.msecsTo (QTime::currentTime()));

	needMerge();
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::compileSubObject (LDObject* obj, LDObject* topobj, QList< VertexCompiler::CompiledTriangle >& data)
{
	LDObjectList objs;

	switch (obj->getType())
	{
		case LDObject::ETriangle:
		case LDObject::ELine:
		case LDObject::ECondLine:
		{
			compilePolygon (obj, topobj, data);
		} break;

		case LDObject::EQuad:
		{
			for (LDTriangle * triangle : static_cast<LDQuad*> (obj)->splitToTriangles())
				compilePolygon (triangle, topobj, data);
		} break;

		case LDObject::ESubfile:
		{
			QTime t0 = QTime::currentTime();
			objs = static_cast<LDSubfile*> (obj)->inlineContents (LDSubfile::RendererInline | LDSubfile::DeepCacheInline);
			DEBUG_PRINT ("\t- INLINE: %1ms\n", t0.msecsTo (QTime::currentTime()));
			DEBUG_PRINT ("\t- %1 objects\n", objs.size());

			t0 = QTime::currentTime();

			for (LDObject* obj : objs)
			{
				compileSubObject (obj, topobj, data);
				obj->deleteSelf();
			}

			DEBUG_PRINT ("\t- SUB-COMPILATION: %1ms\n", t0.msecsTo (QTime::currentTime()));
		} break;

		default:
		{} break;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::compileDocument()
{
	for (LDObject * obj : m_file->getObjects())
		compileObject (obj);
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::forgetObject (LDObject* obj)
{
	auto it = m_objArrays.find (obj);

	if (it != m_objArrays.end())
		delete *it;

	m_objArrays.remove (obj);
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::setFile (LDDocument* file)
{
	m_file = file;
}

// =============================================================================
// -----------------------------------------------------------------------------
const VertexCompiler::Array* VertexCompiler::getMergedBuffer (GL::VAOType type)
{
	// If there are objects staged for compilation, compile them now.
	if (m_staged.size() > 0)
	{
		for (LDObject * obj : m_staged)
			compileObject (obj);

		m_staged.clear();
	}

	assert (type < GL::NumArrays);

	if (m_changed[type])
	{
		m_changed[type] = false;
		m_mainArrays[type].clear();

		for (LDObject* obj : m_file->getObjects())
		{
			if (!obj->isScemantic())
				continue;

			auto it = m_objArrays.find (obj);

			if (it != m_objArrays.end())
				m_mainArrays[type].merge (& (*it) [type]);
		}

		DEBUG_PRINT ("merged array %1: %2 bytes\n", (int) type, m_mainArrays[type].writtenSize());
	}

	return &m_mainArrays[type];
}

// =============================================================================
// This turns a compiled triangle into usable VAO vertices
// -----------------------------------------------------------------------------
VertexCompiler::Array* VertexCompiler::postprocess (const CompiledTriangle& triangle, GL::VAOType type)
{
	Array* va = new Array;
	QList<Vertex> verts;

	for (int i = 0; i < triangle.numVerts; ++i)
	{
		alias v0 = triangle.verts[i];
		Vertex v;
		v.x = v0.x();
		v.y = v0.y();
		v.z = v0.z();

		switch (type)
		{
			case GL::MainArray:
			case GL::EdgeArray:
			case GL::CondEdgeArray:
			{
				v.color = triangle.rgb;
			} break;

			case GL::PickArray:
			case GL::EdgePickArray:
			{
				v.color = triangle.pickrgb;
			} break;

			case GL::BFCArray:
			case GL::NumArrays:
				break; // handled separately
		}

		verts << v;
	}

	if (type == GL::BFCArray)
	{
		int32 rgb = getObjectColor (triangle.obj, BFCFront).rgb();

		for (Vertex v : verts)
		{
			v.color = rgb;
			va->write (v);
		}

		rgb = getObjectColor (triangle.obj, BFCBack).rgb();

		for (int i = verts.size() - 1; i >= 0; --i)
		{
			Vertex v = verts[i];
			v.color = rgb;
			va->write (v);
		}
	}
	else
	{
		for (Vertex v : verts)
			va->write (v);
	}

	return va;
}

// =============================================================================
// -----------------------------------------------------------------------------
uint32 VertexCompiler::getColorRGB (QColor& color)
{
	return
		(color.red()   & 0xFF) << 0x00 |
		(color.green() & 0xFF) << 0x08 |
		(color.blue()  & 0xFF) << 0x10 |
		(color.alpha() & 0xFF) << 0x18;
}

// =============================================================================
// -----------------------------------------------------------------------------
QColor VertexCompiler::getObjectColor (LDObject* obj, ColorType colotype) const
{
	QColor qcol;

	if (!obj->isColored())
		return QColor();

	if (colotype == PickColor)
	{
		// Make the color by the object's ID if we're picking, so we can make the
		// ID again from the color we get from the picking results. Be sure to use
		// the top level parent's index since we want a subfile's children point
		// to the subfile itself.
		long i = obj->topLevelParent()->getID();

		// Calculate a color based from this index. This method caters for
		// 16777216 objects. I don't think that'll be exceeded anytime soon. :)
		// ATM biggest is 53588.dat with 12600 lines.
		int r = (i / (256 * 256)) % 256,
			g = (i / 256) % 256,
			b = i % 256;

		return QColor (r, g, b);
	}

	if ( (colotype == BFCFront || colotype == BFCBack) &&
			obj->getType() != LDObject::ELine &&
			obj->getType() != LDObject::ECondLine
	   )
	{
		if (colotype == BFCFront)
			qcol = QColor (40, 192, 0);
		else
			qcol = QColor (224, 0, 0);
	}
	else
	{
		if (obj->getColor() == maincolor)
			qcol = GL::getMainColor();
		else
		{
			LDColor* col = getColor (obj->getColor());

			if (col)
				qcol = col->faceColor;
		}

		if (obj->getColor() == edgecolor)
		{
			qcol = QColor (32, 32, 32); // luma (m_bgcolor) < 40 ? QColor (64, 64, 64) : Qt::black;
			LDColor* col;

			if (!gl_blackedges && obj->getParent() && (col = getColor (obj->getParent()->getColor())))
				qcol = col->edgeColor;
		}

		if (qcol.isValid() == false)
		{
			// The color was unknown. Use main color to make the object at least
			// not appear pitch-black.
			if (obj->getColor() != edgecolor)
				qcol = GL::getMainColor();
			else
				qcol = Qt::black;

			// Warn about the unknown color, but only once.
			for (short i : g_warnedColors)
				if (obj->getColor() == i)
					return qcol;

			log ("%1: Unknown color %2!\n", __func__, obj->getColor());
			g_warnedColors << obj->getColor();
			return qcol;
		}
	}

	if (obj->topLevelParent()->isSelected())
	{
		// Brighten it up if selected.
		const int add = 51;

		qcol.setRed (min (qcol.red() + add, 255));
		qcol.setGreen (min (qcol.green() + add, 255));
		qcol.setBlue (min (qcol.blue() + add, 255));
	}

	return qcol;
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::needMerge()
{
	// Set all of m_changed to true
	memset (m_changed, 0xFF, sizeof m_changed);
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::initObject (LDObject* obj)
{
	if (m_objArrays.find (obj) == m_objArrays.end())
		m_objArrays[obj] = new Array[GL::NumArrays];
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::stageForCompilation (LDObject* obj)
{
	m_staged << obj;
	removeDuplicates (m_staged);
}

