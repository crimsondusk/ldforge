/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
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

#include <QtGui>
#include <QGLWidget>
#include <GL/glu.h>
#include "common.h"
#include "config.h"
#include "file.h"
#include "gldraw.h"
#include "bbox.h"
#include "colors.h"
#include "gui.h"
#include "misc.h"

static double g_objOffset[3];
static double g_storedBBoxSize;

static short g_pulseTick = 0;
static const short g_numPulseTicks = 8;
static const short g_pulseInterval = 65;

static const struct staticCameraMeta {
	const char glrotate[3];
	const Axis axisX, axisY;
	const bool negX, negY;
} g_staticCameras[6] = {
	{ { 1, 0, 0 }, X, Z, false, false },
	{ { 0, 0, 0 }, X, Y, false, true },
	{ { 0, 1, 0 }, Z, Y, true, true },
	{ { -1, 0, 0 }, X, Z, false, true },
	{ { 0, 0, 0 }, X, Y, true, true },
	{ { 0, -1, 0 }, Z, Y, false, true },
};

cfg (str, gl_bgcolor, "#CCCCD9");
cfg (str, gl_maincolor, "#707078");
cfg (float, gl_maincolor_alpha, 1.0);
cfg (int, gl_linethickness, 2);
cfg (bool, gl_colorbfc, true);
cfg (bool, gl_selflash, false);
cfg (int, gl_camera, GLRenderer::Free);
cfg (bool, gl_blackedges, true);
cfg (bool, gl_axes, false);

// CameraIcon::img is a heap-allocated QPixmap because otherwise it gets
// initialized before program gets to main() and constructs a QApplication
// and Qt doesn't like that.
struct CameraIcon {
	QPixmap* img;
	QRect srcRect, destRect, selRect;
	GLRenderer::Camera cam;
} g_CameraIcons[7];

const char* g_CameraNames[7] = { "Top", "Front", "Left", "Bottom", "Back", "Right", "Free" };

const GLRenderer::Camera g_Cameras[7] = {
	GLRenderer::Top,
	GLRenderer::Front,
	GLRenderer::Left,
	GLRenderer::Bottom,
	GLRenderer::Back,
	GLRenderer::Right,
	GLRenderer::Free
};
	
// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
GLRenderer::GLRenderer (QWidget* parent) : QGLWidget (parent) {
	resetAngles ();
	picking = rangepick = false;
	m_camera = (GLRenderer::Camera) gl_camera.value;
	drawToolTip = false;
	
	pulseTimer = new QTimer (this);
	connect (pulseTimer, SIGNAL (timeout ()), this, SLOT (slot_timerUpdate ()));
	
	toolTipTimer = new QTimer (this);
	toolTipTimer->setSingleShot (true);
	connect (toolTipTimer, SIGNAL (timeout ()), this, SLOT (slot_toolTipTimer ()));
	
	thickBorderPen = QPen (QColor (0, 0, 0, 208), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	thinBorderPen = thickBorderPen;
	thinBorderPen.setWidth (1);
	
	// Init camera icons
	for (const GLRenderer::Camera cam : g_Cameras) {
		str iconname;
		iconname.format ("camera-%s", str (g_CameraNames[cam]).tolower ().chars ());
		
		CameraIcon* info = &g_CameraIcons[cam];
		info->img = new QPixmap (getIcon (iconname));
		info->cam = cam;
	}
	
	calcCameraIconRects ();
}

// =============================================================================
GLRenderer::~GLRenderer() {
	for (CameraIcon& info : g_CameraIcons)
		delete info.img;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::calcCameraIconRects () {
	ushort i = 0;
	
	for (CameraIcon& info : g_CameraIcons) {
		const long x1 = (width - (info.cam != Free ? 48 : 16)) + ((i % 3) * 16) - 1,
			y1 = ((i / 3) * 16) + 1;
		
		info.srcRect = QRect (0, 0, 16, 16);
		info.destRect = QRect (x1, y1, 16, 16);
		info.selRect = QRect (info.destRect.x (), info.destRect.y (),
			info.destRect.width () + 1, info.destRect.height () + 1);
		++i;
	}
}

// =============================================================================
void GLRenderer::resetAngles () {
	rotX = 30.0f;
	rotY = 325.f;
	panX = panY = rotZ = 0.0f;
	zoom = 5.0f;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::initializeGL () {
	setBackground ();
	
	glEnable (GL_POLYGON_OFFSET_FILL);
	glPolygonOffset (1.0f, 1.0f);
	
	glEnable (GL_DEPTH_TEST);
	glShadeModel (GL_SMOOTH);
	glEnable (GL_MULTISAMPLE);
	
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glEnable (GL_LINE_SMOOTH);
	glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
	
	glLineWidth (gl_linethickness);
	
	setAutoFillBackground (false);
	setMouseTracking (true);
	setFocusPolicy (Qt::WheelFocus);
	compileObjects ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
QColor GLRenderer::getMainColor () {
	QColor col (gl_maincolor.value.chars());
	
	if (!col.isValid ())
		return QColor (0, 0, 0);
	
	col.setAlpha (gl_maincolor_alpha * 255.f);
	return col;
}

// -----------------------------------------------------------------------------
void GLRenderer::setBackground () {
	QColor col (gl_bgcolor.value.chars());
	
	if (!col.isValid ())
		return;
	
	darkbg = luma (col) < 80;
	
	col.setAlpha (255);
	qglClearColor (col);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static vector<short> g_daWarnedColors;
void GLRenderer::setObjectColor (LDObject* obj) {
	QColor qCol;
	
	if (picking) {
		// Make the color by the object's index color if we're picking, so we can
		// make the index from the color we get from the picking results.
		long i = obj->getIndex (g_CurrentFile);
		
		// If we couldn't find the index, this object must not be from this file,
		// therefore it must be an object inlined from a subfile reference or
		// decomposed from a radial. Find the top level parent object and use
		// its index.
		if (i == -1)
			i = obj->topLevelParent ()->getIndex (g_CurrentFile);
		
		// We should have the index now.
		assert (i != -1);
		
		// Calculate a color based from this index. This method caters for
		// 16777216 objects. I don't think that'll be exceeded anytime soon. :)
		// ATM biggest is 53588.dat with 12600 lines.
		double r = (i / (256 * 256)) % 256,
			g = (i / 256) % 256,
			b = i % 256;
		
		qglColor (QColor (r, g, b, 255));
		return;
	}
	
	if (obj->dColor == -1)
		return;
	
#if 0
	if (gl_colorbfc &&
		obj->getType () != OBJ_Line &&
		obj->getType () != OBJ_CondLine)
	{
		if (bBackSide)
			glColor4f (0.9f, 0.0f, 0.0f, 1.0f);
		else
			glColor4f (0.0f, 0.8f, 0.0f, 1.0f);
		return;
	}
#endif
	
	if (obj->dColor == maincolor)
		qCol = getMainColor ();
	else {
		color* col = getColor (obj->dColor);
		qCol = col->qColor;
	}
	
	if (obj->dColor == edgecolor) {
		qCol = Qt::black;
		color* col;
		
		if (!gl_blackedges && obj->parent != null && (col = getColor (obj->parent->dColor)) != null)
			qCol = col->qEdge;
	}
	
	if (qCol.isValid () == false) {
		// The color was unknown. Use main color to make the object at least
		// not appear pitch-black.
		if (obj->dColor != edgecolor)
			qCol = getMainColor ();
		
		// Warn about the unknown colors, but only once.
		for (short i : g_daWarnedColors)
			if (obj->dColor == i)
				return;
		
		printf ("%s: Unknown color %d!\n", __func__, obj->dColor);
		g_daWarnedColors.push_back (obj->dColor);
		return;
	}
	
	long r = qCol.red (),
		g = qCol.green (),
		b = qCol.blue (),
		a = qCol.alpha ();
	
	// If it's selected, brighten it up, also pulse flash it if desired.
	if (g_ForgeWindow->isSelected (obj)) {
		short tick, numTicks;
		
		if (gl_selflash) {
			tick = (g_pulseTick < (g_numPulseTicks / 2)) ? g_pulseTick : (g_numPulseTicks - g_pulseTick);
			numTicks = g_numPulseTicks;
		} else {
			tick = 2;
			numTicks = 5;
		}
		
		const long add = ((tick * 128) / numTicks);
		r = min (r + add, 255l);
		g = min (g + add, 255l);
		b = min (b + add, 255l);
		
		// a = 255;
	}
	
	glColor4f (
		((double) r) / 255.0f,
		((double) g) / 255.0f,
		((double) b) / 255.0f,
		((double) a) / 255.0f);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::refresh () {
	update ();
	swapBuffers ();
}

// =============================================================================
void GLRenderer::hardRefresh () {
	compileObjects ();
	refresh ();
	
	glLineWidth (gl_linethickness);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::resizeGL (int w, int h) {
	width = w;
	height = h;
	
	calcCameraIconRects ();
	
	glViewport (0, 0, w, h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (45.0f, (double)w / (double)h, 1.0f, 100.0f);
	glMatrixMode (GL_MODELVIEW);
}

const struct GLAxis {
	const QColor col;
	const vertex vert;
} g_GLAxes[3] = {
	{ QColor (255, 0, 0), vertex (10000, 0, 0) },
	{ QColor (128, 192, 0), vertex (0, 10000, 0) },
	{ QColor (0, 160, 192), vertex (0, 0, 10000) },
};

void GLRenderer::drawGLScene () {
	if (g_CurrentFile == null)
		return;
	
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable (GL_DEPTH_TEST);
	
	if (m_camera != GLRenderer::Free) {
		glMatrixMode (GL_PROJECTION);
		glPushMatrix ();
		
		glLoadIdentity ();
		glOrtho (-vw, vw, -vh, vh, -100.0f, 100.0f);
		glTranslatef (panX, panY, 0.0f);
		glRotatef (90.0f, g_staticCameras[m_camera].glrotate[0],
			g_staticCameras[m_camera].glrotate[1],
			g_staticCameras[m_camera].glrotate[2]);
		
		// Back camera needs to be handled differently
		if (m_camera == GLRenderer::Back) {
			glRotatef (180.0f, 1.0f, 0.0f, 0.0f);
			glRotatef (180.0f, 0.0f, 0.0f, 1.0f);
		}
	} else {
		glMatrixMode (GL_MODELVIEW);
		glPushMatrix ();
		glLoadIdentity ();
		
		glTranslatef (0.0f, 0.0f, -2.0f);
		glTranslatef (panX, panY, -zoom);
		glRotatef (rotX, 1.0f, 0.0f, 0.0f);
		glRotatef (rotY, 0.0f, 1.0f, 0.0f);
		glRotatef (rotZ, 0.0f, 0.0f, 1.0f);
	}
	
	for (LDObject* obj : g_CurrentFile->objects)
		glCallList (picking == false ? obj->uGLList : obj->uGLPickList);
	
	if (gl_axes && !picking)
		glCallList (axeslist);
	
	glPopMatrix ();
	glMatrixMode (GL_MODELVIEW);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::paintEvent (QPaintEvent* ev) {
	Q_UNUSED (ev)
	vw = zoom;
	vh = (height * vw) / width;
	drawGLScene ();
	
	QPainter paint (this);
	QFontMetrics metrics = QFontMetrics (QFont ());
	paint.setRenderHint (QPainter::Antialiasing);
	
	m_hoverpos = g_Origin;
	
	if (m_camera != Free) {
		const staticCameraMeta* cam = &g_staticCameras[m_camera];
		const Axis axisX = cam->axisX;
		const Axis axisY = cam->axisY;
		const short negXFac = cam->negX ? -1 : 1,
			negYFac = cam->negY ? -1 : 1;
		
		// Calculate cx and cy - these are the LDraw unit coords the cursor is at.
		double cx = Grid::snap ((-vw + ((2 * mouseX * vw) / width) - panX) * g_storedBBoxSize -
			(negXFac * g_objOffset[axisX]), (Grid::Config) axisX);
		double cy = Grid::snap ((vh - ((2 * mouseY * vh) / height) - panY) * g_storedBBoxSize -
			(negYFac * g_objOffset[axisY]), (Grid::Config) axisY);
		cx *= negXFac;
		cy *= negYFac;
		
		// Set the position we're hovering over based on this
		m_hoverpos[axisX] = cx;
		m_hoverpos[axisY] = cy;
		
		// Paint the coordinates onto the screen.
		str text;
		text.format ("X: %s, Y: %s, Z: %s", ftoa (m_hoverpos[X]).chars (),
			ftoa (m_hoverpos[Y]).chars (), ftoa (m_hoverpos[Z]).chars ());
		
		QFontMetrics metrics = QFontMetrics (font ());
		QRect textSize = metrics.boundingRect (0, 0, width, height, Qt::AlignCenter, text);
		
		paint.drawText (width - textSize.width (), height - 16, textSize.width (),
			textSize.height (), Qt::AlignCenter, text);
	}
	
	// Camera icons
	if (!picking) {
		// Draw a background for the selected camera
		paint.setPen (thinBorderPen);
		paint.setBrush (QBrush (QColor (0, 128, 160, 128)));
		paint.drawRect (g_CameraIcons[camera ()].selRect);
		
		// Draw the actual icons
		for (CameraIcon& info : g_CameraIcons)
			paint.drawPixmap (info.destRect, *info.img, info.srcRect);
		
		// Draw a label for the current camera in the top left corner
		{
			const ushort margin = 4;
			
			str label;
			label.format ("%s Camera", g_CameraNames[camera ()]);
			paint.setBrush (Qt::black);
			paint.drawText (QPoint (margin, margin + metrics.ascent ()), label);
		}
		
		// Tool tips
		if (drawToolTip) {
			if (g_CameraIcons[toolTipCamera].destRect.contains (QPoint (mouseX, mouseY)) == false)
				drawToolTip = false;
			else {
				QPen bord = thinBorderPen;
				bord.setBrush (Qt::black);
				
				const ushort margin = 2;
				ushort x0 = mouseX,
					y0 = mouseY;
				
				str label;
				label.format ("%s Camera", g_CameraNames[toolTipCamera]);
				
				const ushort textWidth = metrics.width (label),
					textHeight = metrics.height (),
					fullWidth = textWidth + (2 * margin),
					fullHeight = textHeight + (2 * margin);
				
				QRect area (mouseX, mouseY, fullWidth, fullHeight);
				
				if (x0 + fullWidth > width)
					x0 -= fullWidth;
				
				if (y0 + fullHeight > height)
					y0 -= fullHeight;
				
				paint.setBrush (QColor (0, 128, 255, 208));
				paint.setPen (bord);
				paint.drawRect (x0, y0, fullWidth, fullHeight);
				
				paint.setBrush (Qt::black);
				paint.drawText (QPoint (x0 + margin, y0 + margin + metrics.ascent ()), label);
			}
		}
	}
	
	// If we're range-picking, draw a rectangle encompassing the selection area.
	if (rangepick && !picking) {
		const short x0 = rangeStart.x (),
			y0 = rangeStart.y (),
			x1 = pos.x (),
			y1 = pos.y ();
		
		QRect rect (x0, y0, x1 - x0, y1 - y0);
		QColor fillColor = (addpick ? "#80FF00" : "#00CCFF");
		fillColor.setAlphaF (0.2f);
		
		paint.setPen (thickBorderPen);
		paint.setBrush (QBrush (fillColor));
		paint.drawRect (rect);
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::compileObjects () {
	objLists.clear ();
	
	g_objOffset[0] = -(g_BBox.v0[X] + g_BBox.v1[X]) / 2;
	g_objOffset[1] = -(g_BBox.v0[Y] + g_BBox.v1[Y]) / 2;
	g_objOffset[2] = -(g_BBox.v0[Z] + g_BBox.v1[Z]) / 2;
	g_storedBBoxSize = g_BBox.size ();
	
	if (!g_CurrentFile) {
		printf ("renderer: no files loaded, cannot compile anything\n");
		return;
	}
	
	for (LDObject* obj : g_CurrentFile->objects) {
		GLuint* upaLists[2] = {
			&obj->uGLList,
			&obj->uGLPickList
		};
		
		for (GLuint* upMemberList : upaLists) {
			GLuint uList = glGenLists (1);
			glNewList (uList, GL_COMPILE);
			
			picking = (upMemberList == &obj->uGLPickList);
			compileOneObject (obj);
			picking = false;
			
			glEndList ();
			*upMemberList = uList;
		}
		
		objLists.push_back (obj->uGLList);
	}
	
	// Compile axes
	axeslist = glGenLists (1);
	glNewList (axeslist, GL_COMPILE);
	glBegin (GL_LINES);
	for (const GLAxis& ax : g_GLAxes) {
		qglColor (ax.col);
		compileVertex (ax.vert);
		compileVertex (-ax.vert);
	}
	glEnd ();
	glEndList ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
template<class T> void GLRenderer::compileSubObject (LDObject* obj,
	const GLenum eGLType, const short dVerts)
{
	T* newobj = static_cast<T*> (obj);
	glBegin (eGLType);
	
	for (short i = 0; i < dVerts; ++i)
		compileVertex (newobj->vaCoords[i]);
	
	glEnd ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::compileOneObject (LDObject* obj) {
	setObjectColor (obj);
	
	switch (obj->getType ()) {
	case OBJ_Line:
		compileSubObject<LDLine> (obj, GL_LINES, 2);
		break;
	
	case OBJ_CondLine:
		glLineStipple (1, 0x6666);
		glEnable (GL_LINE_STIPPLE);
		
		compileSubObject<LDCondLine> (obj, GL_LINES, 2);
		
		glDisable (GL_LINE_STIPPLE);
		break;
	
	case OBJ_Triangle:
		compileSubObject<LDTriangle> (obj, GL_TRIANGLES, 3);
		break;
	
	case OBJ_Quad:
		compileSubObject<LDQuad> (obj, GL_QUADS, 4);
		break;
	
	case OBJ_Subfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			vector<LDObject*> objs = ref->inlineContents (true, true);
			
			for (LDObject* obj : objs) {
				compileOneObject (obj);
				delete obj;
			}
		}
		break;
	
	case OBJ_Radial:
		{
			LDRadial* pRadial = static_cast<LDRadial*> (obj);
			std::vector<LDObject*> objs = pRadial->decompose (true);
			
			for (LDObject* obj : objs) {
				compileOneObject (obj);
				delete obj;
			}
		}
		break;
	
#if 0
	TODO: find a proper way to draw vertices without having them be affected by zoom.
	case OBJ_Vertex:
		{
			LDVertex* pVert = static_cast<LDVertex*> (obj);
			LDTriangle* pPoly;
			vertex* vPos = &(pVert->vPosition);
			const double fPolyScale = max (fZoom, 1.0);
			
#define BIPYRAMID_COORD(N) ((((i + N) % 4) >= 2 ? 1 : -1) * 0.3f * fPolyScale)
			
			for (int i = 0; i < 8; ++i) {
				pPoly = new LDTriangle;
				pPoly->vaCoords[0] = {vPos->x, vPos->y + ((i >= 4 ? 1 : -1) * 0.4f * fPolyScale), vPos->z};
				pPoly->vaCoords[1] = {
					vPos->x + BIPYRAMID_COORD (0),
					vPos->y,
					vPos->z + BIPYRAMID_COORD (1)
				};
				
				pPoly->vaCoords[2] = {
					vPos->x + BIPYRAMID_COORD (1),
					vPos->y,
					vPos->z + BIPYRAMID_COORD (2)
				};
				
				pPoly->dColor = pVert->dColor;
				compileOneObject (pPoly);
				delete pPoly;
			}
		}
		break;
#endif // 0
	
	default:
		break;
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::compileVertex (const vertex& vrt) {
	glVertex3d (
		(vrt[X] + g_objOffset[0]) / g_storedBBoxSize,
		-(vrt[Y] + g_objOffset[1]) / g_storedBBoxSize,
		-(vrt[Z] + g_objOffset[2]) / g_storedBBoxSize);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::clampAngle (double& angle) {
	while (angle < 0)
		angle += 360.0;
	while (angle > 360.0)
		angle -= 360.0;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::mouseReleaseEvent (QMouseEvent* ev) {
	if ((lastButtons & Qt::LeftButton) && !(ev->buttons() & Qt::LeftButton)) {
		if (!rangepick)
			addpick = (keymods & Qt::ControlModifier);
		
		if (totalmove < 10 || rangepick)
			pick (ev->x (), ev->y ());
		
		rangepick = false;
		totalmove = 0;
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::mousePressEvent (QMouseEvent* ev) {
	if (ev->buttons () & Qt::LeftButton)
		totalmove = 0;
	
	if (ev->modifiers () & Qt::ShiftModifier) {
		rangepick = true;
		rangeStart.setX (ev->x ());
		rangeStart.setY (ev->y ());
		
		addpick = (keymods & Qt::ControlModifier);
	}
	
	lastButtons = ev->buttons ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::mouseMoveEvent (QMouseEvent* ev) {
	mouseX = pos.x ();
	mouseY = pos.y ();
	
	int dx = ev->x () - pos.x ();
	int dy = ev->y () - pos.y ();
	totalmove += abs (dx) + abs (dy);
	
	if (ev->buttons () & Qt::LeftButton && !rangepick) {
		rotX = rotX + (dy);
		rotY = rotY + (dx);
		
		clampAngle (rotX);
		clampAngle (rotY);
	}
	
	if (ev->buttons () & Qt::MidButton) {
		panX += 0.03f * dx * (zoom / 7.5f);
		panY -= 0.03f * dy * (zoom / 7.5f);
	}
	
	// Start the tool tip timer
	if (!drawToolTip)
		toolTipTimer->start (1000);
	
	pos = ev->pos ();
	update ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::keyPressEvent (QKeyEvent* ev) {
	keymods = ev->modifiers ();
}

void GLRenderer::keyReleaseEvent (QKeyEvent* ev) {
	keymods = ev->modifiers ();
}

// =============================================================================
void GLRenderer::wheelEvent (QWheelEvent* ev) {
	zoom *= (ev->delta () < 0) ? 1.2f : (1.0f / 1.2f);
	zoom = clamp (zoom, 0.01, 100.0);
	
	update ();
	ev->accept ();
}

// =============================================================================
void GLRenderer::leaveEvent (QEvent* ev) {
	Q_UNUSED (ev);
	drawToolTip = false;
	toolTipTimer->stop ();
	update ();
}

// =============================================================================
void GLRenderer::contextMenuEvent (QContextMenuEvent* ev) {
	g_ForgeWindow->spawnContextMenu (ev->globalPos ());
}

// =============================================================================
void GLRenderer::setCamera (const GLRenderer::Camera cam) {
	m_camera = cam;
	gl_camera = (int) cam;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::updateSelFlash () {
	if (gl_selflash && g_ForgeWindow->sel.size() > 0) {
		pulseTimer->start (g_pulseInterval);
		g_pulseTick = 0;
	} else
		pulseTimer->stop ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::pick (uint mouseX, uint mouseY) {
	// Check if we selected a camera icon
	if (!rangepick) {
		QPoint pos (mouseX, mouseY);
		
		for (CameraIcon& info : g_CameraIcons) {
			if (info.destRect.contains (pos)) {
				setCamera (info.cam);
				update ();
				return;
			}
		}
	}
	
	GLint viewport[4];
	LDObject* removedObject = null;
	
	// Clear the selection if we do not wish to add to it.
	if (!addpick) {
		std::vector<LDObject*> paOldSelection = g_ForgeWindow->sel;
		g_ForgeWindow->sel.clear ();
		
		// Recompile the prior selection to remove the highlight color
		for (LDObject* obj : paOldSelection)
			recompileObject (obj);
	}
	
	picking = true;
	
	// Paint the picking scene
	glDisable (GL_DITHER);
	glClearColor (1.0f, 1.0f, 1.0f, 1.0f);
	drawGLScene ();
	
	glGetIntegerv (GL_VIEWPORT, viewport);
	
	short x0 = mouseX,
		y0 = mouseY;
	short x1, y1;
	
	// Determine how big an area to read - with range picking, we pick by
	// the area given, with single pixel picking, we use an 1 x 1 area.
	if (rangepick) {
		x1 = rangeStart.x ();
		y1 = rangeStart.y ();
	} else {
		x1 = x0 + 1;
		y1 = y0 + 1;
	}
	
	// x0 and y0 must be less than x1 and y1, respectively.
	if (x0 > x1)
		dataswap (x0, x1);
	
	if (y0 > y1)
		dataswap (y0, y1);
	
	// Clamp the values to ensure they're within bounds
	x0 = max<short> (0, x0);
	y0 = max<short> (0, y0);
	x1 = min<short> (x1, width);
	y1 = min<short> (y1, height);
	
	const short areawidth = (x1 - x0);
	const short areaheight = (y1 - y0);
	const long numpixels = areawidth * areaheight;
	
	// Allocate space for the pixel data.
	uchar* const pixeldata = new uchar[4 * numpixels];
	uchar* pixelptr = &pixeldata[0];
	
	assert (viewport[3] == height);
	
	// Read pixels from the color buffer.
	glReadPixels (x0, viewport[3] - y1, areawidth, areaheight, GL_RGBA, GL_UNSIGNED_BYTE, pixeldata);
	
	// Go through each pixel read and add them to the selection.
	for (long i = 0; i < numpixels; ++i) {
		uint32 idx =
			(*(pixelptr) * 0x10000) +
			(*(pixelptr + 1) * 0x00100) +
			(*(pixelptr + 2) * 0x00001);
		pixelptr += 4;
		
		if (idx == 0xFFFFFF)
			continue; // White is background; skip
		
		LDObject* obj = g_CurrentFile->object (idx);
		
		// If this is an additive single pick and the object is currently selected,
		// we remove it from selection instead.
		if (!rangepick && addpick) {
			bool removed = false;
			
			for (ulong i = 0; i < g_ForgeWindow->sel.size(); ++i) {
				if (g_ForgeWindow->sel[i] == obj) {
					g_ForgeWindow->sel.erase (g_ForgeWindow->sel.begin () + i);
					removedObject = obj;
					removed = true;
				}
			}
			
			if (removed)
				break;
		}
		
		g_ForgeWindow->sel.push_back (obj);
	}
	
	delete[] pixeldata;
	
	// Remove duplicate entries. For this to be effective, the vector must be
	// sorted first.
	std::vector<LDObject*>& sel = g_ForgeWindow->sel;
	std::sort (sel.begin(), sel.end ());
	std::vector<LDObject*>::iterator pos = std::unique (sel.begin (), sel.end ());
	sel.resize (std::distance (sel.begin (), pos));
	
	// Update everything now.
	g_ForgeWindow->buildObjList ();
	
	picking = false;
	rangepick = false;
	glEnable (GL_DITHER);
	
	setBackground ();
	updateSelFlash ();
	
	for (LDObject* obj : g_ForgeWindow->sel)
		recompileObject (obj);
	
	if (removedObject != null)
		recompileObject (removedObject);
	
	drawGLScene ();
	swapBuffers ();
	update ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void GLRenderer::recompileObject (LDObject* obj) {
	// Replace the old list with the new one.
	for (ulong i = 0; i < objLists.size(); ++i)
		if (objLists[i] == obj->uGLList)
			objLists.erase (objLists.begin() + i);
	
	GLuint uList = glGenLists (1);
	glNewList (uList, GL_COMPILE);
	
	compileOneObject (obj);
	
	glEndList ();
	objLists.push_back (uList);
	obj->uGLList = uList;
}

// =============================================================================
uchar* GLRenderer::screencap (ushort& w, ushort& h) {
	w = width;
	h = height;
	uchar* cap = new uchar[4 * w * h];
	
	screencapping = true;
	update ();
	screencapping = false;
	
	// Capture the pixels
	glReadPixels (0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, cap);
	
	// Restore the background
	setBackground ();
	
	return cap;
}

// =============================================================================
void GLRenderer::slot_timerUpdate () {
	++g_pulseTick %= g_numPulseTicks;
	
	for (LDObject* obj : g_ForgeWindow->sel)
		recompileObject (obj);
	
	update ();
}

// =============================================================================
void GLRenderer::slot_toolTipTimer () {
	// We come here if the cursor has stayed in one place for longer than a
	// a second. Check if we're holding it over a camera icon - if so, draw
	// a tooltip.
	for (CameraIcon& icon : g_CameraIcons) {
		if (icon.destRect.contains (QPoint (mouseX, mouseY))) {
			toolTipCamera = icon.cam;
			drawToolTip = true;
			update ();
			break;
		}
	}
}