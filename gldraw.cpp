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

#define GL_VERTEX(V) glVertex3d (V.x, V.y, V.z);

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
renderer::renderer (QWidget* parent) {
	parent = parent; // shhh, GCC
	fRotX = fRotY = fRotZ = 0.0;
	fZoom = 1.0;
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
	glLineWidth (4 * 2.0f);
	
	compileObjects ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void renderer::setBackground () {
	QColor col (gl_bgcolor.value.chars());
	
	if (col.isValid ()) {
		glClearColor (
			((double)col.red()) / 255.0f,
			((double)col.green()) / 255.0f,
			((double)col.blue()) / 255.0f,
			1.0f);
	} else {
		glClearColor (0.8f, 0.8f, 0.85f, 1.0f);
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void renderer::hardRefresh () {
	compileObjects ();
	paintGL ();
	swapBuffers ();
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
	printf ("painting..\n");
	glMatrixMode (GL_MODELVIEW);
	
	glPushMatrix ();
		glLoadIdentity ();
		
	/*
		glTranslatef (
			(g_BBox.v0.x + g_BBox.v1.x) / -2.0,
			(g_BBox.v0.y + g_BBox.v1.y) / -2.0,
			(g_BBox.v0.z + g_BBox.v1.z) / -2.0
		);
	*/
		
		glTranslatef (0.0f, 0.0f, -5.0f);
		glTranslatef (0.0f, 0.0f, -fZoom);
		
		// glScalef (0.75f, 1.15f, 0.0f);
		glRotatef (fRotX, 1.0f, 0.0f, 0.0f);
		glRotatef (fRotY, 0.0f, 1.0f, 0.0f);
	//	glRotatef (fRotZ, 0.0f, 0.0f, 1.0f);
		
		glCallList (uObjList);
	glPopMatrix ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void renderer::compileObjects () {
	printf ("compile all objects\n");
	
	uObjList = glGenLists (1);
	glNewList (uObjList, GL_COMPILE);
	
	if (!g_CurrentFile) {
		printf ("renderer: no files loaded, cannot compile anything\n");
		return;
	}
	
	for (ulong i = 0; i < g_CurrentFile->objects.size(); i++)
		compileOneObject (g_CurrentFile->objects[i]);
	
	glEndList ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void renderer::compileOneObject (LDObject* obj) {
	if (!obj)
		return;
	
	switch (obj->getType ()) {
	case OBJ_CondLine: // For now, treat condlines like normal lines.
	case OBJ_Line:
		{
			glColor3f (0.0f, 0.0f, 0.0f); // Draw all lines black for now
			// draw lines
			LDLine* line = static_cast<LDLine*> (obj);
			glBegin (GL_LINES);
			for (short i = 0; i < 2; ++i)
				GL_VERTEX (line->vaCoords[i])
			glEnd ();
		}
		break;
	
	case OBJ_Triangle:
		{
			LDTriangle* tri = static_cast<LDTriangle*> (obj);
			glColor3f (0.5f, 0.0f, 0.0f); // Draw all polygons red for now
			glBegin (GL_TRIANGLES);
			for (short i = 0; i < 3; ++i)
				GL_VERTEX (tri->vaCoords[i])
			glEnd ();
		}
		break;
		
	case OBJ_Quad:
		{
			LDQuad* quad = static_cast<LDQuad*> (obj);
			glColor3f (0.5f, 0.0f, 0.0f);
			glBegin (GL_QUADS);
			for (short i = 0; i < 4; ++i)
				GL_VERTEX (quad->vaCoords[i])
			glEnd ();
		}
		break;
	
	default:
		break;
	}
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
	
	printf ("%.3f %.3f %.3f %.3f\n",
		fRotX, fRotY, fRotZ, fZoom);
	lastPos = event->pos();
	updateGL ();
}