#define GL_GLEXT_PROTOTYPES
#include <GL/glu.h>
#include <GL/glext.h>
#include "GLCompiler.h"
#include "LDObject.h"
#include "Colors.h"
#include "Document.h"
#include "Misc.h"
#include "GLRenderer.h"

#define DEBUG_PRINT(...) fprint (stdout, __VA_ARGS__)

extern_cfg (Bool, gl_blackedges);
static QList<short> g_warnedColors;
GLCompiler g_vertexCompiler;

static const QColor g_BFCFrontColor(40, 192, 0);
static const QColor g_BFCBackColor(224, 0, 0);

// =============================================================================
//
GLCompiler::GLCompiler() :
	m_Document (null)
{
	glGenBuffers (VBO_NumArrays, &m_mainVBOIndices[0]);
	needMerge();
}

GLCompiler::~GLCompiler()
{
	glDeleteBuffers (VBO_NumArrays, &m_mainVBOIndices[0]);
}

// =============================================================================
//
uint32 GLCompiler::getColorRGB (const QColor& color)
{
	return
		(color.red()   & 0xFF) << 0x00 |
		(color.green() & 0xFF) << 0x08 |
		(color.blue()  & 0xFF) << 0x10 |
		(color.alpha() & 0xFF) << 0x18;
}

// =============================================================================
//
QColor GLCompiler::getObjectColor (LDObject* obj, GLCompiler::E_ColorType colortype) const
{
	QColor qcol;

	if (!obj->isColored())
		return QColor();

	if (colortype == E_PickColor)
	{
		// Make the color by the object's ID if we're picking, so we can make the
		// ID again from the color we get from the picking results. Be sure to use
		// the top level parent's index since we want a subfile's children point
		// to the subfile itself.
		long i = obj->topLevelParent()->getID();

		// Calculate a color based from this index. This method caters for
		// 16777216 objects. I don't think that'll be exceeded anytime soon. :)
		// ATM biggest is 53588.dat with 12600 lines.
		int r = (i / 0x10000) % 0x100,
			g = (i / 0x100) % 0x100,
			b = i % 0x100;

		return QColor (r, g, b);
	}

	if (obj->getColor() == maincolor)
		qcol = GLRenderer::getMainColor();
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
			qcol = GLRenderer::getMainColor();
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
//
void GLCompiler::needMerge()
{
	// Set all of m_changed to true
	// memset (m_changed, 0xFF, sizeof m_changed);
	for (int i = 0; i < VBO_NumArrays; ++i)
		m_changed[i] = true;
}

// =============================================================================
//
void GLCompiler::stageForCompilation (LDObject* obj)
{
	m_staged << obj;
	removeDuplicates (m_staged);
}

// =============================================================================
//
void GLCompiler::compileDocument()
{
	for (LDObject* obj : getDocument()->getObjects())
		compileObject (obj);
}

// =============================================================================
//
void GLCompiler::compileStaged()
{
	for (LDObject* obj : m_staged)
		compileObject (obj);

	m_staged.clear();
}

// =============================================================================
//
void GLCompiler::prepareVBOArray (E_VBOArray type)
{
	// Compile anything that still awaits it
	compileStaged();

	if (!m_changed[type])
		return;

	m_mainArrays[type].clear();

	for (auto it = m_objArrays.begin(); it != m_objArrays.end(); ++it)
		m_mainArrays[type] += (*it)[type];

	glBindBuffer (GL_ARRAY_BUFFER, m_mainVBOIndices[type]);
	glBufferData (GL_ARRAY_BUFFER, m_mainArrays[type].size() * sizeof(float),
		m_mainArrays[type].constData(), GL_DYNAMIC_DRAW);
	glBindBuffer (GL_ARRAY_BUFFER, m_mainVBOIndices[type]);
	m_changed[type] = false;
	log ("VBO array %1 prepared: %2 coordinates", (int) type, m_mainArrays[type].size());
}

// =============================================================================
//
void GLCompiler::forgetObject (LDObject* obj)
{
	auto it = m_objArrays.find (obj);

	if (it != m_objArrays.end())
	{
		delete *it;
		m_objArrays.erase (it);
	}
}

// =============================================================================
//
void GLCompiler::compileObject (LDObject* obj)
{
	// Ensure we have valid arrays to write to.
	if (m_objArrays.find (obj) == m_objArrays.end())
		m_objArrays[obj] = new QVector<float>[VBO_NumArrays];
	else
	{
		// Arrays exist already, clear them.
		for (int i = 0; i < VBO_NumArrays; ++i)
			m_objArrays[obj][i].clear();
	}

	compileSubObject (obj, obj);
	QList<int> data;

	for (int i = 0; i < VBO_NumArrays; ++i)
		data << m_objArrays[obj][i].size();

	dlog ("Compiled #%1: %2 coordinates", obj->getID(), data);
	needMerge();
}

// =============================================================================
//
void GLCompiler::compileSubObject (LDObject* obj, LDObject* topobj)
{
	switch (obj->getType())
	{
		// Note: We cannot split the quad into triangles here, it would
		// mess up the wireframe view. Quads must go into a separate array.
		case LDObject::ETriangle:
		case LDObject::EQuad:
		case LDObject::ELine:
		case LDObject::ECondLine:
		{
			E_VBOArray arraynum;
			int verts;

			switch (obj->getType())
			{
				case LDObject::ETriangle:	arraynum = VBO_Triangles;	verts = 3; break;
				case LDObject::EQuad:		arraynum = VBO_Quads;		verts = 4; break;
				case LDObject::ELine:		arraynum = VBO_Lines;		verts = 2; break;
				case LDObject::ECondLine:	arraynum = VBO_CondLines;	verts = 2; break;
				default: break;
			}

			QVector<float>* ap = m_objArrays[topobj];
			QColor normalColor = getObjectColor (obj, E_NormalColor);
			QColor pickColor = getObjectColor (topobj, E_PickColor);

			for (int i = 0; i < verts; ++i)
			{
				// Write coordinates
				for (int j = 0; j < 3; ++j)
					ap[arraynum] << obj->getVertex (i).getCoordinate (j);

				// Colors
				writeColor (ap[VBO_NormalColors], normalColor);
				writeColor (ap[VBO_PickColors], pickColor);
				writeColor (ap[VBO_BFCFrontColors], g_BFCFrontColor);
				writeColor (ap[VBO_BFCBackColors], g_BFCBackColor);
			}
		} break;

		case LDObject::ESubfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			LDObjectList subobjs = ref->inlineContents (LDSubfile::DeepCacheInline | LDSubfile::RendererInline);

			for (LDObject* subobj : subobjs)
			{
				compileSubObject (subobj, topobj);
				subobj->deleteSelf();
			}
		} break;

		default:
			break;
	}
}

// =============================================================================
//
void GLCompiler::writeColor (QVector<float>& array, const QColor& color)
{
	array	<< color.red()
			<< color.green()
			<< color.blue()
			<< color.alpha();
}