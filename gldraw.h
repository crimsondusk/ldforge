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

#ifndef __GLDRAW_H__
#define __GLDRAW_H__

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
	void pick (uint uMouseX, uint uMouseY, bool bAdd);
	QColor getMainColor ();
	void recompileObject (LDObject* obj);
	void refresh ();
	void updateSelFlash();
	
	double fRotX, fRotY, fRotZ;
	QPoint lastPos;
	double fZoom;
	bool bPicking;

protected:
	void initializeGL ();
	void resizeGL (int w, int h);
	void paintGL ();
	
	void mousePressEvent (QMouseEvent* event);
	void mouseMoveEvent (QMouseEvent* event);
	void mouseReleaseEvent (QMouseEvent* event);
	void keyPressEvent (QKeyEvent* qEvent);
	void keyReleaseEvent (QKeyEvent* qEvent);

private:
	std::vector<GLuint> uaObjLists;
	void compileOneObject (LDObject* obj);
	template<class T> void compileSubObject (LDObject* obj, const GLenum eGLType,
		const short dVerts);
	void compileVertex (vertex& vrt);
	void clampAngle (double& fAngle);
	void setObjectColor (LDObject* obj);
	
	QTimer* qPulseTimer;
	
	Qt::MouseButtons qMouseButtons;
	Qt::KeyboardModifiers qKeyMods;
	ulong ulTotalMouseMove;
	
private slots:
	void slot_timerUpdate ();
};

#endif // __GLDRAW_H__