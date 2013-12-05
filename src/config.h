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

#ifndef LDFORGE_CONFIG_H
#define LDFORGE_CONFIG_H

// =============================================================================
#include <QString>
#include <QVariant>
#include <QKeySequence>
class QSettings;

typedef QChar QChar;
typedef QString str;

#define MAX_INI_LINE 512
#define MAX_CONFIG 512

#define cfg(T, NAME, DEFAULT) T##Config NAME (DEFAULT, #NAME, #DEFAULT)
#define extern_cfg(T, NAME)   extern T##Config NAME

// =========================================================
class Config
{	public:
		enum Type
		{	Int,
			String,
			Float,
			Bool,
			KeySequence,
			List,
		};

		Config (const char* name, const char* defstring);
		const char* name;

		virtual Type getType() const
		{	return (Type) 0;
		}

		virtual void resetValue() {}
		virtual void loadFromVariant (const QVariant& val)
		{	(void) val;
		}

		virtual bool isDefault() const
		{	return false;
		}

		virtual QVariant toVariant() const
		{	return QVariant();
		}

		virtual QVariant defaultVariant() const
		{	return QVariant();
		}

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
	virtual void resetValue() override { value = defval; } \
	virtual bool isDefault() const override { return value == defval; } \
	virtual QVariant toVariant() const override { return QVariant::fromValue<T> (value); } \
	virtual QVariant defaultVariant() const override { return QVariant::fromValue<T> (defval); } \
	virtual void loadFromVariant (const QVariant& val) override { value = val.value<T>(); } \

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
class IntConfig : public Config
{	public:
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
class StringConfig : public Config
{	public:
		IMPLEMENT_CONFIG (String, str)

		DEFINE_COMPARE_OPERATOR (str, ==)
		DEFINE_COMPARE_OPERATOR (str, !=)
		DEFINE_ASSIGN_OPERATOR (str, =)
		DEFINE_ASSIGN_OPERATOR (str, +=)

		QChar operator[] (int n)
		{	return value[n];
		}
};

// =============================================================================
class FloatConfig : public Config
{	public:
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
class BoolConfig : public Config
{	public:
		IMPLEMENT_CONFIG (Bool, bool)
		DEFINE_ALL_COMPARE_OPERATORS (bool)
		DEFINE_ASSIGN_OPERATOR (bool, =)
};

// =============================================================================
class KeySequenceConfig : public Config
{	public:
		IMPLEMENT_CONFIG (KeySequence, QKeySequence)
		DEFINE_ALL_COMPARE_OPERATORS (QKeySequence)
		DEFINE_ASSIGN_OPERATOR (QKeySequence, =)
};

// =============================================================================
class ListConfig : public Config
{	public:
		IMPLEMENT_CONFIG (List, QList<QVariant>)
		DEFINE_ASSIGN_OPERATOR (QList<QVariant>, =)

		typedef QList<QVariant>::iterator it;
		typedef QList<QVariant>::const_iterator c_it;

		it begin()
		{	return value.begin();
		}

		c_it begin() const
		{	return value.constBegin();
		}

		it end()
		{	return value.end();
		}

		c_it end() const
		{	return value.constEnd();
		}
};

#endif // LDFORGE_CONFIG_H
