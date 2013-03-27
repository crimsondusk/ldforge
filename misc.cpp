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

#include <math.h>
#include <locale.h>
#include "common.h"
#include "misc.h"

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void stripWhitespace (str& s) {
	str other;
	
	for (size_t i = 0; i < ~s; i++)
		if (s[i] > 32 && s[i] < 127)
			other += s[i];
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str ftoa (double fCoord) {
	// Disable the locale first so that the decimal point will not
	// turn into anything weird (like commas)
	setlocale (LC_NUMERIC, "C");
	
	str zRep = str::mkfmt ("%f", fCoord);
	
	// Remove trailing zeroes
	while (zRep[~zRep - 1] == '0')
		zRep -= 1;
	
	// If there was only zeroes in the decimal place, remove
	// the decimal point now.
	if (zRep[~zRep - 1] == '.')
		zRep -= 1;
	
	// Reset the locale
	setlocale (LC_NUMERIC, "");
	return zRep;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool isNumber (str& zToken) {
	char* cpPointer = &zToken[0];
	bool bGotDot = false;
	
	// Allow leading hyphen for negatives
	if (*cpPointer == '-')
		cpPointer++;
	
	while (*cpPointer != '\0') {
		if (*cpPointer == '.' && !bGotDot) {
			// Decimal point
			bGotDot = true;
			cpPointer++;
			continue;
		}
		
		if (*cpPointer >= '0' && *cpPointer <= '9') {
			cpPointer++;
			continue; // Digit
		}
		
		// If the above cases didn't catch this character, it was
		// illegal and this is therefore not a number.
		return false;
	}
	
	return true;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
stringparser::stringparser (str zInText, char cSeparator) {
	zaTokens = zInText.split (cSeparator, true);
	dPos = -1;
}

// -----------------------------------------------------------------------------
bool stringparser::atBeginning () {
	return (dPos == -1);
}

// -----------------------------------------------------------------------------
bool stringparser::atEnd () {
	return (dPos == zaTokens.size() - 1);
}

// -----------------------------------------------------------------------------
bool stringparser::getToken (str& zVal, const ushort uInPos) {
	if (uInPos < 0 || uInPos >= zaTokens.size())
		return false;
	
	zVal = zaTokens[uInPos];
	return true;
}

// -----------------------------------------------------------------------------
bool stringparser::next (str& zVal) {
	return getToken (zVal, ++dPos);
}

// -----------------------------------------------------------------------------
bool stringparser::peekNext (str& zVal) {
	return getToken (zVal, dPos + 1);
}

// -----------------------------------------------------------------------------
bool stringparser::findToken (short& dResult, char const* sNeedle, short dArgs) {
	for (short i = 0; i < (zaTokens.size() - dArgs); ++i) {
		if (zaTokens[i] == sNeedle) {
			dResult = i;
			return true;
		}
	}
	
	return false;
}

// -----------------------------------------------------------------------------
void stringparser::rewind () {
	dPos = -1;
}

// -----------------------------------------------------------------------------
void stringparser::seek (short int dAmount, bool bRelative) {
	dPos = (bRelative ? dPos : 0) + dAmount;
}

// -----------------------------------------------------------------------------
size_t stringparser::size () {
	return zaTokens.size();
}

// -----------------------------------------------------------------------------
bool stringparser::tokenCompare (short int dInPos, const char* sOther) {
	str tok;
	if (!getToken (tok, dInPos))
		return false;
	
	return (tok == sOther);
}