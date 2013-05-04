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

typedef struct {
	vertex pos3d;
	QPoint pos2d;
} GLPlaneDrawVertex;

// =============================================================================
// GLRenderer
// 
// The main renderer object, draws the brick on the screen, manages the camera
// and selection picking. The instance of GLRenderer is accessible as
// g_win->R ()
// =============================================================================
class GLRenderer : public QGLWidget {
	Q_OBJECT
	
public:
	enum Camera {
		Top,
		Front,
		Left,
		Bottom,
		Back,
		Right,
		Free
	};
	
	GLRenderer (QWidget* parent = null);
	~GLRenderer ();
	
	void hardRefresh ();
	void compileObjects ();
	void setBackground ();
	void pick (uint mouseX, uint mouseY);
	QColor getMainColor ();
	void recompileObject (LDObject* obj);
	void refresh ();
	void updateSelFlash ();
	void resetAngles ();
	uchar* screencap (ushort& w, ushort& h);
	void beginPlaneDraw ();
	GLRenderer::Camera camera () { return m_camera; }
	void setCamera (const GLRenderer::Camera cam);
	bool picking () { return m_picking; }

protected:
	void initializeGL ();
	void resizeGL (int w, int h);
	
	void mousePressEvent (QMouseEvent* ev);
	void mouseMoveEvent (QMouseEvent* ev);
	void mouseReleaseEvent (QMouseEvent* ev);
	void keyPressEvent (QKeyEvent* ev);
	void keyReleaseEvent (QKeyEvent* ev);
	void wheelEvent (QWheelEvent* ev);
	void paintEvent (QPaintEvent* ev);
	void leaveEvent (QEvent* ev);
	void contextMenuEvent (QContextMenuEvent* ev);

private:
	QTimer* m_pulseTimer, *m_toolTipTimer;
	Qt::MouseButtons m_lastButtons;
	Qt::KeyboardModifiers m_keymods;
	ulong m_totalmove;
	vertex m_hoverpos;
	double m_virtWidth, m_virtHeight, m_rotX, m_rotY, m_rotZ, m_panX, m_panY, m_zoom;
	bool m_darkbg, m_picking, m_rangepick, m_addpick, m_drawToolTip, m_screencap, m_planeDraw;
	QPoint m_pos, m_rangeStart;
	QPen m_thinBorderPen, m_thickBorderPen;
	Camera m_camera, m_toolTipCamera;
	uint m_axeslist;
	ushort m_width, m_height;
	std::vector<GLPlaneDrawVertex> m_planeDrawVerts;
	
	void compileOneObject (LDObject* obj);
	void compileSubObject (LDObject* obj, const GLenum gltype);
	void compileVertex (const vertex& vrt);
	void clampAngle (double& angle);
	void setObjectColor (LDObject* obj);
	void drawGLScene () const;
	void calcCameraIconRects ();
	
	vertex coord_2to3 (const QPoint& pos2d, const bool snap) const;
	QPoint coord_3to2 (const vertex& pos3d) const;
	
private slots:
	void slot_timerUpdate ();
	void slot_toolTipTimer ();
};

#endif // GLDRAW_H