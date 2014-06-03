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

#define GL_GLEXT_PROTOTYPES
#include <GL/glu.h>
#include <GL/glext.h>
#include "glCompiler.h"
#include "ldObject.h"
#include "colors.h"
#include "ldDocument.h"
#include "miscallenous.h"
#include "glRenderer.h"
#include "dialogs.h"

struct GLErrorInfo
{
	GLenum	value;
	QString	text;
};

static const GLErrorInfo g_GLErrors[] =
{
	{ GL_NO_ERROR,						"No error" },
	{ GL_INVALID_ENUM,					"Unacceptable enumerator passed" },
	{ GL_INVALID_VALUE,					"Numeric argument out of range" },
	{ GL_INVALID_OPERATION,				"The operation is not allowed to be done in this state" },
	{ GL_INVALID_FRAMEBUFFER_OPERATION,	"Framebuffer object is not complete"},
	{ GL_OUT_OF_MEMORY,					"Out of memory" },
	{ GL_STACK_UNDERFLOW,				"The operation would have caused an underflow" },
	{ GL_STACK_OVERFLOW,				"The operation would have caused an overflow" },
};

CFGENTRY (String, selectColorBlend, "#0080FF")
EXTERN_CFGENTRY (Bool, blackEdges);
EXTERN_CFGENTRY (String, backgroundColor);

static QList<int>		g_warnedColors;
static const QColor		g_BFCFrontColor (64, 192, 80);
static const QColor		g_BFCBackColor (208, 64, 64);

// static QMap<LDObjectPtr, String> g_objectOrigins;

// =============================================================================
//
void checkGLError_private (const char* file, int line)
{
	QString errmsg;
	GLenum errnum = glGetError();

	if (errnum == GL_NO_ERROR)
		return;

	for (const GLErrorInfo& err : g_GLErrors)
	{
		if (err.value == errnum)
		{
			errmsg = err.text;
			break;
		}
	}

	print ("OpenGL ERROR: at %1:%2: %3", basename (QString (file)), line, errmsg);
}

// =============================================================================
//
GLCompiler::GLCompiler (GLRenderer* renderer) :
	m_renderer (renderer)
{
	needMerge();
	memset (m_vboSizes, 0, sizeof m_vboSizes);
}

// =============================================================================
//
void GLCompiler::initialize()
{
	glGenBuffers (g_numVBOs, &m_vbo[0]);
	checkGLError();
}

// =============================================================================
//
GLCompiler::~GLCompiler()
{
	glDeleteBuffers (g_numVBOs, &m_vbo[0]);
	checkGLError();
}

// =============================================================================
//
uint32 GLCompiler::colorToRGB (const QColor& color)
{
	return
		(color.red()   & 0xFF) << 0x00 |
		(color.green() & 0xFF) << 0x08 |
		(color.blue()  & 0xFF) << 0x10 |
		(color.alpha() & 0xFF) << 0x18;
}

// =============================================================================
//
QColor GLCompiler::indexColorForID (int id) const
{
	// Calculate a color based from this index. This method caters for
	// 16777216 objects. I don't think that will be exceeded anytime soon. :)
	int r = (id / 0x10000) % 0x100,
		g = (id / 0x100) % 0x100,
		b = id % 0x100;

	return QColor (r, g, b);
}

// =============================================================================
//
QColor GLCompiler::getColorForPolygon (LDPolygon& poly, LDObjectPtr topobj,
									   EVBOComplement complement) const
{
	QColor qcol;

	switch (complement)
	{
		case VBOCM_Surfaces:
		case VBOCM_NumComplements:
			return QColor();

		case VBOCM_BFCFrontColors:
			qcol = g_BFCFrontColor;
			break;

		case VBOCM_BFCBackColors:
			qcol = g_BFCBackColor;
			break;

		case VBOCM_PickColors:
			return indexColorForID (topobj->id());

		case VBOCM_RandomColors:
			qcol = topobj->randomColor();
			break;

		case VBOCM_NormalColors:
			if (poly.color == maincolor)
			{
				if (topobj->color() == maincolor)
					qcol = GLRenderer::getMainColor();
				else
					qcol = getColor (topobj->color())->faceColor;
			}
			elif (poly.color == edgecolor)
			{
				qcol = luma (QColor (cfg::backgroundColor)) > 40 ? Qt::black : Qt::white;
			}
			else
			{
				LDColor* col = getColor (poly.color);

				if (col)
					qcol = col->faceColor;
			}
			break;
	}

	if (not qcol.isValid())
	{
		// The color was unknown. Use main color to make the polygon at least
		// not appear pitch-black.
		if (poly.num != 2 && poly.num != 5)
			qcol = GLRenderer::getMainColor();
		else
			qcol = Qt::black;

		// Warn about the unknown color, but only once.
		if (not g_warnedColors.contains (poly.color))
		{
			print ("Unknown color %1!\n", poly.color);
			g_warnedColors << poly.color;
		}

		return qcol;
	}

	double blendAlpha = 0.0;

	if (topobj->isSelected())
		blendAlpha = 1.0;
	elif (topobj == m_renderer->objectAtCursor())
		blendAlpha = 0.5;

	if (blendAlpha != 0.0)
	{
		QColor selcolor (cfg::selectColorBlend);
		double denom = blendAlpha + 1.0;
		qcol.setRed ((qcol.red() + (selcolor.red() * blendAlpha)) / denom);
		qcol.setGreen ((qcol.green() + (selcolor.green() * blendAlpha)) / denom);
		qcol.setBlue ((qcol.blue() + (selcolor.blue() * blendAlpha)) / denom);
	}

	return qcol;
}

