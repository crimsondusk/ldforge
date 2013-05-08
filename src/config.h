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

#ifndef CONFIG_H
#define CONFIG_H

#include "common.h"
#include "str.h"

// =============================================================================
#include <QString>
#include <qkeysequence.h>

#define MAX_INI_LINE 512
#define NUM_CONFIG (g_pConfigPointers.size ())

#define cfg(T, NAME, DEFAULT) \
	T##config NAME (DEFAULT, #NAME, #T, #DEFAULT)

#define extern_cfg(T, NAME) \
	extern T##config NAME

// =============================================================================
enum configtype_e {
	CONFIG_none,
	CONFIG_int,
	CONFIG_str,
	CONFIG_float,
	CONFIG_bool,
	CONFIG_keyseq,
};

// =========================================================
class config {
public:
	const char* name, *typestring, *defaultstring;
	
	virtual configtype_e getType () {
		return CONFIG_none;
	}
	
	virtual void resetValue () {}
	
	// ------------------------------------------
	static bool load ();
	static bool save ();
	static void reset ();
	static str dirpath ();
	static str filepath ();
};

extern std::vector<config*> g_configPointers;

// =============================================================================
#define DEFINE_UNARY_OPERATOR(T, OP) \
	T operator OP () { \
		return (OP value); \
	} \

#define DEFINE_BINARY_OPERATOR(T, OP) \
	T operator OP (const T other) { \
		return (value OP other); \
	} \

#define DEFINE_ASSIGN_OPERATOR(T, OP) \
	T& operator OP (const T other) { \
		return (value OP other); \
	} \

#define DEFINE_COMPARE_OPERATOR(T, OP) \
	bool operator OP (const T other) { \
		return (value OP other); \
	} \

#define DEFINE_CAST_OPERATOR(T) \
	operator T () { \
		return (T) value; \
	} \

#define DEFINE_ALL_COMPARE_OPERATORS(T) \
	DEFINE_COMPARE_OPERATOR (T, ==) \
	DEFINE_COMPARE_OPERATOR (T, !=) \
	DEFINE_COMPARE_OPERATOR (T, >) \
	DEFINE_COMPARE_OPERATOR (T, <) \
	DEFINE_COMPARE_OPERATOR (T, >=) \
	DEFINE_COMPARE_OPERATOR (T, <=) \

#define DEFINE_INCREMENT_OPERATORS(T) \
	T operator++ () {return ++value;} \
	T operator++ (int) {return value++;} \
	T operator-- () {return --value;} \
	T operator-- (int) {return value--;}

#define CONFIGTYPE(T) \
class T##config : public config

#define IMPLEMENT_CONFIG(T) \
	T value, defval; \
	\
	T##config (T _defval, const char* _name, const char* _typestring, \
		const char* _defaultstring) \
	{ \
		value = defval = _defval; \
		name = _name; \
		typestring = _typestring; \
		defaultstring = _defaultstring; \
		g_pConfigPointers.push_back (this); \
	} \
	operator T () { \
		return value; \
	} \
	configtype_e getType () { \
		return CONFIG_##T; \
	} \
	virtual void resetValue () { \
		value = defval; \
	}

// =============================================================================
CONFIGTYPE (int) {
public:
	IMPLEMENT_CONFIG (int)
	
	// Int-specific operators
	DEFINE_ALL_COMPARE_OPERATORS (int)
	DEFINE_INCREMENT_OPERATORS (int)
	DEFINE_BINARY_OPERATOR (int, +)
	DEFINE_BINARY_OPERATOR (int, -)
	DEFINE_BINARY_OPERATOR (int, *)
	DEFINE_BINARY_OPERATOR (int, /)
	DEFINE_BINARY_OPERATOR (int, %)
	DEFINE_BINARY_OPERATOR (int, ^)
	DEFINE_BINARY_OPERATOR (int, |)
	DEFINE_BINARY_OPERATOR (int, &)
	DEFINE_BINARY_OPERATOR (int, >>)
	DEFINE_BINARY_OPERATOR (int, <<)
	DEFINE_UNARY_OPERATOR (int, !)
	DEFINE_UNARY_OPERATOR (int, ~)
	DEFINE_UNARY_OPERATOR (int, -)
	DEFINE_UNARY_OPERATOR (int, +)
	DEFINE_ASSIGN_OPERATOR (int, =)
	DEFINE_ASSIGN_OPERATOR (int, +=)
	DEFINE_ASSIGN_OPERATOR (int, -=)
	DEFINE_ASSIGN_OPERATOR (int, *=)
	DEFINE_ASSIGN_OPERATOR (int, /=)
	DEFINE_ASSIGN_OPERATOR (int, %=)
	DEFINE_ASSIGN_OPERATOR (int, >>=)
	DEFINE_ASSIGN_OPERATOR (int, <<=)
};

// =============================================================================
CONFIGTYPE (str) {
public:
	IMPLEMENT_CONFIG (str)
	
	DEFINE_ALL_COMPARE_OPERATORS (str)
	DEFINE_BINARY_OPERATOR (str, -)
	DEFINE_BINARY_OPERATOR (str, *)
	DEFINE_UNARY_OPERATOR (str, !)
	DEFINE_ASSIGN_OPERATOR (str, =)
	DEFINE_ASSIGN_OPERATOR (str, +=)
	DEFINE_ASSIGN_OPERATOR (str, -=)
	DEFINE_ASSIGN_OPERATOR (str, *=)
	DEFINE_CAST_OPERATOR (char*)
	
	char operator[] (size_t n) {
		return value[n];
	}
	
#ifdef CONFIG_WITH_QT
	operator QString () {
		return QString (value.chars());
	}
#endif // CONFIG_WITH_QT
};

// =============================================================================
CONFIGTYPE (float) {
public:
	IMPLEMENT_CONFIG (float)
	
	DEFINE_ALL_COMPARE_OPERATORS (float)
	DEFINE_INCREMENT_OPERATORS (float)
	DEFINE_BINARY_OPERATOR (float, +)
	DEFINE_BINARY_OPERATOR (float, -)
	DEFINE_BINARY_OPERATOR (float, *)
	DEFINE_UNARY_OPERATOR (float, !)
	DEFINE_ASSIGN_OPERATOR (float, =)
	DEFINE_ASSIGN_OPERATOR (float, +=)
	DEFINE_ASSIGN_OPERATOR (float, -=)
	DEFINE_ASSIGN_OPERATOR (float, *=)
};

// =============================================================================
CONFIGTYPE (bool) {
public:
	IMPLEMENT_CONFIG (bool)
	DEFINE_ALL_COMPARE_OPERATORS (bool)
	DEFINE_ASSIGN_OPERATOR (bool, =)
};

// =============================================================================
typedef QKeySequence keyseq;

CONFIGTYPE (keyseq) {
public:
	IMPLEMENT_CONFIG (keyseq)
	DEFINE_ALL_COMPARE_OPERATORS (keyseq)
	DEFINE_ASSIGN_OPERATOR (keyseq, =)
};

#endif // CONFIG_H