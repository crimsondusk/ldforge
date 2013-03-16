#include "common.h"
#include <math.h>

double GetWordFloat (str& s, const ushort n) {
	return atof ((s / " ")[n]);
}

long GetWordInt (str& s, const ushort n) {
	return atol ((s / " ")[n]);
}

vertex ParseVertex (str& s, const ushort n) {
	vertex v;
	v.x = GetWordFloat (s, n);
	v.y = GetWordFloat (s, n + 1);
	v.z = GetWordFloat (s, n + 2);
	
	return v;
}

void StripWhitespace (str& s) {
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