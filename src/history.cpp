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
	std::vector<HistoryEntry*> s_entries;
	static long s_pos = -1;
	
	// =========================================================================
	void addEntry (HistoryEntry* entry) {
		// If there's any entries ahead the current position, we need to
		// remove them now
		for (ulong i = s_pos + 1; i < s_entries.size(); ++i) {
			delete s_entries[i];
			s_entries.erase (s_entries.begin() + i);
		}
		
		s_entries.push_back (entry);
		s_pos++;
		
		updateActions ();
	}
	
	// =========================================================================
	void undo () {
		if (s_pos == -1)
			return; // nothing to undo
		
		s_entries[s_pos--]->undo ();
		updateActions ();
	}
	
	// =========================================================================
	void redo () {
		if (s_pos == (long) s_entries.size () - 1)
			return; // nothing to redo;
		
		s_entries[++s_pos]->redo ();
		updateActions ();
	}
	
	// =========================================================================
	void clear () {
		for (HistoryEntry* entry : s_entries)
			delete entry;
		
		s_entries.clear ();
		s_pos = -1;
		updateActions ();
	}
	
	// =========================================================================
	void updateActions () {
		ACTION (undo)->setEnabled (s_pos > -1);
		ACTION (redo)->setEnabled (s_pos < (long) s_entries.size () - 1);
		
		// Update the window title as well
		g_win->setTitle ();
	}
	
	// =========================================================================
	long pos () {
		return s_pos;
	}
	
	// =========================================================================
	std::vector<HistoryEntry*>& entries () {
		return s_entries;
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void DelHistory::undo () {
	for (ulong i = 0; i < cache.size(); ++i) {
		ulong idx = cache.size() - i - 1;
		LDObject* obj = cache[idx]->clone ();
		g_curfile->insertObj (indices[idx], obj);
	}
	
	g_win->refresh ();
}

// =============================================================================
void DelHistory::redo () {
	for (ulong i = 0; i < cache.size(); ++i) {
		LDObject* obj = g_curfile->m_objs[indices[i]];
		
		g_curfile->forgetObject (obj);
		delete obj;
	}
	
	g_win->refresh ();
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
		g_curfile->m_objs[ulaIndices[i]]->color = daColors[i];
	
	g_win->refresh ();
}

void SetColorHistory::redo () {
	// Re-set post color
	for (ulong i = 0; i < ulaIndices.size (); ++i)
		g_curfile->m_objs[ulaIndices[i]]->color = dNewColor;
	
	g_win->refresh ();
}

SetColorHistory::~SetColorHistory () {}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void EditHistory::undo () {
	for (ulong idx : ulaIndices)
		g_curfile->object (idx)->replace (paOldObjs[idx]->clone ());
	
	g_win->refresh ();
}

void EditHistory::redo () {
	for (ulong idx : ulaIndices)
		g_curfile->object (idx)->replace (paNewObjs[idx]->clone ());
	
	g_win->refresh ();
}

void EditHistory::addEntry (LDObject* const oldObj, LDObject* const newObj) {
	printf ("%s at %lu, replaced by a %s\n",
		g_saObjTypeNames[oldObj->getType()],
		oldObj->getIndex (g_curfile),
		g_saObjTypeNames[newObj->getType()]);
	
	ulaIndices.push_back (oldObj->getIndex (g_curfile));
	paOldObjs.push_back (oldObj->clone ());
	paNewObjs.push_back (newObj->clone ());
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
		objs.push_back (g_curfile->m_objs[idx + ofs]);
	
	return objs;
}

void ListMoveHistory::undo () {
	std::vector<LDObject*> objs = getObjects (bUp ? -1 : 1);
	LDObject::moveObjects (objs, !bUp);
	g_win->buildObjList ();
}

void ListMoveHistory::redo () {
	std::vector<LDObject*> objs = getObjects (0);
	LDObject::moveObjects (objs, bUp);
	g_win->buildObjList ();
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
		LDObject* obj = g_curfile->m_objs[idx];
		
		g_curfile->forgetObject (obj);
		delete obj;
	}
	
	g_win->refresh ();
}

void AddHistory::redo () {
	for (ulong i = 0; i < paObjs.size(); ++i) {
		ulong idx = ulaIndices[i];
		LDObject* obj = paObjs[i]->clone ();
		
		g_curfile->insertObj (idx, obj);
	}
	
	g_win->refresh ();
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
		
		LDTriangle* tri1 = static_cast<LDTriangle*> (g_curfile->m_objs[idx]),
			*tri2 = static_cast<LDTriangle*> (g_curfile->m_objs[idx + 1]);
		LDQuad* pCopy = paQuads[i]->clone ();
		
		tri1->replace (pCopy);
		g_curfile->forgetObject (tri2);
		delete tri2;
	}
	
	g_win->refresh ();
}

void QuadSplitHistory::redo () {
	for (long i = paQuads.size() - 1; i >= 0; --i) {
		ulong idx = ulaIndices[i];
		
		LDQuad* pQuad = static_cast<LDQuad*> (g_curfile->m_objs[idx]);
		std::vector<LDTriangle*> paTriangles = pQuad->splitToTriangles ();
		
		g_curfile->m_objs[idx] = paTriangles[0];
		g_curfile->insertObj (idx + 1, paTriangles[1]);
		delete pQuad;
	}
	
	g_win->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void InlineHistory::undo () {
	for (long i = ulaBitIndices.size() - 1; i >= 0; --i) {
		LDObject* obj = g_curfile->m_objs [ulaBitIndices[i]];
		g_curfile->forgetObject (obj);
		delete obj;
	}
	
	for (ulong i = 0; i < ulaRefIndices.size(); ++i) {
		LDSubfile* obj = paRefs[i]->clone ();
		g_curfile->insertObj (ulaRefIndices[i], obj);
	}
	
	g_win->refresh ();
}

void InlineHistory::redo () {
	for (long i = ulaRefIndices.size() - 1; i >= 0; --i) {
		ulong idx = ulaRefIndices[i];
		
		assert (g_curfile->object (idx)->getType () == LDObject::Subfile);
		LDSubfile* ref = static_cast<LDSubfile*> (g_curfile->object (idx));
		vector<LDObject*> objs = ref->inlineContents (bDeep, false);
		
		for (LDObject* obj : objs)
			g_curfile->insertObj (idx++, obj);
		
		g_curfile->forgetObject (ref);
		delete ref;
	}
	
	g_win->refresh ();
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
		g_curfile->object (i)->move (vInverse);
	g_win->refresh ();
}

void MoveHistory::redo () {
	for (ulong i : ulaIndices)
		g_curfile->object (i)->move (vVector);
	g_win->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ComboHistory::~ComboHistory () {
	for (HistoryEntry* pEntry : paEntries)
		delete pEntry;
}

void ComboHistory::undo () {
	for (long i = paEntries.size() - 1; i >= 0; --i) {
		HistoryEntry* pEntry = paEntries[i];
		pEntry->undo ();
	}
}

void ComboHistory::redo () {
	for (HistoryEntry* pEntry : paEntries)
		pEntry->redo ();
}