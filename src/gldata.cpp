#include "gldata.h"
#include "ldtypes.h"
#include "colors.h"
#include "file.h"
#include "misc.h"
#include "gldraw.h"

cfg (bool, gl_blackedges, true);

// =============================================================================
// -----------------------------------------------------------------------------
VertexCompiler::Array::Array() :
	m_data (null)
{
	clear();
}

// =============================================================================
// -----------------------------------------------------------------------------
VertexCompiler::Array::~Array() {
	delete[] m_data;
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::Array::clear() {
	delete[] m_data;
	
	m_data = new Vertex[64];
	m_size = 64;
	m_ptr = &m_data[0];
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::Array::resizeToFit (Size newSize) {
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
const VertexCompiler::Vertex* VertexCompiler::Array::data() const {
	return m_data;
}

// =============================================================================
// -----------------------------------------------------------------------------
const VertexCompiler::Array::Size& VertexCompiler::Array::allocatedSize() const {
	return m_size;
}

// =============================================================================
// -----------------------------------------------------------------------------
VertexCompiler::Array::Size VertexCompiler::Array::writtenSize() const {
	return (m_ptr - m_data) * sizeof (Vertex);
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::Array::write (const Vertex& f) {
	// Ensure there's enoughspace for the new vertex
	resizeToFit (writtenSize() + sizeof f);
	
	// Write the float in
	*m_ptr++ = f;
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::Array::merge (
	Array* other
) {
	// Ensure there's room for both buffers
	resizeToFit (writtenSize() + other->writtenSize());
	
	memcpy (m_ptr, other->data(), other->writtenSize());
	m_ptr += other->writtenSize() / sizeof (Vertex);
}

// =============================================================================
// -----------------------------------------------------------------------------
VertexCompiler::VertexCompiler() : m_file (null) {
	for (int i = 0; i < NumArrays; ++i) {
		m_mainArrays[i] = new Array;
		m_changed[i] = false;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
VertexCompiler::~VertexCompiler() {
	for (Array* array : m_mainArrays)
		delete array;
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::compileVertex (::vertex v, QColor col, Array* array) {
	VertexCompiler::Vertex glvertex;
	glvertex.x = v.x();
	glvertex.y = -v.y();
	glvertex.z = v.z();
	glvertex.color =
		(col.red()   & 0xFF) << 0x00 |
		(col.green() & 0xFF) << 0x08 |
		(col.blue()  & 0xFF) << 0x10 |
		(col.alpha() & 0xFF) << 0x18;
	
	array->write (glvertex);
}

// =============================================================================
// -----------------------------------------------------------------------------
// Note: we use the true object's color but the draw object's vertices. This is
// so that the index color is generated correctly - it has to reference the true
// object's ID. This is crucial for picking to work.
void VertexCompiler::compilePolygon (LDObject* drawobj, LDObject* trueobj) {
	Array** arrays = m_objArrays[trueobj];
	LDObject::Type objtype = drawobj->getType();
	const bool isline = (objtype == LDObject::Line || objtype == LDObject::CondLine);
	const int verts = isline ? 2 : drawobj->vertices();
	
	QColor normalColor = getObjectColor (trueobj, Normal),
		pickColor = getObjectColor (trueobj, PickColor);
	
	for (int i = 0; i < verts; ++i) {
		compileVertex (drawobj->getVertex (i), normalColor, arrays[isline ? EdgeArray : MainArray]);
		compileVertex (drawobj->getVertex (i), pickColor, arrays[isline ? EdgePickArray : PickArray]);
	}
	
	// For non-lines, compile BFC data. Note that the front objects are what get
	// reversed here! This is because we invert the Y axis, which inverts the
	// entire part model, so we remedy that here.
	if (!isline) {
		QColor col = getObjectColor (trueobj, BFCBack);
		for (int i = 0; i < verts; ++i)
			compileVertex (drawobj->getVertex(i), col, arrays[BFCArray]);
		
		col = getObjectColor (trueobj, BFCFront);
		for (int i = verts - 1; i >= 0; --i)
			compileVertex (drawobj->getVertex(i), col, arrays[BFCArray]);
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::compileObject (LDObject* obj, LDObject* topobj) {
	if (m_objArrays.find (obj) == m_objArrays.end()) {
		m_objArrays[obj] = new Array*[NumArrays];
		
		for (int i = 0; i < NumArrays; ++i)
			m_objArrays[obj][i] = new Array;
	} else {
		for (int i = 0; i < NumArrays; ++i)
			m_objArrays[obj][i]->clear();
	}
	
	List<LDObject*> objs;
	
	switch (obj->getType()) {
	case LDObject::Triangle:
		compilePolygon (obj, topobj);
		m_changed[MainArray] = true;
		break;
	
	case LDObject::Quad:
		for (LDTriangleObject* triangle : static_cast<LDQuadObject*> (obj)->splitToTriangles())
			compilePolygon (triangle, topobj);
		m_changed[MainArray] = true;
		break;
	
	case LDObject::Line:
		compilePolygon (obj, topobj);
		break;
	
	case LDObject::Subfile:
		objs = static_cast<LDSubfileObject*> (obj)->inlineContents (true, true);
		
		for (LDObject* obj : objs) {
			compileObject (obj, topobj);
			delete obj;
		}
		break;
	
	default:
		break;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::compileFile() {
	for (LDObject* obj : *m_file)
		compileObject (obj, obj);
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::forgetObject (LDObject* obj) {
	for (int i = 0; i < NumArrays; ++i)
		delete m_objArrays[obj][i];
	
	delete m_objArrays[obj];
	m_objArrays.remove (obj);
}

// =============================================================================
// -----------------------------------------------------------------------------
void VertexCompiler::setFile (LDFile* file) {
	m_file = file;
}

// =============================================================================
// -----------------------------------------------------------------------------
VertexCompiler::Array* VertexCompiler::getMergedBuffer (ArrayType type) {
	assert (type < NumArrays);
	
	if (m_changed) {
		m_changed[type] = false;
		m_mainArrays[type]->clear();
		
		for (LDObject* obj : *m_file) {
			auto it = m_objArrays.find (obj);
			
			if (it != m_objArrays.end())
				m_mainArrays[type]->merge ((*it)[type]);
		}
	}
	
	return m_mainArrays[type];
}

// =============================================================================
// -----------------------------------------------------------------------------
static List<short> g_warnedColors;
QColor VertexCompiler::getObjectColor (LDObject* obj, ColorType colotype) const {
	QColor qcol;
	
	if (!obj->isColored())
		return QColor();
	
	if (colotype == PickColor) {
		// Make the color by the object's ID if we're picking, so we can make the
		// ID again from the color we get from the picking results. Be sure to use
		// the top level parent's index since we want a subfile's children point
		// to the subfile itself.
		long i = obj->topLevelParent()->id();
		
		// Calculate a color based from this index. This method caters for
		// 16777216 objects. I don't think that'll be exceeded anytime soon. :)
		// ATM biggest is 53588.dat with 12600 lines.
		int r = (i / (256 * 256)) % 256,
			g = (i / 256) % 256,
			b = i % 256;
		
		return QColor (r, g, b);
	}
	
	if ((colotype == BFCFront || colotype == BFCBack) &&
		obj->getType() != LDObject::Line &&
		obj->getType() != LDObject::CondLine) {
		
		if (colotype == BFCFront)
			qcol = QColor (40, 192, 0);
		else
			qcol = QColor (224, 0, 0);
	} else {
		if (obj->color() == maincolor)
			qcol = GL::getMainColor();
		else {
			LDColor* col = getColor (obj->color());
			
			if (col)
				qcol = col->faceColor;
		}
		
		if (obj->color() == edgecolor) {
			qcol = QColor (32, 32, 32); // luma (m_bgcolor) < 40 ? QColor (64, 64, 64) : Qt::black;
			LDColor* col;
			
			if (!gl_blackedges && obj->parent() && (col = getColor (obj->parent()->color())))
				qcol = col->edgeColor;
		}
		
		if (qcol.isValid() == false) {
			// The color was unknown. Use main color to make the object at least
			// not appear pitch-black.
			if (obj->color() != edgecolor)
				qcol = GL::getMainColor();
			
			// Warn about the unknown colors, but only once.
			for (short i : g_warnedColors)
				if (obj->color() == i)
					return Qt::black;
			
			print ("%1: Unknown color %2!\n", __func__, obj->color());
			g_warnedColors << obj->color();
			return Qt::black;
		}
	}
	
	if (obj->topLevelParent()->selected()) {
		// Brighten it up for the select list.
		const uchar add = 51;
		
		qcol.setRed (min (qcol.red() + add, 255));
		qcol.setGreen (min (qcol.green() + add, 255));
		qcol.setBlue (min (qcol.blue() + add, 255));
	}
	
	return qcol;
}