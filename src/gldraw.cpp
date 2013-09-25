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

#include <QGLWidget>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QToolTip>
#include <QTimer>
#include <GL/glu.h>
#include "common.h"
#include "config.h"
#include "file.h"
#include "gldraw.h"
#include "colors.h"
#include "gui.h"
#include "misc.h"
#include "history.h"
#include "dialogs.h"
#include "addObjectDialog.h"
#include "messagelog.h"
#include "gldata.h"
#include "build/moc_gldraw.cpp"

static const struct staticCameraMeta {
	const int8 glrotate[3];
	const Axis axisX, axisY;
	const bool negX, negY;
} g_staticCameras[6] = {
	{{  1,  0, 0 }, X, Z, false, false },
	{{  0,  0, 0 }, X, Y, false,  true },
	{{  0,  1, 0 }, Z, Y,  true,  true },
	{{ -1,  0, 0 }, X, Z, false,  true },
	{{  0,  0, 0 }, X, Y,  true,  true },
	{{  0, -1, 0 }, Z, Y, false,  true },
};

cfg (String, gl_bgcolor, "#CCCCD9");
cfg (String, gl_maincolor, "#707078");
cfg (Float, gl_maincolor_alpha, 1.0);
cfg (Int, gl_linethickness, 2);
cfg (Bool, gl_colorbfc, false);
cfg (Int, gl_camera, GLRenderer::Free);
cfg (Bool, gl_axes, false);
cfg (Bool, gl_wireframe, false);
cfg (Bool, gl_logostuds, false);

// argh
const char* g_CameraNames[7] = {
	QT_TRANSLATE_NOOP ("GLRenderer", "Top"),
	QT_TRANSLATE_NOOP ("GLRenderer", "Front"),
	QT_TRANSLATE_NOOP ("GLRenderer", "Left"),
	QT_TRANSLATE_NOOP ("GLRenderer", "Bottom"),
	QT_TRANSLATE_NOOP ("GLRenderer", "Back"),
	QT_TRANSLATE_NOOP ("GLRenderer", "Right"),
	QT_TRANSLATE_NOOP ("GLRenderer", "Free")
};

const GL::Camera g_Cameras[7] = {
	GL::Top,
	GL::Front,
	GL::Left,
	GL::Bottom,
	GL::Back,
	GL::Right,
	GL::Free
};

const struct GLAxis {
	const QColor col;
	const vertex vert;
} g_GLAxes[3] = {
	{ QColor (255,   0,   0), vertex (10000, 0, 0) },
	{ QColor (80,  192,   0), vertex (0, 10000, 0) },
	{ QColor (0,   160, 192), vertex (0, 0, 10000) },
};

