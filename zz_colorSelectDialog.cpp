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

#include "common.h"
#include "gui.h"
#include <qgraphicsscene.h>
#include <qgraphicsview.h>
#include <qicon.h>
#include <qboxlayout.h>
#include <qgraphicsitem.h>
#include <qevent.h>
#include <qscrollbar.h>
#include "zz_colorSelectDialog.h"
#include "colors.h"
#include "config.h"
#include "misc.h"

static const short g_dNumColumns = 8;
static const short g_dNumRows = 10;
static const short g_dSquareSize = 32;
static const long g_lWidth = (g_dNumColumns * g_dSquareSize);
static const long g_lHeight = (g_dNumRows * g_dSquareSize);
static const long g_lMaxHeight = ((MAX_COLORS / g_dNumColumns) * g_dSquareSize);

extern_cfg (str, gl_maincolor);
extern_cfg (float, gl_maincolor_alpha);

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ColorSelectDialog::ColorSelectDialog (short int defval, QWidget* parent) : QDialog (parent) {
	// Remove the default color if it's invalid
	if (!getColor (defval))
		defval = -1;
	
	gs_scene = new QGraphicsScene;
	gv_view = new QGraphicsView (gs_scene);
	selColor = defval;
	
	// not really an icon but eh
	gs_scene->setBackgroundBrush (getIcon ("checkerboard"));
	
	gs_scene->setSceneRect (0, 0, g_lWidth, g_lMaxHeight);
	gv_view->setSceneRect (0, 0, g_lWidth, g_lMaxHeight);
	
	drawScene ();
	
	IMPLEMENT_DIALOG_BUTTONS
	
	// Set the size of the view
	const long lWidth = g_lWidth + 21; // HACK
	gv_view->setMaximumWidth (lWidth);
	gv_view->setMinimumWidth (lWidth);
	gv_view->setMaximumHeight (g_lHeight);
	gv_view->setMinimumHeight (g_lHeight);
	gv_view->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
	
	// If we have a default color selected, scroll down so that it is visible.
	// TODO: find a better way to do this
	if (defval >= ((g_dNumColumns * g_dNumRows) - 2)) {
		ulong ulNewY = ((defval / g_dNumColumns) - 3) * g_dSquareSize;
		gv_view->verticalScrollBar ()->setSliderPosition (ulNewY);
	}
	
	lb_colorInfo = new QLabel;
	drawColorInfo ();
	
	QVBoxLayout* qLayout = new QVBoxLayout;
	qLayout->addWidget (gv_view);
	qLayout->addWidget (lb_colorInfo);
	qLayout->addWidget (bbx_buttons);
	setLayout (qLayout);
	
	setWindowIcon (getIcon ("palette"));
	setWindowTitle (APPNAME);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ColorSelectDialog::drawScene () {
	const double fPenWidth = 1.0f;
	QPen qPen (Qt::black, fPenWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
	
	// Draw the color rectangles.
	gs_scene->clear ();
	for (short i = 0; i < MAX_COLORS; ++i) {
		color* meta = getColor (i);
		if (!meta)
			continue;
		
		const double x = (i % g_dNumColumns) * g_dSquareSize;
		const double y = (i / g_dNumColumns) * g_dSquareSize;
		const double w = (g_dSquareSize) - (fPenWidth / 2);
		
		QColor col = meta->qColor;
		
		if (i == maincolor) {
			// Use the user preferences for main color here
			col = gl_maincolor.value.chars ();
			col.setAlpha (gl_maincolor_alpha * 255.0f);
		}
		
		bool dark = (luma (col) < 80);
		
		gs_scene->addRect (x, y, w, w, qPen, col);
		QGraphicsTextItem* qText = gs_scene->addText (fmt ("%lu", i).chars());
		qText->setDefaultTextColor ((dark) ? Qt::white : Qt::black);
		qText->setPos (x, y);
		
		if (i == selColor) {
			auto curspic = gs_scene->addPixmap (getIcon ("colorcursor"));
			curspic->setPos (x, y);
		}
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ColorSelectDialog::drawColorInfo () {
	color* col = getColor (selColor);
	
	if (selColor == -1 || !col) {
		lb_colorInfo->setText ("---");
		return;
	}
	
	lb_colorInfo->setText (fmt ("%d - %s",
		selColor, col->zName.chars()));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ColorSelectDialog::mousePressEvent (QMouseEvent* event) {
	QPointF qPoint = gv_view->mapToScene (event->pos ());
	
	ulong x = ((ulong)qPoint.x () - (g_dSquareSize / 2)) / g_dSquareSize;
	ulong y = ((ulong)qPoint.y () - (g_dSquareSize / 2)) / g_dSquareSize;
	ulong idx = (y * g_dNumColumns) + x;
	
	color* col = getColor (idx);
	if (!col)
		return;
	
	selColor = idx;
	drawScene ();
	drawColorInfo ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool ColorSelectDialog::staticDialog (short& val, short int defval, QWidget* parent) {
	ColorSelectDialog dlg (defval, parent);
	
	if (dlg.exec () && dlg.selColor != -1) {
		val = dlg.selColor;
		return true;
	}
	
	return false;
}