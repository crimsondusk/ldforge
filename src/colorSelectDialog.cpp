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

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QBoxLayout>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QScrollBar>
#include <QLabel>
#include <QDialogButtonBox>

#include "common.h"
#include "gui.h"
#include "colorSelectDialog.h"
#include "colors.h"
#include "config.h"
#include "misc.h"

static const short g_numCols = 8;
static const short g_numRows = 10;
static const short g_squareSize = 32;
static const long g_width = (g_numCols * g_squareSize);
static const long g_height = (g_numRows * g_squareSize);
static const long g_maxHeight = ((MAX_COLORS / g_numCols) * g_squareSize);

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
	
	gs_scene->setSceneRect (0, 0, g_width, g_maxHeight);
	gv_view->setSceneRect (0, 0, g_width, g_maxHeight);
	
	drawScene ();
	
	// Set the size of the view
	const long viewWidth = g_width + 21; // HACK: 21 for scrollbar
	gv_view->setMaximumWidth (viewWidth);
	gv_view->setMinimumWidth (viewWidth);
	gv_view->setMaximumHeight (g_height);
	gv_view->setMinimumHeight (g_height);
	gv_view->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
	
	// If we have a default color selected, scroll down so that it is visible.
	// HACK: find a better way to do this
	if (defval >= ((g_numCols * g_numRows) - 2)) {
		ulong ulNewY = ((defval / g_numCols) - 3) * g_squareSize;
		gv_view->verticalScrollBar ()->setSliderPosition (ulNewY);
	}
	
	lb_colorInfo = new QLabel;
	drawColorInfo ();
	
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget (gv_view);
	layout->addWidget (lb_colorInfo);
	layout->addWidget (makeButtonBox (*this));
	setLayout (layout);
	
	setWindowIcon (getIcon ("palette"));
	setWindowTitle (APPNAME);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ColorSelectDialog::drawScene () {
	const double penWidth = 1.0f;
	QPen pen (Qt::black, penWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
	
	// Draw the color rectangles.
	gs_scene->clear ();
	for (short i = 0; i < MAX_COLORS; ++i) {
		color* meta = getColor (i);
		if (!meta)
			continue;
		
		const double x = (i % g_numCols) * g_squareSize;
		const double y = (i / g_numCols) * g_squareSize;
		const double w = (g_squareSize) - (penWidth / 2);
		
		QColor col = meta->faceColor;
		
		if (i == maincolor) {
			// Use the user preferences for main color here
			col = gl_maincolor.value.chars ();
			col.setAlpha (gl_maincolor_alpha * 255.0f);
		}
		
		gs_scene->addRect (x, y, w, w, pen, col);
		QGraphicsTextItem* numtext = gs_scene->addText (fmt ("%d", i).chars());
		numtext->setDefaultTextColor ((luma (col) < 80) ? Qt::white : Qt::black);
		numtext->setPos (x, y);
		
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
		selColor, col->name.chars()));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ColorSelectDialog::mousePressEvent (QMouseEvent* event) {
	QPointF scenepos = gv_view->mapToScene (event->pos ());
	
	ulong x = ((ulong) scenepos.x () - (g_squareSize / 2)) / g_squareSize;
	ulong y = ((ulong) scenepos.y () - (g_squareSize / 2)) / g_squareSize;
	ulong idx = (y * g_numCols) + x;
	
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