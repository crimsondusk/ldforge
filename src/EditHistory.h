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
#include "Main.h"
#include "LDObject.h"

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
	PROPERTY (private,	int,			Position,	NUM_OPS,	STOCK_WRITE)
	PROPERTY (public,	LDDocument*,	Document,	NO_OPS,		STOCK_WRITE)
	PROPERTY (public,	bool,			Ignoring,	BOOL_OPS,	STOCK_WRITE)

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
		Changeset m_currentChangeset;
		QList<Changeset> m_changesets;
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class AbstractHistoryEntry
{
	PROPERTY (public,	History*,	Parent,	NO_OPS,	STOCK_WRITE)

	public:
		virtual ~AbstractHistoryEntry() {}
		virtual void undo() const = 0;
		virtual void redo() const = 0;
		virtual History::EHistoryType getType() const = 0;
		virtual QString getTypeName() const = 0;
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class DelHistory : public AbstractHistoryEntry
{
	PROPERTY (private,	int,		Index,		NO_OPS,	STOCK_WRITE)
	PROPERTY (private,	QString,	Code,		NO_OPS,	STOCK_WRITE)

	public:
		IMPLEMENT_HISTORY_TYPE (Del)
		DelHistory (int idx, LDObject* obj);
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class EditHistory : public AbstractHistoryEntry
{
	PROPERTY (private,	int, 		Index,		NO_OPS,	STOCK_WRITE)
	PROPERTY (private,	QString,	OldCode,	NO_OPS,	STOCK_WRITE)
	PROPERTY (private,	QString,	NewCode,	NO_OPS,	STOCK_WRITE)

	public:
		IMPLEMENT_HISTORY_TYPE (Edit)

		EditHistory (int idx, QString oldCode, QString newCode) :
				m_Index (idx),
				m_OldCode (oldCode),
				m_NewCode (newCode) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class AddHistory : public AbstractHistoryEntry
{
	PROPERTY (private,	int,		Index,	NO_OPS,	STOCK_WRITE)
	PROPERTY (private,	QString,	Code,		NO_OPS,	STOCK_WRITE)

	public:
		IMPLEMENT_HISTORY_TYPE (Add)

		AddHistory (int idx, LDObject* obj) :
				m_Index (idx),
				m_Code (obj->asText()) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
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
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
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
