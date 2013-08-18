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

#ifndef MISC_H
#define MISC_H

#include "config.h"
#include "common.h"
#include "types.h"

#define NUM_PRIMES 500

class QColor;
class QAction;

// Prime numbers
extern const ushort g_primes[NUM_PRIMES];

// Returns whether a given string represents a floating point number.
bool isNumber (const str& tok);

// Converts a float value to a string value.
str ftoa (double num);

double atof (str val);

// Simplifies the given fraction.
void simplify (short& numer, short& denom);

str join (initlist<StringFormatArg> vals, str delim = " ");

// Grid stuff
typedef struct {
	const char* const name;
	floatconfig* const confs[4];
} gridinfo;

extern_cfg (int, grid);
static const short g_NumGrids = 3;
extern const gridinfo g_GridInfo[3];

inline const gridinfo& currentGrid() {
	return g_GridInfo[grid];
}

// =============================================================================
enum RotationPoint
{
	ObjectOrigin,
	WorldOrigin,
	CustomPoint
};

vertex rotPoint (const List<LDObject*>& objs);
void configRotationPoint();

template<class T, class R> R container_cast (const T& a) {
	R b;
	
	for (auto i : a)
		b << i;
	
	return b;
}

// =============================================================================
namespace Grid {
	enum Type {
		Coarse,
		Medium,
		Fine
	};
	
	enum Config {
		X,
		Y,
		Z,
		Angle
	};
	
	double snap (double value, const Grid::Config axis);
};

// =============================================================================
template<class T> void dataswap (T& a, T& b) {
	T c = a;
	a = b;
	b = c;
}

// -----------------------------------------------------------------------------
// Plural expression
template<class T> static inline const char* plural (T n) {
	return (n != 1) ? "s" : "";
}

// -----------------------------------------------------------------------------
// Templated clamp
template<class T> static inline T clamp (T a, T min, T max) {
	return (a > max) ? max : (a < min) ? min : a;
}

// Templated minimum
template<class T> static inline T min (T a, T b) {
	return (a < b) ? a : b;
}

// Templated maximum
template<class T> static inline T max (T a, T b) {
	return (a > b) ? a : b;
}

// Templated absolute value
template<class T> static inline T abs (T a) {
	return (a >= 0) ? a : -a;
}

#endif // MISC_H