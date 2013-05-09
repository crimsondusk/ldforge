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

#ifndef STR_H
#define STR_H

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <vector>
#include <QString>

char* vdynformat (const char* fmtstr, va_list va, long size);

class vertex;

// Dynamic string object, allocates memory when needed and
// features a good bunch of manipulation methods
class str {
private:
	char* m_text;
	ushort m_writepos;
	ushort m_allocated;
	void resize (uint len);
	
public:
	// ======================================================================
	str ();
	str (const char* c);
	str (char c);
	str (QString c);
	~str ();
	
	void clear ();
	size_t len () { return strlen (m_text); }
	char* chars ();
	void dump ();
	void append (const char c);
	void append (const char* c);
	void append (str c);
	void append (QString c);
	void format (const char* fmt, ...);
	void appendformat (const char* c, ...);
	int first (const char* c, uint a = 0);
	int last (const char* c, int a = -1);
	str substr (uint a, uint b);
	void replace (const char* o, const char* n, uint a = 0);
	void remove (uint idx, uint dellen = 1);
	str trim (int dellen);
	void insert (char* c, uint pos);
	str reverse ();
	str repeat (int n);
	bool isnumber ();
	bool isword ();
	str tolower ();
	str toupper ();
	int compare (const char* c);
	int compare (str c);
	int icompare (str c);
	int icompare (const char* c);
	uint count (char* c);
	uint count (char s);
	int instanceof (const char* s, uint n);
	char subscript (uint pos) { return operator[] (pos); }
	std::vector<str> split (str del, bool bNoBlanks = false);
	str strip (char c);
	str strip (std::initializer_list<char> unwanted);
	char* begin () { return &m_text[0]; }
	char* end () { return &m_text[len () - 1]; }
	
	str operator+ (str& c) { append (c); return *this; }
	str& operator+= (char c) { append (c); return *this; }
	str& operator+= (const char* c) { append (c); return *this; }
	str& operator+= (const str c) { append (c); return *this; }
	str& operator+= (const QString c) { append (c); return *this; }
	str& operator+= (vertex vrt);
	str operator* (const int repcount) { repeat (repcount); return *this; }
	str operator- (const int trimcount) { return trim (trimcount); }std::vector<str> operator/ (str splitstring);
	std::vector<str> operator/ (char* splitstring);
	std::vector<str> operator/ (const char* splitstring);
	int operator% (str splitstring) { return count (splitstring.chars()); }
	int operator% (char* splitstring) { return count (splitstring); }
	int operator% (const char* splitstring) { return count (str (splitstring).chars()); }
	str operator+ () { return toupper (); }
	str operator- () { return tolower (); }
	str operator! () { return reverse (); }
	size_t operator~ () { return len (); }
	char& operator[] (int pos) { return m_text[pos]; }
	operator char* () const { return m_text; }
	operator QString () const { return m_text; }
	operator int () const { return atoi (m_text); }
	operator uint () const { return operator int(); }
	
	str& operator*= (const int repcount) {
		str other = repeat (repcount);
		clear ();
		append (other);
		return *this;
	}
	
	str& operator-= (const int trimcount) {
		str other = trim (trimcount);
		clear ();
		append (other);
		return *this;
	}
	
#define DEFINE_OPERATOR_TYPE(OPER, TYPE) \
	bool operator OPER (TYPE other) {return compare(other) OPER 0;}
#define DEFINE_OPERATOR(OPER) \
	DEFINE_OPERATOR_TYPE (OPER, str) \
	DEFINE_OPERATOR_TYPE (OPER, char*) \
	DEFINE_OPERATOR_TYPE (OPER, const char*)
	
	DEFINE_OPERATOR (==)
	DEFINE_OPERATOR (!=)
	DEFINE_OPERATOR (>)
	DEFINE_OPERATOR (<)
	DEFINE_OPERATOR (>=)
	DEFINE_OPERATOR (<=)
};

str fmt (const char* fmt, ...);

#endif // STR_H
