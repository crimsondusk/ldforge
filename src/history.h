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

#ifndef LDFORGE_HISTORY_H
#define LDFORGE_HISTORY_H

#include "common.h"
#include "ldtypes.h"

#define IMPLEMENT_HISTORY_TYPE(N) \
	virtual ~N##History(){} \
	virtual void undo() const override; \
	virtual void redo() const override; \
	virtual History::Type getType() const override { return History::N; }

class AbstractHistoryEntry;

// =============================================================================
class History
{	PROPERTY (private,	long,		Position,	NUM_OPS,		STOCK_WRITE)
	PROPERTY (public,		LDFile*,	File,			NO_OPS,		STOCK_WRITE)
	PROPERTY (public,		bool,		Ignoring,	BOOL_OPS,	STOCK_WRITE)

	public:
		typedef QList<AbstractHistoryEntry*> Changeset;

		enum Type
		{	Del,
			Edit,
			Add,
			Move,
			Swap,
		};

		History();
		void undo();
		void redo();
		void clear();
		void updateActions() const;

		void addStep();
		void add (AbstractHistoryEntry* entry);

		inline long getSize() const
		{	return m_changesets.size();
		}

		inline History& operator<< (AbstractHistoryEntry* entry)
		{	add (entry);
			return *this;
		}

		inline const Changeset& getChangeset (long pos) const
		{	return m_changesets[pos];
		}

	private:
		Changeset m_currentChangeset;
		QList<Changeset> m_changesets;
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class AbstractHistoryEntry
{	PROPERTY (public,	History*,	Parent,	NO_OPS,	STOCK_WRITE)

	public:
		virtual ~AbstractHistoryEntry() {}
		virtual void undo() const {}
		virtual void redo() const {}
		virtual History::Type getType() const
		{	return (History::Type) 0;
		}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class DelHistory : public AbstractHistoryEntry
{	PROPERTY (private,	int,	Index,	NO_OPS,	STOCK_WRITE)
	PROPERTY (private,	str,	Code,		NO_OPS,	STOCK_WRITE)

	public:
		IMPLEMENT_HISTORY_TYPE (Del)

		DelHistory (int idx, LDObject* obj) :
				m_Index (idx),
				m_Code (obj->raw()) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class EditHistory : public AbstractHistoryEntry
{	PROPERTY (private,	int, Index,		NO_OPS,	STOCK_WRITE)
	PROPERTY (private,	str, OldCode,	NO_OPS,	STOCK_WRITE)
	PROPERTY (private,	str, NewCode,	NO_OPS,	STOCK_WRITE)

	public:
		IMPLEMENT_HISTORY_TYPE (Edit)

		EditHistory (int idx, str oldCode, str newCode) :
				m_Index (idx),
				m_OldCode (oldCode),
				m_NewCode (newCode) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class AddHistory : public AbstractHistoryEntry
{	PROPERTY (private,	int,	Index,	NO_OPS,	STOCK_WRITE)
	PROPERTY (private,	str,	Code,		NO_OPS,	STOCK_WRITE)

	public:
		IMPLEMENT_HISTORY_TYPE (Add)

		AddHistory (int idx, LDObject* obj) :
				m_Index (idx),
				m_Code (obj->raw()) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class MoveHistory : public AbstractHistoryEntry
{	public:
		IMPLEMENT_HISTORY_TYPE (Move)

		QList<int> indices;
		vertex dest;

		MoveHistory (QList<int> indices, vertex dest) :
				indices (indices),
				dest (dest) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class SwapHistory : public AbstractHistoryEntry
{	public:
		IMPLEMENT_HISTORY_TYPE (Swap)

		SwapHistory (int a, int b) :
				a (a),
				b (b) {}

	private:
		int a, b;
};

#endif // LDFORGE_HISTORY_H
