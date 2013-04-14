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

static double g_faObjectOffset[3];
static double g_StoredBBoxSize;
static bool g_bPicking = false;

static short g_dPulseTick = 0;
static const short g_dNumPulseTicks = 8;
static const short g_dPulseInterval = 65;

cfg (str, gl_bgcolor, "#CCCCD9");
cfg (str, gl_maincolor, "#707078");
cfg (float, gl_maincolor_alpha, 1.0);
cfg (int, gl_linethickness, 2);
cfg (bool, gl_colorbfc, true);
cfg (bool, gl_selflash, false);

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
GLRenderer::GLRenderer (QWidget* parent) {
	parent = parent; // shhh, GCC
	fRotX = fRotY = fRotZ = 0.0f;
	fZoom = 1.0f;
	
	qPulseTimer = new QTimer (this);
	connect (qPulseTimer, SIGNAL (timeout ()), this, SLOT (slot_timerUpdate ()));
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void GLRenderer::initializeGL () {
	glLoadIdentity();
	glMatrixMode (GL_MODELVIEW);
	
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
	
	setMouseTracking (true);
	compileObjects ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
QColor GLRenderer::getMainColor () {
	QColor col (gl_maincolor.value.chars());
	
	if (!col.isValid ())
		return col; // shouldn't happen
	
	col.setAlpha (gl_maincolor_alpha * 255.f);
	return col;
}

// ------------------------------------------------------------------------- //
void GLRenderer::setBackground () {
	QColor col (gl_bgcolor.value.chars());
	
	if (!col.isValid ())
		return;
	
	glClearColor (
		((double)col.red()) / 255.0f,
		((double)col.green()) / 255.0f,
		((double)col.blue()) / 255.0f,
		1.0f);
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
static vector<short> g_daWarnedColors;
void GLRenderer::setObjectColor (LDObject* obj) {
	QColor qCol;
	
	if (g_bPicking) {
		// Make the color by the object's index color if we're picking, so we can
		// make the index from the color we get from the picking results.
		long i = obj->getIndex (g_CurrentFile);
		
		// If we couldn't find the index, this object must not be from this file,
		// therefore it must be an object inlined from another file through a
		// subfile reference. Use the reference's index.
		if (i == -1)
			i = obj->topLevelParent ()->getIndex (g_CurrentFile);
		
		// We should have the index now.
		assert (i != -1);
		
		// Calculate a color based from this index. This method caters for
		// 16777216 objects. I don't think that'll be exceeded anytime soon. :)
		// ATM biggest is 53588.dat with 12600 lines.
		double r = i % 256;
		double g = (i / 256) % 256;
		double b = (i / (256 * 256)) % 256;
		
		glColor3f (r / 255.f, g / 255.f, b / 255.f);
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
	
	if (obj->dColor == dMainColor)
		qCol = getMainColor ();
	else {
		color* col = getColor (obj->dColor);
		
		if (col != null)
			qCol = col->qColor;
		else {
			// The color was unknown. Use main color to make the object at least
			// not appear pitch-black.
			qCol = getMainColor ();
			
			// Warn about the unknown colors, but only once.
			for (short i : g_daWarnedColors)
				if (obj->dColor == i)
					return;
			
			printf ("%s: Unknown color %d!\n", __func__, obj->dColor);
			g_daWarnedColors.push_back (obj->dColor);
			return;
		}
	}
	
	long r = qCol.red (),
		g = qCol.green (),
		b = qCol.blue (),
		a = qCol.alpha ();
	
	// If it's selected, brighten it up, also pulse flash it if desired.
	if (g_ForgeWindow->isSelected (obj)) {
		short dTick, dNumTicks;
		
		if (gl_selflash) {
			dTick = (g_dPulseTick < (g_dNumPulseTicks / 2)) ? g_dPulseTick : (g_dNumPulseTicks - g_dPulseTick);
			dNumTicks = g_dNumPulseTicks;
		} else {
			dTick = 2;
			dNumTicks = 5;
		}
		
		const long lAdd = ((dTick * 128) / dNumTicks);
		r = min (r + lAdd, 255l);
		g = min (g + lAdd, 255l);
		b = min (b + lAdd, 255l);
		
		a = 255;
	}
	
	glColor4f (
		((double) r) / 255.0f,
		((double) g) / 255.0f,
		((double) b) / 255.0f,
		((double) a) / 255.0f);
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void GLRenderer::hardRefresh () {
	compileObjects ();
	paintGL ();
	swapBuffers ();
	
	glLineWidth (gl_linethickness);
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void GLRenderer::resizeGL (int w, int h) {
	glViewport (0, 0, w, h);
	glLoadIdentity ();
	glMatrixMode (GL_PROJECTION);
	gluPerspective (45.0f, (double)w / (double)h, 0.1f, 100.0f);
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void GLRenderer::paintGL () {
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode (GL_MODELVIEW);
	
	if (g_CurrentFile == null)
		return;
	
	glPushMatrix ();
		glLoadIdentity ();
		
		glTranslatef (0.0f, 0.0f, -5.0f);
		glTranslatef (0.0f, 0.0f, -fZoom);
		
		glRotatef (fRotX, 1.0f, 0.0f, 0.0f);
		glRotatef (fRotY, 0.0f, 1.0f, 0.0f);
		glRotatef (fRotZ, 0.0f, 0.0f, 1.0f);
		
		for (LDObject* obj : g_CurrentFile->objects)
			glCallList ((g_bPicking == false) ? obj->uGLList : obj->uGLPickList);
	glPopMatrix ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void GLRenderer::compileObjects () {
	uaObjLists.clear ();
	
	g_faObjectOffset[0] = -(g_BBox.v0.x + g_BBox.v1.x) / 2;
	g_faObjectOffset[1] = -(g_BBox.v0.y + g_BBox.v1.y) / 2;
	g_faObjectOffset[2] = -(g_BBox.v0.z + g_BBox.v1.z) / 2;
	g_StoredBBoxSize = g_BBox.calcSize ();
	
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
			
			g_bPicking = (upMemberList == &obj->uGLPickList);
			compileOneObject (obj);
			g_bPicking = false;
			
			glEndList ();
			*upMemberList = uList;
		}
		
		uaObjLists.push_back (obj->uGLList);
	}
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
template<class T> void GLRenderer::compileSubObject (LDObject* obj,
	const GLenum eGLType, const short dVerts)
{
	T* newobj = static_cast<T*> (obj);
	glBegin (eGLType);
	
	for (short i = 0; i < dVerts; ++i)
		compileVertex (newobj->vaCoords[i]);
	
	glEnd ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
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

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void GLRenderer::compileVertex (vertex& vrt) {
	glVertex3d (
		(vrt.x + g_faObjectOffset[0]) / g_StoredBBoxSize,
		-(vrt.y + g_faObjectOffset[1]) / g_StoredBBoxSize,
		-(vrt.z + g_faObjectOffset[2]) / g_StoredBBoxSize);
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void GLRenderer::clampAngle (double& fAngle) {
	while (fAngle < 0)
		fAngle += 360.0;
	while (fAngle > 360.0)
		fAngle -= 360.0;
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void GLRenderer::mouseReleaseEvent (QMouseEvent* event) {
	if ((qMouseButtons & Qt::LeftButton) && !(event->buttons() & Qt::LeftButton)) {
		if (ulTotalMouseMove < 10)
			pick (event->x(), event->y(), (qKeyMods & Qt::ControlModifier));
		
		ulTotalMouseMove = 0;
	}
}

// ========================================================================= //
void GLRenderer::mousePressEvent (QMouseEvent* event) {
	qMouseButtons = event->buttons();
	if (event->buttons() & Qt::LeftButton)
		ulTotalMouseMove = 0;
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void GLRenderer::mouseMoveEvent (QMouseEvent *event) {
	int dx = event->x () - lastPos.x ();
	int dy = event->y () - lastPos.y ();
	ulTotalMouseMove += abs (dx) + abs (dy);
	
	if (event->buttons () & Qt::LeftButton) {
		fRotX = fRotX + (dy);
		fRotY = fRotY + (dx);
		clampAngle (fRotX);
		clampAngle (fRotY);
	}
	
	if (event->buttons () & Qt::RightButton) {
		fRotX = fRotX + (dy);
		fRotZ = fRotZ + (dx);
		clampAngle (fRotX);
		clampAngle (fRotZ);
	}
	
	if (event->buttons () & Qt::MidButton) {
		fZoom += (dy / 100.0);
		fZoom = clamp (fZoom, 0.01, 100.0);
	}
	
	lastPos = event->pos();
	updateGL ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void GLRenderer::keyPressEvent (QKeyEvent* qEvent) {
	qKeyMods = qEvent->modifiers ();
}

void GLRenderer::keyReleaseEvent (QKeyEvent* qEvent) {
	qKeyMods = qEvent->modifiers ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void GLRenderer::pick (uint uMouseX, uint uMouseY, bool bAdd) {
	if (bAdd == false) {
		// Clear the selection if we don't wish to add to it.
		std::vector<LDObject*> paOldSelection = g_ForgeWindow->paSelection;
		g_ForgeWindow->paSelection.clear ();
		
		// Recompile the prior selection to remove the highlight color
		for (LDObject* obj : paOldSelection)
			recompileObject (obj);
	}
	
	glDisable (GL_DITHER);
	glClearColor (1.0f, 1.0f, 1.0f, 1.0f);
	
	g_bPicking = true;
	
	paintGL ();
	
	GLubyte ucaPixel[3];
	GLint daViewport[4];
	glGetIntegerv (GL_VIEWPORT, daViewport);
	glReadPixels (uMouseX, daViewport[3] - uMouseY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE,
		reinterpret_cast<GLvoid*> (ucaPixel));
	
	// If we hit a white pixel, we selected the background. This no object is selected.
	const bool bHasSelection = (ucaPixel[0] != 255 || ucaPixel[1] != 255 || ucaPixel[2] != 255);
	
	if (bHasSelection) {
		ulong idx = ucaPixel[0] + (ucaPixel[1] * 256) + (ucaPixel[2] * 256 * 256);
		
		LDObject* obj = g_CurrentFile->object (idx);
		g_ForgeWindow->paSelection.push_back (obj);
	}
	
	if (gl_selflash) {
		if (bHasSelection) {
			qPulseTimer->start (g_dPulseInterval);
			g_dPulseTick = 0;
		} else
			qPulseTimer->stop ();
	}
	
	g_bPicking = false;
	glEnable (GL_DITHER);
	
	g_ForgeWindow->updateSelection ();
	
	setBackground ();
	
	for (LDObject* obj : g_ForgeWindow->getSelectedObjects ())
		recompileObject (obj);
	
	paintGL ();
	swapBuffers ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void GLRenderer::recompileObject (LDObject* obj) {
	// Replace the old list with the new one.
	for (ulong i = 0; i < uaObjLists.size(); ++i)
		if (uaObjLists[i] == obj->uGLList)
			uaObjLists.erase (uaObjLists.begin() + i);
	
	GLuint uList = glGenLists (1);
	glNewList (uList, GL_COMPILE);
	
	compileOneObject (obj);
	
	glEndList ();
	uaObjLists.push_back (uList);
	obj->uGLList = uList;
}

// ========================================================================= //
void GLRenderer::slot_timerUpdate () {
	++g_dPulseTick %= g_dNumPulseTicks;
	
	for (LDObject* obj : g_ForgeWindow->getSelectedObjects ())
		recompileObject (obj);
	
	paintGL ();
	swapBuffers ();
}