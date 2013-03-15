#include <QtGui>
#include <QGLWidget>
#include "common.h"
#include "io.h"
#include "draw.h"
#include "bbox.h"

renderer::renderer (QWidget* parent) {
	parent = parent; // shhh, GCC
	fRotX = fRotY = fRotZ = 0.0;
	fZoom = 1.0;
}

void renderer::initializeGL () {
	glLoadIdentity();
	glMatrixMode (GL_MODELVIEW);
	glClearColor (0.8f, 0.8f, 0.85f, 1.0f);
	glEnable (GL_DEPTH_TEST);
	glShadeModel (GL_SMOOTH);
	glEnable (GL_MULTISAMPLE);
	
	CompileObjects ();
}

void renderer::hardRefresh () {
	CompileObjects ();
	paintGL ();
}

void renderer::resizeGL (int w, int h) {
	glViewport (0, 0, w, h);
}

void renderer::paintGL () {
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glPushMatrix ();
		glTranslatef (
			(g_BBox.v0.x + g_BBox.v1.x) / -2.0,
			(g_BBox.v0.y + g_BBox.v1.y) / -2.0,
			(g_BBox.v0.z + g_BBox.v1.z) / -2.0
		);
		
		// glTranslatef (0.0f, 0.0f, -5.0f);
		glTranslatef (0.0f, 0.0f, fZoom);
		
		// glScalef (0.75f, 1.15f, 0.0f);
		glRotatef (fRotX, 1.0f, 0.0f, 0.0f);
		glRotatef (fRotY, 0.0f, 1.0f, 0.0f);
		glRotatef (fRotZ, 0.0f, 0.0f, 1.0f);
		
		glMatrixMode (GL_MODELVIEW);
		
		glCallList (objlist);
		glColor3f (0.0, 0.5, 1.0);
	glPopMatrix ();
}

void renderer::CompileObjects () {
	printf ("compile all objects\n");
	
	objlist = glGenLists (1);
	glNewList (objlist, GL_COMPILE);
	
	if (!g_CurrentFile) {
		printf ("renderer: no files loaded, cannot compile anything\n");
		return;
	}
	
	for (ulong i = 0; i < g_CurrentFile->objects.size(); i++)
		CompileOneObject (g_CurrentFile->objects[i]);
	
	glEndList ();
}

#define GL_VERTEX(V) \
	glVertex3d (V.x, V.y, V.z);

void renderer::CompileOneObject (LDObject* obj) {
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
			glColor3f (0.5f, 0.5f, 0.5f); // Draw all polygons gray for now
			glBegin (GL_TRIANGLES);
			for (short i = 0; i < 3; ++i)
				GL_VERTEX (tri->vaCoords[i])
			glEnd ();
		}
		break;
		
	case OBJ_Quad:
		{
			LDQuad* quad = static_cast<LDQuad*> (obj);
			glColor3f (0.5f, 0.5f, 0.5f);
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

void renderer::ClampAngle (double& fAngle) {
	while (fAngle < 0)
		fAngle += 360.0;
	while (fAngle > 360.0)
		fAngle -= 360.0;
}

void renderer::mouseMoveEvent (QMouseEvent *event) {
	int dx = event->x () - lastPos.x ();
	int dy = event->y () - lastPos.y ();
	
	if (event->buttons () & Qt::LeftButton) {
		fRotX = fRotX + (dy);
		fRotY = fRotY + (dx);
		ClampAngle (fRotX);
		ClampAngle (fRotY);
	}
	
	if (event->buttons () & Qt::RightButton) {
		fRotX = fRotX + (dy);
		fRotZ = fRotZ + (dx);
		ClampAngle (fRotX);
		ClampAngle (fRotZ);
	}
	
	if (event->buttons () & Qt::MidButton) {
		fZoom += (dy / 100.0);
		fZoom = clamp (fZoom, 0.1, 10.0);
	}
	
	printf ("%.3f %.3f %.3f %.3f\n",
		fRotX, fRotY, fRotZ, fZoom);
	lastPos = event->pos();
	updateGL ();
}