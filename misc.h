#ifndef __MISC_H__
#define __MISC_H__

#include "common.h"
#include "str.h"

inline str GetWord (str& zString, ulong ulIndex) {
	return (zString / " ")[ulIndex];
}

double getWordFloat (str& s, const ushort n);
long getWordInt (str& s, const ushort n);
vertex parseVertex (str& s, const ushort n);
void stripWhitespace (str& s);
bool isNumber (str& zToken);

// Float to string
str ftoa (double fCoord);

#endif // __MISC_H__