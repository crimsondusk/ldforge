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

// Simplifies the given fraction.
void simplify (short& numer, short& denom);

str join (initlist<StringFormatArg> vals, str delim = " ");

// Grid stuff
typedef struct {
	const char* const name;
	floatconfig* const confs[4];
	QAction** const actionptr;
} gridinfo;

extern_cfg (int, grid);
static const short g_NumGrids = 3;
extern const gridinfo g_GridInfo[3];

inline const gridinfo& currentGrid () {
	return g_GridInfo[grid];
}

// =============================================================================
vertex rotPoint (const vector<LDObject*>& objs);
void configRotationPoint ();

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

// =============================================================================
// StringParser
//
// String parsing utility
// =============================================================================
class StringParser {
public:
	StringParser (str inText, char sep);
	
	bool atEnd ();
	bool atBeginning ();
	bool next (str& val);
	bool peekNext (str& val);
	bool getToken (str& val, const ushort pos);
	bool findToken (short& result, char const* needle, short args);
	size_t size ();
	void rewind ();
	void seek (short amount, bool rel);
	bool tokenCompare (short inPos, const char* sOther);
	
	str operator[] (const size_t idx) {
		return m_tokens[idx];
	}
	
private:
	vector<str> m_tokens;
	short m_pos;
};

#endif // MISC_H