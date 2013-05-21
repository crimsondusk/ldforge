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

#include <stdexcept>
#include "common.h"
#include "string.h"

str fmt (const char* fmtstr, ...) {
	va_list va;
	
	va_start (va, fmtstr);
	char* buf = dynafmt (fmtstr, va, 256);
	va_end (va);
	
	str msg = buf;
	delete[] buf;
	return msg;
}

char* dynafmt (const char* fmtstr, va_list va, ulong size) {
	char* buf = null;
	ushort run = 0;
	
	do {
		try {
			buf = new char[size];
		} catch (std::bad_alloc&) {
			fprintf (stderr, "%s: allocation error on run #%u: tried to allocate %lu bytes\n", __func__, run + 1, size);
			abort ();
		}
		
		if (!vsnprintf (buf, size - 1, fmtstr, va)) {
			delete[] buf;
			buf = null;
		}
		
		size *= 2;
		++run;
	} while (buf == null);
	
	return buf;
}

void String::trim (short n) {
	if (n > 0)
		for (short i = 0; i < n; ++i)
			m_string.erase (m_string.end () - 1 - i);
	else
		for (short i = abs (n) - 1; i >= 0; ++i)
			m_string.erase (m_string.begin () + i);
}

String String::strip (std::initializer_list<char> unwanted) {
	String copy (m_string);
	
	for (char c : unwanted) {
		for (long i = len (); i >= 0; --i)
			if (copy[(size_t) i] == c)
				copy.erase (i);
	}
	
	return copy;
}

String String::upper() const {
	String newstr = m_string;
	
	for (char& c : newstr)
		if (c >= 'a' && c <= 'z')
			c -= 'a' - 'A';
	
	return newstr;
}

String String::lower () const {
	String newstr = m_string;

	for (char& c : newstr)
		if (c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
	
	return newstr;
}

vector<String> String::split (char del) const {
	String delimstr;
	delimstr += del;
	return split (delimstr);
}

vector<String> String::split (String del) const {
	std::vector<String> res;
	size_t a = 0;
	
	// Find all separators and store the text left to them.
	while (1) {
		size_t b = first (del, a);
		
		if (b == npos)
			break;
		
		String sub = substr (a, b);
		if (~sub > 0)
			res.push_back (substr (a, b));
		
		a = b + strlen (del);
	}
	
	// Add the string at the right of the last separator
	if (a < len ())
		res.push_back (substr (a, len()));
	
	return res;
}

void String::replace (const char* a, const char* b) {
	size_t pos;
	
	while ((pos = first (a)) != npos)
		m_string = m_string.replace (pos, strlen (a), b);
}

void String::format (const char* fmtstr, ...) {
	va_list va;
	
	va_start (va, fmtstr);
	char* buf = dynafmt (fmtstr, va, 256);
	va_end (va);
	
	m_string = buf;
	delete[] buf;
}

ushort String::count (const char needle) const {
	ushort numNeedles = 0;
	
	for (const char& c : m_string)
		if (c == needle)
			numNeedles++;
	
	return numNeedles;
}

String String::substr (long a, long b) const {
	if (b == -1)
		b = len ();
	
	str sub;
	
	try {
		sub = m_string.substr (a, b - a);
	} catch (const std::out_of_range& e) {
		printf ("%s: %s: caught std::out_of_range, coords were: (%ld, %ld), string: `%s', length: %lu\n",
			__func__, e.what (), a, b, chars (), (ulong) len ());
		abort ();
	}
	
	return sub;
}

// ============================================================================
int String::first (const char* c, int a) const {
	unsigned int r = 0;
	unsigned int index = 0;
	for (; a < (int) len (); a++) {
		if (m_string[a] == c[r]) {
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
int String::last (const char* c, int a) const {
	if (a == -1)
		a = len();
	
	int max = strlen (c) - 1;
	
	int r = max;
	for (; a >= 0; a--) {
		if (m_string[a] == c[r]) {
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