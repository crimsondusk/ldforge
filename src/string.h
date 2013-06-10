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

#include <string>
#include <stdarg.h>
#include <QString>
#include "types.h"

typedef class String {
public:
	typedef typename std::string::iterator it;
	typedef typename std::string::const_iterator c_it;
	typedef vector<String> stringlist;
	
	String ();
	String (const char* data);
	String (const QString data);
	String (std::string data);
	
	void           append         (const char* data);
	void           append         (const char data);
	void           append         (const String data);
	it             begin          ();
	c_it           begin          () const;
	const char*    c              () const;
	size_t         capacity       () const;
	const char*    chars          () const;
	int            compare        (const char* other) const;
	int            compare        (String other) const;
	it             end            ();
	c_it           end            () const;
	void           clear          ();
	ushort         count          (const char needle) const;
	bool           empty          () const;
	void           erase          (size_t pos);
	int            first          (const char* c, int a = 0) const;
	void           format         (const char* fmtstr, ...);
	void           insert         (size_t pos, char c);
	int            last           (const char* c, int a = -1) const;
	size_t         len            () const;
	String         lower          () const;
	size_t         maxSize        () const;
	void           replace        (const char* a, const char* b);
	void           resize         (size_t n);
	void           shrinkToFit    ();
	stringlist     split          (String del) const;
	stringlist     split          (char del) const;
	String         strip          (char unwanted);
	String         strip          (std::initializer_list<char> unwanted);
	String         substr         (long a, long b) const;
	void           trim           (short n);
	String         upper          () const;
	
	String         operator+      (const String data) const;
	String         operator+      (const char* data) const;
	String&        operator+=     (const String data);
	String&        operator+=     (const char* data);
	String&        operator+=     (const char data);
	String         operator+      () const;
	String         operator-      () const;
	String         operator-      (size_t n) const;
	String&        operator-=     (size_t n);
	size_t         operator~      () const;
	vector<String> operator/      (String del) const;
	char&          operator[]     (size_t n);
	const char&    operator[]     (size_t n) const;
	bool           operator==     (const String other) const;
	bool           operator==     (const char* other) const;
	bool           operator!=     (const String other) const;
	bool           operator!=     (const char* other) const;
	bool           operator!      () const;
	operator const char*          () const;
	operator QString              ();
	operator const QString        () const;
	
private:
	std::string m_string;
} str;

// Accessories
char*          dynafmt        (const char* fmtstr, va_list va, ulong size);
str            fmt            (const char* fmtstr, ...);

#endif // STR_H