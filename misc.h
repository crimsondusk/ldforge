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

#ifndef __MISC_H__
#define __MISC_H__

#include "common.h"
#include "str.h"

inline str GetWord (str& zString, ulong ulIndex) {
	return (zString / " ")[ulIndex];
}

void stripWhitespace (str& s);

// Returns whether a given string represents a floating point number
// TODO: Does LDraw support scientific notation?
bool isNumber (str& zToken);

// Converts a float value to a string value.
str ftoa (double fCoord);

template<class T> bool in (T needle, std::initializer_list<T> haystack);

#endif // __MISC_H__