// =============================================================================
// -----------------------------------------------------------------------------
GLRenderer::GLRenderer (QWidget* parent) : QGLWidget (parent) {
	m_picking = m_rangepick = false;
	m_camera = (GL::Camera) gl_camera.value;
	m_drawToolTip = false;
	m_editMode = Select;
	m_rectdraw = false;
	m_panning = false;
	setFile (null);
	setDrawOnly (false);
	resetAngles();
	setMessageLog (null);
	
	m_toolTipTimer = new QTimer (this);
	m_toolTipTimer->setSingleShot (true);
	connect (m_toolTipTimer, SIGNAL (timeout()), this, SLOT (slot_toolTipTimer()));
	
	m_thickBorderPen = QPen (QColor (0, 0, 0, 208), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	m_thinBorderPen = m_thickBorderPen;
	m_thinBorderPen.setWidth (1);
	
	// Init camera icons
	for (const GL::Camera cam : g_Cameras) {
		str iconname = fmt ("camera-%1", tr (g_CameraNames[cam]).toLower());
		
		CameraIcon* info = &m_cameraIcons[cam];
		info->img = new QPixmap (getIcon (iconname));
		info->cam = cam;
	}
	
	for (int i = 0; i < 6; ++i) {
		m_overlays[i].img = null;
		m_depthValues[i] = 0.0f;
	}
	
	calcCameraIcons();
}

// =============================================================================
// -----------------------------------------------------------------------------
GLRenderer::~GLRenderer() {
	for (int i = 0; i < 6; ++i)
		delete m_overlays[i].img;
	
	for (CameraIcon& info : m_cameraIcons)
		delete info.img;
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::calcCameraIcons() {
	ushort i = 0;
	
	for (CameraIcon& info : m_cameraIcons) {
		const long x1 = (m_width - (info.cam != Free ? 48 : 16)) + ((i % 3) * 16) - 1,
			y1 = ((i / 3) * 16) + 1;
		
		info.srcRect = QRect (0, 0, 16, 16);
		info.destRect = QRect (x1, y1, 16, 16);
		info.selRect = QRect (info.destRect.x(), info.destRect.y(),
			info.destRect.width() + 1, info.destRect.height() + 1);
		++i;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::initGLData() {
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_POLYGON_OFFSET_FILL);
	glPolygonOffset (1.0f, 1.0f);
	
	glEnable (GL_DEPTH_TEST);
	glShadeModel (GL_SMOOTH);
	glEnable (GL_MULTISAMPLE);
	
	glEnable (GL_LINE_SMOOTH);
	glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::resetAngles() {
	m_rotX = 30.0f;
	m_rotY = 325.f;
	m_panX = m_panY = m_rotZ = 0.0f;
	zoomToFit();
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::initializeGL() {
	setBackground();
	
	glLineWidth (gl_linethickness);
	glLineStipple (1, 0x6666);
	
	setAutoFillBackground (false);
	setMouseTracking (true);
	setFocusPolicy (Qt::WheelFocus);
	
	g_vertexCompiler.compileFile();
}

// =============================================================================
// -----------------------------------------------------------------------------
QColor GLRenderer::getMainColor() {
	QColor col (gl_maincolor);
	
	if (!col.isValid())
		return QColor (0, 0, 0);
	
	col.setAlpha (gl_maincolor_alpha * 255.f);
	return col;
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::setBackground() {
	QColor col (gl_bgcolor);
	
	if (!col.isValid())
		return;
	
	col.setAlpha (255);
	
	m_darkbg = luma (col) < 80;
	m_bgcolor = col;
	qglClearColor (col);
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::refresh() {
	update();
	swapBuffers();
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::hardRefresh() {
	g_vertexCompiler.compileFile();
	refresh();
	
	glLineWidth (gl_linethickness);
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::resizeGL (int w, int h) {
	m_width = w;
	m_height = h;
	
	calcCameraIcons();
	
	glViewport (0, 0, w, h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluPerspective (45.0f, (double) w / (double) h, 1.0f, 10000.0f);
	glMatrixMode (GL_MODELVIEW);
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::drawGLScene() {
	if (file() == null)
		return;
	
	if (gl_wireframe && !picking())
		glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable (GL_DEPTH_TEST);
	
	if (m_camera != Free) {
		glMatrixMode (GL_PROJECTION);
		glPushMatrix();
		
		glLoadIdentity();
		glOrtho (-m_virtWidth, m_virtWidth, -m_virtHeight, m_virtHeight, -100.0f, 100.0f);
		glTranslatef (m_panX, m_panY, 0.0f);
		
		if (m_camera != Front && m_camera != Back) {
			glRotatef (90.0f, g_staticCameras[m_camera].glrotate[0],
				g_staticCameras[m_camera].glrotate[1],
				g_staticCameras[m_camera].glrotate[2]);
		}
		
		// Back camera needs to be handled differently
		if (m_camera == GL::Back) {
			glRotatef (180.0f, 1.0f, 0.0f, 0.0f);
			glRotatef (180.0f, 0.0f, 0.0f, 1.0f);
		}
	} else {
		glMatrixMode (GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		
		glTranslatef (0.0f, 0.0f, -2.0f);
		glTranslatef (m_panX, m_panY, -zoom());
		glRotatef (m_rotX, 1.0f, 0.0f, 0.0f);
		glRotatef (m_rotY, 0.0f, 1.0f, 0.0f);
		glRotatef (m_rotZ, 0.0f, 0.0f, 1.0f);
	}
	
	// Draw the VAOs now
	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_COLOR_ARRAY);
	glDisableClientState (GL_NORMAL_ARRAY);
	
	if (gl_colorbfc) {
		glEnable (GL_CULL_FACE);
		glCullFace (GL_CCW);
	} else
		glDisable (GL_CULL_FACE);
	
	drawVAOs ((m_picking ? PickArray : gl_colorbfc ? BFCArray : MainArray), GL_TRIANGLES);
	drawVAOs ((m_picking ? EdgePickArray : EdgeArray), GL_LINES);
	
	// Draw conditional lines. Note that conditional lines are drawn into
	// EdgePickArray in the picking scene, so when picking, don't do anything
	// here.
	if (!m_picking) {
		glEnable (GL_LINE_STIPPLE);
		drawVAOs (CondEdgeArray, GL_LINES);
		glDisable (GL_LINE_STIPPLE);
	}
	
	glPopMatrix();
	glMatrixMode (GL_MODELVIEW);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::drawVAOs (VAOType arrayType, GLenum type) {
	const VertexCompiler::Array* array = g_vertexCompiler.getMergedBuffer (arrayType);
	glVertexPointer (3, GL_FLOAT, sizeof (VertexCompiler::Vertex), &array->data()[0].x);
	glColorPointer (4, GL_UNSIGNED_BYTE, sizeof (VertexCompiler::Vertex), &array->data()[0].color);
	glDrawArrays (type, 0, array->writtenSize() / sizeof (VertexCompiler::Vertex));
}

// =============================================================================
// This converts a 2D point on the screen to a 3D point in the model. If 'snap'
// is true, the 3D point will snap to the current grid.
// -----------------------------------------------------------------------------
vertex GLRenderer::coordconv2_3 (const QPoint& pos2d, bool snap) const {
	assert (camera() != Free);
	
	vertex pos3d;
	const staticCameraMeta* cam = &g_staticCameras[m_camera];
	const Axis axisX = cam->axisX;
	const Axis axisY = cam->axisY;
	const short negXFac = cam->negX ? -1 : 1,
		negYFac = cam->negY ? -1 : 1;
	
	// Calculate cx and cy - these are the LDraw unit coords the cursor is at.
	double cx = (-m_virtWidth + ((2 * pos2d.x() * m_virtWidth) / m_width) - m_panX);
	double cy = (m_virtHeight - ((2 * pos2d.y() * m_virtHeight) / m_height) - m_panY);
	
	if (snap) {
		cx = Grid::snap (cx, (Grid::Config) axisX);
		cy = Grid::snap (cy, (Grid::Config) axisY);
	}
	
	cx *= negXFac;
	cy *= negYFac;
	
	str tmp;
	// Create the vertex from the coordinates
	pos3d[axisX] = tmp.sprintf ("%.3f", cx).toDouble();
	pos3d[axisY] = tmp.sprintf ("%.3f", cy).toDouble();
	pos3d[3 - axisX - axisY] = depthValue();
	return pos3d;
}

// =============================================================================
// -----------------------------------------------------------------------------
// Inverse operation for the above - convert a 3D position to a 2D screen
// position
// -----------------------------------------------------------------------------
QPoint GLRenderer::coordconv3_2 (const vertex& pos3d) const {
	GLfloat m[16];
	const staticCameraMeta* cam = &g_staticCameras[m_camera];
	const Axis axisX = cam->axisX;
	const Axis axisY = cam->axisY;
	const short negXFac = cam->negX ? -1 : 1,
		negYFac = cam->negY ? -1 : 1;
	
	glGetFloatv (GL_MODELVIEW_MATRIX, m);
	
	const double x = pos3d.x();
	const double y = pos3d.y();
	const double z = pos3d.z();
	
	vertex transformed;
	transformed[X] = (m[0] * x) + (m[1] * y) + (m[2] * z) + m[3];
	transformed[Y] = (m[4] * x) + (m[5] * y) + (m[6] * z) + m[7];
	transformed[Z] = (m[8] * x) + (m[9] * y) + (m[10] * z) + m[11];
	
	double rx = (((transformed[axisX] * negXFac) + m_virtWidth + m_panX) * m_width) / (2 * m_virtWidth);
	double ry = (((transformed[axisY] * negYFac) - m_virtHeight + m_panY) * m_height) / (2 * m_virtHeight);
	
	return QPoint (rx, -ry);
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::paintEvent (QPaintEvent* ev) {
	Q_UNUSED (ev)
	
	makeCurrent();
	m_virtWidth = zoom();
	m_virtHeight = (m_height * m_virtWidth) / m_width;
	
	initGLData();
	drawGLScene();
	
	QPainter paint (this);
	QFontMetrics metrics = QFontMetrics (QFont());
	paint.setRenderHint (QPainter::HighQualityAntialiasing);
	
	// If we wish to only draw the brick, stop here
	if (drawOnly())
		return;
	
	if (m_camera != Free && !picking()) {
		// Paint the overlay image if we have one
		const overlayMeta& overlay = m_overlays[m_camera];
		if (overlay.img != null) {
			QPoint v0 = coordconv3_2 (m_overlays[m_camera].v0),
				v1 = coordconv3_2 (m_overlays[m_camera].v1);
			
			QRect targRect (v0.x(), v0.y(), abs (v1.x() - v0.x()), abs (v1.y() - v0.y())),
				srcRect (0, 0, overlay.img->width(), overlay.img->height());
			paint.drawImage (targRect, *overlay.img, srcRect);
		}
		
		// Paint the coordinates onto the screen.
		str text = fmt (tr ("X: %1, Y: %2, Z: %3"), m_hoverpos[X], m_hoverpos[Y], m_hoverpos[Z]);
		
		QFontMetrics metrics = QFontMetrics (font());
		QRect textSize = metrics.boundingRect (0, 0, m_width, m_height, Qt::AlignCenter, text);
		
		paint.setPen (getTextPen());
		paint.drawText (m_width - textSize.width(), m_height - 16, textSize.width(),
			textSize.height(), Qt::AlignCenter, text);
		
		// If we're drawing, draw the vertices onto the screen.
		if (editMode() == Draw) {
			const short blipsize = 8;
			int numverts = 4;
			
			if (!m_rectdraw)
				numverts = m_drawedVerts.size() + 1;
			
			if (numverts > 0) {
				QPoint poly[4];
				vertex polyverts[4];
				
				if (!m_rectdraw) {
					uchar i = 0;
					for (vertex& vert : m_drawedVerts) {
						poly[i] = coordconv3_2 (vert);
						polyverts[i] = vert;
						++i;
					}
					
					// Draw the cursor vertex as the last one in the list.
					if (numverts <= 4) {
						poly[i] = coordconv3_2 (m_hoverpos);
						polyverts[i] = m_hoverpos;
					} else {
						numverts = 4;
					}
				} else {
					if (m_drawedVerts.size() > 0) {
						// Get vertex information from m_rectverts
						for (int i = 0; i < numverts; ++i) {
							polyverts[i] = m_rectverts[i];
							poly[i] = coordconv3_2 (polyverts[i]);
						}
					} else {
						poly[0] = coordconv3_2 (m_hoverpos);
						polyverts[0] = m_hoverpos;
					}
				}
				
				// Draw the polygon-to-be
				QPen pen = m_thinBorderPen;
				pen.setWidth (2);
				pen.setColor (luma (m_bgcolor) < 40 ? Qt::white : Qt::black);
				paint.setPen (pen);
				paint.setBrush (QColor (64, 192, 0, 128));
				paint.drawPolygon (poly, numverts);
				
				// Draw vertex blips
				pen = m_thinBorderPen;
				pen.setWidth (1);
				paint.setPen (pen);
				paint.setBrush (QColor (64, 192, 0));
				
				for (ushort i = 0; i < numverts; ++i) {
					QPoint& blip = poly[i];
					paint.drawEllipse (blip.x() - blipsize / 2, blip.y() - blipsize / 2,
						blipsize, blipsize);
					
					// Draw their coordinates
					paint.drawText (blip.x(), blip.y() - 8, polyverts[i].stringRep (true));
				}
			}
		}
	}
	
	// Camera icons
	if (!m_picking) {
		// Draw a background for the selected camera
		paint.setPen (m_thinBorderPen);
		paint.setBrush (QBrush (QColor (0, 128, 160, 128)));
		paint.drawRect (m_cameraIcons[camera()].selRect);
		
		// Draw the actual icons
		for (CameraIcon& info : m_cameraIcons) {
			// Don't draw the free camera icon when in draw mode
			if (&info == &m_cameraIcons[GL::Free] && editMode() != Select)
				continue;
			
			paint.drawPixmap (info.destRect, *info.img, info.srcRect);
		}
		
		str fmtstr = tr ("%1 Camera");
		
		// Draw a label for the current camera in the bottom left corner
		{
			const ushort margin = 4;
			
			str label;
			label = fmt (fmtstr, tr (g_CameraNames[camera()]));
			paint.setPen (getTextPen());
			paint.drawText (QPoint (margin, height() -  (margin + metrics.descent())), label);
		}
		
		// Tool tips
		if (m_drawToolTip) {
			if (m_cameraIcons[m_toolTipCamera].destRect.contains (m_pos) == false)
				m_drawToolTip = false;
			else {
				str label = fmt (fmtstr, tr (g_CameraNames[m_toolTipCamera]));
				QToolTip::showText (m_globalpos, label);
			}
		}
	}
	
	// Message log
	if (msglog()) {
		int y = 0;
		const int margin = 2;
		QColor penColor = getTextPen();
		
		for (const MessageManager::Line& line : msglog()->getLines()) {
			penColor.setAlphaF (line.alpha);
			paint.setPen (penColor);
			paint.drawText (QPoint (margin, y + margin + metrics.ascent()), line.text);
			y += metrics.height();
		}
	}
	
	// If we're range-picking, draw a rectangle encompassing the selection area.
	if (m_rangepick && !m_picking && m_totalmove >= 10) {
		const short x0 = m_rangeStart.x(),
			y0 = m_rangeStart.y(),
			x1 = m_pos.x(),
			y1 = m_pos.y();
		
		QRect rect (x0, y0, x1 - x0, y1 - y0);
		QColor fillColor = (m_addpick ? "#40FF00" : "#00CCFF");
		fillColor.setAlphaF (0.2f);
		
		paint.setPen (m_thickBorderPen);
		paint.setBrush (QBrush (fillColor));
		paint.drawRect (rect);
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
QColor GLRenderer::getTextPen () const {
	return m_darkbg ? Qt::white : Qt::black;
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::compileAllObjects() {
	g_vertexCompiler.compileFile();
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::clampAngle (double& angle) const {
	while (angle < 0)
		angle += 360.0;
	while (angle > 360.0)
		angle -= 360.0;
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::addDrawnVertex (vertex pos) {
	// If we picked an already-existing vertex, stop drawing
	for (vertex& vert : m_drawedVerts) {
		if (vert == pos) {
			endDraw (true);
			return;
		}
	}
	
	m_drawedVerts << pos;
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::mouseReleaseEvent (QMouseEvent* ev) {
	const bool wasLeft = (m_lastButtons & Qt::LeftButton) && !(ev->buttons() & Qt::LeftButton),
		wasRight = (m_lastButtons & Qt::RightButton) && !(ev->buttons() & Qt::RightButton),
		wasMid = (m_lastButtons & Qt::MidButton) && !(ev->buttons() & Qt::MidButton);
	
	if (m_panning)
		m_panning = false;
	
	if (wasLeft) {
		// Check if we selected a camera icon
		if (!m_rangepick) {
			for (CameraIcon& info : m_cameraIcons) {
				if (info.destRect.contains (ev->pos())) {
					setCamera (info.cam);
					goto end;
				}
			}
		}
		
		switch (editMode()) {
		case Draw:
			if (m_rectdraw) {
				if (m_drawedVerts.size() == 2) {
					endDraw (true);
					return;
				}
			} else {
				// If we have 4 verts, stop drawing.
				if (m_drawedVerts.size() >= 4) {
					endDraw (true);
					return;
				}
				
				if (m_drawedVerts.size() == 0 && ev->modifiers() & Qt::ShiftModifier) {
					m_rectdraw = true;
					updateRectVerts();
				}
			}
			
			addDrawnVertex (m_hoverpos);
			break;
		
		case Select:
			if (!drawOnly()) {
				if (m_totalmove < 10)
					m_rangepick = false;
				
				if (!m_rangepick)
					m_addpick = (m_keymods & Qt::ControlModifier);
				
				if (m_totalmove < 10 || m_rangepick)
					pick (ev->x(), ev->y());
			}
			
			break;
		}
		
		m_rangepick = false;
	}
	
	if (wasMid && editMode() == Draw && m_drawedVerts.size() < 4 && m_totalmove < 10) {
		// Find the closest vertex to our cursor
		double mindist = 1024.0f;
		vertex closest;
		bool valid = false;
		
		QPoint curspos = coordconv3_2 (m_hoverpos);
		
		for (const vertex& pos3d: m_knownVerts) {
			QPoint pos2d = coordconv3_2 (pos3d);
			
			// Measure squared distance
			const double dx = abs (pos2d.x() - curspos.x()),
				dy = abs (pos2d.y() - curspos.y()),
				distsq = (dx * dx) + (dy * dy);
			
			if (distsq >= 1024.0f) // 32.0f ** 2
				continue; // too far away
			
			if (distsq < mindist) {
				mindist = distsq;
				closest = pos3d;
				valid = true;
				
				/* If it's only 4 pixels away, I think we found our vertex now. */
				if (distsq <= 16.0f) // 4.0f ** 2
					break;
			}
		}
		
		if (valid)
			addDrawnVertex (closest);
	}
	
	if (wasRight && m_drawedVerts.size() > 0) {
		// Remove the last vertex
		m_drawedVerts.erase (m_drawedVerts.size() - 1);
		
		if (m_drawedVerts.size() == 0)
			m_rectdraw = false;
	}
	
end:
	update();
	m_totalmove = 0;
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::mousePressEvent (QMouseEvent* ev) {
	m_totalmove = 0;
	
	if (ev->modifiers() & Qt::ControlModifier) {
		m_rangepick = true;
		m_rangeStart.setX (ev->x());
		m_rangeStart.setY (ev->y());
		m_addpick = (m_keymods & Qt::AltModifier);
		ev->accept();
	}
	
	m_lastButtons = ev->buttons();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void GLRenderer::mouseMoveEvent (QMouseEvent* ev) {
	int dx = ev->x() - m_pos.x();
	int dy = ev->y() - m_pos.y();
	m_totalmove += abs (dx) + abs (dy);
	
	const bool left = ev->buttons() & Qt::LeftButton,
		mid = ev->buttons() & Qt::MidButton,
		shift = ev->modifiers() & Qt::ShiftModifier;
	
	if (mid || (left && shift)) {
		m_panX += 0.03f * dx * (zoom() / 7.5f);
		m_panY -= 0.03f * dy * (zoom() / 7.5f);
		m_panning = true;
	} elif (left && !m_rangepick && camera() == Free) {
		m_rotX = m_rotX + (dy);
		m_rotY = m_rotY + (dx);
		
		clampAngle (m_rotX);
		clampAngle (m_rotY);
	}
	
	// Start the tool tip timer
	if (!m_drawToolTip)
		m_toolTipTimer->start (500);
	
	// Update 2d position
	m_pos = ev->pos();
	m_globalpos = ev->globalPos();
	
	// Calculate 3d position of the cursor
	m_hoverpos = (camera() != Free) ? coordconv2_3 (m_pos, true) : g_origin;
	
	// Update rect vertices since m_hoverpos may have changed
	updateRectVerts();
	
	update();
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::keyPressEvent (QKeyEvent* ev) {
	m_keymods = ev->modifiers();
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::keyReleaseEvent (QKeyEvent* ev) {
	m_keymods = ev->modifiers();
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::wheelEvent (QWheelEvent* ev) {
	makeCurrent();
	
	zoomNotch (ev->delta() > 0);
	setZoom (clamp<double> (zoom(), 0.01f, 10000.0f));
	
	update();
	ev->accept();
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::leaveEvent (QEvent* ev) {
	(void) ev;
	m_drawToolTip = false;
	m_toolTipTimer->stop();
	update();
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::contextMenuEvent (QContextMenuEvent* ev) {
	g_win->spawnContextMenu (ev->globalPos());
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::setCamera (const GL::Camera cam) {
	m_camera = cam;
	gl_camera = (int) cam;
	g_win->updateEditModeActions();
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::pick (uint mouseX, uint mouseY) {
	GLint viewport[4];
	makeCurrent();
	
	// Use particularly thick lines while picking ease up selecting lines.
	glLineWidth (max<double> (gl_linethickness, 6.5f));
	
	// Clear the selection if we do not wish to add to it.
	if (!m_addpick) {
		List<LDObject*> oldsel = g_win->sel();
		g_win->sel().clear();
		
		for (LDObject* obj : oldsel) {
			obj->setSelected (false);
			compileObject (obj);
		}
	}
	
	m_picking = true;
	
	// Paint the picking scene
	glDisable (GL_DITHER);
	glClearColor (1.0f, 1.0f, 1.0f, 1.0f);
	
	drawGLScene();
	
	glGetIntegerv (GL_VIEWPORT, viewport);
	
	int x0 = mouseX,
		y0 = mouseY,
		x1, y1;
	
	// Determine how big an area to read - with range picking, we pick by
	// the area given, with single pixel picking, we use an 1 x 1 area.
	if (m_rangepick) {
		x1 = m_rangeStart.x();
		y1 = m_rangeStart.y();
	} else {
		x1 = x0 + 1;
		y1 = y0 + 1;
	}
	
	// x0 and y0 must be less than x1 and y1, respectively.
	if (x0 > x1)
		dataswap (x0, x1);
	
	if (y0 > y1)
		dataswap (y0, y1);
	
	// Clamp the values to ensure they're within bounds
	x0 = max (0, x0);
	y0 = max (0, y0);
	x1 = min (x1, m_width);
	y1 = min (y1, m_height);
	
	const int areawidth = (x1 - x0);
	const int areaheight = (y1 - y0);
	const int64 numpixels = areawidth * areaheight;
	
	// Allocate space for the pixel data.
	uchar* const pixeldata = new uchar[4 * numpixels];
	uchar* pixelptr = &pixeldata[0];
	
	assert (viewport[3] == m_height);
	
	// Read pixels from the color buffer.
	glReadPixels (x0, viewport[3] - y1, areawidth, areaheight, GL_RGBA, GL_UNSIGNED_BYTE, pixeldata);
	
	LDObject* removedObj = null;
	
	// Go through each pixel read and add them to the selection.
	for (int64 i = 0; i < numpixels; ++i) {
		printf ("Color: #%X%X%X\n", pixelptr[0], pixelptr[1], pixelptr[2]);
		
		int32 idx =
			(*(pixelptr + 0) * 0x00001) +
			(*(pixelptr + 1) * 0x00100) +
			(*(pixelptr + 2) * 0x10000);
		
		pixelptr += 4;
		
		if (idx == 0xFFFFFF)
			continue; // White is background; skip
		
		LDObject* obj = LDObject::fromID (idx);
		if (!obj) {
			log ("WARNING: Object #%1 doesn't exist!", idx);
			continue;
		}
		
		// If this is an additive single pick and the object is currently selected,
		// we remove it from selection instead.
		if (!m_rangepick && m_addpick) {
			bool removed = false;
			
			for (ulong i = 0; i < g_win->sel().size(); ++i) {
				if (g_win->sel()[i] == obj) {
					g_win->sel().erase (i);
					obj->setSelected (false);
					removed = true;
					removedObj = obj;
				}
			}
			
			if (removed)
				break;
		}
		
		g_win->sel() << obj;
	}
	
	delete[] pixeldata;
	
	// Remove duplicated entries
	g_win->sel().makeUnique();
	
	// Update everything now.
	g_win->updateSelection();
	
	// Recompile the objects now to update their color
	for (LDObject* obj : g_win->sel())
		compileObject (obj);
	
	if (removedObj)
		compileObject (removedObj);
	
	// Restore line thickness
	glLineWidth (gl_linethickness);
	
	m_picking = false;
	m_rangepick = false;
	glEnable (GL_DITHER);
	
	setBackground();
	repaint();
}

// =============================================================================
// -----------------------------------------------------------------------------
READ_ACCESSOR (EditMode, GLRenderer::editMode) {
	return m_editMode;
}

// =============================================================================
// -----------------------------------------------------------------------------
SET_ACCESSOR (EditMode, GLRenderer::setEditMode) {
	m_editMode = val;
	
	switch (editMode()) {
	case Select:
		unsetCursor();
		setContextMenuPolicy (Qt::DefaultContextMenu);
		break;
	
	case Draw:
		// Cannot draw into the free camera - use top instead.
		if (m_camera == Free)
			setCamera (Top);
		
		// Disable the context menu - we need the right mouse button
		// for removing vertices.
		setContextMenuPolicy (Qt::NoContextMenu);
		
		// Use the crosshair cursor when drawing.
		setCursor (Qt::CrossCursor);
		
		// Clear the selection when beginning to draw.
		// FIXME: make the selection clearing stuff in ::pick a method and use it
		// here! This code doesn't update the GL lists.
		g_win->sel().clear();
		g_win->updateSelection();
		m_drawedVerts.clear();
		break;
	}
	
	g_win->updateEditModeActions();
	update();
}

// =============================================================================
// -----------------------------------------------------------------------------
READ_ACCESSOR (LDFile*, GLRenderer::file) {
	return m_file;
}

// =============================================================================
// -----------------------------------------------------------------------------
SET_ACCESSOR (LDFile*, GLRenderer::setFile) {
	m_file = val;
	g_vertexCompiler.setFile (val);
	
	if (val != null)
		overlaysFromObjects();
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::endDraw (bool accept) {
	(void) accept;
	
	// Clean the selection and create the object
	List<vertex>& verts = m_drawedVerts;
	LDObject* obj = null;
	
	if (m_rectdraw) {
		LDQuad* quad = new LDQuad;
		
		// Copy the vertices from m_rectverts
		updateRectVerts();
		
		for (int i = 0; i < quad->vertices(); ++i)
			quad->setVertex (i, m_rectverts[i]);
		
		quad->setColor (maincolor);
		obj = quad;
	} else {
		switch (verts.size()) {
		case 1:
			// 1 vertex - add a vertex object
			obj = new LDVertex;
			static_cast<LDVertex*> (obj)->pos = verts[0];
			obj->setColor (maincolor);
			break;
		
		case 2:
			// 2 verts - make a line
			obj = new LDLine (verts[0], verts[1]);
			obj->setColor (edgecolor);
			break;
			
		case 3:
		case 4:
			obj = (verts.size() == 3) ?
				static_cast<LDObject*> (new LDTriangle) :
				static_cast<LDObject*> (new LDQuad);
			
			obj->setColor (maincolor);
			for (ushort i = 0; i < obj->vertices(); ++i)
				obj->setVertex (i, verts[i]);
			break;
		}
	}
	
	if (obj) {
		g_win->beginAction (null);
		file()->addObject (obj);
		compileObject (obj);
		g_win->fullRefresh();
		g_win->endAction();
	}
	
	m_drawedVerts.clear();
	m_rectdraw = false;
}

// =============================================================================
// -----------------------------------------------------------------------------
static List<vertex> getVertices (LDObject* obj) {
	List<vertex> verts;
	
	if (obj->vertices() >= 2) {
		for (int i = 0; i < obj->vertices(); ++i)
			verts << obj->getVertex (i);
	} elif (obj->getType() == LDObject::Subfile) {
		LDSubfile* ref = static_cast<LDSubfile*> (obj);
		List<LDObject*> objs = ref->inlineContents (LDSubfile::DeepCacheInline);
		
		for(LDObject* obj : objs) {
			verts << getVertices (obj);
			delete obj;
		}
	}
	
	return verts;
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::compileObject (LDObject* obj) {
	g_vertexCompiler.stageForCompilation (obj);
	obj->m_glinit = true;
}

// =============================================================================
// -----------------------------------------------------------------------------
uchar* GLRenderer::screencap (ushort& w, ushort& h) {
	w = m_width;
	h = m_height;
	uchar* cap = new uchar[4 * w * h];
	
	m_screencap = true;
	update();
	m_screencap = false;
	
	// Capture the pixels
	glReadPixels (0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, cap);
	
	return cap;
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::slot_toolTipTimer() {
	// We come here if the cursor has stayed in one place for longer than a
	// a second. Check if we're holding it over a camera icon - if so, draw
	// a tooltip.
	for (CameraIcon& icon : m_cameraIcons) {
		if (icon.destRect.contains (m_pos)) {
			m_toolTipCamera = icon.cam;
			m_drawToolTip = true;
			update();
			break;
		}
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::deleteLists (LDObject* obj) {
	// Delete the lists but only if they have been initialized
	if (!obj->m_glinit)
		return;
	
	for (const GL::ListType listType : g_glListTypes)
		glDeleteLists (obj->glLists[listType], 1);
	
	obj->m_glinit = false;
}

// =============================================================================
// -----------------------------------------------------------------------------
Axis GLRenderer::cameraAxis (bool y, GL::Camera camid) {
	if (camid == (GL::Camera) -1)
		camid = m_camera;
	
	const staticCameraMeta* cam = &g_staticCameras[camid];
	return (y) ? cam->axisY : cam->axisX;
}

// =============================================================================
// -----------------------------------------------------------------------------
bool GLRenderer::setupOverlay (GL::Camera cam, str file, int x, int y, int w, int h) {
	QImage* img = new QImage (file);
	overlayMeta& info = getOverlay (cam);
	
	if (img->isNull()) {
		critical (tr ("Failed to load overlay image!"));
		delete img;
		return false;
	}
	
	delete info.img; // delete the old image
	
	info.fname = file;
	info.lw = w;
	info.lh = h;
	info.ox = x;
	info.oy = y;
	info.img = img;
	
	if (info.lw == 0)
		info.lw = (info.lh * img->width()) / img->height();
	elif (info.lh == 0)
		info.lh = (info.lw * img->height()) / img->width();
	
	const Axis x2d = cameraAxis (false, cam),
		y2d = cameraAxis (true, cam);
	
	double negXFac = g_staticCameras[cam].negX ? -1 : 1,
		negYFac = g_staticCameras[cam].negY ? -1 : 1;
	
	info.v0 = info.v1 = g_origin;
	info.v0[x2d] = - (info.ox * info.lw * negXFac) / img->width();
	info.v0[y2d] = (info.oy * info.lh * negYFac) / img->height();
	info.v1[x2d] = info.v0[x2d] + info.lw;
	info.v1[y2d] = info.v0[y2d] + info.lh;
	
	// Set alpha of all pixels to 0.5
	for (long i = 0; i < img->width(); ++i)
	for (long j = 0; j < img->height(); ++j) {
		uint32 pixel = img->pixel (i, j);
		img->setPixel (i, j, 0x80000000 | (pixel & 0x00FFFFFF));
	}
	
	updateOverlayObjects();
	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::clearOverlay() {
	if (camera() == Free)
		return;
	
	overlayMeta& info = m_overlays[camera()];
	delete info.img;
	info.img = null;
	
	updateOverlayObjects();
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::setDepthValue (double depth) {
	assert (camera() < Free);
	m_depthValues[camera()] = depth;
}

// =============================================================================
// -----------------------------------------------------------------------------
double GLRenderer::depthValue() const {
	assert (camera() < Free);
	return m_depthValues[camera()];
}

// =============================================================================
// -----------------------------------------------------------------------------
const char* GLRenderer::cameraName() const {
	return g_CameraNames[camera()];
}

// =============================================================================
// -----------------------------------------------------------------------------
overlayMeta& GLRenderer::getOverlay (int newcam) {
	 return m_overlays[newcam];
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::zoomNotch (bool inward) {
	if (zoom() > 15)
		setZoom (zoom() * (inward ? 0.833f : 1.2f));
	else
		setZoom (zoom() + (inward ? -1.2f : 1.2f));
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::zoomToFit() {
	if (file() == null) {
		setZoom (30.0f);
		return;
	}
	
	bool lastfilled = false;
	bool firstrun = true;
	const uint32 white = 0xFFFFFFFF;
	bool inward = true;
	ulong run = 0;
	const ushort w = m_width, h = m_height;
	
	glClearColor (1.0, 1.0, 1.0, 1.0);
	glDisable (GL_DITHER);
	
	// Use the pick list while drawing the scene, this way we can tell whether borders
	// are background or not.
	m_picking = true;
	
	for (;;) {
		if (zoom() > 10000.0f || zoom() < 0.0f) {
			// Obviously, there's nothing to draw if we get here.
			// Default to 30.0f and break out.
			setZoom (30.0f);
			break;
		}
		
		zoomNotch (inward);
		
		uchar* cap = new uchar[4 * w * h];
		drawGLScene();
		glReadPixels (0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, cap);
		uint32* imgdata = reinterpret_cast<uint32*> (cap);
		bool filled = false;
		
		// Check the top and bottom rows
		for (ushort i = 0; i < w && !filled; ++i)
			if (imgdata[i] != white || imgdata[((h - 1) * w) + i] != white)
				filled = true;
		
		// Left and right edges
		for (ushort i = 0; i < h && !filled; ++i)
			if (imgdata[i * w] != white || imgdata[(i * w) + (w - 1)] != white)
				filled = true;
		
		if (firstrun) {
			// If this is the first run, we don't know enough to determine
			// whether the zoom was to fit, so we mark in our knowledge so
			// far and start over.
			inward = !filled;
			firstrun = false;
		} else {
			// If this run filled the screen and the last one did not, the
			// last run had ideal zoom - zoom a bit back and we should reach it.
			if (filled && !lastfilled) {
				zoomNotch (false);
				break;
			}
			
			// If this run did not fill the screen and the last one did, we've
			// now reached ideal zoom so we're done here.
			if (!filled && lastfilled)
				break;
			
			inward = !filled;
		}
		
		delete[] cap;
		lastfilled = filled;
		++run;
	}
	
	setBackground();
	m_picking = false;
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::updateRectVerts() {
	if (!m_rectdraw)
		return;
	
	if (m_drawedVerts.size() == 0) {
		for (int i = 0; i < 4; ++i)
			m_rectverts[i] = m_hoverpos;
		
		return;
	}
	
	vertex v0 = m_drawedVerts[0],
		v1 = (m_drawedVerts.size() >= 2) ? m_drawedVerts[1] : m_hoverpos;
	
	const Axis ax = cameraAxis (false),
		ay = cameraAxis (true),
		az = (Axis) (3 - ax - ay);
	
	for (int i = 0; i < 4; ++i)
		m_rectverts[i][az] = depthValue();
	
	m_rectverts[0][ax] = v0[ax];
	m_rectverts[0][ay] = v0[ay];
	m_rectverts[1][ax] = v1[ax];
	m_rectverts[1][ay] = v0[ay];
	m_rectverts[2][ax] = v1[ax];
	m_rectverts[2][ay] = v1[ay];
	m_rectverts[3][ax] = v0[ax];
	m_rectverts[3][ay] = v1[ay];
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::mouseDoubleClickEvent (QMouseEvent* ev) {
	if (!(ev->buttons() & Qt::LeftButton) || editMode() != Select)
		return;
	
	pick (ev->x(), ev->y());
	
	if (g_win->sel().size() == 0)
		return;
	
	g_win->beginAction (null);
	LDObject* obj = g_win->sel()[0];
	AddObjectDialog::staticDialog (obj->getType(), obj);
	g_win->endAction();
	ev->accept();
}

// =============================================================================
// -----------------------------------------------------------------------------
LDOverlay* GLRenderer::findOverlayObject (GLRenderer::Camera cam) {
	LDOverlay* ovlobj = null;
	
	for (LDObject * obj : file()->objects()) {
		if (obj->getType() == LDObject::Overlay && static_cast<LDOverlay*> (obj)->camera() == cam) {
			ovlobj = static_cast<LDOverlay*> (obj);
			break;
		}
	}
	
	return ovlobj;
}

// =============================================================================
// -----------------------------------------------------------------------------
// Read in overlays from the current file and update overlay info accordingly.
// -----------------------------------------------------------------------------
void GLRenderer::overlaysFromObjects() {
	for (Camera cam : g_Cameras) {
		if (cam == Free)
			continue;
		
		overlayMeta& meta = m_overlays[cam];
		LDOverlay* ovlobj = findOverlayObject (cam);
		
		if (!ovlobj && meta.img) {
			delete meta.img;
			meta.img = null;
		} elif (ovlobj && (!meta.img || meta.fname != ovlobj->filename()))
			setupOverlay (cam, ovlobj->filename(), ovlobj->x(), ovlobj->y(), ovlobj->width(), ovlobj->height());
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void GLRenderer::updateOverlayObjects() {
	for (Camera cam : g_Cameras) {
		if (cam == Free)
			continue;
		
		overlayMeta& meta = m_overlays[cam];
		LDOverlay* ovlobj = findOverlayObject (cam);
		
		if (!meta.img && ovlobj) {
			// If this is the last overlay image, we need to remove the empty space after it as well.
			LDObject* nextobj = ovlobj->next();
			
			if (nextobj && nextobj->getType() == LDObject::Empty) {
				m_file->forgetObject (nextobj);
				delete nextobj;
			}
			
			// If the overlay object was there and the overlay itself is
			// not, remove the object.
			m_file->forgetObject (ovlobj);
			delete ovlobj;
		} elif (meta.img && !ovlobj) {
			// Inverse case: image is there but the overlay object is
			// not, thus create the object.
			ovlobj = new LDOverlay;
			
			// Find a suitable position to place this object. We want to place
			// this into the header, which is everything up to the first scemantic
			// object. If we find another overlay object, place this object after
			// the last one found. Otherwise, place it before the first schemantic
			// object and put an empty object after it (though don't do this if
			// there was no schemantic elements at all)
			ulong i, lastOverlay = -1u;
			bool found = false;
			
			for (i = 0; i < file()->numObjs(); ++i) {
				LDObject* obj = file()->obj (i);
				
				if (obj->isScemantic()) {
					found = true;
					break;
				}
				
				if (obj->getType() == LDObject::Overlay)
					lastOverlay = i;
			}
			
			if (lastOverlay != -1u)
				file()->insertObj (lastOverlay + 1, ovlobj);
			else {
				file()->insertObj (i, ovlobj);
				
				if (found)
					file()->insertObj (i + 1, new LDEmpty);
			}
		}
		
		if (meta.img && ovlobj) {
			ovlobj->setCamera (cam);
			ovlobj->setFilename (meta.fname);
			ovlobj->setX (meta.ox);
			ovlobj->setY (meta.oy);
			ovlobj->setWidth (meta.lw);
			ovlobj->setHeight (meta.lh);
		}
	}
	
	if (g_win->R() == this)
		g_win->refresh();
}
#include "moc_gldraw.cpp"
