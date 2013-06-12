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
	virtual ~N##History (); \
	virtual void undo () const; \
	virtual void redo () const; \
	virtual History::Type type () { return History::N; }

class AbstractHistoryEntry;

// =============================================================================
class History {
	PROPERTY (long, pos, setPos)
	PROPERTY (LDOpenFile*, file, setFile)
	READ_PROPERTY (bool, opened, setOpened)
	
public:
	typedef vector<AbstractHistoryEntry*> list;
	
	enum Type {
		Del,
		Edit,
		ListMove,
		Add,
		Move,
	};
	
	History ();
	void undo ();
	void redo ();
	void clear ();
	void updateActions () const;
	
	void open ();
	void close ();
	void add (AbstractHistoryEntry* entry);
	long size () const { return m_changesets.size (); }
	
	History& operator<< (AbstractHistoryEntry* entry) {
		add (entry);
		return *this;
	}
	
	const list& changeset (long pos) const {
		return m_changesets[pos];
	}
	
private:
	list m_currentArchive;
	vector<list> m_changesets;
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class AbstractHistoryEntry {
	PROPERTY (History*, parent, setParent)
	
public:
	virtual void undo () const {}
	virtual void redo () const {}
	virtual ~AbstractHistoryEntry () {}
	virtual History::Type type () { return (History::Type) 0; }
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class DelHistory : public AbstractHistoryEntry {
public:
	enum Type {
		Cut,	// was deleted with a cut operation
		Other,	// was deleted witout specific reason
	};
	
	PROPERTY (ulong, index, setIndex)
	PROPERTY (str, code, setCode)
	PROPERTY (DelHistory::Type, type, setType)
	
public:
	IMPLEMENT_HISTORY_TYPE (Del)
	
	DelHistory (ulong idx, LDObject* obj, Type type = Other) :
		m_index (idx), m_code (obj->raw ()), m_type (type) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class EditHistory : public AbstractHistoryEntry {
	PROPERTY (ulong, index, setIndex)
	PROPERTY (str, oldCode, setOldCode)
	PROPERTY (str, newCode, setNewCode)
	
public:
	IMPLEMENT_HISTORY_TYPE (Edit)
	
	EditHistory (ulong idx, str oldCode, str newCode) :
		m_index (idx), m_oldCode (oldCode), m_newCode (newCode) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class ListMoveHistory : public AbstractHistoryEntry {
public:
	IMPLEMENT_HISTORY_TYPE (ListMove)
	
	vector<ulong> idxs;
	long dest;
	
	ListMoveHistory (vector<ulong> idxs, long dest) :
		idxs (idxs), dest (dest) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class AddHistory : public AbstractHistoryEntry {
public:
	enum Type {
		Other,	// was "just added"
		Paste,	// was added through a paste operation
	};
	
	PROPERTY (ulong, index, setIndex)
	PROPERTY (str, code, setCode)
	PROPERTY (AddHistory::Type, type, setType)
	
public:
	IMPLEMENT_HISTORY_TYPE (Add)
	
	AddHistory (ulong idx, LDObject* obj, Type type = Other) :
		m_index (idx), m_code (obj->raw ()), m_type (type) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class MoveHistory : public AbstractHistoryEntry {
public:
	IMPLEMENT_HISTORY_TYPE (Move)
	
	vector<ulong> indices;
	vertex dest;
	
	MoveHistory (vector<ulong> indices, vertex dest) :
		indices (indices), dest (dest) {}
};

#endif // HISTORY_H