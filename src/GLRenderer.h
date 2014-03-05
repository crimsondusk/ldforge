/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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

#pragma once
#include <QGLWidget>
#include "Main.h"
#include "LDObject.h"
#include "Document.h"

class MessageManager;
class QDialogButtonBox;
class RadioGroup;
class QDoubleSpinBox;
class QSpinBox;
class QLineEdit;
class QTimer;

enum EditMode
{
	ESelectMode,
	EDrawMode,
	ECircleMode,
};

// Meta for overlays
struct LDGLOverlay
{
	Vertex			v0,
					v1;
	int				ox,
					oy;
	double			lw,
					lh;
	QString			fname;
	QImage*			img;
};

struct LDFixedCameraInfo
{
	const char		glrotate[3];
	const Axis		axisX,
					axisY;
	const bool		negX,
					negY;
};

// =============================================================================
// Document-specific data
//
struct LDGLData
{
	double			rotX,
					rotY,
					rotZ,
					panX[7],
					panY[7],
					zoom[7];
	double			depthValues[6];
	LDGLOverlay		overlays[6];
	bool			init;

	LDGLData()
	{
		for (int i = 0; i < 6; ++i)
		{
			overlays[i].img = null;
			depthValues[i] = 0.0f;
		}

		init = false;
	}
};

// =============================================================================
// The main renderer object, draws the brick on the screen, manages the camera
// and selection picking. The instance of GLRenderer is accessible as
// g_win->R()
//
class GLRenderer : public QGLWidget
{
	public:
		enum EFixedCamera
		{
			ETopCamera,
			EFrontCamera,
			ELeftCamera,
			EBottomCamera,
			EBackCamera,
			ERightCamera,
			EFreeCamera
		};

		enum ListType
		{
			NormalList,
			PickList,
			BFCFrontList,
			BFCBackList
		};

		// CameraIcon::img is a heap-allocated QPixmap because otherwise it gets
		// initialized before program gets to main() and constructs a QApplication
		// and Qt doesn't like that.
		struct CameraIcon
		{
			QPixmap*			img;
			QRect				srcRect,
								destRect,
								selRect;
			EFixedCamera	cam;
		};

		Q_OBJECT
		PROPERTY (public,	bool,				isDrawOnly,	setDrawOnly,	STOCK_WRITE)
		PROPERTY (public,	MessageManager*,	messageLog, setMessageLog,	STOCK_WRITE)
		PROPERTY (private,	bool,				isPicking,	setPicking,		STOCK_WRITE)
		PROPERTY (public,	LDDocument*,		document,	setDocument,	CUSTOM_WRITE)
		PROPERTY (public,	EditMode,			editMode,	setEditMode,	CUSTOM_WRITE)

	public:
		GLRenderer (QWidget* parent = null);
		~GLRenderer();

		inline EFixedCamera camera() const
		{
			return m_camera;
		}

		void           clearOverlay();
		void           compileObject (LDObject* obj);
		void           compileAllObjects();
		void           drawGLScene();
		void           endDraw (bool accept);
		Axis           getCameraAxis (bool y, EFixedCamera camid = (EFixedCamera) - 1);
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
		void           setCamera (const EFixedCamera cam);
		void           setDepthValue (double depth);
		bool           setupOverlay (EFixedCamera cam, QString file, int x, int y, int w, int h);
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
		CameraIcon					m_cameraIcons[7];
		QTimer*						m_toolTipTimer;
		Qt::MouseButtons			m_lastButtons;
		Qt::KeyboardModifiers		m_keymods;
		Vertex						m_hoverpos;
		double						m_virtWidth,
									m_virtHeight;
		bool						m_darkbg,
									m_rangepick,
									m_addpick,
									m_drawToolTip,
									m_screencap,
									m_panning;
		QPoint						m_pos,
									m_globalpos,
									m_rangeStart;
		QPen						m_thickBorderPen,
									m_thinBorderPen;
		EFixedCamera				m_camera,
									m_toolTipCamera;
		GLuint						m_axeslist;
		int							m_width,
									m_height,
									m_totalmove;
		QList<Vertex>				m_drawedVerts;
		bool						m_rectdraw;
		Vertex						m_rectverts[4];
		QColor						m_bgcolor;
		QList<Vertex>				m_knownVerts;

		void           addDrawnVertex (Vertex m_hoverpos);
		LDOverlay*     findOverlayObject (EFixedCamera cam);
		void           updateRectVerts();
		void           getRelativeAxes (Axis& relX, Axis& relY) const;
		Matrix         getCircleDrawMatrix (double scale);
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
		void           compileVertex (const Vertex& vrt);

		// Convert a 2D point to a 3D point
		Vertex         coordconv2_3 (const QPoint& pos2d, bool snap) const;

		// Convert a 3D point to a 2D point
		QPoint         coordconv3_2 (const Vertex& pos3d) const;

		// Perform object selection
		void           pick (int mouseX, int mouseY);

		// Set the color to an object list
		void           setObjectColor (LDObject* obj, const ListType list);

		LDGLData& currentDocumentData() const
		{
			return *document()->getGLData();
		}

		// Get a rotation value
		inline double& rot (Axis ax)
		{
			return
				(ax == X) ? currentDocumentData().rotX :
				(ax == Y) ? currentDocumentData().rotY :
				            currentDocumentData().rotZ;
		}

		// Get a panning value
		inline double& pan (Axis ax)
		{
			return (ax == X) ? currentDocumentData().panX[camera()] :
				currentDocumentData().panY[camera()];
		}

		// Same except const (can be used in const methods)
		inline const double& pan (Axis ax) const
		{
			return (ax == X) ? currentDocumentData().panX[camera()] :
				currentDocumentData().panY[camera()];
		}

		// Get the zoom value
		inline double& zoom()
		{
			return currentDocumentData().zoom[camera()];
		}

	private slots:
		void           slot_toolTipTimer();
};

// Alias for short namespaces
typedef GLRenderer GL;

static const GLRenderer::ListType g_glListTypes[] =
{
	GL::NormalList,
	GL::PickList,
	GL::BFCFrontList,
	GL::BFCBackList,
};

extern const GL::EFixedCamera g_Cameras[7];
extern const char* g_CameraNames[7];