// =============================================================================
//
void GLCompiler::needMerge()
{
	for (int i = 0; i < countof (m_vboChanged); ++i)
		m_vboChanged[i] = true;
}

// =============================================================================
//
void GLCompiler::stageForCompilation (LDObjectPtr obj)
{
	/*
	g_objectOrigins[obj] = format ("%1:%2 (%3)",
		obj->document()->getDisplayName(), obj->lineNumber(), obj->typeName());
	*/

	m_staged << LDObjectWeakPtr (obj);
}

// =============================================================================
//
void GLCompiler::unstage (LDObjectPtr obj)
{
	m_staged.removeOne (LDObjectWeakPtr (obj));
}

// =============================================================================
//
void GLCompiler::compileDocument (LDDocumentPtr doc)
{
	if (doc == null)
		return;

	for (LDObjectPtr obj : doc->objects())
		compileObject (obj);
}

// =============================================================================
//
void GLCompiler::compileStaged()
{
	removeDuplicates (m_staged);

	for (LDObjectPtr obj : m_staged)
		compileObject (obj);

	m_staged.clear();
}

// =============================================================================
//
void GLCompiler::prepareVBO (int vbonum)
{
	// Compile anything that still awaits it
	compileStaged();

	if (not m_vboChanged[vbonum])
		return;

	QVector<GLfloat> vbodata;

	for (auto it = m_objectInfo.begin(); it != m_objectInfo.end(); ++it)
	{
		if (it.key() == null)
			it = m_objectInfo.erase (it);
		elif (it.key().toStrongRef()->document() == getCurrentDocument() && not it.key().toStrongRef()->isHidden())
			vbodata += it->data[vbonum];
	}

	glBindBuffer (GL_ARRAY_BUFFER, m_vbo[vbonum]);
	glBufferData (GL_ARRAY_BUFFER, vbodata.size() * sizeof(GLfloat), vbodata.constData(), GL_STATIC_DRAW);
	glBindBuffer (GL_ARRAY_BUFFER, 0);
	checkGLError();
	m_vboChanged[vbonum] = false;
	m_vboSizes[vbonum] = vbodata.size();
}

// =============================================================================
//
void GLCompiler::dropObject (LDObjectPtr obj)
{
	auto it = m_objectInfo.find (obj);

	if (it != m_objectInfo.end())
	{
		m_objectInfo.erase (it);
		needMerge();
	}

	unstage (obj);
}

// =============================================================================
//
void GLCompiler::compileObject (LDObjectPtr obj)
{
//	print ("Compile %1\n", g_objectOrigins[obj]);

	if (obj == null || obj->document() == null || obj->document().toStrongRef()->isImplicit())
		return;

	ObjectVBOInfo info;
	info.isChanged = true;
	dropObject (obj);

	switch (obj->type())
	{
		// Note: We cannot split quads into triangles here, it would mess up the
		// wireframe view. Quads must go into separate vbos.
		case OBJ_Triangle:
		case OBJ_Quad:
		case OBJ_Line:
		case OBJ_CondLine:
		{
			LDPolygon* poly = obj->getPolygon();
			poly->id = obj->id();
			compilePolygon (*poly, obj, &info);
			delete poly;
			break;
		}

		case OBJ_Subfile:
		{
			LDSubfilePtr ref = obj.staticCast<LDSubfile>();
			auto data = ref->inlinePolygons();

			for (LDPolygon& poly : data)
			{
				poly.id = obj->id();
				compilePolygon (poly, obj, &info);
			}
			break;
		}

		default:
			break;
	}

	m_objectInfo[obj] = info;
	needMerge();
}

// =============================================================================
//
void GLCompiler::compilePolygon (LDPolygon& poly, LDObjectPtr topobj, ObjectVBOInfo* objinfo)
{
	EVBOSurface surface;
	int numverts;

	switch (poly.num)
	{
		case 2:	surface = VBOSF_Lines;		numverts = 2; break;
		case 3:	surface = VBOSF_Triangles;	numverts = 3; break;
		case 4:	surface = VBOSF_Quads;		numverts = 4; break;
		case 5:	surface = VBOSF_CondLines;	numverts = 2; break;
		default: return;
	}

	for (EVBOComplement complement = VBOCM_First; complement < VBOCM_NumComplements; ++complement)
	{
		const int vbonum			= vboNumber (surface, complement);
		QVector<GLfloat>& vbodata	= objinfo->data[vbonum];
		const QColor color			= getColorForPolygon (poly, topobj, complement);

		for (int vert = 0; vert < numverts; ++vert)
		{
			if (complement == VBOCM_Surfaces)
			{
				// Write coordinates. Apparently Z must be flipped too?
				vbodata	<< poly.vertices[vert].x()
						<< -poly.vertices[vert].y()
						<< -poly.vertices[vert].z();
			}
			else
			{
				vbodata	<< ((GLfloat) color.red()) / 255.0f
						<< ((GLfloat) color.green()) / 255.0f
						<< ((GLfloat) color.blue()) / 255.0f
						<< ((GLfloat) color.alpha()) / 255.0f;
			}
		}
	}
}
