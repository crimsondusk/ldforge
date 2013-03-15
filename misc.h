#ifndef __MISC_H__
#define __MISC_H__

#include "common.h"
#include "str.h"

inline str GetWord (str& zString, ulong ulIndex) {
	return (zString / " ")[ulIndex];
}

double GetWordFloat (str& s, int n);
long GetWordInt (str& s, int n);
vertex ParseVertex (str& s, int n);
void StripWhitespace (str& s);

#endif // __MISC_H__