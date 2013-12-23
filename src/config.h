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

#include "property.h"

// =============================================================================
#include <QString>
#include <QVariant>
#include <QKeySequence>
class QSettings;

typedef QChar QChar;
typedef QString str;

#define MAX_INI_LINE 512
#define MAX_CONFIG 512

#define cfg(T, NAME, DEFAULT) \
	Config::T##Type NAME; \
	T##Config config_##NAME (&NAME, #NAME, DEFAULT);

#define extern_cfg(T, NAME) extern Config::T##Type NAME;

// =========================================================
class Config
{	PROPERTY (private, str, Name, STR_OPS, STOCK_WRITE)

	public:
		enum Type
		{	Int,
			String,
			Float,
			Bool,
			KeySequence,
			List,
		};

		using IntType = int;
		using StringType = QString;
		using FloatType = float;
		using BoolType = bool;
		using KeySequenceType = QKeySequence;
		using ListType = QList<QVariant>;

		Config (str name);

		virtual QVariant	getDefaultAsVariant() const = 0;
		virtual Type		getType() const = 0;
		virtual bool		isDefault() const = 0;
		virtual void		loadFromVariant (const QVariant& val) = 0;
		virtual void		resetValue() = 0;
		virtual QVariant	toVariant() const = 0;

		// ------------------------------------------
		static bool load();
		static bool save();
		static void reset();
		static str dirpath();
		static str filepath (str file);

	protected:
		static void addToArray (Config* ptr);
};

// =============================================================================
#define IMPLEMENT_CONFIG(NAME)												\
public:																				\
	using ValueType = Config::NAME##Type;									\
																						\
	NAME##Config (ValueType* valueptr, str name, ValueType def) :	\
		Config (name),																\
		m_valueptr (valueptr),													\
		m_default (def)															\
	{	Config::addToArray (this);												\
		*m_valueptr = def;														\
	}																					\
																						\
	inline ValueType getValue() const										\
	{	return *m_valueptr;														\
	}																					\
																						\
	inline void setValue (ValueType val)									\
	{	*m_valueptr = val;														\
	}																					\
																						\
	virtual Config::Type getType() const									\
	{	return Config::NAME;														\
	}																					\
																						\
	virtual void resetValue()													\
	{	*m_valueptr = m_default;												\
	}																					\
																						\
	virtual const ValueType& getDefault() const							\
	{	return m_default;															\
	}																					\
																						\
	virtual bool isDefault() const											\
	{	return *m_valueptr == m_default;										\
	}																					\
																						\
	virtual void loadFromVariant (const QVariant& val)					\
	{	*m_valueptr = val.value<ValueType>();								\
	}																					\
																						\
	virtual QVariant toVariant() const										\
	{	return QVariant::fromValue<ValueType> (*m_valueptr);			\
	}																					\
																						\
	virtual QVariant getDefaultAsVariant() const							\
	{	return QVariant::fromValue<ValueType> (m_default);				\
	}																					\
																						\
	static NAME##Config* getByName (str name);							\
																						\
private:																				\
	ValueType*	m_valueptr;														\
	ValueType	m_default;

// =============================================================================
class IntConfig : public Config
{	IMPLEMENT_CONFIG (Int)
};

// =============================================================================
class StringConfig : public Config
{	IMPLEMENT_CONFIG (String)
};

// =============================================================================
class FloatConfig : public Config
{	IMPLEMENT_CONFIG (Float)
};

// =============================================================================
class BoolConfig : public Config
{	IMPLEMENT_CONFIG (Bool)
};

// =============================================================================
class KeySequenceConfig : public Config
{	IMPLEMENT_CONFIG (KeySequence)
};

// =============================================================================
class ListConfig : public Config
{	IMPLEMENT_CONFIG (List)
};

#endif // LDFORGE_CONFIG_H
