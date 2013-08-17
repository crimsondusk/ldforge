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
#include "colors.h"
#include "file.h"
#include "misc.h"
#include "gui.h"
#include "ldconfig.h"
#include <QColor>

static LDColor* g_LDColors[MAX_COLORS];

void initColors() {
	LDColor* col;
	print ("%1: initializing color information.\n", __func__);
	
	// Always make sure there's 16 and 24 available. They're special like that.
	col = new LDColor;
	col->hexcode = "#AAAAAA";
	col->faceColor = col->hexcode;
	col->edgeColor = Qt::black;
	g_LDColors[maincolor] = col;
	
	col = new LDColor;
	col->hexcode = "#000000";
	col->edgeColor = col->faceColor = Qt::black;
	g_LDColors[edgecolor] = col;
	
	parseLDConfig();
}

// =============================================================================
// -----------------------------------------------------------------------------
LDColor* getColor (short colnum) {
	// Check bounds
	if (colnum < 0 || colnum >= MAX_COLORS)
		return null;
	
	return g_LDColors[colnum];
}

// =============================================================================
// -----------------------------------------------------------------------------
void setColor (short colnum, LDColor* col) {
	if (colnum < 0 || colnum >= MAX_COLORS)
		return;
	
	g_LDColors[colnum] = col;
}

// =============================================================================
uchar luma (QColor& col) {
	return (0.2126f * col.red()) +
	       (0.7152f * col.green()) +
	       (0.0722f * col.blue());
}