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

#ifndef LDFORGE_GLDRAW_H
#define LDFORGE_GLDRAW_H

#include <QGLWidget>
#include "main.h"
#include "ldtypes.h"

class MessageManager;
class QDialogButtonBox;
class RadioGroup;
class QDoubleSpinBox;
class QSpinBox;
class QLineEdit;
class LDFile;
class QTimer;

enum EditMode
{	Select,
	Draw,
	CircleMode,
};

// Meta for overlays
struct LDGLOverlay
{	vertex v0, v1;
	int ox, oy;
	double lw, lh;
	str fname;
	QImage* img;
};

struct LDFixedCameraInfo
{	const char glrotate[3];
	const Axis axisX, axisY;
	const bool negX, negY;
};

// =============================================================================
// GLRenderer
//
// The main renderer object, draws the brick on the screen, manages the camera
// and selection picking. The instance of GLRenderer is accessible as
// g_win->R()
// =============================================================================
class GLRenderer : public QGLWidget
{	Q_OBJECT

	PROPERTY (public,		bool,					DrawOnly,	BOOL_OPS,	STOCK_WRITE)
	PROPERTY (public,		MessageManager*,	MessageLog, NO_OPS,		STOCK_WRITE)
	PROPERTY (private,	bool,					Picking,		BOOL_OPS,	STOCK_WRITE)
	PROPERTY (public,		LDFile*,				File,			NO_OPS,		CUSTOM_WRITE)
	PROPERTY (public,		EditMode,			EditMode,	NO_OPS,		CUSTOM_WRITE)

	public:
		enum Camera { Top, Front, Left, Bottom, Back, Right, Free };
		enum ListType { NormalList, PickList, BFCFrontList, BFCBackList };

		GLRenderer (QWidget* parent = null);
		~GLRenderer();

		inline Camera camera() const
		{	return m_camera;
		}

		void           clearOverlay();
		void           compileObject (LDObject* obj);
		void           compileAllObjects();
		void           drawGLScene();
		void           endDraw (bool accept);
		Axis           getCameraAxis (bool y, Camera camid = (Camera) - 1);
		const char*    getCameraName() const;
		double         getDepthValue() const;
		QColor         getMainColor();
		LDGLOverlay&   getOverlay (int newcam);
		uchar*         getScreencap (int& w, int& h);
		void           hardRefresh();
		void           initGLData();
		void           initOverlaysFromObjects();
		void           refresh();
		void           resetAngles();
		void           resetAllAngles();
		void           setBackground();
		void           setCamera (const Camera cam);
		void           setDepthValue (double depth);
		bool           setupOverlay (GLRenderer::Camera cam, str file, int x, int y, int w, int h);
		void           updateOverlayObjects();
		void           zoomNotch (bool inward);
		void           zoomToFit();
		void           zoomAllToFit();

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
		struct CameraIcon
		{	QPixmap* img;
			QRect srcRect, destRect, selRect;
			Camera cam;
		};

		CameraIcon					m_cameraIcons[7];
		QTimer*						m_toolTipTimer;
		Qt::MouseButtons			m_lastButtons;
		Qt::KeyboardModifiers	m_keymods;
		vertex						m_hoverpos;
		double						m_virtWidth,
										m_virtHeight,
										m_rotX[7],
										m_rotY[7],
										m_rotZ[7],
										m_panX[7],
										m_panY[7],
										m_zoom[7];
		bool							m_darkbg,
										m_rangepick,
										m_addpick,
										m_drawToolTip,
										m_screencap,
										m_panning;
		QPoint						m_pos,
										m_globalpos,
										m_rangeStart;
		QPen							m_thickBorderPen,
										m_thinBorderPen;
		Camera						m_camera,
										m_toolTipCamera;
		GLuint						m_axeslist;
		int							m_width,
										m_height,
										m_totalmove;
		QList<vertex>				m_drawedVerts;
		bool							m_rectdraw;
		vertex						m_rectverts[4];
		QColor						m_bgcolor;
		double						m_depthValues[6];
		LDGLOverlay					m_overlays[6];
		QList<vertex>				m_knownVerts;

		void           addDrawnVertex (vertex m_hoverpos);
		LDOverlay*     findOverlayObject (Camera cam);
		void           updateRectVerts();
		void           getRelativeAxes (Axis& relX, Axis& relY) const;
		matrix         getCircleDrawMatrix (double scale);
		void           drawBlip (QPainter& paint, QPoint pos) const;

		// Compute geometry for camera icons
		void           calcCameraIcons();

		// How large is the circle we're drawing right now?
		double         getCircleDrawDist (int pos) const;

		// Clamps an angle to [0, 360]
		void           clampAngle (double& angle) const;

		// Compile one of the lists of an object
		void           compileList (LDObject* obj, const ListType list);

		// Sub-routine for object compiling
		void           compileSubObject (LDObject* obj, const GLenum gltype);

		// Compile a single vertex to a list
		void           compileVertex (const vertex& vrt);

		// Convert a 2D point to a 3D point
		vertex         coordconv2_3 (const QPoint& pos2d, bool snap) const;

		// Convert a 3D point to a 2D point
		QPoint         coordconv3_2 (const vertex& pos3d) const;

		// Determine which color to draw text with
		QColor         getTextPen() const;

		// Perform object selection
		void           pick (int mouseX, int mouseY);

		// Set the color to an object list
		void           setObjectColor (LDObject* obj, const ListType list);

		// Get a rotation value
		inline double& rot (Axis ax)
		{	return
				(ax == X) ? m_rotX[camera()] :
				(ax == Y) ? m_rotY[camera()] :
				            m_rotZ[camera()];
		}

		// Get a panning value
		inline double& pan (Axis ax)
		{	return (ax == X) ? m_panX[camera()] : m_panY[camera()];
		}

		// Same except const (can be used in const methods)
		inline const double& pan (Axis ax) const
		{	return (ax == X) ? m_panX[camera()] : m_panY[camera()];
		}

		// Get the zoom value
		inline double& zoom()
		{	return m_zoom[camera()];
		}

	private slots:
		void           slot_toolTipTimer();
};

// Alias for short namespaces
typedef GLRenderer GL;

static const GLRenderer::ListType g_glListTypes[] =
{	GL::NormalList,
	GL::PickList,
	GL::BFCFrontList,
	GL::BFCBackList,
};

extern const GL::Camera g_Cameras[7];
extern const char* g_CameraNames[7];

#endif // LDFORGE_GLDRAW_H
