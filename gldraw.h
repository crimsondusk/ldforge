#ifndef __GLDRAW_H__
#define __GLDRAW_H__

#include <QGLWidget>
#include "common.h"
#include "ldtypes.h"

class renderer : public QGLWidget {
	Q_OBJECT
	
public:
	renderer(QWidget* parent = NULL);
	void hardRefresh ();
	void CompileObjects ();
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