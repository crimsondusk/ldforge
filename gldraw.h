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

#ifndef __GLDRAW_H__
#define __GLDRAW_H__

#include <QGLWidget>
#include "common.h"
#include "ldtypes.h"

class renderer : public QGLWidget {
	Q_OBJECT
	
public:
	renderer (QWidget* parent = nullptr);
	void hardRefresh ();
	void compileObjects ();
    void setBackground ();
	
	double fRotX, fRotY, fRotZ;
	QPoint lastPos;
	double fZoom;

protected:
	void initializeGL ();
	void resizeGL (int w, int h);
	void paintGL ();
	
	void mouseMoveEvent (QMouseEvent *event);

private:
	GLuint objlist;
	void compileOneObject (LDObject* obj);
	void clampAngle (double& fAngle);
};

#endif // __GLDRAW_H__