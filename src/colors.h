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

#ifndef COLORS_H
#define COLORS_H

#include <qcolor.h>
#include "common.h"

#define MAX_COLORS 512

class LDColor {
public:
	str name, hexcode;
	QColor faceColor, edgeColor;
	short index;
};

void initColors();
uchar luma (QColor& col);

// Safely gets a color with the given number or null if no such color.
LDColor* getColor (short colnum);
void setColor (short colnum, LDColor* col);

// Main and edge color identifiers
static const short maincolor = 16;
static const short edgecolor = 24;

#endif // COLORS_H