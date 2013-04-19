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

#include "history.h"
#include "ldtypes.h"
#include "file.h"
#include "misc.h"
#include "gui.h"

EXTERN_ACTION (undo)
EXTERN_ACTION (redo)

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
		
		updateActions ();
	}
	
	// =========================================================================
	void undo () {
		if (lPos == -1)
			return; // nothing to undo
		
		entries[lPos--]->undo ();
		updateActions ();
	}
	
	// =========================================================================
	void redo () {
		if (lPos == (long) entries.size () - 1)
			return; // nothing to redo;
		
		entries[++lPos]->redo ();
		updateActions ();
	}
	
	// =========================================================================
	void clear () {
		for (HistoryEntry* entry : entries)
			delete entry;
		
		entries.clear ();
		lPos = -1;
		updateActions ();
	}
	
	// =========================================================================
	void updateActions () {
		ACTION_NAME (undo)->setEnabled (lPos > -1);
		ACTION_NAME (redo)->setEnabled (lPos < (long) entries.size () - 1);
	}
	
	// =========================================================================
	long pos () {
		return lPos;
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void DelHistory::undo () {
	for (ulong i = 0; i < cache.size(); ++i) {
		ulong idx = cache.size() - i - 1;
		LDObject* obj = cache[idx]->clone ();
		g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + indices[idx], obj);
	}
	
	g_ForgeWindow->refresh ();
}

// =============================================================================
void DelHistory::redo () {
	for (ulong i = 0; i < cache.size(); ++i) {
		LDObject* obj = g_CurrentFile->objects[indices[i]];
		
		g_CurrentFile->forgetObject (obj);
		delete obj;
	}
	
	g_ForgeWindow->refresh ();
}

// =============================================================================
DelHistory::~DelHistory () {
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
void EditHistory::undo () {
	for (ulong idx : ulaIndices) {
		LDObject* obj = g_CurrentFile->objects[idx];
		obj->replace (paOldObjs[idx]->clone ());
	}
	
	g_ForgeWindow->refresh ();
}

void EditHistory::redo () {
	for (ulong idx : ulaIndices) {
		LDObject* obj = g_CurrentFile->objects[idx];
		obj->replace (paNewObjs[idx]->clone ());
	}
	
	g_ForgeWindow->refresh ();
}

EditHistory::~EditHistory () {
	for (ulong idx : ulaIndices) {
		delete paOldObjs[idx];
		delete paNewObjs[idx];
	}
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

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
AddHistory::~AddHistory () {
	for (LDObject* pObj : paObjs)
		delete pObj;
}

void AddHistory::undo () {
	for (ulong i = 0; i < paObjs.size(); ++i) {
		ulong idx = ulaIndices[ulaIndices.size() - i - 1];
		LDObject* obj = g_CurrentFile->objects[idx];
		
		g_CurrentFile->forgetObject (obj);
		delete obj;
	}
	
	g_ForgeWindow->refresh ();
}

void AddHistory::redo () {
	for (ulong i = 0; i < paObjs.size(); ++i) {
		ulong idx = ulaIndices[i];
		LDObject* obj = paObjs[i]->clone ();
		
		g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + idx, obj);
	}
	
	g_ForgeWindow->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
QuadSplitHistory::~QuadSplitHistory () {
	for (LDQuad* pQuad : paQuads)
		delete pQuad;
}

void QuadSplitHistory::undo () {
	for (ulong i = 0; i < paQuads.size(); ++i) {
		// The quad was replaced by the first triangle and a second one was
		// added after it. Thus, we remove the second one here and replace
		// the first with a copy of the quad.
		ulong idx = ulaIndices[i];
		
		LDTriangle* tri1 = static_cast<LDTriangle*> (g_CurrentFile->objects[idx]),
			*tri2 = static_cast<LDTriangle*> (g_CurrentFile->objects[idx + 1]);
		LDQuad* pCopy = paQuads[i]->clone ();
		
		tri1->replace (pCopy);
		g_CurrentFile->forgetObject (tri2);
		delete tri2;
	}
	
	g_ForgeWindow->refresh ();
}

void QuadSplitHistory::redo () {
	for (long i = paQuads.size() - 1; i >= 0; --i) {
		ulong idx = ulaIndices[i];
		
		LDQuad* pQuad = static_cast<LDQuad*> (g_CurrentFile->objects[idx]);
		std::vector<LDTriangle*> paTriangles = pQuad->splitToTriangles ();
		
		g_CurrentFile->objects[idx] = paTriangles[0];
		g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + idx + 1, paTriangles[1]);
		delete pQuad;
	}
	
	g_ForgeWindow->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void InlineHistory::undo () {
	for (long i = ulaBitIndices.size() - 1; i >= 0; --i) {
		LDObject* obj = g_CurrentFile->objects [ulaBitIndices[i]];
		g_CurrentFile->forgetObject (obj);
		delete obj;
	}
	
	for (ulong i = 0; i < ulaRefIndices.size(); ++i) {
		LDSubfile* obj = paRefs[i]->clone ();
		g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + ulaRefIndices[i], obj);
	}
	
	g_ForgeWindow->refresh ();
}

void InlineHistory::redo () {
	for (long i = ulaRefIndices.size() - 1; i >= 0; --i) {
		ulong idx = ulaRefIndices[i];
		
		assert (g_CurrentFile->object (idx)->getType () == OBJ_Subfile);
		LDSubfile* ref = static_cast<LDSubfile*> (g_CurrentFile->object (idx));
		vector<LDObject*> objs = ref->inlineContents (bDeep, false);
		
		for (LDObject* obj : objs)
			g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + idx++, obj);
		
		g_CurrentFile->forgetObject (ref);
		delete ref;
	}
	
	g_ForgeWindow->refresh ();
}

InlineHistory::~InlineHistory () {
	for (LDSubfile* ref : paRefs)
		delete ref;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MoveHistory::~MoveHistory () {}

void MoveHistory::undo () {
	const vertex vInverse = -vVector;
	
	for (ulong i : ulaIndices)
		g_CurrentFile->object (i)->move (vInverse);
	g_ForgeWindow->refresh ();
}

void MoveHistory::redo () {
	for (ulong i : ulaIndices)
		g_CurrentFile->object (i)->move (vVector);
	g_ForgeWindow->refresh ();
}