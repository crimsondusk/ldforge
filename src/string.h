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

using std::vector;

// =========================================================================================================================================
char* dynafmt (const char* fmtstr, va_list va, ulong size);

// =========================================================================================================================================
typedef class String {
public:
#define STR_COMPARE_OPERATOR(T, OP) (T other) const { return compare (other) OP 0; }
	
	typedef typename std::string::iterator it;
	typedef typename std::string::const_iterator c_it;
	typedef vector<String> stringlist;
	
	String () {}
	String (const char* data) { m_string = data; }
	String (const QString data) { m_string = data.toStdString (); }
	String (std::string data) { m_string = data; }
	
	void		append			(const char* data) { m_string.append (data); }
	void		append			(const char data) { m_string.push_back (data); }
	void		append			(const String data) { m_string.append (data.chars ()); }
	it			begin			() { return m_string.begin (); }
	const char*	c				() const { return chars (); }
	size_t		capacity		() const { return m_string.capacity (); }
	const char*	chars			() const { return m_string.c_str (); }
	int			compare			(const char* other) const { return m_string.compare (other); }
	int			compare			(String other) const { return m_string.compare (other); }
	it			end				() { return m_string.end (); }
	c_it		cbegin			() const { return m_string.cbegin (); }
	c_it		cend			() const { return m_string.end (); }
	void		clear			() { m_string.clear (); }
	ushort		count			(const char needle) const;
	bool		empty			() const { return m_string.empty (); }
	void		erase			(size_t pos) { m_string.erase (m_string.begin () + pos); }
	int			first			(const char* c, int a = 0) const;
	void		format			(const char* fmtstr, ...);
	void		insert			(size_t pos, char c) { m_string.insert (m_string.begin () + pos, c); }
	int			last			(const char* c, int a = -1) const;
	size_t		len				() const { return m_string.length (); }
	String		lower			() const;
	size_t		maxSize			() const { return m_string.max_size (); }
	void		replace			(const char* a, const char* b);
	void		resize			(size_t n) { m_string.resize (n); }
	void		shrinkToFit		() { m_string.shrink_to_fit (); }
	stringlist	split			(String del) const;
	stringlist	split			(char del) const;
	String		strip			(char unwanted) { return strip ({unwanted}); }
	String		strip			(std::initializer_list<char> unwanted);
	String		substr			(long a, long b) const;
	void		trim			(short n);
	String		upper			() const;
	void		writeToFile		(FILE* fp) const { fwrite (chars (), 1, len (), fp); }
	
	String		operator+		(const String data) const { String newstr = *this; newstr += data; return newstr; }
	String		operator+		(const char* data) const { String newstr = *this; newstr += data; return newstr; }
	String&		operator+=		(const String data) { append (data); return *this; }
	String&		operator+=		(const char* data) { append (data); return *this; }
	String&		operator+=		(const char data) { append (data); return *this; }
	String		operator+		() const { return upper (); }
	String		operator-		() const { return lower (); }
	String		operator-		(short n) const { String newstr = m_string; newstr -= n; return newstr; }
	String&		operator-=		(short n) { trim (n); return *this; }
	size_t		operator~		() const { return len (); }
	vector<String> operator/	(String del) const { return split (del); }
	char&		operator[]		(size_t n) { return m_string[n]; }
	const char&	operator[]		(size_t n) const { return m_string[n]; }
	bool		operator==		STR_COMPARE_OPERATOR (const char*, ==)
	bool		operator==		STR_COMPARE_OPERATOR (String, ==)
	bool		operator!=		STR_COMPARE_OPERATOR (const char*, !=)
	bool		operator!=		STR_COMPARE_OPERATOR (String,  !=)
	bool		operator!		() const { return empty (); }
	operator const char*		() const { return chars (); }
	operator QString			() { return chars (); }
	operator const QString	() const { return chars (); }
	
private:
	std::string m_string;
} str;

static const std::size_t npos = std::string::npos;

// =========================================================================================================================================
str fmt (const char* fmtstr, ...);

#endif // STR_H