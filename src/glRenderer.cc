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
#include <QGLWidget>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QToolTip>
#include <qtextdocument.h>
#include <QTimer>
#include <GL/glu.h>

#include "main.h"
#include "configuration.h"
#include "ldDocument.h"
#include "glRenderer.h"
#include "colors.h"
#include "mainWindow.h"
#include "miscallenous.h"
#include "editHistory.h"
#include "dialogs.h"
#include "addObjectDialog.h"
#include "messageLog.h"
#include "primitives.h"
#include "misc/ringFinder.h"
#include "glCompiler.h"

static const LDFixedCameraInfo g_FixedCameras[6] =
{
	{{  1,  0, 0 }, X, Z, false, false, false }, // top
	{{  0,  0, 0 }, X, Y, false,  true, false }, // front
	{{  0,  1, 0 }, Z, Y,  true,  true, false }, // left
	{{ -1,  0, 0 }, X, Z, false,  true, true }, // bottom
	{{  0,  0, 0 }, X, Y,  true,  true, true }, // back
	{{  0, -1, 0 }, Z, Y, false,  true, true }, // right
};

// Matrix templates for circle drawing. 2 is substituted with
// the scale value, 1 is inverted to -1 if needed.
static const Matrix g_circleDrawMatrixTemplates[3] =
{
	{ 2, 0, 0, 0, 1, 0, 0, 0, 2 },
	{ 2, 0, 0, 0, 0, 2, 0, 1, 0 },
	{ 0, 1, 0, 2, 0, 0, 0, 0, 2 },
};

CFGENTRY (String,	backgroundColor,			"#FFFFFF")
CFGENTRY (String,	mainColor,					"#A0A0A0")
CFGENTRY (Float,	mainColorAlpha,				1.0)
CFGENTRY (Int,		lineThickness,				2)
CFGENTRY (Bool,		bfcRedGreenView,			false)
CFGENTRY (Int,		camera,						EFreeCamera)
CFGENTRY (Bool,		blackEdges,					false)
CFGENTRY (Bool,		drawAxes,					false)
CFGENTRY (Bool,		drawWireframe,				false)
CFGENTRY (Bool,		useLogoStuds,				false)
CFGENTRY (Bool,		antiAliasedLines,			true)
CFGENTRY (Bool,		drawLineLengths,			true)
CFGENTRY (Bool,		drawAngles,					false)
CFGENTRY (Bool,		randomColors,				false)
CFGENTRY (Bool,		highlightObjectBelowCursor,	true)

// argh
const char* g_CameraNames[7] =
{
	QT_TRANSLATE_NOOP ("GLRenderer",  "Top"),
	QT_TRANSLATE_NOOP ("GLRenderer",  "Front"),
	QT_TRANSLATE_NOOP ("GLRenderer",  "Left"),
	QT_TRANSLATE_NOOP ("GLRenderer",  "Bottom"),
	QT_TRANSLATE_NOOP ("GLRenderer",  "Back"),
	QT_TRANSLATE_NOOP ("GLRenderer",  "Right"),
	QT_TRANSLATE_NOOP ("GLRenderer",  "Free")
};

struct LDGLAxis
{
	const QColor col;
	const Vertex vert;
};

// Definitions for visual axes, drawn on the screen
static const LDGLAxis g_GLAxes[3] =
{
	{ QColor (192,  96,  96), Vertex (10000, 0, 0) }, // X
	{ QColor (48,  192,  48), Vertex (0, 10000, 0) }, // Y
	{ QColor (48,  112, 192), Vertex (0, 0, 10000) }, // Z
};

static GLuint g_GLAxes_VBO;
static GLuint g_GLAxes_ColorVBO;

