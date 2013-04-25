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

#ifndef GLDRAW_H
#define GLDRAW_H

#include <QGLWidget>
#include <qtimer.h>
#include "common.h"
#include "ldtypes.h"

// =============================================================================
// GLRenderer
// 
// The main renderer object, draws the brick on the screen, manages the camera
// and selection picking. The instance of GLRenderer is accessible as
// g_ForgeWindow->R
// =============================================================================
class GLRenderer : public QGLWidget {
	Q_OBJECT
	
public:
	GLRenderer (QWidget* parent = null);
	void hardRefresh ();
	void compileObjects ();
	void setBackground ();
	void pick (uint mouseX, uint mouseY);
	QColor getMainColor ();
	void recompileObject (LDObject* obj);
	void refresh ();
	void updateSelFlash ();
	void resetAngles ();
	
	double rotX, rotY, rotZ;
	double panX, panY;
	QPoint pos;
	double zoom;
	bool picking;
	bool rangepick;
	bool addpick;
	short width, height;
	QPoint rangeStart;

protected:
	void initializeGL ();
	void resizeGL (int w, int h);
	void paintGL ();
	
	void mousePressEvent (QMouseEvent* ev);
	void mouseMoveEvent (QMouseEvent* ev);
	void mouseReleaseEvent (QMouseEvent* ev);
	void keyPressEvent (QKeyEvent* ev);
	void keyReleaseEvent (QKeyEvent* ev);
	void wheelEvent (QWheelEvent* ev);

private:
	std::vector<GLuint> objLists;
	QTimer* pulseTimer;
	Qt::MouseButtons lastButtons;
	Qt::KeyboardModifiers keymods;
	ulong totalmove;
	bool darkbg;
	
	void compileOneObject (LDObject* obj);
	template<class T> void compileSubObject (LDObject* obj, const GLenum eGLType,
		const short dVerts);
	void compileVertex (vertex& vrt);
	void clampAngle (double& fAngle);
	void setObjectColor (LDObject* obj);
	
private slots:
	void slot_timerUpdate ();
};

#endif // GLDRAW_H