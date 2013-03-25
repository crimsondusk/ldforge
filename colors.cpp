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

#include "common.h"
#include "colors.h"

// Placeholder static color table until I make an LDConfig.ldr parser
static TemporaryColorMeta g_LDColorInfo[] = {
	{0,		"Black",		"#101010",	1.0},
	{1,		"Blue",			"#0000FF",	1.0},
	{2,		"Green",		"#008000",	1.0},
	{3,		"Teal",			"#008080",	1.0},
	{4,		"Red",			"#C00000",	1.0},
	{5,		"Dark pink",	"#C00060",	1.0},
	{6,		"Brown",		"#604000",	1.0},
	{7,		"Gray",			"#989890",	1.0},
	{8,		"Dark Gray",	"#6E6D62",	1.0},
	{9,		"Light Blue",	"#60A0C0",	1.0},
	{10,	"Bright Green",	"#40C040",	1.0},
	{11,	"Cyan",			"#00FFFF",	1.0},
	{12,	"Salmon",		"#FF8080",	1.0},
	{13,	"Pink",			"#FF2080",	1.0},
	{14,	"Yellow",		"#FFEE00",	1.0},
	{15,	"White",		"#FFFFFF",	1.0},
	{16,	"Main Color",	"#808080",	1.0},
	{17,	"Light Green",	"#80FF80",	1.0},
	{18,	"Light Yellow",	"#FFFF80",	1.0},
	{19,	"Tan",			"#EECC99",	1.0},
	{21,	"Phosphorus",	"#E0FFB0",	0.975},
	{22,	"Purple",		"#A000A0",	1.0},
	{24,	"Edge Color",	"#000000",	1.0},
	{25,	"Orange",		"#FF8000",	1.0},
	{26,	"Magenta",		"#FFA0FF",	1.0},
	{27,	"Lime",			"#00FF00",	1.0},
	{28,	"Sand",			"#989070",	1.0},
	{32,	"Lens Black",	"#101010",	0.8},
	{33,	"Trans Blue",	"#0000FF",	0.5},
	{34,	"Trans Green",	"#008000",	0.5},
	{35,	"Trans Teal",	"#008080",	0.5},
	{36,	"Trans Red",	"#C00000",	0.5},
	{37,	"Trans Dk Pink",	"#C00060",	0.5},
	{38,	"Trans Brown",	"#604000",	0.5},
	{39,	"Trans Gray",	"#989890",	0.5},
	{40,	"Smoke",		"#6E6D62",	0.5},
	{41,	"Trans Lt Blue",	"#60A0C0",	0.5},
	{42,	"Trans Bt Green",	"#40C040",	0.5},
	{43,	"Trans Cyan",	"#00FFFF",	0.5},
	{44,	"Trans Salmon",	"#FF8080",	0.5},
	{45,	"Trans Pink",	"#FF2080",	0.5},
	{46,	"Trans Yellow",	"#FFEE00",	0.5},
	{47,	"Clear",		"#FFFFFF",	0.5},
	{71,	"Medium Stone",	"#A0A0AA",	1.0},
	{72,	"Dark Stone",	"#60606A",	1.0},
	{79,	"Ghost White",	"#FFFFFF",	0.875},
	{294,	"Trans Phosphorus",	"#E0FFB0",	0.6},
	{378,	"Sand Green",	"#80A080",	1.0},
	{511,	"Rubber White",	"#F8F8F8",	1.0},
};

static color* g_LDColors[MAX_COLORS];
static bool g_bColorsInit = false;

void initColors () {
	if (g_bColorsInit)
		return;
	
	memset (g_LDColors, 0, sizeof g_LDColors);
	for (ulong i = 0; i < sizeof g_LDColorInfo / sizeof *g_LDColorInfo; ++i) {
		color* col = new color;
		col->zColor = g_LDColorInfo[i].sColor;
		col->zName = g_LDColorInfo[i].sName;
		col->fAlpha = g_LDColorInfo[i].fAlpha;
		
		g_LDColors[g_LDColorInfo[i].dIndex] = col;
	}
	
	g_bColorsInit = true;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
color* getColor (short dColorNum) {
	// Check bounds
	if (dColorNum < 0 || dColorNum >= MAX_COLORS)
		return nullptr;
	
	return g_LDColors[dColorNum];
}