// =============================================================================
//
GLRenderer::GLRenderer (QWidget* parent) : QGLWidget (parent)
{
	m_isPicking = m_rangepick = false;
	m_camera = (ECamera) cfg::camera;
	m_drawToolTip = false;
	m_editMode = ESelectMode;
	m_rectdraw = false;
	m_panning = false;
	m_compiler = new GLCompiler (this);
	setDocument (null);
	setDrawOnly (false);
	setMessageLog (null);
	m_width = m_height = -1;
	m_hoverpos = g_origin;
	m_toolTipTimer = new QTimer (this);
	m_toolTipTimer->setSingleShot (true);
	m_objectAtCursor = null;
	m_isCameraMoving = false;
	connect (m_toolTipTimer, SIGNAL (timeout()), this, SLOT (slot_toolTipTimer()));

	m_thickBorderPen = QPen (QColor (0, 0, 0, 208), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	m_thinBorderPen = m_thickBorderPen;
	m_thinBorderPen.setWidth (1);

	// Init camera icons
	for (ECamera cam = EFirstCamera; cam < ENumCameras; ++cam)
	{
		String iconname = format ("camera-%1", tr (g_CameraNames[cam]).toLower());
		CameraIcon* info = &m_cameraIcons[cam];
		info->img = new QPixmap (getIcon (iconname));
		info->cam = cam;
	}

	calcCameraIcons();
}

// =============================================================================
//
GLRenderer::~GLRenderer()
{
	for (int i = 0; i < 6; ++i)
		delete currentDocumentData().overlays[i].img;

	for (CameraIcon& info : m_cameraIcons)
		delete info.img;

	delete m_compiler;
}

// =============================================================================
// Calculates the "hitboxes" of the camera icons so that we can tell when the
// cursor is pointing at the camera icon.
//
void GLRenderer::calcCameraIcons()
{
	int i = 0;

	for (CameraIcon& info : m_cameraIcons)
	{
		// MATH
		const long x1 = (m_width - (info.cam != EFreeCamera ? 48 : 16)) + ((i % 3) * 16) - 1,
			y1 = ((i / 3) * 16) + 1;

		info.srcRect = QRect (0, 0, 16, 16);
		info.destRect = QRect (x1, y1, 16, 16);
		info.selRect = QRect (
			info.destRect.x(),
			info.destRect.y(),
			info.destRect.width() + 1,
			info.destRect.height() + 1
		);

		++i;
	}
}

// =============================================================================
//
void GLRenderer::initGLData()
{
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_POLYGON_OFFSET_FILL);
	glPolygonOffset (1.0f, 1.0f);

	glEnable (GL_DEPTH_TEST);
	glShadeModel (GL_SMOOTH);
	glEnable (GL_MULTISAMPLE);

	if (cfg::antiAliasedLines)
	{
		glEnable (GL_LINE_SMOOTH);
		glEnable (GL_POLYGON_SMOOTH);
		glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
		glHint (GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	} else
	{
		glDisable (GL_LINE_SMOOTH);
		glDisable (GL_POLYGON_SMOOTH);
	}
}

// =============================================================================
//
void GLRenderer::needZoomToFit()
{
	if (document() != null)
		currentDocumentData().needZoomToFit = true;
}

// =============================================================================
//
void GLRenderer::resetAngles()
{
	rot (X) = 30.0f;
	rot (Y) = 325.f;
	pan (X) = pan (Y) = rot (Z) = 0.0f;
	needZoomToFit();
}

// =============================================================================
//
void GLRenderer::resetAllAngles()
{
	ECamera oldcam = camera();

	for (int i = 0; i < 7; ++i)
	{
		setCamera ((ECamera) i);
		resetAngles();
	}

	setCamera (oldcam);
}

// =============================================================================
//
void GLRenderer::initializeGL()
{
	setBackground();
	glLineWidth (cfg::lineThickness);
	glLineStipple (1, 0x6666);
	setAutoFillBackground (false);
	setMouseTracking (true);
	setFocusPolicy (Qt::WheelFocus);
	compiler()->initialize();
	initializeAxes();
}

// =============================================================================
//
void GLRenderer::initializeAxes()
{
	float axesdata[18];
	float colordata[18];
	memset (axesdata, 0, sizeof axesdata);

	for (int i = 0; i < 3; ++i)
	{
		for_axes (ax)
		{
			axesdata[(i * 6) + ax] = g_GLAxes[i].vert[ax];
			axesdata[(i * 6) + 3 + ax] = -g_GLAxes[i].vert[ax];
		}

		for (int j = 0; j < 2; ++j)
		{
			colordata[(i * 6) + (j * 3) + 0] = g_GLAxes[i].col.red();
			colordata[(i * 6) + (j * 3) + 1] = g_GLAxes[i].col.green();
			colordata[(i * 6) + (j * 3) + 2] = g_GLAxes[i].col.blue();
		}
	}

	glGenBuffers (1, &g_GLAxes_VBO);
	glBindBuffer (GL_ARRAY_BUFFER, g_GLAxes_VBO);
	glBufferData (GL_ARRAY_BUFFER, sizeof axesdata, axesdata, GL_STATIC_DRAW);
	glGenBuffers (1, &g_GLAxes_ColorVBO);
	glBindBuffer (GL_ARRAY_BUFFER, g_GLAxes_ColorVBO);
	glBufferData (GL_ARRAY_BUFFER, sizeof colordata, colordata, GL_STATIC_DRAW);
	glBindBuffer (GL_ARRAY_BUFFER, 0);
}

// =============================================================================
//
QColor GLRenderer::getMainColor()
{
	QColor col (cfg::mainColor);

	if (not col.isValid())
		return QColor (0, 0, 0);

	col.setAlpha (cfg::mainColorAlpha * 255.f);
	return col;
}

// =============================================================================
//
void GLRenderer::setBackground()
{
	if (isPicking())
	{
		glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
		return;
	}

	QColor col (cfg::backgroundColor);

	if (not col.isValid())
		return;

	col.setAlpha (255);

	m_darkbg = luma (col) < 80;
	m_bgcolor = col;
	qglClearColor (col);
}

// =============================================================================
//
void GLRenderer::refresh()
{
	update();
	swapBuffers();
}

// =============================================================================
//
void GLRenderer::hardRefresh()
{
	compiler()->compileDocument (getCurrentDocument());
	refresh();
	glLineWidth (cfg::lineThickness); // TODO: ...?
}

// =============================================================================
//
void GLRenderer::resizeGL (int w, int h)
{
	m_width = w;
	m_height = h;

	calcCameraIcons();

	glViewport (0, 0, w, h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluPerspective (45.0f, (double) w / (double) h, 1.0f, 10000.0f);
	glMatrixMode (GL_MODELVIEW);
}

// =============================================================================
//
void GLRenderer::drawGLScene()
{
	if (document() == null)
		return;

	if (currentDocumentData().needZoomToFit)
	{
		currentDocumentData().needZoomToFit = false;
		zoomAllToFit();
	}

	if (cfg::drawWireframe && not isPicking())
		glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable (GL_DEPTH_TEST);

	if (camera() != EFreeCamera)
	{
		glMatrixMode (GL_PROJECTION);
		glPushMatrix();

		glLoadIdentity();
		glOrtho (-m_virtWidth, m_virtWidth, -m_virtHeight, m_virtHeight, -100.0f, 100.0f);
		glTranslatef (pan (X), pan (Y), 0.0f);

		if (camera() != EFrontCamera && camera() != EBackCamera)
		{
			glRotatef (90.0f, g_FixedCameras[camera()].glrotate[0],
				g_FixedCameras[camera()].glrotate[1],
				g_FixedCameras[camera()].glrotate[2]);
		}

		// Back camera needs to be handled differently
		if (camera() == EBackCamera)
		{
			glRotatef (180.0f, 1.0f, 0.0f, 0.0f);
			glRotatef (180.0f, 0.0f, 0.0f, 1.0f);
		}
	}
	else
	{
		glMatrixMode (GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glTranslatef (0.0f, 0.0f, -2.0f);
		glTranslatef (pan (X), pan (Y), -zoom());
		glRotatef (rot (X), 1.0f, 0.0f, 0.0f);
		glRotatef (rot (Y), 0.0f, 1.0f, 0.0f);
		glRotatef (rot (Z), 0.0f, 0.0f, 1.0f);
	}

	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_COLOR_ARRAY);

	if (isPicking())
	{
		drawVBOs (VBOSF_Triangles, VBOCM_PickColors, GL_TRIANGLES);
		drawVBOs (VBOSF_Quads, VBOCM_PickColors, GL_QUADS);
		drawVBOs (VBOSF_Lines, VBOCM_PickColors, GL_LINES);
		drawVBOs (VBOSF_CondLines, VBOCM_PickColors, GL_LINES);
	}
	else
	{
		if (cfg::bfcRedGreenView)
		{
			glEnable (GL_CULL_FACE);
			glCullFace (GL_BACK);
			drawVBOs (VBOSF_Triangles, VBOCM_BFCFrontColors, GL_TRIANGLES);
			drawVBOs (VBOSF_Quads, VBOCM_BFCFrontColors, GL_QUADS);
			glCullFace (GL_FRONT);
			drawVBOs (VBOSF_Triangles, VBOCM_BFCBackColors, GL_TRIANGLES);
			drawVBOs (VBOSF_Quads, VBOCM_BFCBackColors, GL_QUADS);
			glDisable (GL_CULL_FACE);
		}
		else
		{
			if (cfg::randomColors)
			{
				drawVBOs (VBOSF_Triangles, VBOCM_RandomColors, GL_TRIANGLES);
				drawVBOs (VBOSF_Quads, VBOCM_RandomColors, GL_QUADS);
			}
			else
			{
				drawVBOs (VBOSF_Triangles, VBOCM_NormalColors, GL_TRIANGLES);
				drawVBOs (VBOSF_Quads, VBOCM_NormalColors, GL_QUADS);
			}
		}

		drawVBOs (VBOSF_Lines, VBOCM_NormalColors, GL_LINES);
		glEnable (GL_LINE_STIPPLE);
		drawVBOs (VBOSF_CondLines, VBOCM_NormalColors, GL_LINES);
		glDisable (GL_LINE_STIPPLE);

		if (cfg::drawAxes)
		{
			glBindBuffer (GL_ARRAY_BUFFER, g_GLAxes_VBO);
			glVertexPointer (3, GL_FLOAT, 0, NULL);
			glBindBuffer (GL_ARRAY_BUFFER, g_GLAxes_VBO);
			glColorPointer (3, GL_FLOAT, 0, NULL);
			glDrawArrays (GL_LINES, 0, 6);
			checkGLError();
		}
	}

	glPopMatrix();
	glBindBuffer (GL_ARRAY_BUFFER, 0);
	glDisableClientState (GL_VERTEX_ARRAY);
	glDisableClientState (GL_COLOR_ARRAY);
	checkGLError();
	glDisable (GL_CULL_FACE);
	glMatrixMode (GL_MODELVIEW);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
}

// =============================================================================
//
void GLRenderer::drawVBOs (EVBOSurface surface, EVBOComplement colors, GLenum type)
{
	int surfacenum = m_compiler->vboNumber (surface, VBOCM_Surfaces);
	int colornum = m_compiler->vboNumber (surface, colors);
	m_compiler->prepareVBO (surfacenum);
	m_compiler->prepareVBO (colornum);
	GLuint surfacevbo = m_compiler->vbo (surfacenum);
	GLuint colorvbo = m_compiler->vbo (colornum);
	GLsizei count = m_compiler->vboSize (surfacenum) / 3;

	if (count > 0)
	{
		glBindBuffer (GL_ARRAY_BUFFER, surfacevbo);
		glVertexPointer (3, GL_FLOAT, 0, null);
		checkGLError();
		glBindBuffer (GL_ARRAY_BUFFER, colorvbo);
		glColorPointer (4, GL_FLOAT, 0, null);
		checkGLError();
		glDrawArrays (type, 0, count);
		checkGLError();
	}
}

// =============================================================================
// This converts a 2D point on the screen to a 3D point in the model. If 'snap'
// is true, the 3D point will snap to the current grid.
//
Vertex GLRenderer::coordconv2_3 (const QPoint& pos2d, bool snap) const
{
	assert (camera() != EFreeCamera);

	Vertex pos3d;
	const LDFixedCameraInfo* cam = &g_FixedCameras[camera()];
	const Axis axisX = cam->axisX;
	const Axis axisY = cam->axisY;
	const int negXFac = cam->negX ? -1 : 1,
				negYFac = cam->negY ? -1 : 1;

	// Calculate cx and cy - these are the LDraw unit coords the cursor is at.
	double cx = (-m_virtWidth + ((2 * pos2d.x() * m_virtWidth) / m_width) - pan (X));
	double cy = (m_virtHeight - ((2 * pos2d.y() * m_virtHeight) / m_height) - pan (Y));

	if (snap)
	{
		cx = Grid::snap (cx, Grid::Coordinate);
		cy = Grid::snap (cy, Grid::Coordinate);
	}

	cx *= negXFac;
	cy *= negYFac;

	roundToDecimals (cx, 4);
	roundToDecimals (cy, 4);

	// Create the vertex from the coordinates
	pos3d.setCoordinate (axisX, cx);
	pos3d.setCoordinate (axisY, cy);
	pos3d.setCoordinate ((Axis) (3 - axisX - axisY), getDepthValue());
	return pos3d;
}

// =============================================================================
//
// Inverse operation for the above - convert a 3D position to a 2D screen
// position. Don't ask me how this code manages to work, I don't even know.
//
QPoint GLRenderer::coordconv3_2 (const Vertex& pos3d) const
{
	GLfloat m[16];
	const LDFixedCameraInfo* cam = &g_FixedCameras[camera()];
	const Axis axisX = cam->axisX;
	const Axis axisY = cam->axisY;
	const int negXFac = cam->negX ? -1 : 1,
				negYFac = cam->negY ? -1 : 1;

	glGetFloatv (GL_MODELVIEW_MATRIX, m);

	const double x = pos3d.x();
	const double y = pos3d.y();
	const double z = pos3d.z();

	Vertex transformed;
	transformed.setX ((m[0] * x) + (m[1] * y) + (m[2] * z) + m[3]);
	transformed.setY ((m[4] * x) + (m[5] * y) + (m[6] * z) + m[7]);
	transformed.setZ ((m[8] * x) + (m[9] * y) + (m[10] * z) + m[11]);

	double rx = (((transformed[axisX] * negXFac) + m_virtWidth + pan (X)) * m_width) / (2 * m_virtWidth);
	double ry = (((transformed[axisY] * negYFac) - m_virtHeight + pan (Y)) * m_height) / (2 * m_virtHeight);

	return QPoint (rx, -ry);
}

// =============================================================================
//
void GLRenderer::paintEvent (QPaintEvent*)
{
	makeCurrent();
	m_virtWidth = zoom();
	m_virtHeight = (m_height * m_virtWidth) / m_width;
	initGLData();
	drawGLScene();

	const QPen textpen (m_darkbg ? Qt::white : Qt::black);
	const QBrush polybrush (QColor (64, 192, 0, 128));
	QPainter paint (this);
	QFontMetrics metrics = QFontMetrics (QFont());
	paint.setRenderHint (QPainter::HighQualityAntialiasing);

	// If we wish to only draw the brick, stop here
	if (isDrawOnly())
		return;

#ifndef RELEASE
	if (not isPicking())
	{
		String text = format ("Rotation: (%1, %2, %3)\nPanning: (%4, %5), Zoom: %6",
			rot(X), rot(Y), rot(Z), pan(X), pan(Y), zoom());
		QRect textSize = metrics.boundingRect (0, 0, m_width, m_height, Qt::AlignCenter, text);
		paint.setPen (textpen);
		paint.drawText ((width() - textSize.width()) / 2, height() - textSize.height(), textSize.width(),
			textSize.height(), Qt::AlignCenter, text);
	}
#endif

	if (camera() != EFreeCamera && !isPicking())
	{
		// Paint the overlay image if we have one
		const LDGLOverlay& overlay = currentDocumentData().overlays[camera()];

		if (overlay.img != null)
		{
			QPoint v0 = coordconv3_2 (currentDocumentData().overlays[camera()].v0),
					   v1 = coordconv3_2 (currentDocumentData().overlays[camera()].v1);

			QRect targRect (v0.x(), v0.y(), abs (v1.x() - v0.x()), abs (v1.y() - v0.y())),
				  srcRect (0, 0, overlay.img->width(), overlay.img->height());
			paint.drawImage (targRect, *overlay.img, srcRect);
		}

		// Paint the coordinates onto the screen.
		String text = format (tr ("X: %1, Y: %2, Z: %3"), m_hoverpos[X], m_hoverpos[Y], m_hoverpos[Z]);
		QFontMetrics metrics = QFontMetrics (font());
		QRect textSize = metrics.boundingRect (0, 0, m_width, m_height, Qt::AlignCenter, text);
		paint.setPen (textpen);
		paint.drawText (m_width - textSize.width(), m_height - 16, textSize.width(),
			textSize.height(), Qt::AlignCenter, text);

		QPen linepen = m_thinBorderPen;
		linepen.setWidth (2);
		linepen.setColor (luma (m_bgcolor) < 40 ? Qt::white : Qt::black);

		// Mode-specific rendering
		if (editMode() == EDrawMode)
		{
			QPoint poly[4];
			Vertex poly3d[4];
			int numverts = 4;

			// Calculate polygon data
			if (not m_rectdraw)
			{
				numverts = m_drawedVerts.size() + 1;
				int i = 0;

				for (Vertex& vert : m_drawedVerts)
					poly3d[i++] = vert;

				// Draw the cursor vertex as the last one in the list.
				if (numverts <= 4)
					poly3d[i] = m_hoverpos;
				else
					numverts = 4;
			}
			else
			{
				// Get vertex information from m_rectverts
				if (m_drawedVerts.size() > 0)
					for (int i = 0; i < numverts; ++i)
						poly3d[i] = m_rectverts[i];
				else
					poly3d[0] = m_hoverpos;
			}

			// Convert to 2D
			for (int i = 0; i < numverts; ++i)
				poly[i] = coordconv3_2 (poly3d[i]);

			if (numverts > 0)
			{
				// Draw the polygon-to-be
				paint.setBrush (polybrush);
				paint.drawPolygon (poly, numverts);

				// Draw vertex blips
				for (int i = 0; i < numverts; ++i)
				{
					QPoint& blip = poly[i];
					paint.setPen (linepen);
					drawBlip (paint, blip);

					// Draw their coordinates
					paint.setPen (textpen);
					paint.drawText (blip.x(), blip.y() - 8, poly3d[i].toString (true));
				}

				// Draw line lenghts and angle info if appropriate
				if (numverts >= 2)
				{
					int numlines = (m_drawedVerts.size() == 1) ? 1 : m_drawedVerts.size() + 1;
					paint.setPen (textpen);

					for (int i = 0; i < numlines; ++i)
					{
						const int j = (i + 1 < numverts) ? i + 1 : 0;
						const int h = (i - 1 >= 0) ? i - 1 : numverts - 1;

						if (cfg::drawLineLengths)
						{
							const String label = String::number ((poly3d[j] - poly3d[i]).length());
							QPoint origin = QLineF (poly[i], poly[j]).pointAt (0.5).toPoint();
							paint.drawText (origin, label);
						}

						if (cfg::drawAngles)
						{
							QLineF l0 (poly[h], poly[i]),
								l1 (poly[i], poly[j]);

							double angle = 180 - l0.angleTo (l1);

							if (angle < 0)
								angle = 180 - l1.angleTo (l0);

							String label = String::number (angle) + String::fromUtf8 (QByteArray ("\302\260"));
							QPoint pos = poly[i];
							pos.setY (pos.y() + metrics.height());

							paint.drawText (pos, label);
						}
					}
				}
			}
		}
		elif (editMode() == ECircleMode)
		{
			// If we have not specified the center point of the circle yet, preview it on the screen.
			if (m_drawedVerts.isEmpty())
				drawBlip (paint, coordconv3_2 (m_hoverpos));
			else
			{
				QVector<Vertex> verts, verts2;
				const double dist0 = getCircleDrawDist (0),
					dist1 = (m_drawedVerts.size() >= 2) ? getCircleDrawDist (1) : -1;
				const int segs = g_lores;
				const double angleUnit = (2 * pi) / segs;
				Axis relX, relY;
				QVector<QPoint> ringpoints, circlepoints, circle2points;

				getRelativeAxes (relX, relY);

				// Calculate the preview positions of vertices
				for (int i = 0; i < segs; ++i)
				{
					Vertex v = g_origin;
					v.setCoordinate (relX, m_drawedVerts[0][relX] + (cos (i * angleUnit) * dist0));
					v.setCoordinate (relY, m_drawedVerts[0][relY] + (sin (i * angleUnit) * dist0));
					verts << v;

					if (dist1 != -1)
					{
						v.setCoordinate (relX, m_drawedVerts[0][relX] + (cos (i * angleUnit) * dist1));
						v.setCoordinate (relY, m_drawedVerts[0][relY] + (sin (i * angleUnit) * dist1));
						verts2 << v;
					}
				}

				int i = 0;
				for (const Vertex& v : verts + verts2)
				{
					// Calculate the 2D point of the vertex
					QPoint point = coordconv3_2 (v);

					// Draw a green blip at where it is
					drawBlip (paint, point);

					// Add it to the list of points for the green ring fill.
					ringpoints << point;

					// Also add the circle points to separate lists
					if (i < verts.size())
						circlepoints << point;
					else
						circle2points << point;

					++i;
				}

				// Insert the first point as the seventeenth one so that
				// the ring polygon is closed properly.
				if (ringpoints.size() >= 16)
					ringpoints.insert (16, ringpoints[0]);

				// Same for the outer ring. Note that the indices are offset by 1
				// because of the insertion done above bumps the values.
				if (ringpoints.size() >= 33)
					ringpoints.insert (33, ringpoints[17]);

				// Draw the ring
				paint.setBrush ((m_drawedVerts.size() >= 2) ? polybrush : Qt::NoBrush);
				paint.setPen (Qt::NoPen);
				paint.drawPolygon (QPolygon (ringpoints));

				// Draw the circles
				paint.setBrush (Qt::NoBrush);
				paint.setPen (linepen);
				paint.drawPolygon (QPolygon (circlepoints));
				paint.drawPolygon (QPolygon (circle2points));

				{ // Draw the current radius in the middle of the circle.
					QPoint origin = coordconv3_2 (m_drawedVerts[0]);
					String label = String::number (dist0);
					paint.setPen (textpen);
					paint.drawText (origin.x() - (metrics.width (label) / 2), origin.y(), label);

					if (m_drawedVerts.size() >= 2)
					{
						label = String::number (dist1);
						paint.drawText (origin.x() - (metrics.width (label) / 2), origin.y() + metrics.height(), label);
					}
				}
			}
		}
	}

	// Camera icons
	if (not isPicking())
	{
		// Draw a background for the selected camera
		paint.setPen (m_thinBorderPen);
		paint.setBrush (QBrush (QColor (0, 128, 160, 128)));
		paint.drawRect (m_cameraIcons[camera()].selRect);

		// Draw the actual icons
		for (CameraIcon& info : m_cameraIcons)
		{
			// Don't draw the free camera icon when in draw mode
			if (&info == &m_cameraIcons[EFreeCamera] && editMode() != ESelectMode)
				continue;

			paint.drawPixmap (info.destRect, *info.img, info.srcRect);
		}

		String formatstr = tr ("%1 Camera");

		// Draw a label for the current camera in the bottom left corner
		{
			const int margin = 4;

			String label;
			label = format (formatstr, tr (g_CameraNames[camera()]));
			paint.setPen (textpen);
			paint.drawText (QPoint (margin, height() - (margin + metrics.descent())), label);
		}

		// Tool tips
		if (m_drawToolTip)
		{
			if (not m_cameraIcons[m_toolTipCamera].destRect.contains (m_pos))
				m_drawToolTip = false;
			else
			{
				String label = format (formatstr, tr (g_CameraNames[m_toolTipCamera]));
				QToolTip::showText (m_globalpos, label);
			}
		}
	}

	// Message log
	if (messageLog())
	{
		int y = 0;
		const int margin = 2;
		QColor penColor = textpen.color();

		for (const MessageManager::Line& line : messageLog()->getLines())
		{
			penColor.setAlphaF (line.alpha);
			paint.setPen (penColor);
			paint.drawText (QPoint (margin, y + margin + metrics.ascent()), line.text);
			y += metrics.height();
		}
	}

	// If we're range-picking, draw a rectangle encompassing the selection area.
	if (m_rangepick && not isPicking() && m_totalmove >= 10)
	{
		int x0 = m_rangeStart.x(),
			y0 = m_rangeStart.y(),
			x1 = m_pos.x(),
			y1 = m_pos.y();

		QRect rect (x0, y0, x1 - x0, y1 - y0);
		QColor fillColor = (m_addpick ? "#40FF00" : "#00CCFF");
		fillColor.setAlphaF (0.2f);

		paint.setPen (m_thickBorderPen);
		paint.setBrush (QBrush (fillColor));
		paint.drawRect (rect);
	}
}

// =============================================================================
//
void GLRenderer::drawBlip (QPainter& paint, QPoint pos) const
{
	QPen pen = m_thinBorderPen;
	const int blipsize = 8;
	pen.setWidth (1);
	paint.setPen (pen);
	paint.setBrush (QColor (64, 192, 0));
	paint.drawEllipse (pos.x() - blipsize / 2, pos.y() - blipsize / 2, blipsize, blipsize);
}

// =============================================================================
//
void GLRenderer::clampAngle (double& angle) const
{
	while (angle < 0)
		angle += 360.0;

	while (angle > 360.0)
		angle -= 360.0;
}

// =============================================================================
//
void GLRenderer::addDrawnVertex (Vertex pos)
{
	// If we picked an already-existing vertex, stop drawing
	if (editMode() == EDrawMode)
	{
		for (Vertex& vert : m_drawedVerts)
		{
			if (vert == pos)
			{
				endDraw (true);
				return;
			}
		}
	}

	m_drawedVerts << pos;
}

// =============================================================================
//
void GLRenderer::mouseReleaseEvent (QMouseEvent* ev)
{
	const bool wasLeft = (m_lastButtons & Qt::LeftButton) && not (ev->buttons() & Qt::LeftButton),
	   wasRight = (m_lastButtons & Qt::RightButton) && not (ev->buttons() & Qt::RightButton),
	   wasMid = (m_lastButtons & Qt::MidButton) && not (ev->buttons() & Qt::MidButton);

	if (m_panning)
		m_panning = false;

	if (wasLeft)
	{
		// Check if we selected a camera icon
		if (not m_rangepick)
		{
			for (CameraIcon & info : m_cameraIcons)
			{
				if (info.destRect.contains (ev->pos()))
				{
					setCamera (info.cam);
					goto end;
				}
			}
		}

		switch (editMode())
		{
			case EDrawMode:
			{
				if (m_rectdraw)
				{
					if (m_drawedVerts.size() == 2)
					{
						endDraw (true);
						return;
					}
				}
				else
				{
					// If we have 4 verts, stop drawing.
					if (m_drawedVerts.size() >= 4)
					{
						endDraw (true);
						return;
					}

					if (m_drawedVerts.isEmpty() && ev->modifiers() & Qt::ShiftModifier)
					{
						m_rectdraw = true;
						updateRectVerts();
					}
				}

				addDrawnVertex (m_hoverpos);
				break;
			}

			case ECircleMode:
			{
				if (m_drawedVerts.size() == 3)
				{
					endDraw (true);
					return;
				}

				addDrawnVertex (m_hoverpos);
				break;
			}

			case ESelectMode:
			{
				if (not isDrawOnly())
				{
					if (m_totalmove < 10)
						m_rangepick = false;

					if (not m_rangepick)
						m_addpick = (m_keymods & Qt::ControlModifier);

					if (m_totalmove < 10 || m_rangepick)
						pick (ev->x(), ev->y());
				}
				break;
			}
		}

		m_rangepick = false;
	}

	if (wasMid && editMode() != ESelectMode && m_drawedVerts.size() < 4 && m_totalmove < 10)
	{
		// Find the closest vertex to our cursor
		double			minimumDistance = 1024.0;
		const Vertex*	closest = null;
		Vertex			cursorPosition = coordconv2_3 (m_pos, false);
		QPoint			cursorPosition2D (m_pos);
		const Axis		relZ = getRelativeZ();
		QList<Vertex>	vertices;

		for (auto it = document()->vertices().begin(); it != document()->vertices().end(); ++it)
			vertices << it.key();

		// Sort the vertices in order of distance to camera
		std::sort (vertices.begin(), vertices.end(), [&](const Vertex& a, const Vertex& b) -> bool
		{
			if (g_FixedCameras[camera()].negatedDepth)
				return a[relZ] > b[relZ];

			return a[relZ] < b[relZ];
		});

		for (const Vertex& vrt : vertices)
		{
			// If the vertex in 2d space is very close to the cursor then we use
			// it regardless of depth.
			QPoint vect2d = coordconv3_2 (vrt) - cursorPosition2D;
			const double distance2DSquared = std::pow (vect2d.x(), 2) + std::pow (vect2d.y(), 2);
			if (distance2DSquared < 16.0 * 16.0)
			{
				closest = &vrt;
				break;
			}

			// Check if too far away from the cursor.
			if (distance2DSquared > 64.0 * 64.0)
				continue;

			// Not very close to the cursor. Compare using true distance,
			// including depth.
			const double distanceSquared = (vrt - cursorPosition).lengthSquared();

			if (distanceSquared < minimumDistance)
			{
				minimumDistance = distanceSquared;
				closest = &vrt;
			}
		}

		if (closest != null)
			addDrawnVertex (*closest);
	}

	if (wasRight && not m_drawedVerts.isEmpty())
	{
		// Remove the last vertex
		m_drawedVerts.removeLast();

		if (m_drawedVerts.isEmpty())
			m_rectdraw = false;
	}

end:
	update();
	m_totalmove = 0;
}

// =============================================================================
//
void GLRenderer::mousePressEvent (QMouseEvent* ev)
{
	m_totalmove = 0;

	if (ev->modifiers() & Qt::ControlModifier)
	{
		m_rangepick = true;
		m_rangeStart.setX (ev->x());
		m_rangeStart.setY (ev->y());
		m_addpick = (m_keymods & Qt::AltModifier);
		ev->accept();
	}

	m_lastButtons = ev->buttons();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::mouseMoveEvent (QMouseEvent* ev)
{
	int dx = ev->x() - m_pos.x();
	int dy = ev->y() - m_pos.y();
	m_totalmove += abs (dx) + abs (dy);
	setCameraMoving (false);

	const bool left = ev->buttons() & Qt::LeftButton,
			   mid = ev->buttons() & Qt::MidButton,
			   shift = ev->modifiers() & Qt::ShiftModifier;

	if (mid || (left && shift))
	{
		pan (X) += 0.03f * dx * (zoom() / 7.5f);
		pan (Y) -= 0.03f * dy * (zoom() / 7.5f);
		m_panning = true;
		setCameraMoving (true);
	}
	elif (left && not m_rangepick && camera() == EFreeCamera)
	{
		rot (X) = rot (X) + dy;
		rot (Y) = rot (Y) + dx;

		clampAngle (rot (X));
		clampAngle (rot (Y));
		setCameraMoving (true);
	}

	// Start the tool tip timer
	if (not m_drawToolTip)
		m_toolTipTimer->start (500);

	// Update 2d position
	m_pos = ev->pos();
	m_globalpos = ev->globalPos();

	// Calculate 3d position of the cursor
	m_hoverpos = (camera() != EFreeCamera) ? coordconv2_3 (m_pos, true) : g_origin;

	// Update rect vertices since m_hoverpos may have changed
	updateRectVerts();
	highlightCursorObject();
	update();
}

// =============================================================================
//
void GLRenderer::keyPressEvent (QKeyEvent* ev)
{
	m_keymods = ev->modifiers();
}

// =============================================================================
//
void GLRenderer::keyReleaseEvent (QKeyEvent* ev)
{
	m_keymods = ev->modifiers();
}

// =============================================================================
//
void GLRenderer::wheelEvent (QWheelEvent* ev)
{
	makeCurrent();

	zoomNotch (ev->delta() > 0);
	zoom() = clamp (zoom(), 0.01, 10000.0);
	setCameraMoving (true);
	update();
	ev->accept();
}

// =============================================================================
//
void GLRenderer::leaveEvent (QEvent* ev)
{
	(void) ev;
	m_drawToolTip = false;
	m_toolTipTimer->stop();
	update();
}

// =============================================================================
//
void GLRenderer::contextMenuEvent (QContextMenuEvent* ev)
{
	g_win->spawnContextMenu (ev->globalPos());
}

// =============================================================================
//
void GLRenderer::setCamera (const ECamera cam)
{
	m_camera = cam;
	cfg::camera = (int) cam;
	g_win->updateEditModeActions();
}

// =============================================================================
//
void GLRenderer::pick (int mouseX, int mouseY)
{
	makeCurrent();

	// Use particularly thick lines while picking ease up selecting lines.
	glLineWidth (max<double> (cfg::lineThickness, 6.5f));

	// Clear the selection if we do not wish to add to it.
	if (not m_addpick)
	{
		LDObjectList oldsel = selection();
		getCurrentDocument()->clearSelection();

		for (LDObject* obj : oldsel)
			compileObject (obj);
	}

	// Paint the picking scene
	setPicking (true);
	drawGLScene();

	int x0 = mouseX,
		  y0 = mouseY;
	int x1, y1;

	// Determine how big an area to read - with range picking, we pick by
	// the area given, with single pixel picking, we use an 1 x 1 area.
	if (m_rangepick)
	{
		x1 = m_rangeStart.x();
		y1 = m_rangeStart.y();
	}
	else
	{
		x1 = x0 + 1;
		y1 = y0 + 1;
	}

	// x0 and y0 must be less than x1 and y1, respectively.
	if (x0 > x1)
		qSwap (x0, x1);

	if (y0 > y1)
		qSwap (y0, y1);

	// Clamp the values to ensure they're within bounds
	x0 = max (0, x0);
	y0 = max (0, y0);
	x1 = min (x1, m_width);
	y1 = min (y1, m_height);
	const int areawidth = (x1 - x0);
	const int areaheight = (y1 - y0);
	const qint32 numpixels = areawidth * areaheight;

	// Allocate space for the pixel data.
	uchar* const pixeldata = new uchar[4 * numpixels];
	uchar* pixelptr = &pixeldata[0];

	// Read pixels from the color buffer.
	glReadPixels (x0, m_height - y1, areawidth, areaheight, GL_RGBA, GL_UNSIGNED_BYTE, pixeldata);

	LDObject* removedObj = null;
	QList<qint32> indices;

	// Go through each pixel read and add them to the selection.
	// Note: black is background, those indices are skipped.
	for (qint32 i = 0; i < numpixels; ++i)
	{
		qint32 idx =
			(*(pixelptr + 0) * 0x10000) +
			(*(pixelptr + 1) * 0x100) +
			*(pixelptr + 2);
		pixelptr += 4;

		if (idx != 0)
			indices << idx;
	}

	removeDuplicates (indices);

	for (qint32 idx : indices)
	{
		LDObject* obj = LDObject::fromID (idx);
		assert (obj != null);

		// If this is an additive single pick and the object is currently selected,
		// we remove it from selection instead.
		if (not m_rangepick && m_addpick)
		{
			if (obj->isSelected())
			{
				obj->unselect();
				removedObj = obj;
				break;
			}
		}

		obj->select();
	}

	delete[] pixeldata;

	// Update everything now.
	g_win->updateSelection();

	// Recompile the objects now to update their color
	for (LDObject* obj : selection())
		compileObject (obj);

	if (removedObj)
		compileObject (removedObj);

	// Restore line thickness
	glLineWidth (cfg::lineThickness);

	setPicking (false);
	m_rangepick = false;
	repaint();
}

// =============================================================================
//
void GLRenderer::setEditMode (EditMode const& a)
{
	m_editMode = a;

	switch (a)
	{
		case ESelectMode:
		{
			unsetCursor();
			setContextMenuPolicy (Qt::DefaultContextMenu);
		} break;

		case EDrawMode:
		case ECircleMode:
		{
			// Cannot draw into the free camera - use top instead.
			if (camera() == EFreeCamera)
				setCamera (ETopCamera);

			// Disable the context menu - we need the right mouse button
			// for removing vertices.
			setContextMenuPolicy (Qt::NoContextMenu);

			// Use the crosshair cursor when drawing.
			setCursor (Qt::CrossCursor);

			// Clear the selection when beginning to draw.
			LDObjectList priorsel = selection();
			getCurrentDocument()->clearSelection();

			for (LDObject* obj : priorsel)
				compileObject (obj);

			g_win->updateSelection();
			m_drawedVerts.clear();
		} break;
	}

	g_win->updateEditModeActions();
	update();
}

// =============================================================================
//
void GLRenderer::setDocument (LDDocument* const& a)
{
	m_document = a;

	if (a != null)
	{
		initOverlaysFromObjects();

		if (not currentDocumentData().init)
		{
			resetAllAngles();
			currentDocumentData().init = true;
		}

		currentDocumentData().needZoomToFit = true;
	}
}

// =============================================================================
//
void GLRenderer::setPicking (const bool& a)
{
	m_isPicking = a;
	setBackground();

	if (a)
		glDisable (GL_DITHER);
	else
		glEnable (GL_DITHER);
}

// =============================================================================
//
Matrix GLRenderer::getCircleDrawMatrix (double scale)
{
	Matrix transform = g_circleDrawMatrixTemplates[camera() % 3];

	for (int i = 0; i < 9; ++i)
	{
		if (transform[i] == 2)
			transform[i] = scale;
		elif (transform[i] == 1 && camera() >= 3)
			transform[i] = -1;
	}

	return transform;
}

// =============================================================================
//
void GLRenderer::endDraw (bool accept)
{
	(void) accept;

	// Clean the selection and create the object
	QList<Vertex>& verts = m_drawedVerts;
	LDObjectList objs;

	switch (editMode())
	{
		case EDrawMode:
		{
			if (m_rectdraw)
			{
				LDQuad* quad = new LDQuad;

				// Copy the vertices from m_rectverts
				updateRectVerts();

				for (int i = 0; i < quad->vertices(); ++i)
					quad->setVertex (i, m_rectverts[i]);

				quad->setColor (maincolor);
				objs << quad;
			}
			else
			{
				switch (verts.size())
				{
					case 1:
					{
						// 1 vertex - add a vertex object
						LDVertex* obj = new LDVertex;
						obj->pos = verts[0];
						obj->setColor (maincolor);
						objs << obj;
					} break;

					case 2:
					{
						// 2 verts - make a line
						LDLine* obj = new LDLine (verts[0], verts[1]);
						obj->setColor (edgecolor);
						objs << obj;
					} break;

					case 3:
					case 4:
					{
						LDObject* obj = (verts.size() == 3) ?
							  static_cast<LDObject*> (new LDTriangle) :
							  static_cast<LDObject*> (new LDQuad);

						obj->setColor (maincolor);

						for (int i = 0; i < obj->vertices(); ++i)
							obj->setVertex (i, verts[i]);

						objs << obj;
					} break;
				}
			}
		} break;

		case ECircleMode:
		{
			const int segs = g_lores, divs = g_lores; // TODO: make customizable
			double dist0 = getCircleDrawDist (0),
				dist1 = getCircleDrawDist (1);
			LDDocument* refFile = null;
			Matrix transform;
			bool circleOrDisc = false;

			if (dist1 < dist0)
				std::swap<double> (dist0, dist1);

			if (dist0 == dist1)
			{
				// If the radii are the same, there's no ring space to fill. Use a circle.
				refFile = ::getDocument ("4-4edge.dat");
				transform = getCircleDrawMatrix (dist0);
				circleOrDisc = true;
			}
			elif (dist0 == 0 || dist1 == 0)
			{
				// If either radii is 0, use a disc.
				refFile = ::getDocument ("4-4disc.dat");
				transform = getCircleDrawMatrix ((dist0 != 0) ? dist0 : dist1);
				circleOrDisc = true;
			}
			elif (g_RingFinder.findRings (dist0, dist1))
			{
				// The ring finder found a solution, use that. Add the component rings to the file.
				for (const RingFinder::Component& cmp : g_RingFinder.bestSolution()->getComponents())
				{
					// Get a ref file for this primitive. If we cannot find it in the
					// LDraw library, generate it.
					if ((refFile = ::getDocument (radialFileName (::Ring, g_lores, g_lores, cmp.num))) == null)
					{
						refFile = generatePrimitive (::Ring, g_lores, g_lores, cmp.num);
						refFile->setImplicit (false);
					}

					LDSubfile* ref = new LDSubfile;
					ref->setFileInfo (refFile);
					ref->setTransform (getCircleDrawMatrix (cmp.scale));
					ref->setPosition (m_drawedVerts[0]);
					ref->setColor (maincolor);
					objs << ref;
				}
			}
			else
			{
				// Ring finder failed, last resort: draw the ring with quads
				QList<QLineF> c0, c1;
				Axis relX, relY, relZ;
				getRelativeAxes (relX, relY);
				relZ = (Axis) (3 - relX - relY);
				double x0 = m_drawedVerts[0][relX],
					y0 = m_drawedVerts[0][relY];

				Vertex templ;
				templ.setCoordinate (relX, x0);
				templ.setCoordinate (relY, y0);
				templ.setCoordinate (relZ, getDepthValue());

				// Calculate circle coords
				makeCircle (segs, divs, dist0, c0);
				makeCircle (segs, divs, dist1, c1);

				for (int i = 0; i < segs; ++i)
				{
					Vertex v0, v1, v2, v3;
					v0 = v1 = v2 = v3 = templ;
					v0.setCoordinate (relX, v0[relX] + c0[i].x1());
					v0.setCoordinate (relY, v0[relY] + c0[i].y1());
					v1.setCoordinate (relX, v1[relX] + c0[i].x2());
					v1.setCoordinate (relY, v1[relY] + c0[i].y2());
					v2.setCoordinate (relX, v2[relX] + c1[i].x2());
					v2.setCoordinate (relY, v2[relY] + c1[i].y2());
					v3.setCoordinate (relX, v3[relX] + c1[i].x1());
					v3.setCoordinate (relY, v3[relY] + c1[i].y1());

					LDQuad* q = new LDQuad (v0, v1, v2, v3);
					q->setColor (maincolor);

					// Ensure the quads always are BFC-front towards the camera
					if (camera() % 3 <= 0)
						q->invert();

					objs << q;
				}
			}

			if (circleOrDisc)
			{
				LDSubfile* ref = new LDSubfile;
				ref->setFileInfo (refFile);
				ref->setTransform (transform);
				ref->setPosition (m_drawedVerts[0]);
				ref->setColor (maincolor);
				objs << ref;
			}
		} break;

		case ESelectMode:
		{
			// this shouldn't happen
			assert (false);
			return;
		} break;
	}

	if (objs.size() > 0)
	{
		for (LDObject* obj : objs)
		{
			document()->addObject (obj);
			compileObject (obj);
		}

		g_win->refresh();
		g_win->endAction();
	}

	m_drawedVerts.clear();
	m_rectdraw = false;
}

// =============================================================================
//
double GLRenderer::getCircleDrawDist (int pos) const
{
	assert (m_drawedVerts.size() >= pos + 1);
	const Vertex& v1 = (m_drawedVerts.size() >= pos + 2) ? m_drawedVerts[pos + 1] : m_hoverpos;
	Axis relX, relY;
	getRelativeAxes (relX, relY);

	const double dx = m_drawedVerts[0][relX] - v1[relX];
	const double dy = m_drawedVerts[0][relY] - v1[relY];
	return sqrt ((dx * dx) + (dy * dy));
}

// =============================================================================
//
void GLRenderer::getRelativeAxes (Axis& relX, Axis& relY) const
{
	const LDFixedCameraInfo* cam = &g_FixedCameras[camera()];
	relX = cam->axisX;
	relY = cam->axisY;
}

// =============================================================================
//
Axis GLRenderer::getRelativeZ() const
{
	const LDFixedCameraInfo* cam = &g_FixedCameras[camera()];
	return (Axis) (3 - cam->axisX - cam->axisY);
}

// =============================================================================
//
static QList<Vertex> getVertices (LDObject* obj)
{
	QList<Vertex> verts;

	if (obj->vertices() >= 2)
	{
		for (int i = 0; i < obj->vertices(); ++i)
			verts << obj->vertex (i);
	}
	elif (obj->type() == LDObject::ESubfile)
	{
		LDSubfile* ref = static_cast<LDSubfile*> (obj);
		LDObjectList objs = ref->inlineContents (true, false);

		for (LDObject* obj : objs)
		{
			verts << getVertices (obj);
			obj->destroy();
		}
	}

	return verts;
}

// =============================================================================
//
void GLRenderer::compileObject (LDObject* obj)
{
	compiler()->stageForCompilation (obj);
}

// =============================================================================
//
void GLRenderer::forgetObject (LDObject* obj)
{
	compiler()->dropObject (obj);
}

// =============================================================================
//
uchar* GLRenderer::getScreencap (int& w, int& h)
{
	w = m_width;
	h = m_height;
	uchar* cap = new uchar[4 * w * h];

	m_screencap = true;
	update();
	m_screencap = false;

	// Capture the pixels
	glReadPixels (0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, cap);

	return cap;
}

// =============================================================================
//
void GLRenderer::slot_toolTipTimer()
{
	// We come here if the cursor has stayed in one place for longer than a
	// a second. Check if we're holding it over a camera icon - if so, draw
	// a tooltip.
	for (CameraIcon & icon : m_cameraIcons)
	{
		if (icon.destRect.contains (m_pos))
		{
			m_toolTipCamera = icon.cam;
			m_drawToolTip = true;
			update();
			break;
		}
	}
}

// =============================================================================
//
Axis GLRenderer::getCameraAxis (bool y, ECamera camid)
{
	if (camid == (ECamera) -1)
		camid = camera();

	const LDFixedCameraInfo* cam = &g_FixedCameras[camid];
	return (y) ? cam->axisY : cam->axisX;
}

// =============================================================================
//
bool GLRenderer::setupOverlay (ECamera cam, String file, int x, int y, int w, int h)
{
	QImage* img = new QImage (QImage (file).convertToFormat (QImage::Format_ARGB32));
	LDGLOverlay& info = getOverlay (cam);

	if (img->isNull())
	{
		critical (tr ("Failed to load overlay image!"));
		currentDocumentData().overlays[cam].invalid = true;
		delete img;
		return false;
	}

	delete info.img; // delete the old image

	info.fname = file;
	info.lw = w;
	info.lh = h;
	info.ox = x;
	info.oy = y;
	info.img = img;
	info.invalid = false;

	if (info.lw == 0)
		info.lw = (info.lh * img->width()) / img->height();
	elif (info.lh == 0)
		info.lh = (info.lw * img->height()) / img->width();

	const Axis x2d = getCameraAxis (false, cam),
		y2d = getCameraAxis (true, cam);
	const double negXFac = g_FixedCameras[cam].negX ? -1 : 1,
		negYFac = g_FixedCameras[cam].negY ? -1 : 1;

	info.v0 = info.v1 = g_origin;
	info.v0.setCoordinate (x2d, -(info.ox * info.lw * negXFac) / img->width());
	info.v0.setCoordinate (y2d, (info.oy * info.lh * negYFac) / img->height());
	info.v1.setCoordinate (x2d, info.v0[x2d] + info.lw);
	info.v1.setCoordinate (y2d, info.v0[y2d] + info.lh);

	// Set alpha of all pixels to 0.5
	for (long i = 0; i < img->width(); ++i)
	for (long j = 0; j < img->height(); ++j)
	{
		uint32 pixel = img->pixel (i, j);
		img->setPixel (i, j, 0x80000000 | (pixel & 0x00FFFFFF));
	}

	updateOverlayObjects();
	return true;
}

// =============================================================================
//
void GLRenderer::clearOverlay()
{
	if (camera() == EFreeCamera)
		return;

	LDGLOverlay& info = currentDocumentData().overlays[camera()];
	delete info.img;
	info.img = null;

	updateOverlayObjects();
}

// =============================================================================
//
void GLRenderer::setDepthValue (double depth)
{
	assert (camera() < EFreeCamera);
	currentDocumentData().depthValues[camera()] = depth;
}

// =============================================================================
//
double GLRenderer::getDepthValue() const
{
	assert (camera() < EFreeCamera);
	return currentDocumentData().depthValues[camera()];
}

// =============================================================================
//
const char* GLRenderer::getCameraName() const
{
	return g_CameraNames[camera()];
}

// =============================================================================
//
LDGLOverlay& GLRenderer::getOverlay (int newcam)
{
	return currentDocumentData().overlays[newcam];
}

// =============================================================================
//
void GLRenderer::zoomNotch (bool inward)
{
	if (zoom() > 15)
		zoom() *= inward ? 0.833f : 1.2f;
	else
		zoom() += inward ? -1.2f : 1.2f;
}

// =============================================================================
//
void GLRenderer::zoomToFit()
{
	zoom() = 30.0f;

	if (document() == null || m_width == -1 || m_height == -1)
		return;

	bool lastfilled = false;
	bool firstrun = true;
	const uint32 white = 0xFFFFFFFF;
	bool inward = true;
	const int w = m_width, h = m_height;
	int runaway = 50;

	glClearColor (1.0, 1.0, 1.0, 1.0);
	glDisable (GL_DITHER);

	// Use the pick list while drawing the scene, this way we can tell whether borders
	// are background or not.
	setPicking (true);

	while (--runaway)
	{
		if (zoom() > 10000.0 || zoom() < 0.0)
		{
			// Obviously, there's nothing to draw if we get here.
			// Default to 30.0f and break out.
			zoom() = 30.0;
			break;
		}

		zoomNotch (inward);

		uchar* cap = new uchar[4 * w * h];
		drawGLScene();
		glReadPixels (0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, cap);
		uint32* imgdata = reinterpret_cast<uint32*> (cap);
		bool filled = false;

		// Check the top and bottom rows
		for (int i = 0; i < w; ++i)
		{
			if (imgdata[i] != white || imgdata[((h - 1) * w) + i] != white)
			{
				filled = true;
				break;
			}
		}

		// Left and right edges
		if (filled == false)
		{
			for (int i = 0; i < h; ++i)
			{
				if (imgdata[i * w] != white || imgdata[(i * w) + w - 1] != white)
				{
					filled = true;
					break;
				}
			}
		}

		delete[] cap;

		if (firstrun)
		{
			// If this is the first run, we don't know enough to determine
			// whether the zoom was to fit, so we mark in our knowledge so
			// far and start over.
			inward = not filled;
			firstrun = false;
		}
		else
		{
			// If this run filled the screen and the last one did not, the
			// last run had ideal zoom - zoom a bit back and we should reach it.
			if (filled && not lastfilled)
			{
				zoomNotch (false);
				break;
			}

			// If this run did not fill the screen and the last one did, we've
			// now reached ideal zoom so we're done here.
			if (not filled && lastfilled)
				break;

			inward = not filled;
		}

		lastfilled = filled;
	}

	setBackground();
	setPicking (false);
}

// =============================================================================
//
void GLRenderer::zoomAllToFit()
{
	ECamera oldcam = camera();

	for (ECamera cam = EFirstCamera; cam < ENumCameras; ++cam)
	{
		setCamera (cam);
		zoomToFit();
	}

	setCamera (oldcam);
}

// =============================================================================
//
void GLRenderer::updateRectVerts()
{
	if (not m_rectdraw)
		return;

	if (m_drawedVerts.isEmpty())
	{
		for (int i = 0; i < 4; ++i)
			m_rectverts[i] = m_hoverpos;

		return;
	}

	Vertex v0 = m_drawedVerts[0],
		   v1 = (m_drawedVerts.size() >= 2) ? m_drawedVerts[1] : m_hoverpos;

	const Axis ax = getCameraAxis (false),
			   ay = getCameraAxis (true),
			   az = (Axis) (3 - ax - ay);

	for (int i = 0; i < 4; ++i)
		m_rectverts[i].setCoordinate (az, getDepthValue());

	m_rectverts[0].setCoordinate (ax, v0[ax]);
	m_rectverts[0].setCoordinate (ay, v0[ay]);
	m_rectverts[1].setCoordinate (ax, v1[ax]);
	m_rectverts[1].setCoordinate (ay, v0[ay]);
	m_rectverts[2].setCoordinate (ax, v1[ax]);
	m_rectverts[2].setCoordinate (ay, v1[ay]);
	m_rectverts[3].setCoordinate (ax, v0[ax]);
	m_rectverts[3].setCoordinate (ay, v1[ay]);
}

// =============================================================================
//
void GLRenderer::mouseDoubleClickEvent (QMouseEvent* ev)
{
	if (not (ev->buttons() & Qt::LeftButton) || editMode() != ESelectMode)
		return;

	pick (ev->x(), ev->y());

	if (selection().isEmpty())
		return;

	LDObject* obj = selection().first();
	AddObjectDialog::staticDialog (obj->type(), obj);
	g_win->endAction();
	ev->accept();
}

// =============================================================================
//
LDOverlay* GLRenderer::findOverlayObject (ECamera cam)
{
	LDOverlay* ovlobj = null;

	for (LDObject* obj : document()->objects())
	{
		if (obj->type() == LDObject::EOverlay && static_cast<LDOverlay*> (obj)->camera() == cam)
		{
			ovlobj = static_cast<LDOverlay*> (obj);
			break;
		}
	}

	return ovlobj;
}

// =============================================================================
//
// Read in overlays from the current file and update overlay info accordingly.
//
void GLRenderer::initOverlaysFromObjects()
{
	for (ECamera cam = EFirstCamera; cam < ENumCameras; ++cam)
	{
		if (cam == EFreeCamera)
			continue;

		LDGLOverlay& meta = currentDocumentData().overlays[cam];
		LDOverlay* ovlobj = findOverlayObject (cam);

		if (not ovlobj && meta.img)
		{
			delete meta.img;
			meta.img = null;
		}
		elif (ovlobj && (meta.img == null || meta.fname != ovlobj->fileName()) && meta.invalid == false)
			setupOverlay (cam, ovlobj->fileName(), ovlobj->x(),
				ovlobj->y(), ovlobj->width(), ovlobj->height());
	}
}

// =============================================================================
//
void GLRenderer::updateOverlayObjects()
{
	for (ECamera cam = EFirstCamera; cam < ENumCameras; ++cam)
	{
		if (cam == EFreeCamera)
			continue;

		LDGLOverlay& meta = currentDocumentData().overlays[cam];
		LDOverlay* ovlobj = findOverlayObject (cam);

		if (meta.img == null && ovlobj != null)
		{
			// If this is the last overlay image, we need to remove the empty space after it as well.
			LDObject* nextobj = ovlobj->next();

			if (nextobj && nextobj->type() == LDObject::EEmpty)
				nextobj->destroy();

			// If the overlay object was there and the overlay itself is
			// not, remove the object.
			ovlobj->destroy();
		}
		elif (meta.img != null && ovlobj == null)
		{
			// Inverse case: image is there but the overlay object is
			// not, thus create the object.
			ovlobj = new LDOverlay;

			// Find a suitable position to place this object. We want to place
			// this into the header, which is everything up to the first scemantic
			// object. If we find another overlay object, place this object after
			// the last one found. Otherwise, place it before the first schemantic
			// object and put an empty object after it (though don't do this if
			// there was no schemantic elements at all)
			int i, lastOverlay = -1;
			bool found = false;

			for (i = 0; i < document()->getObjectCount(); ++i)
			{
				LDObject* obj = document()->getObject (i);

				if (obj->isScemantic())
				{
					found = true;
					break;
				}

				if (obj->type() == LDObject::EOverlay)
					lastOverlay = i;
			}

			if (lastOverlay != -1)
				document()->insertObj (lastOverlay + 1, ovlobj);
			else
			{
				document()->insertObj (i, ovlobj);

				if (found)
					document()->insertObj (i + 1, new LDEmpty);
			}
		}

		if (meta.img != null && ovlobj != null)
		{
			ovlobj->setCamera (cam);
			ovlobj->setFileName (meta.fname);
			ovlobj->setX (meta.ox);
			ovlobj->setY (meta.oy);
			ovlobj->setWidth (meta.lw);
			ovlobj->setHeight (meta.lh);
		}
	}

	if (g_win->R() == this)
		g_win->refresh();
}

// =============================================================================
//
void GLRenderer::highlightCursorObject()
{
	if (not cfg::highlightObjectBelowCursor && objectAtCursor() == null)
		return;

	LDObject* newObject = null;
	LDObject* oldObject = objectAtCursor();
	qint32 newIndex;

	if (isCameraMoving() || not cfg::highlightObjectBelowCursor)
	{
		newIndex = 0;
	}
	else
	{
		setPicking (true);
		drawGLScene();
		setPicking (false);

		unsigned char pixel[4];
		glReadPixels (m_pos.x(), m_height - m_pos.y(), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel[0]);
		newIndex = pixel[0] * 0x10000 | pixel[1] * 0x100 | pixel[2];
	}

	if (newIndex != (oldObject != null ? oldObject->id() : 0))
	{
		if (newIndex != 0)
			newObject = LDObject::fromID (newIndex);

		setObjectAtCursor (newObject);

		if (oldObject != null)
			compileObject (oldObject);

		if (newObject != null)
			compileObject (newObject);
	}

	update();
}

