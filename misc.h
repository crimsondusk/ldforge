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

#ifndef __MISC_H__
#define __MISC_H__

#include "common.h"
#include "str.h"

#define NUM_PRIMES 500

// Prime numbers
extern const ushort g_uaPrimes[NUM_PRIMES];

// Returns whether a given string represents a floating point number.
bool isNumber (str& zToken);

// Converts a float value to a string value.
str ftoa (double fCoord);

// Simplifies the given fraction.
void simplify (short& dNum, short& dDenom);

// Grid stuff
typedef struct {
	const char* const name;
	floatconfig* const confs[4];
} gridinfo;

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
};

extern_cfg (int, grid);
static const short g_NumGrids = 3;
extern const gridinfo g_GridInfo[3];

inline const gridinfo& currentGrid () {
	return g_GridInfo[grid];
}

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
	std::vector<str> zaTokens;
	short dPos;
	
	StringParser (str zInText, char cSeparator);
	
	bool atEnd ();
	bool atBeginning ();
	bool next (str& zVal);
	bool peekNext (str& zVal);
	bool getToken (str& zVal, const ushort uInPos);
	bool findToken (short& dResult, char const* sNeedle, short dArgs);
	size_t size ();
	void rewind ();
	void seek (short dAmount, bool bRelative);
	bool tokenCompare (short int dInPos, const char* sOther);
	
	str operator[] (const size_t uIndex) {
		return zaTokens[uIndex];
	}
};

#endif // __MISC_H__