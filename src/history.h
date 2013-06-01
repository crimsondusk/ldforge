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
	virtual void undo (); \
	virtual void redo (); \
	virtual HistoryType type () { return HISTORY_##N; }

class AbstractHistoryEntry;

// =============================================================================
enum HistoryType {
	HISTORY_Del,
	HISTORY_SetColor,
	HISTORY_Edit,
	HISTORY_ListMove,
	HISTORY_Add,
	HISTORY_QuadSplit,
	HISTORY_Inline,
	HISTORY_Move,
	HISTORY_Combo,
};

class History {
	PROPERTY (long, pos, setPos)
	
public:
	History ();
	void undo ();
	void redo ();
	void clear ();
	void updateActions ();
	
private:
	vector<vector<AbstractHistoryEntry*>> m_entries;
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class AbstractHistoryEntry {
public:
	virtual void undo () {}
	virtual void redo () {}
	virtual ~AbstractHistoryEntry () {}
	virtual HistoryType type () { return (HistoryType)(0); }
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
	PROPERTY (LDObject*, copy, setCopy)
	PROPERTY (DelHistory::Type, type, setType)
	
public:
	IMPLEMENT_HISTORY_TYPE (Del)
	
	DelHistory (ulong idx, LDObject* copy, Type type) :
		m_index (idx), m_copy (copy), m_type (type) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class EditHistory : public AbstractHistoryEntry {
	PROPERTY (ulong, index, setIndex)
	PROPERTY (LDObject*, oldCopy, setOldCopy)
	PROPERTY (LDObject*, newCopy, setNewCopy)
	
public:
	IMPLEMENT_HISTORY_TYPE (Edit)
	
	EditHistory (ulong idx, LDObject* oldcopy, LDObject* newcopy) :
		m_index (idx), m_oldCopy (oldcopy), m_newCopy (newcopy) {}
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
	PROPERTY (LDObject*, copy, setCopy)
	PROPERTY (AddHistory::Type, type, setType)
	
public:
	IMPLEMENT_HISTORY_TYPE (Add)
	
	AddHistory (ulong idx, LDObject* copy, Type type = Other) :
		m_index (idx), m_copy (copy), m_type (type) {}
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