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
#include <qcolor.h>

static color* g_LDColors[MAX_COLORS];
static bool g_bColorsInit = false;

void initColors () {
	if (g_bColorsInit)
		return;
	
	logf ("%s: initializing color information.\n", __func__);
	
	color* col;
	
	// Always make sure there's 16 and 24 available. They're special like that.
	col = new color;
	col->zColorString = "#AAAAAA";
	col->qColor = col->zColorString.chars ();
	col->qEdge = Qt::black;
	g_LDColors[maincolor] = col;
	
	col = new color;
	col->zColorString = "#000000";
	col->qEdge = col->qColor = Qt::black;
	g_LDColors[edgecolor] = col;
	
	parseLDConfig ();
	
	g_bColorsInit = true;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
color* getColor (short dColorNum) {
	// Check bounds
	if (dColorNum < 0 || dColorNum >= MAX_COLORS)
		return null;
	
	return g_LDColors[dColorNum];
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static bool parseLDConfigTag (StringParser& pars, char const* sTag, str& zVal) {
	short dPos;
	if (!pars.findToken (dPos, sTag, 1))
		return false;
	
	return pars.getToken (zVal, dPos + 1);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
short color::index () {
	short idx = 0;
	for (color* it : g_LDColors) {
		if (it == this)
			return idx;
		idx++;
	}
	
	return -1;
}

// =============================================================================
uchar luma (QColor& col) {
	return (0.2126f * col.red ()) +
		(0.7152f * col.green ()) +
		(0.0722f * col.blue ());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void parseLDConfig () {
	FILE* fp = openLDrawFile ("LDConfig.ldr", false);
	
	if (!fp)
		return;
	
	// Even though LDConfig.ldr is technically an LDraw file, parsing it as one
	// would be overkill by any standard.
	char line[1024];
	while (fgets (line, sizeof line, fp)) {
		if (strlen (line) == 0 || line[0] != '0')
			continue; // empty or illogical
		
		str zLine = line;
		zLine.replace ("\n", "");
		zLine.replace ("\r", "");
		
		StringParser pars (zLine, ' ');
		short dCode = 0, dAlpha = 255;
		str zName, zColor, zEdge, zValue;
		
		// Check 0 !COLOUR, parse the name
		if (!pars.tokenCompare (0, "0") || !pars.tokenCompare (1, "!COLOUR") || !pars.getToken (zName, 2))
			continue;
		
		// Replace underscores in the name with spaces for readability
		zName.replace ("_", " ");
		
		// get the CODE tag
		if (!parseLDConfigTag (pars, "CODE", zValue))
			continue;
		
		// Ensure that the code is within range. must be within 0 - 512
		dCode = atoi (zValue);
		if (dCode < 0 || dCode >= 512)
			continue;
		
		// Don't let LDConfig.ldr override the special colors 16 and 24. However,
		// do take the name it gives for the color
		if (dCode == maincolor || dCode == edgecolor) {
			g_LDColors[dCode]->zName = zName;
			continue;
		}
		
		// VALUE tag
		if (!parseLDConfigTag (pars, "VALUE", zColor))
			continue;
		
		// EDGE tag
		if (!parseLDConfigTag (pars, "EDGE", zEdge))
			continue;
		
		// Ensure that our colors are correct
		QColor qColor (zColor.chars()),
			qEdge (zEdge.chars());
		
		if (!qColor.isValid () || !qEdge.isValid ())
			continue;
		
		// Parse alpha if given.
		if (parseLDConfigTag (pars, "ALPHA", zValue))
			dAlpha = clamp<short> (atoi (zValue), 0, 255);
		
		color* col = new color;
		col->zName = zName;
		col->qColor = qColor;
		col->qEdge = qEdge;
		col->zColorString = zColor;
		col->qColor.setAlpha (dAlpha);
		
		g_LDColors[dCode] = col;
	}
	
	fclose (fp);
}