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
 *  colors.cpp: LDraw color management. LDConfig.ldr parsing is not here!
 *  TODO: Make LDColor more full-fledged, add support for direct colors.
 *  TODO: g_LDColors should probably be a map.
 */

#include "main.h"
#include "colors.h"
#include "document.h"
#include "misc.h"
#include "gui.h"
#include "ldconfig.h"
#include <QColor>

static LDColor* g_LDColors[MAX_COLORS];

// =============================================================================
// -----------------------------------------------------------------------------
void initColors()
{
	LDColor* col;
	log ("%1: initializing color information.\n", __func__);

	// Always make sure there's 16 and 24 available. They're special like that.
	col = new LDColor;
	col->faceColor = col->hexcode = "#AAAAAA";
	col->edgeColor = Qt::black;
	g_LDColors[maincolor] = col;

	col = new LDColor;
	col->faceColor = col->edgeColor = col->hexcode = "#000000";
	g_LDColors[edgecolor] = col;

	parseLDConfig();
}

// =============================================================================
// -----------------------------------------------------------------------------
LDColor* getColor (int colnum)
{
	// Check bounds
	if (colnum < 0 || colnum >= MAX_COLORS)
		return null;

	return g_LDColors[colnum];
}

// =============================================================================
// -----------------------------------------------------------------------------
void setColor (int colnum, LDColor* col)
{
	if (colnum < 0 || colnum >= MAX_COLORS)
		return;

	g_LDColors[colnum] = col;
}

// =============================================================================
// -----------------------------------------------------------------------------
int luma (QColor& col)
{
	return (0.2126f * col.red()) +
		   (0.7152f * col.green()) +
		   (0.0722f * col.blue());
}
