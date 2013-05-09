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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "str.h"
#include "common.h"
#include "misc.h"

#define ITERATE_STRING(u) \
	for (uint u = 0; u < strlen (m_text); u++)

// ============================================================================
// vdynformat: Try to write to a formatted string with size bytes first, if
// that fails, double the size and keep recursing until it works.
char* vdynformat (const char* fmtstr, va_list va, long size) {
	char* buffer = new char[size];
	int r = vsnprintf (buffer, size - 1, fmtstr, va);
	if (r > (signed)(size - 1) || r < 0) {
		delete[] buffer;
		buffer = vdynformat (fmtstr, va, size * 2);
	}
	return buffer;
}

// ============================================================================
str::str () {
	m_text = new char[1];
	clear();
	m_allocated = strlen (m_text);
}

str::str (const char* c) {
	m_text = new char[1];
	m_text[0] = '\0';
	m_writepos = m_allocated = 0;
	append (c);
}

str::str (char c) {
	m_text = new char[1];
	m_text[0] = '\0';
	m_writepos = m_allocated = 0;
	append (c);
}

str::str (QString c) {
	m_text = new char[1];
	m_text[0] = '\0';
	m_writepos = m_allocated = 0;
	append (c);
}

str::~str () {
	// delete[] text;
}

// ============================================================================
void str::clear () {
	delete[] m_text;
	m_text = new char[1];
	m_text[0] = '\0';
	m_writepos = 0;
	m_allocated = 0;
}

// ============================================================================
void str::resize (uint len) {
	uint oldlen = strlen (m_text);
	char* oldtext = new char[oldlen];
	strncpy (oldtext, m_text, oldlen);
	
	delete[] m_text;
	m_text = new char[len+1];
	for (uint u = 0; u < len+1; u++)
		m_text[u] = 0;
	strncpy (m_text, oldtext, len);
	delete[] oldtext;
	
	m_allocated = len;
}

// ============================================================================
void str::dump () {
	for (uint u = 0; u <= m_allocated; u++)
		printf ("\t%u. %u (%c)\n", u, m_text[u], m_text[u]);
}

// ============================================================================
// Adds a new character at the end of the string.
void str::append (const char c) {
	// Out of space, thus resize
	if (m_writepos == m_allocated)
		resize (m_allocated + 1);
	m_text[m_writepos] = c;
	m_writepos++;
}

void str::append (const char* c) {
	resize (m_allocated + strlen (c));
	
	for (uint u = 0; u < strlen (c); u++) {
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

// ============================================================================
char* str::chars () {
	return m_text;
}

// ============================================================================
int str::first (const char* c, uint a) {
	uint r = 0;
	uint index = 0;
	for (; a < m_allocated; a++) {
		if (m_text[a] == c[r]) {
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
		if (m_text[a] == c[r]) {
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
str str::substr (uint a, uint b) {
	if (a > len()) a = len();
	if (b > len()) b = len();
	
	if (b == a)
		return "";
	
	if (b < a) {
		printf ("str::substring:: indices %u and %u given, should be the other way around, swapping..\n", a, b);
		
		// Swap the variables
		uint c = a;
		a = b;
		b = c;
	}
	
	char* s = new char[b - a + 1];
	strncpy (s, m_text + a, b - a);
	s[b - a] = '\0';
	
	str other = s;
	delete[] s;
	return other;
}

// ============================================================================
void str::remove (uint idx, uint dellen) {
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
void str::replace (const char* o, const char* n, uint a) {
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
str str::strip (char c) {
	return strip ({c});
}

str str::strip (std::initializer_list<char> unwanted) {
	str cache = m_text;
	uint oldlen = len();
	
	char* buf = new char[oldlen];
	char* bufptr = buf;
	for (uint i = 0; i < oldlen; i++) {
		bool valid = true;
		for (const char* j = unwanted.begin(); j < unwanted.end() && valid; j++)
			if (m_text[i] == *j)
				valid = false;
		
		if (valid)
			*bufptr++ = m_text[i];
	}
	
	*bufptr = '\0';
	assert (bufptr <= buf + oldlen);
	
	str zResult = buf;
	delete[] buf;
	
	return zResult;
}

void str::insert (char* c, uint pos) {
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
		buf[i] = m_text[len() - i - 1];
	buf[len()] = '\0';
	
	str other = buf;
	delete[] buf;
	return other;
}

str str::repeat (int n) {
	assert (n >= 0);
	
	str other;
	for (int i = 0; i < n; i++)
		other += m_text;
	return other;
}

// ============================================================================
bool str::isnumber () {
	ITERATE_STRING (u) {
		// Minus sign as the first character is allowed for negatives
		if (!u && m_text[u] == '-')
			continue;
		
		if (m_text[u] < '0' || m_text[u] > '9')
			return false;
	}
	return true;
}

// ============================================================================
bool str::isword () {
	ITERATE_STRING (u) {
		// lowercase letters
		if (m_text[u] >= 'a' || m_text[u] <= 'z')
			continue;
		
		// uppercase letters
		if (m_text[u] >= 'A' || m_text[u] <= 'Z')
			continue;
		
		return false;
	}
	return true;
}

int str::instanceof (const char* c, uint n) {
	uint r = 0;
	uint index = 0;
	uint x = 0;
	for (uint a = 0; a < m_allocated; a++) {
		if (m_text[a] == c[r]) {
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
	return strcmp (m_text, c);
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
	str n = m_text;
	
	for (uint u = 0; u < len(); u++) {
		if (n[u] >= 'A' && n[u] < 'Z')
			n.m_text[u] += ('a' - 'A');
	}
	
	return n;
}

// ============================================================================
str str::toupper () {
	str n = m_text;
	
	for (uint u = 0; u < len(); u++) {
		if (n[u] >= 'a' && n[u] < 'z')
			n.m_text[u] -= ('a' - 'A');
	}
	
	return n;
}

// ============================================================================
uint str::count (char c) {
	uint n = 0;
	ITERATE_STRING (u)
		if (m_text[u] == c)
			n++;
	return n;
}

uint str::count (char* c) {
	uint r = 0;
	uint tmp = 0;
	ITERATE_STRING (u) {
		if (m_text[u] == c[r]) {
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
	uint a = 0;
	
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
	appendformat ("%s", vrt.stringRep (false).chars());
	return *this;
}

str fmt (const char* fmt, ...) {
	va_list va;
	char* buf;
	
	va_start (va, fmt);
	buf = vdynformat (fmt, va, 256);
	va_end (va);
	
	str val = buf;
	delete[] buf;
	return val;
}