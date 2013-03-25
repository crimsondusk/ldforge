/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri `arezey` Piippo
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
#include "file.h"
#include "gldraw.h"
#include "bbox.h"
#include "colors.h"

static double g_faObjectOffset[3];
static double g_StoredBBoxSize;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
renderer::renderer (QWidget* parent) {
	parent = parent; // shhh, GCC
	fRotX = fRotY = fRotZ = 0.0f;
	fZoom = 1.0f;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
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

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
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

// -----------------------------------------------------------------------------
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

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static vector<short> g_daWarnedColors;
void renderer::setObjectColor (LDObject* obj, bool bBackSide) {
	if (obj->dColor == -1)
		return;
	
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
		for (long i = 0; i < (long)g_daWarnedColors.size(); ++i)
			if (g_daWarnedColors[i] == obj->dColor)
				return;
		
		printf ("%s: Unknown color %d!\n", __func__, obj->dColor);
		g_daWarnedColors.push_back (obj->dColor);
		return;
	}
	
	QColor qCol (col->zColor.chars());
	
	if (qCol.isValid ())
		glColor4f (
			((double)qCol.red()) / 255.0f,
			((double)qCol.green()) / 255.0f,
			((double)qCol.blue()) / 255.0f,
			col->fAlpha);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void renderer::hardRefresh () {
	compileObjects ();
	paintGL ();
	swapBuffers ();
	
	glLineWidth (gl_linethickness);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void renderer::resizeGL (int w, int h) {
	glViewport (0, 0, w, h);
	glLoadIdentity ();
	glMatrixMode (GL_PROJECTION);
	gluPerspective (45.0f, (double)w / (double)h, 0.1f, 100.0f);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
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
		
		if (gl_colorbfc) {
			glEnable (GL_CULL_FACE);
			
			glCullFace (GL_FRONT);
			glCallList (uObjList);
			
			glCullFace (GL_BACK);
			glCallList (uObjListBack);
			
			glDisable (GL_CULL_FACE);
		} else
			glCallList (uObjList);
	glPopMatrix ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void renderer::compileObjects () {
	g_faObjectOffset[0] = -(g_BBox.v0.x + g_BBox.v1.x) / 2;
	g_faObjectOffset[1] = -(g_BBox.v0.y + g_BBox.v1.y) / 2;
	g_faObjectOffset[2] = -(g_BBox.v0.z + g_BBox.v1.z) / 2;
	g_StoredBBoxSize = g_BBox.calcSize ();
	printf ("bbox size is %f\n", g_StoredBBoxSize);
	
	if (!g_CurrentFile) {
		printf ("renderer: no files loaded, cannot compile anything\n");
		return;
	}
	
	GLuint* upaLists[2] = {
		&uObjList,
		&uObjListBack,
	};
	
	for (uchar j = 0; j < 2; ++j) {
		if (j && !gl_colorbfc)
			continue;
		
		*upaLists[j] = glGenLists (1);
		glNewList (*upaLists[j], GL_COMPILE);
		
		for (ulong i = 0; i < g_CurrentFile->objects.size(); i++)
			compileOneObject (g_CurrentFile->objects[i], j);
		
		glEndList ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
template<class T>void renderer::compileSubObject (LDObject* obj,
	const bool bBackSide, const GLenum eGLType, const short dVerts)
{
	setObjectColor (obj, bBackSide);
	T* newobj = static_cast<T*> (obj);
	glBegin (eGLType);
	
	for (short i = 0; i < dVerts; ++i)
		compileVertex (newobj->vaCoords[i]);
	
	glEnd ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void renderer::compileOneObject (LDObject* obj, bool bBackSide) {
	if (!obj)
		return;
	
	switch (obj->getType ()) {
	case OBJ_Line:
		compileSubObject<LDLine> (obj, bBackSide, GL_LINES, 2);
		break;
	
	case OBJ_CondLine:
		glLineStipple (1, 0x6666);
		glEnable (GL_LINE_STIPPLE);
		
		compileSubObject<LDCondLine> (obj, bBackSide, GL_LINES, 2);
		
		glDisable (GL_LINE_STIPPLE);
		break;
	
	case OBJ_Triangle:
		compileSubObject<LDTriangle> (obj, bBackSide, GL_TRIANGLES, 3);
		break;
	
	case OBJ_Quad:
		compileSubObject<LDQuad> (obj, bBackSide, GL_QUADS, 4);
		break;
	
	case OBJ_Subfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			
			vector<LDObject*> objs = ref->inlineContents (true, true);
			
			for (ulong i = 0; i < (ulong)objs.size(); ++i) {
				compileOneObject (objs[i], bBackSide);
				delete objs[i];
			}
		}
		break;
	
	default:
		break;
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void renderer::compileVertex (vertex& vrt) {
	glVertex3d (
		(vrt.x + g_faObjectOffset[0]) / g_StoredBBoxSize,
		-(vrt.y + g_faObjectOffset[1]) / g_StoredBBoxSize,
		-(vrt.z + g_faObjectOffset[2]) / g_StoredBBoxSize);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void renderer::clampAngle (double& fAngle) {
	while (fAngle < 0)
		fAngle += 360.0;
	while (fAngle > 360.0)
		fAngle -= 360.0;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void renderer::mouseMoveEvent (QMouseEvent *event) {
	int dx = event->x () - lastPos.x ();
	int dy = event->y () - lastPos.y ();
	
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