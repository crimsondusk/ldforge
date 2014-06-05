/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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

#pragma once
#include <QString>
#include <QVariant>
#include <QKeySequence>
#include "macros.h"
#include "basics.h"

class QSettings;

#define CFGENTRY(T, NAME, DEFAULT) \
	namespace cfg \
	{ \
		ConfigEntry::T##Type NAME; \
		T##ConfigEntry config_##NAME (&NAME, #NAME, DEFAULT); \
	}

#define EXTERN_CFGENTRY(T, NAME) \
	namespace cfg \
	{ \
		extern ConfigEntry::T##Type NAME; \
	}

namespace Config
{
	bool load();
	bool save();
	void reset();
	QString dirpath();
	QString filepath (QString file);
}

class ConfigEntry
{
	PROPERTY (private, QString, name, setName, STOCK_WRITE)

public:
	enum Type
	{
		EIntType,
		EStringType,
		EFloatType,
		EBoolType,
		EKeySequenceType,
		EListType,
		EVertexType,
	};

	using IntType			= int;
	using StringType		= QString;
	using FloatType			= float;
	using BoolType			= bool;
	using KeySequenceType	= QKeySequence;
	using ListType			= QList<QVariant>;
	using VertexType		= Vertex;

	ConfigEntry (QString name);

	virtual QVariant	getDefaultAsVariant() const = 0;
	virtual Type		getType() const = 0;
	virtual bool		isDefault() const = 0;
	virtual void		loadFromVariant (const QVariant& val) = 0;
	virtual void		resetValue() = 0;
	virtual QVariant	toVariant() const = 0;

protected:
	static void addToArray (ConfigEntry* ptr);
};

// =============================================================================
#define IMPLEMENT_CONFIG(NAME)														\
public:																				\
	using ValueType = ConfigEntry::NAME##Type;										\
																					\
	NAME##ConfigEntry (ValueType* valueptr, QString name, ValueType def) :			\
		ConfigEntry (name),															\
		m_valueptr (valueptr),														\
		m_default (def)																\
	{																				\
		ConfigEntry::addToArray (this);												\
		*m_valueptr = def;															\
	}																				\
																					\
	inline ValueType getValue() const												\
	{																				\
		return *m_valueptr;															\
	}																				\
																					\
	inline void setValue (ValueType val)											\
	{																				\
		*m_valueptr = val;															\
	}																				\
																					\
	virtual ConfigEntry::Type getType() const										\
	{																				\
		return ConfigEntry::E##NAME##Type;											\
	}																				\
																					\
	virtual void resetValue()														\
	{																				\
		*m_valueptr = m_default;													\
	}																				\
																					\
	virtual const ValueType& getDefault() const										\
	{																				\
		return m_default;															\
	}																				\
																					\
	virtual bool isDefault() const													\
	{																				\
		return *m_valueptr == m_default;											\
	}																				\
																					\
	virtual void loadFromVariant (const QVariant& val)								\
	{																				\
		*m_valueptr = val.value<ValueType>();										\
	}																				\
																					\
	virtual QVariant toVariant() const												\
	{																				\
		return QVariant::fromValue<ValueType> (*m_valueptr);						\
	}																				\
																					\
	virtual QVariant getDefaultAsVariant() const									\
	{																				\
		return QVariant::fromValue<ValueType> (m_default);							\
	}																				\
																					\
	static NAME##ConfigEntry* getByName (QString name);								\
																					\
private:																			\
	ValueType*	m_valueptr;															\
	ValueType	m_default;

// =============================================================================
//
class IntConfigEntry : public ConfigEntry
{
	IMPLEMENT_CONFIG (Int)
};

class StringConfigEntry : public ConfigEntry
{
	IMPLEMENT_CONFIG (String)
};

class FloatConfigEntry : public ConfigEntry
{
	IMPLEMENT_CONFIG (Float)
};

class BoolConfigEntry : public ConfigEntry
{
	IMPLEMENT_CONFIG (Bool)
};

class KeySequenceConfigEntry : public ConfigEntry
{
	IMPLEMENT_CONFIG (KeySequence)
};

class ListConfigEntry : public ConfigEntry
{
	IMPLEMENT_CONFIG (List)
};

class VertexConfigEntry : public ConfigEntry
{
	IMPLEMENT_CONFIG (Vertex)
};
