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

#ifndef COLORSELECTOR_H
#define COLORSELECTOR_H

#include <qdialog.h>
#include <qdialogbuttonbox.h>
#include <qgraphicsscene.h>
#include <qlabel.h>
#include "common.h"

class ColorSelectDialog : public QDialog {
	Q_OBJECT
	
public:
	explicit ColorSelectDialog (short dDefault = -1, QWidget* parent = null);
	static bool staticDialog (short& dValue, short dDefault = -1, QWidget* parent = null);
	
	QGraphicsScene* gs_scene;
	QGraphicsView* gv_view;
	QLabel* lb_colorInfo;
	QDialogButtonBox* bbx_buttons;
	short selColor;
	
private:
	void drawScene ();
	void drawColorInfo ();
	
private slots:
	void mousePressEvent (QMouseEvent* event);
};

#endif // COLORSELECTOR_H