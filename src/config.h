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

// =============================================================================
#include <QString>
#include <QKeySequence>
class QSettings;

typedef QChar qchar;
typedef QString str;

#define MAX_INI_LINE 512
#define MAX_CONFIG 512

#define cfg(T, NAME, DEFAULT) T##Config NAME (DEFAULT, #NAME, #DEFAULT)
#define extern_cfg(T, NAME)   extern T##Config NAME

// =========================================================
class Config {
public:
	enum Type {
		None,
		Int,
		String,
		Float,
		Bool,
		KeySequence,
	};

	Config (const char* name, const char* defstring);
	const char* name;

	virtual Type getType() const {
		return None;
	}
	
	str toString() const;
	str defaultString() const;
	virtual void resetValue() {}
	virtual void loadFromConfig (const QSettings* cfg) { (void) cfg; }
	virtual bool isDefault() const { return false; }
	
	// ------------------------------------------
	static bool load();
	static bool save();
	static void reset();
	static str dirpath();
	static str filepath (str file);
	
protected:
	static void addToArray (Config* ptr);
	
private:
	const char* m_defstring;
};

// =============================================================================
#define IMPLEMENT_CONFIG(NAME, T) \
	T value, defval; \
	NAME##Config (T defval, const char* name, const char* defstring) : \
		Config (name, defstring), value (defval), defval (defval) \
		{ Config::addToArray (this); } \
	\
	operator const T&() const { return value; } \
	Config::Type getType() const override { return Config::NAME; } \
	virtual void resetValue() { value = defval; } \
	virtual bool isDefault() const { return value == defval; } \
	virtual void loadFromConfig (const QSettings* cfg) override;
	
#define DEFINE_UNARY_OPERATOR(T, OP) \
	T operator OP() { \
		return OP value; \
	}
	
#define DEFINE_BINARY_OPERATOR(T, OP) \
	T operator OP (const T other) { \
		return value OP other; \
	}
	
#define DEFINE_ASSIGN_OPERATOR(T, OP) \
	T& operator OP (const T other) { \
		return value OP other; \
	}
	
#define DEFINE_COMPARE_OPERATOR(T, OP) \
	bool operator OP (const T other) { \
		return value OP other; \
	}
	
#define DEFINE_CAST_OPERATOR(T) \
	operator T() { \
		return (T) value; \
	}
	
#define DEFINE_ALL_COMPARE_OPERATORS(T) \
	DEFINE_COMPARE_OPERATOR (T, ==) \
	DEFINE_COMPARE_OPERATOR (T, !=) \
	DEFINE_COMPARE_OPERATOR (T, >) \
	DEFINE_COMPARE_OPERATOR (T, <) \
	DEFINE_COMPARE_OPERATOR (T, >=) \
	DEFINE_COMPARE_OPERATOR (T, <=) \
	 
#define DEFINE_INCREMENT_OPERATORS(T) \
	T operator++() { return ++value; } \
	T operator++(int) { return value++; } \
	T operator--() { return --value; } \
	T operator--(int) { return value--; }

// =============================================================================
class IntConfig : public Config {
public:
	IMPLEMENT_CONFIG (Int, int)
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
class StringConfig : public Config {
public:
	IMPLEMENT_CONFIG (String, str)
	
	DEFINE_COMPARE_OPERATOR (str, ==)
	DEFINE_COMPARE_OPERATOR (str, !=)
	DEFINE_ASSIGN_OPERATOR (str, =)
	DEFINE_ASSIGN_OPERATOR (str, +=)
	
	QChar operator[] (int n) {
		return value[n];
	}
};

// =============================================================================
class FloatConfig : public Config {
public:
	IMPLEMENT_CONFIG (Float, float)
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
class BoolConfig : public Config {
public:
	IMPLEMENT_CONFIG (Bool, bool)
	DEFINE_ALL_COMPARE_OPERATORS (bool)
	DEFINE_ASSIGN_OPERATOR (bool, =)
};

// =============================================================================
class KeySequenceConfig : public Config {
public:
	IMPLEMENT_CONFIG (KeySequence, QKeySequence)
	DEFINE_ALL_COMPARE_OPERATORS (QKeySequence)
	DEFINE_ASSIGN_OPERATOR (QKeySequence, =)
};

#endif // CONFIG_H