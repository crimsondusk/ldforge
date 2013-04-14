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

cfg (str, gl_bgcolor, "#CCCCD9");
cfg (str, gl_maincolor, "#707078");
cfg (float, gl_maincolor_alpha, 1.0);
cfg (int, gl_linethickness, 2);
cfg (bool, gl_colorbfc, true);

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
renderer::renderer (QWidget* parent) {
	parent = parent; // shhh, GCC
	fRotX = fRotY = fRotZ = 0.0f;
	fZoom = 1.0f;
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void renderer::initializeGL () {
	glLoadIdentity();
	glMatrixMode (GL_MODELVIEW);
	
	setBackground ();
	
	glEnable (GL_POLYGON_OFFSET_FILL);
	glPolygonOffset (1.0f, 1.0f);
	
	glEnable (GL_DEPTH_TEST);
	glShadeModel (GL_SMOOTH);
	glEnable (GL_MULTISAMPLE);
	
	glEnable (GL_DITHER);
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
void renderer::setMainColor () {
	QColor col (gl_maincolor.value.chars());
	
	if (!col.isValid ())
		return;
	
	glColor4f (
		((double)col.red()) / 255.0f,
		((double)col.green()) / 255.0f,
		((double)col.blue()) / 255.0f,
		gl_maincolor_alpha);
}

// ------------------------------------------------------------------------- //
void renderer::setBackground () {
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
void renderer::setObjectColor (LDObject* obj) {
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
	
	if (obj->dColor == dMainColor) {
		setMainColor ();
		return;
	}
	
	color* col = getColor (obj->dColor);
	
	if (!col) {
		// The color was unknown. Use main color to make the object at least
		// not appear pitch-black.
		setMainColor ();
		
		// Warn about the unknown colors, but only once.
		for (short i : g_daWarnedColors)
			if (obj->dColor == i)
				return;
		
		printf ("%s: Unknown color %d!\n", __func__, obj->dColor);
		g_daWarnedColors.push_back (obj->dColor);
		return;
	}
	
	glColor4f (
		((double)col->qColor.red()) / 255.0f,
		((double)col->qColor.green()) / 255.0f,
		((double)col->qColor.blue()) / 255.0f,
		((double)col->qColor.alpha()) / 255.0f);
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void renderer::hardRefresh () {
	compileObjects ();
	paintGL ();
	swapBuffers ();
	
	glLineWidth (gl_linethickness);
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void renderer::resizeGL (int w, int h) {
	glViewport (0, 0, w, h);
	glLoadIdentity ();
	glMatrixMode (GL_PROJECTION);
	gluPerspective (45.0f, (double)w / (double)h, 0.1f, 100.0f);
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void renderer::paintGL () {
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode (GL_MODELVIEW);
	
	glPushMatrix ();
		glLoadIdentity ();
		
		glTranslatef (0.0f, 0.0f, -5.0f);
		glTranslatef (0.0f, 0.0f, -fZoom);
		
		glRotatef (fRotX, 1.0f, 0.0f, 0.0f);
		glRotatef (fRotY, 0.0f, 1.0f, 0.0f);
		glRotatef (fRotZ, 0.0f, 0.0f, 1.0f);
		
		for (GLuint uList : uaObjLists)
			glCallList (uList);
	glPopMatrix ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void renderer::compileObjects () {
	uaObjLists.clear ();
	
	g_faObjectOffset[0] = -(g_BBox.v0.x + g_BBox.v1.x) / 2;
	g_faObjectOffset[1] = -(g_BBox.v0.y + g_BBox.v1.y) / 2;
	g_faObjectOffset[2] = -(g_BBox.v0.z + g_BBox.v1.z) / 2;
	g_StoredBBoxSize = g_BBox.calcSize ();
	printf ("bbox size is %f\n", g_StoredBBoxSize);
	
	if (!g_CurrentFile) {
		printf ("renderer: no files loaded, cannot compile anything\n");
		return;
	}
	
	for (LDObject* obj : g_CurrentFile->objects) 
		compileOneObject (obj);
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
template<class T> void renderer::compileSubObject (LDObject* obj,
	const GLenum eGLType, const short dVerts)
{
	setObjectColor (obj);
	T* newobj = static_cast<T*> (obj);
	glBegin (eGLType);
	
	for (short i = 0; i < dVerts; ++i)
		compileVertex (newobj->vaCoords[i]);
	
	glEnd ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void renderer::compileOneObject (LDObject* obj) {
	GLuint uList = glGenLists (1);
	glNewList (uList, GL_COMPILE);
	
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
	
	glEndList ();
	uaObjLists.push_back (uList);
	obj->uGLList = uList;
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void renderer::compileVertex (vertex& vrt) {
	glVertex3d (
		(vrt.x + g_faObjectOffset[0]) / g_StoredBBoxSize,
		-(vrt.y + g_faObjectOffset[1]) / g_StoredBBoxSize,
		-(vrt.z + g_faObjectOffset[2]) / g_StoredBBoxSize);
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void renderer::clampAngle (double& fAngle) {
	while (fAngle < 0)
		fAngle += 360.0;
	while (fAngle > 360.0)
		fAngle -= 360.0;
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void renderer::mouseReleaseEvent (QMouseEvent* event) {
	if ((qMouseButtons & Qt::LeftButton) && !(event->buttons() & Qt::LeftButton)) {
		if (ulTotalMouseMove < 10)
			pick (event->x(), event->y());
		
		ulTotalMouseMove = 0;
	}
}

void renderer::mousePressEvent (QMouseEvent* event) {
	qMouseButtons = event->buttons();
	if (event->buttons() & Qt::LeftButton)
		ulTotalMouseMove = 0;
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void renderer::mouseMoveEvent (QMouseEvent *event) {
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
void renderer::pick (uint mx, uint my) {
	glDisable (GL_DITHER);
	glClearColor (1.0f, 1.0f, 1.0f, 1.0f);
	
	g_bPicking = true;
	
	compileObjects ();
	paintGL ();
	
	GLubyte pixel[3];
	GLint viewport[4];
	glGetIntegerv (GL_VIEWPORT, viewport);
	glReadPixels (mx, viewport[3] - my, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, reinterpret_cast<GLvoid*> (pixel));
	
	if (pixel[0] != 255 || pixel[1] != 255 || pixel[2] != 255) {
		ulong idx = pixel[0] + (pixel[1] * 256) + (pixel[2] * 256 * 256);
		printf ("idx: %lu\n", idx);
		
		LDObject* obj = g_CurrentFile->object (idx);
		
		g_ForgeWindow->paSelection.clear ();
		g_ForgeWindow->paSelection.push_back (obj);
	}
	
	g_bPicking = false;
	glEnable (GL_DITHER);
	
	setBackground ();
	compileObjects ();
	paintGL ();
	
	g_ForgeWindow->updateSelection ();
}