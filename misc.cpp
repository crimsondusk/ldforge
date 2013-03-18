#include "common.h"
#include <math.h>

double getWordFloat (str& s, const ushort n) {
	return atof ((s / " ")[n]);
}

long getWordInt (str& s, const ushort n) {
	return atol ((s / " ")[n]);
}

vertex parseVertex (str& s, const ushort n) {
	vertex v;
	v.x = getWordFloat (s, n);
	v.y = getWordFloat (s, n + 1);
	v.z = getWordFloat (s, n + 2);
	
	return v;
}

void stripWhitespace (str& s) {
	str other;
	
	for (size_t i = 0; i < ~s; i++)
		if (s[i] > 32 && s[i] < 127)
			other += s[i];
}

vertex bearing::project (vertex& vSource, ulong ulLength) {
	vertex vDest = vSource;
	
	vDest.x += (cos (fAngle) * ulLength);
	vDest.y += (sin (fAngle) * ulLength);
	vDest.x += (cos (fPitch) * ulLength);
	vDest.z += (sin (fPitch) * ulLength);
	return vDest;
}

// =============================================================================
// str ftoa (double)
//
// Converts a double-precision float to a string value.
// =============================================================================
str ftoa (double fCoord) {
	str zRep = str::mkfmt ("%.3f", fCoord);
	
	// Remove trailing zeroes
	while (zRep[~zRep - 1] == '0')
		zRep -= 1;
	
	// If there was only zeroes in the decimal place, remove
	// the decimal point now.
	if (zRep[~zRep - 1] == '.')
		zRep -= 1;
	
	return zRep;
}

// =============================================================================
// isNumber (str&)
//
// Returns whether a given string represents a floating point number
// TODO: Does LDraw support scientific notation?
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