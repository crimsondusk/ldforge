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

#ifndef __COLORS_H__
#define __COLORS_H__

#include <qcolor.h>
#include "common.h"

#define MAX_COLORS 512

class color {
public:
	str zName, zColorString;
	QColor qColor, qEdge;
	
	short index ();
};

typedef struct {
	const short dIndex;
	const char* sName, *sColor;
	const float fAlpha;
} TemporaryColorMeta;

void initColors ();
void parseLDConfig ();

// Safely gets a color with the given number or nullptr if no such color.
color* getColor (short dColorNum);

#endif // __COLORS_H__