#define GL_GLEXT_PROTOTYPES
#include <GL/glu.h>
#include <GL/glext.h>
#include "GLCompiler.h"
#include "LDObject.h"
#include "Colors.h"
#include "Document.h"
#include "Misc.h"
#include "GLRenderer.h"
#include "Dialogs.h"

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
static QList<short>		gWarnedColors;
static const QColor		gBFCFrontColor (40, 192, 40);
static const QColor		gBFCBackColor (224, 40, 40);

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
	memset (mVBOSizes, 0, sizeof mVBOSizes);
}

// =============================================================================
//
void GLCompiler::initialize()
{
	glGenBuffers (gNumVBOs, &mVBOs[0]);
	checkGLError();
}

// =============================================================================
//
GLCompiler::~GLCompiler()
{
	glDeleteBuffers (gNumVBOs, &mVBOs[0]);
	checkGLError();
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
QColor GLCompiler::getIndexColor (int id) const
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
QColor GLCompiler::getPolygonColor (LDPolygon& poly, LDObject* topobj) const
{
	QColor qcol;

	if (poly.color == maincolor)
		qcol = GLRenderer::getMainColor();
	else
	{
		LDColor* col = getColor (poly.color);

		if (col)
			qcol = col->faceColor;
	}

	if (poly.color == edgecolor)
	{
		qcol = QColor (32, 32, 32); // luma (m_bgcolor) < 40 ? QColor (64, 64, 64) : Qt::black;

		/*
		if (!gl_blackedges && poly.obj->getParent() && (col = getColor (poly.obj->getParent()->getColor())))
			qcol = col->edgeColor;
		*/
	}

	if (qcol.isValid() == false)
	{
		// The color was unknown. Use main color to make the poly.object at least
		// not appear pitch-black.
		if (poly.color != edgecolor)
			qcol = GLRenderer::getMainColor();
		else
			qcol = Qt::black;

		// Warn about the unknown color, but only once.
		for (short i : gWarnedColors)
			if (poly.color == i)
				return qcol;

		print ("%1: Unknown color %2!\n", __func__, poly.color);
		gWarnedColors << poly.color;
		return qcol;
	}

	if (topobj->isSelected())
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
	// Set all of mChanged to true
	// memset (mChanged, 0xFF, sizeof mChanged);
	for (int i = 0; i < ((int) (sizeof mChanged / sizeof *mChanged)); ++i)
		mChanged[i] = true;
}

// =============================================================================
//
void GLCompiler::stageForCompilation (LDObject* obj)
{
	mStaged << obj;
	removeDuplicates (mStaged);
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
	for (LDObject* obj : mStaged)
		compileObject (obj);

	mStaged.clear();
}

// =============================================================================
//
void GLCompiler::prepareVBO (int vbonum)
{
	// Compile anything that still awaits it
	compileStaged();

	if (mChanged[vbonum] == false)
		return;

	QVector<GLfloat> vbodata;

	for (auto it = mObjectInfo.begin(); it != mObjectInfo.end(); ++it)
	{
		if (it.key()->document() == getCurrentDocument())
			vbodata += it->data[vbonum];
	}

	glBindBuffer (GL_ARRAY_BUFFER, mVBOs[vbonum]);
	glBufferData (GL_ARRAY_BUFFER, vbodata.size() * sizeof(GLfloat), vbodata.constData(), GL_DYNAMIC_DRAW);
	glBindBuffer (GL_ARRAY_BUFFER, 0);
	checkGLError();
	mChanged[vbonum] = false;
	mVBOSizes[vbonum] = vbodata.size();
}

// =============================================================================
//
void GLCompiler::dropObject (LDObject* obj)
{
	auto it = mObjectInfo.find (obj);

	if (it != mObjectInfo.end())
	{
		mObjectInfo.erase (it);
		needMerge();
	}
}

// =============================================================================
//
void GLCompiler::compileObject (LDObject* obj)
{
	print ("compiling #%1\n", obj->id());
	ObjectVBOInfo info;
	dropObject (obj);
	compileSubObject (obj, obj, &info);
	mObjectInfo[obj] = info;
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
		case 3:	surface = vboTriangles;	numverts = 3; break;
		case 4:	surface = vboQuads;		numverts = 4; break;
		case 2:	surface = vboLines;		numverts = 2; break;
		case 5:	surface = vboCondLines;	numverts = 2; break;

		default:
			print ("OMGWTFBBQ weird polygon with number %1 (topobj: #%2, %3), origin: %4",
				(int) poly.num, topobj->id(), topobj->typeName(), poly.origin);
			assert (false);
	}

	for (int complement = 0; complement < vboNumComplements; ++complement)
	{
		const int vbonum			= getVBONumber (surface, (EVBOComplement) complement);
		QVector<GLfloat>& vbodata	= objinfo->data[vbonum];
		const QColor normalColor	= getPolygonColor (poly, topobj);
		const QColor pickColor		= getIndexColor (topobj->id());

		for (int vert = 0; vert < numverts; ++vert)
		{
			switch ((EVBOComplement) complement)
			{
				case vboSurfaces:
				{
					// Write coordinates. Apparently Z must be flipped too?
					vbodata	<< poly.vertices[vert].x()
							<< -poly.vertices[vert].y()
							<< -poly.vertices[vert].z();
					break;
				}

				case vboNormalColors:
				{
					writeColor (vbodata, normalColor);
					break;
				}

				case vboPickColors:
				{
					writeColor (vbodata, pickColor);
					break;
				}

				case vboBFCFrontColors:
				{
					writeColor (vbodata, gBFCFrontColor);
					break;
				}

				case vboBFCBackColors:
				{
					writeColor (vbodata, gBFCBackColor);
					break;
				}

				case vboNumComplements:
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
	array	<< ((float) color.red()) / 255.0f
			<< ((float) color.green()) / 255.0f
			<< ((float) color.blue()) / 255.0f
			<< ((float) color.alpha()) / 255.0f;
}
