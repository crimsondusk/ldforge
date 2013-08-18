/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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
#include "common.h"
#include "ldtypes.h"

class MessageManager;
class QDialogButtonBox;
class RadioBox;
class QDoubleSpinBox;
class QSpinBox;
class QLineEdit;
class LDFile;
class QTimer;

enum EditMode {
	Select,
	Draw
};

// Meta for overlays
struct overlayMeta {
	vertex v0, v1;
	ushort ox, oy;
	double lw, lh;
	str fname;
	QImage* img;
};

// =============================================================================
// GLRenderer
// 
// The main renderer object, draws the brick on the screen, manages the camera
// and selection picking. The instance of GLRenderer is accessible as
// g_win->R()
// =============================================================================
class GLRenderer : public QGLWidget {
	Q_OBJECT
	
	PROPERTY (bool, drawOnly, setDrawOnly)
	PROPERTY (double, zoom, setZoom)
	PROPERTY (MessageManager*, msglog, setMessageLog)
	READ_PROPERTY (bool, picking, setPicking)
	DECLARE_PROPERTY (LDFile*, file, setFile)
	DECLARE_PROPERTY (EditMode, editMode, setEditMode)
	
public:
	enum Camera { Top, Front, Left, Bottom, Back, Right, Free };
	enum ListType { NormalList, PickList, BFCFrontList, BFCBackList };
	
	GLRenderer (QWidget* parent = null);
	~GLRenderer();
	
	Camera         camera() const { return m_camera; }
	Axis           cameraAxis (bool y, Camera camid = (Camera) -1);
	const char*    cameraName() const;
	void           clearOverlay();
	void           compileObject (LDObject* obj);
	void           compileAllObjects();
	double         depthValue() const;
	void           drawGLScene();
	void           endDraw (bool accept);
	QColor         getMainColor();
	overlayMeta&   getOverlay (int newcam);
	void           hardRefresh();
	void           initGLData();
	void           overlaysFromObjects();
	void           refresh();
	void           resetAngles();
	uchar*         screencap (ushort& w, ushort& h);
	void           setBackground();
	void           setCamera (const Camera cam);
	void           setDepthValue (double depth);
	bool           setupOverlay (GLRenderer::Camera cam, str file, int x, int y, int w, int h);
	void           updateOverlayObjects();
	void           zoomNotch (bool inward);
	void           zoomToFit();
	
	static void    deleteLists (LDObject* obj);

protected:
	void           contextMenuEvent (QContextMenuEvent* ev);
	void           initializeGL();
	void           keyPressEvent (QKeyEvent* ev);
	void           keyReleaseEvent (QKeyEvent* ev);
	void           leaveEvent (QEvent* ev);
	void           mouseDoubleClickEvent (QMouseEvent* ev);
	void           mousePressEvent (QMouseEvent* ev);
	void           mouseMoveEvent (QMouseEvent* ev);
	void           mouseReleaseEvent (QMouseEvent* ev);
	void           paintEvent (QPaintEvent* ev);
	void           resizeGL (int w, int h);
	void           wheelEvent (QWheelEvent* ev);

private:
	// CameraIcon::img is a heap-allocated QPixmap because otherwise it gets
	// initialized before program gets to main() and constructs a QApplication
	// and Qt doesn't like that.
	struct CameraIcon {
		QPixmap* img;
		QRect srcRect, destRect, selRect;
		Camera cam;
	} m_cameraIcons[7];
	
	QTimer* m_toolTipTimer;
	Qt::MouseButtons m_lastButtons;
	Qt::KeyboardModifiers m_keymods;
	ulong m_totalmove;
	vertex m_hoverpos;
	double m_virtWidth, m_virtHeight, m_rotX, m_rotY, m_rotZ, m_panX, m_panY;
	bool m_darkbg, m_rangepick, m_addpick, m_drawToolTip, m_screencap;
	QPoint m_pos, m_globalpos, m_rangeStart;
	QPen m_thickBorderPen, m_thinBorderPen;
	Camera m_camera, m_toolTipCamera;
	uint m_axeslist;
	ushort m_width, m_height;
	List<vertex> m_drawedVerts;
	bool m_rectdraw;
	vertex m_rectverts[4];
	QColor m_bgcolor;
	double m_depthValues[6];
	overlayMeta m_overlays[6];
	List<vertex> m_knownVerts;
	bool m_panning;
	
	void           addDrawnVertex (vertex m_hoverpos);
	void           calcCameraIcons();                                      // Compute geometry for camera icons
	void           clampAngle (double& angle) const;                       // Clamps an angle to [0, 360]
	void           compileList (LDObject* obj, const ListType list);       // Compile one of the lists of an object
	void           compileSubObject (LDObject* obj, const GLenum gltype);  // Sub-routine for object compiling
	void           compileVertex (const vertex& vrt);                      // Compile a single vertex to a list
	vertex         coordconv2_3 (const QPoint& pos2d, bool snap) const;    // Convert a 2D point to a 3D point
	QPoint         coordconv3_2 (const vertex& pos3d) const;               // Convert a 3D point to a 2D point
	LDOverlayObject* findOverlayObject (Camera cam);
	void           updateRectVerts();
	void           pick (uint mouseX, uint mouseY);                        // Perform object selection
	void           setObjectColor (LDObject* obj, const ListType list);    // Set the color to an object list
	QColor         getTextPen() const;                                     // Determine which color to draw text with
	
private slots:
	void           slot_toolTipTimer();
};

// Alias for short namespaces
typedef GLRenderer GL;

static const GLRenderer::ListType g_glListTypes[] = {
	GL::NormalList,
	GL::PickList,
	GL::BFCFrontList,
	GL::BFCBackList,
};

extern const GL::Camera g_Cameras[7];
extern const char* g_CameraNames[7];

#endif // GLDRAW_H