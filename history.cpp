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

#include "history.h"
#include "ldtypes.h"
#include "file.h"
#include "misc.h"
#include "gui.h"

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
namespace History {
	std::vector<HistoryEntry*> entries;
	
	static long lPos = -1;
	
	// =========================================================================
	void addEntry (HistoryEntry* entry) {
		// If there's any entries after our current position, we need to remove them now
		for (ulong i = lPos + 1; i < entries.size(); ++i) {
			
			delete entries[i];
			entries.erase (entries.begin() + i);
		}
		
		entries.push_back (entry);
		lPos++;
	}
	
	// =========================================================================
	void undo () {
		if (lPos == -1)
			return; // nothing to undo
		
		entries[lPos--]->undo ();
	}
	
	// =========================================================================
	void redo () {
		if (lPos == (long) entries.size () - 1)
			return; // nothing to redo;
		
		entries[++lPos]->redo ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void DeleteHistory::undo () {
	for (ulong i = 0; i < cache.size(); ++i) {
		LDObject* obj = cache[i]->clone ();
		g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + indices[i], obj);
	}
	
	g_ForgeWindow->refresh ();
}

// =============================================================================
void DeleteHistory::redo () {
	for (ulong i = 0; i < cache.size(); ++i) {
		LDObject* obj = g_CurrentFile->objects[indices[i]];
		
		g_CurrentFile->forgetObject (obj);
		delete obj;
	}
	
	g_ForgeWindow->refresh ();
}

// =============================================================================
DeleteHistory::~DeleteHistory () {
	for (LDObject* obj : cache)
		delete obj;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void SetColorHistory::undo () {
	// Restore colors
	for (ulong i = 0; i < ulaIndices.size (); ++i)
		g_CurrentFile->objects[ulaIndices[i]]->dColor = daColors[i];
	
	g_ForgeWindow->refresh ();
}

void SetColorHistory::redo () {
	// Re-set post color
	for (ulong i = 0; i < ulaIndices.size (); ++i)
		g_CurrentFile->objects[ulaIndices[i]]->dColor = dNewColor;
	
	g_ForgeWindow->refresh ();
}

SetColorHistory::~SetColorHistory () {}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void SetContentsHistory::undo () {
	LDObject* obj = g_CurrentFile->objects[ulIndex];
	obj->replace (oldObj->clone ());
	g_ForgeWindow->refresh ();
}

void SetContentsHistory::redo () {
	LDObject* obj = g_CurrentFile->objects[ulIndex];
	obj->replace (newObj->clone ());
	g_ForgeWindow->refresh ();
}

SetContentsHistory::~SetContentsHistory () {
	delete oldObj;
	delete newObj;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
std::vector<LDObject*> ListMoveHistory::getObjects (short ofs) {
	std::vector<LDObject*> objs;
	
	for (ulong idx : ulaIndices)
		objs.push_back (g_CurrentFile->objects[idx + ofs]);
	
	return objs;
}

void ListMoveHistory::undo () {
	std::vector<LDObject*> objs = getObjects (bUp ? -1 : 1);
	LDObject::moveObjects (objs, !bUp);
	g_ForgeWindow->buildObjList ();
}

void ListMoveHistory::redo () {
	std::vector<LDObject*> objs = getObjects (0);
	LDObject::moveObjects (objs, bUp);
	g_ForgeWindow->buildObjList ();
}

ListMoveHistory::~ListMoveHistory() {}