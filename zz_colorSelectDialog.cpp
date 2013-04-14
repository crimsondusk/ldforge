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
ColorSelectDialog::ColorSelectDialog (short dDefault, QWidget* parent) : QDialog (parent) {
	// Remove the default color if it's invalid
	if (!getColor (dDefault))
		dDefault = -1;
	
	qScene = new QGraphicsScene;
	qView = new QGraphicsView (qScene);
	dSelColor = dDefault;
	
	// not really an icon but eh
	qScene->setBackgroundBrush (QPixmap ("icons/checkerboard.png"));
	
	qScene->setSceneRect (0, 0, g_lWidth, g_lMaxHeight);
	qView->setSceneRect (0, 0, g_lWidth, g_lMaxHeight);
	
	drawScene ();
	
	IMPLEMENT_DIALOG_BUTTONS
	
	// Set the size of the view
	const long lWidth = g_lWidth + 21; // HACK
	qView->setMaximumWidth (lWidth);
	qView->setMinimumWidth (lWidth);
	qView->setMaximumHeight (g_lHeight);
	qView->setMinimumHeight (g_lHeight);
	qView->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
	
	// If we have a default color selected, scroll down so that it is visible.
	// TODO: find a better way to do this
	if (dDefault >= ((g_dNumColumns * g_dNumRows) - 2)) {
		ulong ulNewY = ((dDefault / g_dNumColumns) - 3) * g_dSquareSize;
		qView->verticalScrollBar ()->setSliderPosition (ulNewY);
	}
	
	qColorInfo = new QLabel;
	drawColorInfo ();
	
	QVBoxLayout* qLayout = new QVBoxLayout;
	qLayout->addWidget (qView);
	qLayout->addWidget (qColorInfo);
	qLayout->addWidget (qButtons);
	setLayout (qLayout);
	
	setWindowIcon (QIcon ("icons/palette.png"));
	setWindowTitle (APPNAME_DISPLAY " - choose a color");
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ColorSelectDialog::drawScene () {
	const double fPenWidth = 1.0f;
	QPen qPen (Qt::black, fPenWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
	
	// Draw the color rectangles.
	qScene->clear ();
	for (short i = 0; i < MAX_COLORS; ++i) {
		color* meta = getColor (i);
		if (!meta)
			continue;
		
		const double x = (i % g_dNumColumns) * g_dSquareSize;
		const double y = (i / g_dNumColumns) * g_dSquareSize;
		const double w = (g_dSquareSize) - (fPenWidth / 2);
		
		QColor qColor = meta->qColor;
		
		if (i == dMainColor) {
			// Use the user preferences for main color here
			qColor = gl_maincolor.value.chars ();
			qColor.setAlpha (gl_maincolor_alpha * 255.0f);
		}
		
		uchar ucLuma = (0.2126f * qColor.red()) +
			(0.7152f * qColor.green()) + (0.0722f * qColor.blue());
		bool bDark = (ucLuma < 80);
		
		qScene->addRect (x, y, w, w, qPen, qColor);
		QGraphicsTextItem* qText = qScene->addText (str::mkfmt ("%lu", i).chars());
		qText->setDefaultTextColor ((bDark) ? Qt::white : Qt::black);
		qText->setPos (x, y);
		
		if (i == dSelColor) {
			QGraphicsPixmapItem* qCursorPic;
			qCursorPic = qScene->addPixmap (QPixmap ("icons/colorcursor.png"));
			qCursorPic->setPos (x, y);
		}
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ColorSelectDialog::drawColorInfo () {
	color* col = getColor (dSelColor);
	
	if (dSelColor == -1 || !col) {
		qColorInfo->setText ("---");
		return;
	}
	
	qColorInfo->setText (str::mkfmt ("%d - %s",
		dSelColor, col->zName.chars()));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ColorSelectDialog::mousePressEvent (QMouseEvent* event) {
	QPointF qPoint = qView->mapToScene (event->pos ());
	
	ulong x = ((ulong)qPoint.x () - (g_dSquareSize / 2)) / g_dSquareSize;
	ulong y = ((ulong)qPoint.y () - (g_dSquareSize / 2)) / g_dSquareSize;
	ulong idx = (y * g_dNumColumns) + x;
	
	color* col = getColor (idx);
	if (!col)
		return;
	
	dSelColor = idx;
	drawScene ();
	drawColorInfo ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool ColorSelectDialog::staticDialog (short& dValue, short dDefault, QWidget* parent) {
	ColorSelectDialog dlg (dDefault, parent);
	
	if (dlg.exec () && dlg.dSelColor != -1) {
		dValue = dlg.dSelColor;
		return true;
	}
	
	return false;
}