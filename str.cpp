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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
// #include <initializer_list>
#include "str.h"
#include "common.h"
#include "misc.h"

#define ITERATE_STRING(u) \
	for (unsigned int u = 0; u < strlen (text); u++)

// ============================================================================
// vdynformat: Try to write to a formatted string with size bytes first, if
// that fails, double the size and keep recursing until it works.
char* vdynformat (const char* csFormat, va_list vArgs, long lSize) {
	char* buffer = new char[lSize];
	int r = vsnprintf (buffer, lSize - 1, csFormat, vArgs);
	if (r > (signed)(lSize - 1) || r < 0) {
		delete[] buffer;
		buffer = vdynformat (csFormat, vArgs, lSize * 2);
	}
	return buffer;
}

// ============================================================================
str::str () {
	text = new char[1];
	clear();
	alloclen = strlen (text);
}

str::str (const char* c) {
	text = new char[1];
	text[0] = '\0';
	curs = alloclen = 0;
	append (c);
}

str::str (char c) {
	text = new char[1];
	text[0] = '\0';
	curs = alloclen = 0;
	append (c);
}

str::str (QString c) {
	text = new char[1];
	text[0] = '\0';
	curs = alloclen = 0;
	append (c);
}

str::~str () {
	// delete[] text;
}

// ============================================================================
void str::clear () {
	delete[] text;
	text = new char[1];
	text[0] = '\0';
	curs = 0;
	alloclen = 0;
}

// ============================================================================
void str::resize (unsigned int len) {
	unsigned int oldlen = strlen (text);
	char* oldtext = new char[oldlen];
	strncpy (oldtext, text, oldlen);
	
	delete[] text;
	text = new char[len+1];
	for (unsigned int u = 0; u < len+1; u++)
		text[u] = 0;
	strncpy (text, oldtext, len);
	delete[] oldtext;
	
	alloclen = len;
}

// ============================================================================
void str::dump () {
	for (unsigned int u = 0; u <= alloclen; u++)
		printf ("\t%u. %u (%c)\n", u, text[u], text[u]);
}

// ============================================================================
// Adds a new character at the end of the string.
void str::append (const char c) {
	// Out of space, thus resize
	if (curs == alloclen)
		resize (alloclen + 1);
	text[curs] = c;
	curs++;
}

void str::append (const char* c) {
	resize (alloclen + strlen (c));
	
	for (unsigned int u = 0; u < strlen (c); u++) {
		if (c[u] != 0)
			append (c[u]);
	}
}

void str::append (str c) {
	append (c.chars());
}

void str::append (QString c) {
	append (c.toUtf8 ().constData ());
}

// ============================================================================
void str::appendformat (const char* c, ...) {
	va_list v;
	
	va_start (v, c);
	char* buf = vdynformat (c, v, 256);
	va_end (v);
	
	append (buf);
	delete[] buf;
}

void str::format (const char* fmt, ...) {
	clear ();
	
	va_list v;
	
	va_start (v, fmt);
	char* buf = vdynformat (fmt, v, 256);
	va_end (v);
	
	append (buf);
	delete[] buf;
}

str str::format (...) {
	va_list va;
	char* buf;
	
	va_start (va, this);
	buf = vdynformat (text, va, 256);
	va_end (va);
	
	str val = buf;
	delete[] buf;
	return val;
}

// ============================================================================
char* str::chars () {
	return text;
}

// ============================================================================
int str::first (const char* c, unsigned int a) {
	unsigned int r = 0;
	unsigned int index = 0;
	for (; a < alloclen; a++) {
		if (text[a] == c[r]) {
			if (r == 0)
				index = a;
			
			r++;
			if (r == strlen (c))
				return index;
		} else {
			if (r != 0) {
				// If the string sequence broke at this point, we need to
				// check this character again, for a new sequence just
				// might start right here.
				a--;
			}
			
			r = 0;
		}
	}
	
	return -1;
}

// ============================================================================
int str::last (const char* c, int a) {
	if (a == -1)
		a = len();
	
	int max = strlen (c)-1;
	
	int r = max;
	for (; a >= 0; a--) {
		if (text[a] == c[r]) {
			r--;
			if (r == -1)
				return a;
		} else {
			if (r != max)
				a++;
			
			r = max;
		}
	}
	
	return -1;
}

// ============================================================================
str str::substr (unsigned int a, unsigned int b) {
	if (a > len()) a = len();
	if (b > len()) b = len();
	
	if (b == a)
		return "";
	
	if (b < a) {
		printf ("str::substring:: indices %u and %u given, should be the other way around, swapping..\n", a, b);
		
		// Swap the variables
		unsigned int c = a;
		a = b;
		b = c;
	}
	
	char* s = new char[b - a + 1];
	strncpy (s, text + a, b - a);
	s[b - a] = '\0';
	
	str other = s;
	delete[] s;
	return other;
}

// ============================================================================
void str::remove (unsigned int idx, unsigned int dellen) {
	str s1 = substr (0, idx);
	str s2 = substr (idx + dellen, -1);
	
	clear();
	
	append (s1);
	append (s2);
}

