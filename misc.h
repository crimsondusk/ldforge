#ifndef __MISC_H__
#define __MISC_H__

#include "common.h"
#include "str.h"

inline str GetWord (str& zString, ulong ulIndex) {
	return (zString / " ")[ulIndex];
}

double GetWordFloat (str& s, const ushort n);
long GetWordInt (str& s, const ushort n);
vertex ParseVertex (str& s, const ushort n);
void StripWhitespace (str& s);

// Float to string
str ftoa (double fCoord);

#endif // __MISC_H__