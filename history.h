/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri `arezey` Piippo
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
	HISTORY_Delete,
	HISTORY_SetColor,
	HISTORY_SetContents,
	HISTORY_ListMove,
	HISTORY_Addition,
	HISTORY_QuadSplit,
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
class DeleteHistory : public HistoryEntry {
public:
	IMPLEMENT_HISTORY_TYPE (Delete)
	
	vector<ulong> indices;
	vector<LDObject*> cache;
	
	DeleteHistory (vector<ulong> indices, vector<LDObject*> cache) :
		indices (indices), cache (cache) {}
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
class SetContentsHistory : public HistoryEntry {
public:
	IMPLEMENT_HISTORY_TYPE (SetContents)
	
	ulong ulIndex;
	LDObject* oldObj, *newObj;
	
	SetContentsHistory (ulong ulIndex, LDObject* oldObj, LDObject* newObj) :
		ulIndex (ulIndex), oldObj (oldObj), newObj (newObj) {}
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
class AdditionHistory : public HistoryEntry {
public:
	IMPLEMENT_HISTORY_TYPE (Addition)
	
	std::vector<ulong> ulaIndices;
	std::vector<LDObject*> paObjs;
	
	AdditionHistory (std::vector<ulong> ulaIndices, std::vector<LDObject*> paObjs) :
		ulaIndices (ulaIndices), paObjs (paObjs) {}
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
namespace History {
	extern std::vector<HistoryEntry*> entries;
	
	void addEntry (HistoryEntry* entry);
	void undo ();
	void redo ();
	void clear ();
	void updateActions ();
};

#endif // HISTORY_H