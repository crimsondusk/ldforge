/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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
 *  =====================================================================
 *
 *  colorSelectDialog.cpp: Color selector box.
 */

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QScrollBar>

#include "Main.h"
#include "MainWindow.h"
#include "ColorSelector.h"
#include "Colors.h"
#include "Configuration.h"
#include "Misc.h"
#include "ui_colorsel.h"
#include "moc_ColorSelector.cpp"

static const int g_numColumns = 16;
static const int g_squareSize = 32;

extern_cfg (String, gl_maincolor);
extern_cfg (Float, gl_maincolor_alpha);

// =============================================================================
// -----------------------------------------------------------------------------
ColorSelector::ColorSelector (int defval, QWidget* parent) : QDialog (parent)
{
	// Remove the default color if it's invalid
	if (!getColor (defval))
		defval = -1;

	m_firstResize = true;
	ui = new Ui_ColorSelUI;
	ui->setupUi (this);

	m_scene = new QGraphicsScene;
	ui->viewport->setScene (m_scene);
	setSelection (getColor (defval));

	// not really an icon but eh
	m_scene->setBackgroundBrush (getIcon ("checkerboard"));
	drawScene();

	int width = viewportWidth();
	ui->viewport->setMinimumWidth (width);
	ui->viewport->setMaximumWidth (width);

	drawColorInfo();
}

// =============================================================================
// -----------------------------------------------------------------------------
ColorSelector::~ColorSelector()
{
	delete ui;
}

// =============================================================================
// -----------------------------------------------------------------------------
void ColorSelector::drawScene()
{
	const int numCols = g_numColumns;
	const int square = g_squareSize;
	const int g_maxHeight = (numRows() * square);
	QRect sceneRect (0, 0, viewportWidth(), g_maxHeight);

	m_scene->setSceneRect (sceneRect);
	ui->viewport->setSceneRect (sceneRect);

	const double penWidth = 1.0f;

	// Draw the color rectangles.
	m_scene->clear();

	for (int i = 0; i < MAX_COLORS; ++i)
	{
		LDColor* info = ::getColor (i);

		if (!info)
			continue;

		const double x = (i % numCols) * square;
		const double y = (i / numCols) * square;
		const double w = square - (penWidth / 2);

		QColor col = info->faceColor;

		if (i == maincolor)
		{
			// Use the user preferences for main color here
			col = QColor (gl_maincolor);
			col.setAlpha (gl_maincolor_alpha * 255.0f);
		}

		QPen pen (info->edgeColor, penWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
		m_scene->addRect (x, y, w, w, pen, col);
		QGraphicsTextItem* numtext = m_scene->addText (fmt ("%1", i));
		numtext->setDefaultTextColor ( (luma (col) < 80) ? Qt::white : Qt::black);
		numtext->setPos (x, y);

		if (getSelection() && i == getSelection()->index)
		{
			auto curspic = m_scene->addPixmap (getIcon ("colorcursor"));
			curspic->setPos (x, y);
		}
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
int ColorSelector::numRows() const
{
	return (MAX_COLORS / g_numColumns);
}

// =============================================================================
// -----------------------------------------------------------------------------
int ColorSelector::viewportWidth() const
{
	return g_numColumns * g_squareSize + 21;
}

// =============================================================================
// -----------------------------------------------------------------------------
void ColorSelector::drawColorInfo()
{
	if (!getSelection())
	{
		ui->colorLabel->setText ("---");
		return;
	}

	ui->colorLabel->setText (fmt ("%1 - %2", getSelection()->index, getSelection()->name));
}

// =============================================================================
// -----------------------------------------------------------------------------
void ColorSelector::resizeEvent (QResizeEvent* ev)
{
	// If this is the first resize, check if we need to scroll down to see the
	// currently selected color. We cannot do this in the constructor because the
	// height is not set properly there.
	if (m_firstResize)
	{
		int visibleColors = (ui->viewport->height() / g_squareSize) * g_numColumns;

		if (getSelection() && getSelection()->index >= visibleColors)
		{
			int y = (getSelection()->index / g_numColumns) * g_squareSize;
			ui->viewport->verticalScrollBar()->setValue (y);
		}

		m_firstResize = false;
	}

	(void) ev;
	drawScene();
}

// =============================================================================
// -----------------------------------------------------------------------------
void ColorSelector::mousePressEvent (QMouseEvent* event)
{
	QPointF scenepos = ui->viewport->mapToScene (event->pos());

	int x = (scenepos.x() - (g_squareSize / 2)) / g_squareSize;
	int y = (scenepos.y() - (g_squareSize / 2)) / g_squareSize;
	int idx = (y * g_numColumns) + x;

	LDColor* col = ::getColor (idx);

	if (!col)
		return;

	setSelection (col);
	drawScene();
	drawColorInfo();
}

// =============================================================================
// -----------------------------------------------------------------------------
bool ColorSelector::selectColor (int& val, int defval, QWidget* parent)
{
	ColorSelector dlg (defval, parent);

	if (dlg.exec() && dlg.getSelection() != null)
	{
		val = dlg.getSelection()->index;
		return true;
	}

	return false;
}