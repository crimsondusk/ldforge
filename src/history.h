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

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class HistoryEntry {
public:
	virtual void undo () {}
	virtual void redo () {}
	virtual ~HistoryEntry () {}
	virtual HistoryType type () { return (HistoryType)(0); }
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class DelHistory : public HistoryEntry {
public:
	enum Type {
		Cut,	// were deleted with a cut operation
		Other,	// were deleted witout specific reason
	};
	
	IMPLEMENT_HISTORY_TYPE (Del)
	
	vector<ulong> indices;
	vector<LDObject*> cache;
	const Type eType;
	
	DelHistory (vector<ulong> indices, vector<LDObject*> cache, const Type eType = Other) :
		indices (indices), cache (cache), eType (eType) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class SetColorHistory : public HistoryEntry {
public:
	IMPLEMENT_HISTORY_TYPE (SetColor)
	
	vector<ulong> ulaIndices;
	vector<short> daColors;
	short dNewColor;
	
	SetColorHistory (vector<ulong> ulaIndices, vector<short> daColors, short dNewColor) :
		ulaIndices (ulaIndices), daColors (daColors), dNewColor (dNewColor) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class EditHistory : public HistoryEntry {
public:
	IMPLEMENT_HISTORY_TYPE (Edit)
	
	std::vector<ulong> ulaIndices;
	std::vector<LDObject*> paOldObjs, paNewObjs;
	
	EditHistory () {}
	EditHistory (std::vector<ulong> ulaIndices, std::vector<LDObject*> paOldObjs,
		std::vector<LDObject*> paNewObjs) :
		ulaIndices (ulaIndices), paOldObjs (paOldObjs), paNewObjs (paNewObjs) {}
	
	void	addEntry		(LDObject* const oldObj, LDObject* const newObj);
	void	addEntry		(LDObject* const oldObj, LDObject* const newObj, const ulong idx);
	ulong	numEntries		() const { return ulaIndices.size (); }
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class ListMoveHistory : public HistoryEntry {
public:
	IMPLEMENT_HISTORY_TYPE (ListMove)
	
	std::vector<ulong> ulaIndices;
	bool bUp;
	
	std::vector<LDObject*> getObjects (short ofs);
	ListMoveHistory (vector<ulong> ulaIndices, const bool bUp) :
		ulaIndices (ulaIndices), bUp (bUp) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class AddHistory : public HistoryEntry {
public:
	enum Type {
		Other,	// was "just added"
		Paste,	// was added through a paste operation
	};
	
	IMPLEMENT_HISTORY_TYPE (Add)
	
	std::vector<ulong> ulaIndices;
	std::vector<LDObject*> paObjs;
	const Type eType;
	
	AddHistory (std::vector<ulong> ulaIndices, std::vector<LDObject*> paObjs,
		const Type eType = Other) :
		ulaIndices (ulaIndices), paObjs (paObjs), eType (eType) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class QuadSplitHistory : public HistoryEntry {
public:
	IMPLEMENT_HISTORY_TYPE (QuadSplit)
	
	std::vector<ulong> ulaIndices;
	std::vector<LDQuad*> paQuads;
	
	QuadSplitHistory (std::vector<ulong> ulaIndices, std::vector<LDQuad*> paQuads) :
		ulaIndices (ulaIndices), paQuads (paQuads) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class InlineHistory : public HistoryEntry {
public:
	IMPLEMENT_HISTORY_TYPE (Inline)
	
	const std::vector<ulong> ulaBitIndices, ulaRefIndices;
	const std::vector<LDSubfile*> paRefs;
	const bool bDeep;
	
	InlineHistory (const std::vector<ulong> ulaBitIndices, const std::vector<ulong> ulaRefIndices,
		const std::vector<LDSubfile*> paRefs, const bool bDeep) :
		ulaBitIndices (ulaBitIndices), ulaRefIndices (ulaRefIndices), paRefs (paRefs), bDeep (bDeep) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class MoveHistory : public HistoryEntry {
public:
	IMPLEMENT_HISTORY_TYPE (Move)
	
	const std::vector<ulong> ulaIndices;
	const vertex vVector;
	
	MoveHistory (const std::vector<ulong> ulaIndices, const vertex vVector) :
		ulaIndices (ulaIndices), vVector (vVector) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class ComboHistory : public HistoryEntry {
public:
	IMPLEMENT_HISTORY_TYPE (Combo)
	
	std::vector<HistoryEntry*> paEntries;
	
	ComboHistory () {}
	ComboHistory (std::vector<HistoryEntry*> paEntries) : paEntries (paEntries) {}
	
	void			addEntry		(HistoryEntry* entry) { if (entry) paEntries.push_back (entry); }
	ulong			numEntries		() const { return paEntries.size (); }
	ComboHistory&	operator<<		(HistoryEntry* entry) { addEntry (entry); return *this;}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
namespace History {
	void addEntry (HistoryEntry* entry);
	void undo ();
	void redo ();
	void clear ();
	void updateActions ();
	long pos ();
	std::vector<HistoryEntry*>& entries ();
};

#endif // HISTORY_H