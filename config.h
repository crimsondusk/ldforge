#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include "common.h"
#include "str.h"
#include "color.h"

// =============================================================================
// Determine configuration file. Use APPNAME if given.
#ifdef APPNAME
 #define CONFIGFILE APPNAME ".ini"
#else // APPNAME
 #define APPNAME "(unnamed application)"
 #define CONFIGFILE "config.ini"
#endif // APPNAME

#ifdef CONFIG_WITH_QT
 #include <QString>
#endif // CONFIG_WITH_QT

// -------------------------------
#define CFGSECTNAME(X) CFGSECT_##X

#define MAX_INI_LINE 512
#define NUM_CONFIG (sizeof config::pointers / sizeof *config::pointers)

// =============================================================================
enum configsection_e {
#define CFG(...)
#define SECT(A,B) CFGSECTNAME (A),
 #include "cfgdef.h"
#undef CFG
#undef SECT
	NUM_ConfigSections,
	NO_CONFIG_SECTION = -1
};

// =============================================================================
enum configtype_e {
	CONFIG_none,
	CONFIG_int,
	CONFIG_str,
	CONFIG_float,
	CONFIG_bool,
	CONFIG_color,
};

// =========================================================
class config {
public:
	configsection_e sect;
	const char* description, *name, *fullname, *typestring, *defaultstring;
	
	virtual configtype_e getType () {
		return CONFIG_none;
	}
	
	virtual void resetValue () {}
	
	// ------------------------------------------
	static bool load ();
	static bool save ();
	static void reset ();
	static config* pointers[];
	static const char* sections[];
	static const char* sectionNames[];
	static str dirpath ();
	static str filepath ();
};

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
	T##config (const configsection_e _sect, const char* _description, \
		T _defval, const char* _name, const char* _fullname, const char* _typestring, \
		const char* _defaultstring) \
	{ \
		sect = _sect; \
		description = _description; \
		value = defval = _defval; \
		name = _name; \
		fullname = _fullname; \
		typestring = _typestring; \
		defaultstring = _defaultstring; \
	} \
	operator T () { \
		return value; \
	} \
	configtype_e getType () { \
		return CONFIG_##T; \
	} \
	void resetValue () { \
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
CONFIGTYPE (color) {
public:
	IMPLEMENT_CONFIG (color)
	// DEFINE_COMPARE_OPERATOR (color, ==)
	// DEFINE_COMPARE_OPERATOR (color, !=)
};

// =============================================================================
// Extern the configurations now
#define CFG(TYPE, SECT, NAME, DESCR, DEFAULT) extern TYPE##config SECT##_##NAME;
#define SECT(...)
 #include "cfgdef.h"
#undef CFG
#undef SECT

#endif // __OPTIONS_H__