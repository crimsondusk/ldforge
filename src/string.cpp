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

String::String () {}

String::String (const char* data) {
	m_string = data;
}

String::String (const QString data) {
	m_string = data.toStdString ();
}

String::String (std::string data) {
	m_string = data;
}

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
			// fmt uses dynafmt, so using fmt here is dangerous and could lead
			// into infinite recursion. Thus, use a C string this one time.
			char err[256];
			sprintf (err, "caught std::bad_alloc on run #%u while trying to allocate %lu bytes", run + 1, size);
			fatal (err);
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

void String::append (const char* data) {
	m_string.append (data);
}

void String::append (const char data) {
	m_string.push_back (data);
}

void String::append (const String data) {
	m_string.append (data.chars ());
}

String::it String::begin () {
	return m_string.begin ();
}

String::c_it String::begin () const {
	return m_string.cbegin ();
}

const char* String::c () const {
	return chars ();
}

size_t String::capacity () const {
	return m_string.capacity ();
}

const char* String::chars () const {
	return m_string.c_str ();
}

int String::compare (const char* other) const {
	return m_string.compare (other);
}

int String::compare (String other) const {
	return m_string.compare (other);
}

String::it String::end () {
	return m_string.end ();
}

String::c_it String::end () const {
	return m_string.end ();
}

void String::clear () {
	m_string.clear ();
}

bool String::empty() const {
	return m_string.empty ();
}

void String::erase (size_t pos) {
	m_string.erase (m_string.begin () + pos);
}

void String::insert (size_t pos, char c) {
	m_string.insert (m_string.begin () + pos, c);
}

size_t String::len () const {
	return m_string.length ();
}

size_t String::maxSize () const {
	return m_string.max_size ();
}

void String::resize (size_t n) {
	m_string.resize (n);
}

void String::shrinkToFit () {
	m_string.shrink_to_fit ();
}



void String::trim (short n) {
	if (n > 0)
		for (short i = 0; i < n; ++i)
			m_string.erase (m_string.end () - 1 - i);
	else
		for (short i = abs (n) - 1; i >= 0; ++i)
			m_string.erase (m_string.begin () + i);
}

String String::strip (char unwanted) {
	return strip ({unwanted});
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
	vector<String> res;
	size_t a = 0;
	
	// Find all separators and store the text left to them.
	while (1) {
		long b = first (del, a);
		
		if (b == -1)
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
	long pos;
	
	while ((pos = first (a)) != -1)
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
		fatal (fmt ("caught std::out_of_range, coords were: (%ld, %ld), string: `%s', length: %lu",
			a, b, chars (), (ulong) len ()));
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

String String::operator+ (const String data) const {
	String newstr = *this;
	newstr += data;
	return newstr;
}

String String::operator+ (const char* data) const {
	String newstr = *this;
	newstr += data;
	return newstr;
}

String& String::operator+= (const String data) {
	append (data);
	return *this;
}

String& String::operator+= (const char* data) {
	append (data);
	return *this;
}

String& String::operator+= (const char data) {
	append (data);
	return *this;
}

String String::operator+ () const {
	return upper ();
}

String String::operator- () const {
	return lower ();
}

String String::operator- (size_t n) const {
	String newstr = m_string;
	newstr -= n;
	return newstr;
}

String& String::operator-= (size_t n) {
	trim (n);
	return *this;
}

size_t String::operator~ () const {
	return len ();
}

vector<String> String::operator/ (String del) const {
	return split (del);
}

char& String::operator[] (size_t n) {
	return m_string[n];
}

const char& String::operator[] (size_t n) const {
	return m_string[n];
}

bool String::operator== (const String other) const {
	return compare (other) == 0;
}

bool String::operator== (const char* other) const {
	return compare (other) == 0;
}

bool String::operator!= (const String other) const {
	return compare (other) != 0;
}

bool String::operator!= (const char* other) const {
	return compare (other) != 0;
}

bool String::operator! () const {
	return empty ();
}

String::operator const char* () const {
	return chars ();
}

String::operator QString () {
	return chars ();
}

String::operator const QString () const {
	return chars ();
}

str String::join (const vector<str>& items, const str& delim) {
	str text;
	
	for (const str& item : items) {
		if (item != items[0])
			text += delim;
		
		text += item;
	}
	
	return text;
}