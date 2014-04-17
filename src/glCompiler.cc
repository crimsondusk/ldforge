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

cfg (String,	gl_selectcolor,			"#0080FF")

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

#include <QTime>
#include <QFile>

#define CLOCK_INIT QTime t0;

#define CLOCK_START \
{ \
	t0 = QTime::currentTime(); \
}

#define CLOCK_TIME(A) \
{ \
	fprint (stderr, A ": %1ms\n", t0.msecsTo (QTime::currentTime())); \
}

#define DEBUG_PRINT(...) fprint (stdout, __VA_ARGS__)

extern_cfg (Bool, gl_blackedges);
extern_cfg (String, gl_bgcolor);
static QList<short>		g_warnedColors;
static const QColor		g_BFCFrontColor (40, 192, 40);
static const QColor		g_BFCBackColor (224, 40, 40);
static QMap<LDObject*, QString> g_objectOrigins;

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
GLCompiler::GLCompiler()
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
QColor GLCompiler::polygonColor (LDPolygon& poly, LDObject* topobj) const
{
	QColor qcol;

	if (poly.color == maincolor)
	{
		if (topobj->color() == maincolor)
			qcol = GLRenderer::getMainColor();
		else
			qcol = getColor (topobj->color())->faceColor;
	}
	elif (poly.color == edgecolor)
	{
		qcol = luma (QColor (gl_bgcolor)) > 40 ? Qt::black : Qt::white;
	}
	else
	{
		LDColor* col = getColor (poly.color);

		if (col)
			qcol = col->faceColor;
	}

	if (qcol.isValid() == false)
	{
		// The color was unknown. Use main color to make the poly.object at least
		// not appear pitch-black.
		if (poly.num != 2 && poly.num != 5)
			qcol = GLRenderer::getMainColor();
		else
			qcol = Qt::black;

		// Warn about the unknown color, but only once.
		if (g_warnedColors.contains (poly.color) == false)
		{
			print ("Unknown color %1!\n", poly.color);
			g_warnedColors << poly.color;
		}

		return qcol;
	}

	if (topobj->isSelected())
	{
		// Brighten it up for the select list.
		QColor selcolor (gl_selectcolor);
		qcol.setRed ((qcol.red() + selcolor.red()) / 2);
		qcol.setGreen ((qcol.green() + selcolor.green()) / 2);
		qcol.setBlue ((qcol.blue() + selcolor.blue()) / 2);
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
void GLCompiler::stageForCompilation (LDObject* obj)
{
	m_staged << obj;
}

// =============================================================================
//
void GLCompiler::compileDocument (LDDocument* doc)
{
	if (doc == null)
		return;

	for (LDObject* obj : doc->objects())
		compileObject (obj);
}

// =============================================================================
//
void GLCompiler::compileStaged()
{
	removeDuplicates (m_staged);

	for (LDObject* obj : m_staged)
		compileObject (obj);

	m_staged.clear();
}

// =============================================================================
//
void GLCompiler::prepareVBO (int vbonum)
{
	// Compile anything that still awaits it
	compileStaged();

	if (m_vboChanged[vbonum] == false)
		return;

	QVector<GLfloat> vbodata;

	/*
	if (vbonum == vboNumber (VBOSF_Triangles, VBOCM_Surfaces))
	{
		QFile f ("vbo.log");
		if (f.open (QIODevice::WriteOnly))
		{
			for (auto it = m_objectInfo.begin(); it != m_objectInfo.end(); ++it)
			{
				if (it.key()->document() == getCurrentDocument())
				{
					fprint (f, "----- #%1 (%2:%3)\n", it.key()->id(),
						it.key()->document()->getDisplayName(), it.key()->lineNumber());
					const QVector<GLfloat>& v = it.value().data[vbonum];

					for (int i = 0; i < v.size() / 3; ++i)
						fprint (f, "- %1, %2, %3\n", v[i * 3], v[1 * 3 + 1], v[1 * 3 + 2]);
				}
			}

			f.close();
		}
	}
	*/

	for (auto it = m_objectInfo.begin(); it != m_objectInfo.end(); ++it)
	{
		if (it.key()->document() == getCurrentDocument() && it.key()->isHidden() == false)
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
void GLCompiler::dropObject (LDObject* obj)
{
	auto it = m_objectInfo.find (obj);

	if (it != m_objectInfo.end())
	{
		m_objectInfo.erase (it);
		needMerge();
	}
}

// =============================================================================
//
void GLCompiler::compileObject (LDObject* obj)
{
	if (obj->document()->isImplicit())
		return;

	g_objectOrigins[obj] = obj->document()->getDisplayName() + ":" + QString::number (obj->lineNumber());
	ObjectVBOInfo info;
	info.isChanged = true;
	dropObject (obj);
	compileSubObject (obj, obj, &info);
	m_objectInfo[obj] = info;
	needMerge();
}

// =============================================================================
//
void GLCompiler::compilePolygon (LDPolygon& poly, LDObject* topobj, ObjectVBOInfo* objinfo)
{
	EVBOSurface surface;
	int numverts;

	switch (poly.num)
	{
		case 2:	surface = VBOSF_Lines;		numverts = 2; break;
		case 3:	surface = VBOSF_Triangles;	numverts = 3; break;
		case 4:	surface = VBOSF_Quads;		numverts = 4; break;
		case 5:	surface = VBOSF_CondLines;	numverts = 2; break;

		default:
			print ("OMGWTFBBQ weird polygon with number %1 (topobj: #%2, %3), origin: %4",
				(int) poly.num, topobj->id(), topobj->typeName(), poly.origin);
			assert (false);
	}

	if (poly.num == 3)
		print ("compile triangle %1 for #%2\n", poly.origin, topobj->id());

	for (int complement = 0; complement < VBOCM_NumComplements; ++complement)
	{
		const int vbonum			= vboNumber (surface, (EVBOComplement) complement);
		QVector<GLfloat>& vbodata	= objinfo->data[vbonum];
		const QColor normalColor	= polygonColor (poly, topobj);
		const QColor pickColor		= indexColorForID (topobj->id());

		for (int vert = 0; vert < numverts; ++vert)
		{
			switch ((EVBOComplement) complement)
			{
				case VBOCM_Surfaces:
				{
					// Write coordinates. Apparently Z must be flipped too?
					vbodata	<< poly.vertices[vert].x()
							<< -poly.vertices[vert].y()
							<< -poly.vertices[vert].z();
					break;
				}

				case VBOCM_NormalColors:
				{
					writeColor (vbodata, normalColor);
					break;
				}

				case VBOCM_PickColors:
				{
					writeColor (vbodata, pickColor);
					break;
				}

				case VBOCM_BFCFrontColors:
				{
					writeColor (vbodata, g_BFCFrontColor);
					break;
				}

				case VBOCM_BFCBackColors:
				{
					writeColor (vbodata, g_BFCBackColor);
					break;
				}

				case VBOCM_NumComplements:
					break;
			}
		}
	}
}

// =============================================================================
//
void GLCompiler::compileSubObject (LDObject* obj, LDObject* topobj, ObjectVBOInfo* objinfo)
{
	switch (obj->type())
	{
		// Note: We cannot split quads into triangles here, it would mess up the
		// wireframe view. Quads must go into separate vbos.
		case LDObject::ETriangle:
		case LDObject::EQuad:
		case LDObject::ELine:
		case LDObject::ECondLine:
		{
			LDPolygon* poly = obj->getPolygon();
			poly->id = topobj->id();
			compilePolygon (*poly, topobj, objinfo);
			delete poly;
			break;
		}

		case LDObject::ESubfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			auto data = ref->inlinePolygons();

			for (LDPolygon& poly : data)
			{
				poly.id = topobj->id();
				compilePolygon (poly, topobj, objinfo);
			}
			break;
		}

		default:
			break;
	}
}

// =============================================================================
//
void GLCompiler::writeColor (QVector<GLfloat>& array, const QColor& color)
{
	array	<< ((GLfloat) color.red()) / 255.0f
			<< ((GLfloat) color.green()) / 255.0f
			<< ((GLfloat) color.blue()) / 255.0f
			<< ((GLfloat) color.alpha()) / 255.0f;
}
