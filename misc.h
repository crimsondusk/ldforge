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

template<class T> bool in (T needle, std::initializer_list<T> haystack) {
	for (T hay : haystack) {
		printf ("%s: %ld <-> %ld\n", __func__, needle, hay);
		if (needle == hay)
			return true;
	}
	
	return false;
}

template<class T> std::vector<T> reverseVector (std::vector<T> in) {
	std::vector<T> out;
	
	for (T stuff : in)
		out.insert (out.begin(), in);
	
	return out;
}

// =============================================================================
// stringparser
//
// String parsing utility
// =============================================================================
class stringparser {
public:
	std::vector<str> zaTokens;
	short dPos;
	
	stringparser (str zInText, char cSeparator);
	
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