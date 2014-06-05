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
#include "main.h"
#include "ldObject.h"

#define IMPLEMENT_HISTORY_TYPE(N)							\
	virtual ~N##History() {}								\
	virtual void undo() const override;						\
	virtual void redo() const override;						\
															\
	virtual History::EHistoryType getType() const override	\
	{														\
		return History::E##N##History;						\
	}														\
															\
	virtual QString getTypeName() const						\
	{														\
		return #N;											\
	}

class AbstractHistoryEntry;

// =============================================================================
class History
{
	PROPERTY (private,	int,				position,	setPosition,	STOCK_WRITE)
	PROPERTY (public,	LDDocumentWeakPtr,	document,	setDocument,	STOCK_WRITE)
	PROPERTY (public,	bool,				isIgnoring,	setIgnoring,	STOCK_WRITE)

public:
	typedef QList<AbstractHistoryEntry*> Changeset;

	enum EHistoryType
	{
		EDelHistory,
		EEditHistory,
		EAddHistory,
		EMoveHistory,
		ESwapHistory,
	};

	History();
	void undo();
	void redo();
	void clear();

	void addStep();
	void add (AbstractHistoryEntry* entry);

	inline long getSize() const
	{
		return m_changesets.size();
	}

	inline History& operator<< (AbstractHistoryEntry* entry)
	{
		add (entry);
		return *this;
	}

	inline const Changeset& getChangeset (long pos) const
	{
		return m_changesets[pos];
	}

private:
	Changeset			m_currentChangeset;
	QList<Changeset>	m_changesets;
};

// =============================================================================
//
class AbstractHistoryEntry
{
	PROPERTY (public,	History*,	parent,	setParent,	STOCK_WRITE)

public:
	virtual ~AbstractHistoryEntry() {}
	virtual void undo() const = 0;
	virtual void redo() const = 0;
	virtual History::EHistoryType getType() const = 0;
	virtual QString getTypeName() const = 0;
};

// =============================================================================
//
class DelHistory : public AbstractHistoryEntry
{
	PROPERTY (private,	int,		index,	setIndex,	STOCK_WRITE)
	PROPERTY (private,	QString,	code,	setCode,	STOCK_WRITE)

public:
	IMPLEMENT_HISTORY_TYPE (Del)
	DelHistory (int idx, LDObjectPtr obj);
};

// =============================================================================
//
class EditHistory : public AbstractHistoryEntry
{
	PROPERTY (private,	int, 		index,		setIndex,	STOCK_WRITE)
	PROPERTY (private,	QString,	oldCode,	setOldCode,	STOCK_WRITE)
	PROPERTY (private,	QString,	newCode,	setNewCode,	STOCK_WRITE)

public:
	IMPLEMENT_HISTORY_TYPE (Edit)

	EditHistory (int idx, QString oldCode, QString newCode) :
		m_index (idx),
		m_oldCode (oldCode),
		m_newCode (newCode) {}
};

// =============================================================================
//
class AddHistory : public AbstractHistoryEntry
{
	PROPERTY (private,	int,		index,	setIndex,	STOCK_WRITE)
	PROPERTY (private,	QString,	code,	setCode,	STOCK_WRITE)

public:
	IMPLEMENT_HISTORY_TYPE (Add)

	AddHistory (int idx, LDObjectPtr obj) :
		m_index (idx),
		m_code (obj->asText()) {}
};

// =============================================================================
//
class MoveHistory : public AbstractHistoryEntry
{
public:
	IMPLEMENT_HISTORY_TYPE (Move)

	QList<int> indices;
	Vertex dest;

	MoveHistory (QList<int> indices, Vertex dest) :
			indices (indices),
			dest (dest) {}
};

// =============================================================================
//
class SwapHistory : public AbstractHistoryEntry
{
public:
	IMPLEMENT_HISTORY_TYPE (Swap)

	SwapHistory (int a, int b) :
		a (a),
		b (b) {}

private:
	int a, b;
};
