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

#ifndef HISTORY_H
#define HISTORY_H

#include "common.h"
#include "ldtypes.h"

#define IMPLEMENT_HISTORY_TYPE(N) \
	virtual ~N##History(); \
	virtual void undo() const override; \
	virtual void redo() const override; \
	virtual History::Type getType() const override { return History::N; }

class AbstractHistoryEntry;

// =============================================================================
class History
{	PROPERTY (long, pos, setPos)
	PROPERTY (LDFile*, file, setFile)
	READ_PROPERTY (bool, opened, setOpened)

	public:
		typedef List<AbstractHistoryEntry*> Changeset;

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

		void open();
		void close();
		void add (AbstractHistoryEntry* entry);

		inline long size() const
		{	return m_changesets.size();
		}

		inline History& operator<< (AbstractHistoryEntry* entry)
		{	add (entry);
			return *this;
		}

		inline const Changeset& changeset (long pos) const
		{	return m_changesets[pos];
		}

	private:
		Changeset m_currentArchive;
		List<Changeset> m_changesets;
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class AbstractHistoryEntry
{		PROPERTY (History*, parent, setParent)

	public:
		virtual void undo() const {}
		virtual void redo() const {}
		virtual ~AbstractHistoryEntry() {}
		virtual History::Type getType() const
		{	return (History::Type) 0;
		}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class DelHistory : public AbstractHistoryEntry
{	public:
		enum Type
		{	Cut,	// was deleted with a cut operation
			Other,	// was deleted witout specific reason
		};

		PROPERTY (int, index, setIndex)
		PROPERTY (str, code, setCode)
		PROPERTY (DelHistory::Type, type, setType)

	public:
		IMPLEMENT_HISTORY_TYPE (Del)

		DelHistory (int idx, LDObject* obj, Type type = Other) :
				m_index (idx),
				m_code (obj->raw()),
				m_type (type) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class EditHistory : public AbstractHistoryEntry
{	PROPERTY (int, index, setIndex)
	PROPERTY (str, oldCode, setOldCode)
	PROPERTY (str, newCode, setNewCode)

	public:
		IMPLEMENT_HISTORY_TYPE (Edit)

		EditHistory (int idx, str oldCode, str newCode) :
				m_index (idx),
				m_oldCode (oldCode),
				m_newCode (newCode) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class AddHistory : public AbstractHistoryEntry
{	public:
		enum Type
		{	Other,	// was "just added"
			Paste,	// was added through a paste operation
		};

		PROPERTY (int, index, setIndex)
		PROPERTY (str, code, setCode)
		PROPERTY (AddHistory::Type, type, setType)

	public:
		IMPLEMENT_HISTORY_TYPE (Add)

		AddHistory (int idx, LDObject* obj, Type type = Other) :
				m_index (idx),
				m_code (obj->raw()),
				m_type (type) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class MoveHistory : public AbstractHistoryEntry
{	public:
		IMPLEMENT_HISTORY_TYPE (Move)

		List<int> indices;
		vertex dest;

		MoveHistory (List<int> indices, vertex dest) :
				indices (indices),
				dest (dest) {}
};

class SwapHistory : public AbstractHistoryEntry
{	public:
		IMPLEMENT_HISTORY_TYPE (Swap)
		int a, b;

		SwapHistory (int a, int b) :
				a (a),
				b (b) {}
};

#endif // HISTORY_H