// ============================================================================
str str::trim (int dellen) {
	if (dellen > 0)
		return substr (0, len() - dellen);
	return substr (-dellen, len());
}

// ============================================================================
void str::replace (const char* o, const char* n, unsigned int a) {
	for (int idx; (idx = first (o, a)) != -1;) {
		str s1 = substr (0, idx);
		str s2 = substr (idx + strlen (o), len());
		
		clear();
		
		append (s1);
		append (n);
		append (s2);
	}
}

// ============================================================================
// It works otherwise but I'm having trouble with the initializer_list
/*
void str::strip (char c) {
	strip ({c});
}

void str::strip (std::initializer_list<char> unwanted) {
	str cache = text;
	uint oldlen = len();
	
	char* buf = new char[oldlen];
	char* bufptr = buf;
	for (uint i = 0; i < oldlen; i++) {
		bool valid = true;
		for (const char* j = unwanted.begin(); j < unwanted.end() && valid; j++)
			if (text[i] == *j)
				valid = false;
		
		if (valid)
			*bufptr++ = text[i];
	}
	
	*bufptr = '\0';
	assert (bufptr <= buf + oldlen);
	
	clear();
	append (buf);
	
	delete[] buf;
}
*/

void str::insert (char* c, unsigned int pos) {
	str s1 = substr (0, pos);
	str s2 = substr (pos, len());
	
	clear();
	append (s1);
	append (c);
	append (s2);
}

str str::reverse () {
	char* buf = new char[len() + 1];
	
	for (uint i = 0; i < len(); i++)
		buf[i] = text[len() - i - 1];
	buf[len()] = '\0';
	
	str other = buf;
	delete[] buf;
	return other;
}

str str::repeat (int n) {
	assert (n >= 0);
	
	str other;
	for (int i = 0; i < n; i++)
		other += text;
	return other;
}

// ============================================================================
bool str::isnumber () {
	ITERATE_STRING (u) {
		// Minus sign as the first character is allowed for negatives
		if (!u && text[u] == '-')
			continue;
		
		if (text[u] < '0' || text[u] > '9')
			return false;
	}
	return true;
}

// ============================================================================
bool str::isword () {
	ITERATE_STRING (u) {
		// lowercase letters
		if (text[u] >= 'a' || text[u] <= 'z')
			continue;
		
		// uppercase letters
		if (text[u] >= 'A' || text[u] <= 'Z')
			continue;
		
		return false;
	}
	return true;
}

int str::instanceof (const char* c, uint n) {
	unsigned int r = 0;
	unsigned int index = 0;
	unsigned int x = 0;
	for (uint a = 0; a < alloclen; a++) {
		if (text[a] == c[r]) {
			if (r == 0)
				index = a;
			
			r++;
			if (r == strlen (c)) {
				if (x++ == n)
					return index;
				r = 0;
			}
		} else {
			if (r != 0)
				a--;
			r = 0;
		}
	}
	
	return -1;
}

// ============================================================================
int str::compare (const char* c) {
	return strcmp (text, c);
}

int str::compare (str c) {
	return compare (c.chars());
}

int str::icompare (const char* c) {
	return icompare (str ((char*)c));
}

int str::icompare (str b) {
	return strcmp (tolower().chars(), b.tolower().chars());
}

// ============================================================================
str str::tolower () {
	str n = text;
	
	for (uint u = 0; u < len(); u++) {
		if (n[u] >= 'A' && n[u] < 'Z')
			n.text[u] += ('a' - 'A');
	}
	
	return n;
}

// ============================================================================
str str::toupper () {
	str n = text;
	
	for (uint u = 0; u < len(); u++) {
		if (n[u] >= 'a' && n[u] < 'z')
			n.text[u] -= ('a' - 'A');
	}
	
	return n;
}

// ============================================================================
unsigned str::count (char c) {
	unsigned n = 0;
	ITERATE_STRING (u)
		if (text[u] == c)
			n++;
	return n;
}

unsigned str::count (char* c) {
	unsigned int r = 0;
	unsigned int tmp = 0;
	ITERATE_STRING (u) {
		if (text[u] == c[r]) {
			r++;
			if (r == strlen (c)) {
				r = 0;
				tmp++;
			}
		} else {
			if (r != 0)
				u--;
			r = 0;
		}
	}
	
	return tmp;
}

// ============================================================================
std::vector<str> str::split (str del, bool bNoBlanks) {
	std::vector<str> res;
	unsigned int a = 0;
	
	// Find all separators and store the text left to them.
	while (1) {
		int b = first (del, a);
		
		if (b == -1)
			break;
		
		if (!bNoBlanks || (b - a))
			res.push_back (substr (a, b));
		
		a = b + strlen (del);
	}
	
	// Add the string at the right of the last separator
	if (!bNoBlanks || (len () - a))
		res.push_back (substr (a, len ()));
	return res;
}

std::vector<str> str::operator/ (str splitstring) {return split(splitstring);}
std::vector<str> str::operator/ (char* splitstring) {return split(splitstring);}
std::vector<str> str::operator/ (const char* splitstring) {return split(splitstring);}

str& str::operator+= (vertex vrt) {
	appendformat ("%s", vrt.getStringRep (false).chars());
	return *this;
}