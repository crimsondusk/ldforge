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

#ifndef __STR_H__
#define __STR_H__

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
// #include <initializer_list>
#include <vector>
#include <QString>

char* vdynformat (const char* csFormat, va_list vArgs, long int lSize);

class vertex;

// Dynamic string object, allocates memory when needed and
// features a good bunch of manipulation methods
class str {
private:
	// The actual message
	char* text;
	
	// Where will append() place new characters?
	unsigned short curs;
	
	// Allocated length
	unsigned short alloclen;
	
	// Resize the text buffer to len characters
	void resize (unsigned int len);
	
public:
	// ======================================================================
	str ();
	str (const char* c);
	str (char c);
	str (QString c);
	~str ();
	
	static str mkfmt (const char* fmt, ...) {
		va_list va;
		char* buf;
		
		va_start (va, fmt);
		buf = vdynformat (fmt, va, 256);
		va_end (va);
		
		str val = buf;
		delete[] buf;
		return val;
	}
	
	// ======================================================================
	// METHODS
	
	// Empty the string
	void clear ();
	
	// Length of the string
	size_t len () {
		return strlen (text);
	}
	
	// The char* form of the string
	char* chars ();
	
	// Dumps the character table of the string
	void dump ();
	
	// Appends text to the string
	void append (const char c);
	void append (const char* c);
	void append (str c);
	void append (QString c);
	
	// Formats text to the string.
	void format (const char* fmt, ...);
	str format (...);
	
	// Appends formatted text to the string.
	void appendformat (const char* c, ...);
	
	// Returns the first occurrence of c in the string, optionally starting
	// from a certain position rather than the start.
	int first (const char* c, unsigned int a = 0);
	
	// Returns the last occurrence of c in the string, optionally starting
	// from a certain position rather than the end.
	int last (const char* c, int a = -1);
	
	// Returns a substring of the string, from a to b.
	str substr (unsigned int a, unsigned int b);
	
	// Replace a substring with another substring.
	void replace (const char* o, const char* n, unsigned int a = 0);
	
	// Removes a given index from the string, optionally more characters than just 1.
	void remove (unsigned int idx, unsigned int dellen=1);
	
	// Trims the given amount of characters. If negative, the characters
	// are removed from the beginning of the string, if positive, from the
	// end of the string.
	str trim (int dellen);
	
	// Inserts a substring into a certain position.
	void insert (char* c, unsigned int pos);
	
	// Reverses the string.
	str reverse ();
	
	// Repeats the string a given amount of times.
	str repeat (int n);
	
	// Is the string a number?
	bool isnumber ();
	
	// Is the string a word, i.e consists only of alphabetic letters?
	bool isword ();
	
	// Convert string to lower case
	str tolower ();
	
	// Convert string to upper case
	str toupper ();
	
	// Compare this string with another
	int compare (const char* c);
	int compare (str c);
	int icompare (str c);
	int icompare (const char* c);
	
	// Counts the amount of substrings in the string
	unsigned int count (char* s);
	unsigned int count (char s);
	
	// Counts where the given substring is seen for the nth time
	int instanceof (const char* s, unsigned n);
	
	char subscript (uint pos) {
		return operator[] (pos);
	}
	
	std::vector<str> split (str del, bool bNoBlanks = false);
	
	str strip (char c);
	str strip (std::initializer_list<char> unwanted);
	
	// ======================================================================
	// OPERATORS
	str operator+ (str& c) {
		append (c);
		return *this;
	}
	
	str& operator+= (char c) {
		append (c);
		return *this;
	}
	
	str& operator+= (const char* c) {
		append (c);
		return *this;
	}
	
	str& operator+= (const str c) {
		append (c);
		return *this;
	}
	
	str& operator+= (const QString c) {
		append (c);
		return *this;
	}
	
	str& operator+= (vertex vrt);
	
	str operator* (const int repcount) {
		repeat (repcount);
		return *this;
	}
	
	str& operator*= (const int repcount) {
		str other = repeat (repcount);
		clear ();
		append (other);
		return *this;
	}
	
	str operator- (const int trimcount) {
		return trim (trimcount);
	}
	
	str& operator-= (const int trimcount) {
		str other = trim (trimcount);
		clear ();
		append (other);
		return *this;
	}
	
	std::vector<str> operator/ (str splitstring);
	std::vector<str> operator/ (char* splitstring);
	std::vector<str> operator/ (const char* splitstring);
	
	int operator% (str splitstring) {
		return count (splitstring.chars());
	}
	
	int operator% (char* splitstring) {
		return count (splitstring);
	}
	
	int operator% (const char* splitstring) {
		return count (str (splitstring).chars());
	}
	
	str operator+ () {
		return toupper ();
	}
	
	str operator- () {
		return tolower ();
	}
	
	str operator! () {
		return reverse ();
	}
	
	size_t operator~ () {
		return len ();
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
	
	char& operator[] (int pos) {
		return text[pos];
	}
	
	operator char* () const {
		return text;
	}
	
	operator QString () const {
		return text;
	}
	
	operator int () const {
		return atoi (text);
	}
	
	operator uint () const {
		return operator int();
	}
};

#endif // __STR_H__
