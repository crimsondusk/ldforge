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

#include "gui.h"
#include "common.h"
#include "file.h"
#include "history.h"
#include "zz_colorSelectDialog.h"
#include "zz_historyDialog.h"
#include "zz_setContentsDialog.h"

vector<LDObject*> g_Clipboard;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static bool copyToClipboard () {
	vector<LDObject*> objs = g_ForgeWindow->selection ();
	
	if (objs.size() == 0)
		return false;
	
	// Clear the clipboard first.
	for (LDObject* obj : g_Clipboard)
		delete obj;
	
	g_Clipboard.clear ();
	
	// Now, copy the contents into the clipboard. The objects should be
	// separate objects so that modifying the existing ones does not affect
	// the clipboard. Thus, we add clones of the objects to the clipboard, not
	// the objects themselves.
	for (ulong i = 0; i < objs.size(); ++i)
		g_Clipboard.push_back (objs[i]->clone ());
	
	return true;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (cut, "Cut", "cut", "Cut the current selection to clipboard.", CTRL (X)) {
	vector<ulong> ulaIndices;
	vector<LDObject*> copies;
	
	if (!copyToClipboard ())
		return;
	
	g_ForgeWindow->deleteSelection (&ulaIndices, &copies);
	History::addEntry (new DelHistory (ulaIndices, copies, DelHistory::Cut));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (copy, "Copy", "copy", "Copy the current selection to clipboard.", CTRL (C)) {
	copyToClipboard ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (paste, "Paste", "paste", "Paste clipboard contents.", CTRL (V)) {
	vector<ulong> ulaIndices;
	vector<LDObject*> paCopies;
	
	ulong idx = g_ForgeWindow->getInsertionPoint ();
	
	for (LDObject* obj : g_Clipboard) {
		ulaIndices.push_back (idx);
		paCopies.push_back (obj->clone ());
		
		g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + idx++, obj->clone ());
	}
	
	History::addEntry (new AddHistory (ulaIndices, paCopies, AddHistory::Paste));
	g_ForgeWindow->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (del, "Delete", "delete", "Delete the selection", KEY (Delete)) {
	vector<ulong> ulaIndices;
	vector<LDObject*> copies;
	
	g_ForgeWindow->deleteSelection (&ulaIndices, &copies);
	
	if (copies.size ())
		History::addEntry (new DelHistory (ulaIndices, copies));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void doInline (bool bDeep) {
	vector<LDObject*> sel = g_ForgeWindow->selection ();
	
	// History stuff
	vector<LDSubfile*> paRefs;
	vector<ulong> ulaRefIndices, ulaBitIndices;
	
	for (LDObject* obj : sel) {
		if (obj->getType() != OBJ_Subfile)
			continue;
		
		ulaRefIndices.push_back (obj->getIndex (g_CurrentFile));
		paRefs.push_back (static_cast<LDSubfile*> (obj)->clone ());
	}
	
	for (LDObject* obj : sel) {
		// Obviously, only subfiles can be inlined.
		if (obj->getType() != OBJ_Subfile)
			continue;
		
		// Get the index of the subfile so we know where to insert the
		// inlined contents.
		long idx = obj->getIndex (g_CurrentFile);
		if (idx == -1)
			continue;
		
		LDSubfile* ref = static_cast<LDSubfile*> (obj);
		
		// Get the inlined objects. These are clones of the subfile's contents.
		vector<LDObject*> objs = ref->inlineContents (bDeep, true);
		
		// Merge in the inlined objects
		for (LDObject* inlineobj : objs) {
			ulaBitIndices.push_back (idx);
			
			// This object is now inlined so it has no parent anymore.
			inlineobj->parent = null;
			
			g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + idx++, inlineobj);
		}
		
		// Delete the subfile now as it's been inlined.
		g_CurrentFile->forgetObject (ref);
		delete ref;
	}
	
	History::addEntry (new InlineHistory (ulaBitIndices, ulaRefIndices, paRefs, bDeep));
	g_ForgeWindow->refresh ();
}

ACTION (inlineContents, "Inline", "inline", "Inline selected subfiles.", CTRL (I)) {
	doInline (false);
}

ACTION (deepInline, "Deep Inline", "inline-deep", "Recursively inline selected subfiles "
	"down to polygons only.", CTRL_SHIFT (I))
{
	doInline (true);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (splitQuads, "Split Quads", "quad-split", "Split quads into triangles.", (0)) {
	vector<LDObject*> objs = g_ForgeWindow->selection ();
	
	vector<ulong> ulaIndices;
	vector<LDQuad*> paCopies;
	
	// Store stuff first for history archival
	for (LDObject* obj : objs) {
		if (obj->getType() != OBJ_Quad)
			continue;
		
		ulaIndices.push_back (obj->getIndex (g_CurrentFile));
		paCopies.push_back (static_cast<LDQuad*> (obj)->clone ());
	}
	
	for (LDObject* obj : objs) {
		if (obj->getType() != OBJ_Quad)
			continue;
		
		// Find the index of this quad
		long lIndex = obj->getIndex (g_CurrentFile);
		
		if (lIndex == -1)
			return;
		
		std::vector<LDTriangle*> triangles = static_cast<LDQuad*> (obj)->splitToTriangles ();
		
		// Replace the quad with the first triangle and add the second triangle
		// after the first one.
		g_CurrentFile->objects[lIndex] = triangles[0];
		g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + lIndex + 1, triangles[1]);
		
		// Delete this quad now, it has been split.
		delete obj;
	}
	
	History::addEntry (new QuadSplitHistory (ulaIndices, paCopies));
	g_ForgeWindow->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (setContents, "Set Contents", "set-contents", "Set the raw code of this object.", KEY (F9)) {
	if (g_ForgeWindow->qObjList->selectedItems().size() != 1)
		return;
	
	LDObject* obj = g_ForgeWindow->selection ()[0];
	SetContentsDialog::staticDialog (obj);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (setColor, "Set Color", "palette", "Set the color on given objects.", KEY (F10)) {
	if (g_ForgeWindow->qObjList->selectedItems().size() <= 0)
		return;
	
	short dColor;
	short dDefault = -1;
	
	std::vector<LDObject*> objs = g_ForgeWindow->selection ();
	
	// If all selected objects have the same color, said color is our default
	// value to the color selection dialog.
	dDefault = g_ForgeWindow->getSelectedColor ();
	
	// Show the dialog to the user now and ask for a color.
	if (ColorSelectDialog::staticDialog (dColor, dDefault, g_ForgeWindow)) {
		std::vector<ulong> ulaIndices;
		std::vector<short> daColors;
		
		for (LDObject* obj : objs) {
			if (obj->dColor != -1) {
				ulaIndices.push_back (obj->getIndex (g_CurrentFile));
				daColors.push_back (obj->dColor);
				
				obj->dColor = dColor;
			}
		}
		
		History::addEntry (new SetColorHistory (ulaIndices, daColors, dColor));
		g_ForgeWindow->refresh ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (makeBorders, "Make Borders", "make-borders", "Add borders around given polygons.",
	CTRL_SHIFT (B))
{
	vector<LDObject*> objs = g_ForgeWindow->selection ();
	
	vector<ulong> ulaIndices;
	vector<LDObject*> paObjs;
	
	for (LDObject* obj : objs) {
		if (obj->getType() != OBJ_Quad && obj->getType() != OBJ_Triangle)
			continue;
		
		short dNumLines;
		LDLine* lines[4];
		
		if (obj->getType() == OBJ_Quad) {
			dNumLines = 4;
			
			LDQuad* quad = static_cast<LDQuad*> (obj);
			lines[0] = new LDLine (quad->vaCoords[0], quad->vaCoords[1]);
			lines[1] = new LDLine (quad->vaCoords[1], quad->vaCoords[2]);
			lines[2] = new LDLine (quad->vaCoords[2], quad->vaCoords[3]);
			lines[3] = new LDLine (quad->vaCoords[3], quad->vaCoords[0]);
		} else {
			dNumLines = 3;
			
			LDTriangle* tri = static_cast<LDTriangle*> (obj);
			lines[0] = new LDLine (tri->vaCoords[0], tri->vaCoords[1]);
			lines[1] = new LDLine (tri->vaCoords[1], tri->vaCoords[2]);
			lines[2] = new LDLine (tri->vaCoords[2], tri->vaCoords[0]); 
		}
		
		for (short i = 0; i < dNumLines; ++i) {
			ulong idx = obj->getIndex (g_CurrentFile) + i + 1;
			
			lines[i]->dColor = dEdgeColor;
			g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + idx, lines[i]);
			
			ulaIndices.push_back (idx);
			paObjs.push_back (lines[i]->clone ());
		}
	}
	
	History::addEntry (new AddHistory (ulaIndices, paObjs));
	g_ForgeWindow->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (makeCornerVerts, "Make Corner Vertices", "corner-verts",
	"Adds vertex objects to the corners of the given polygons", (0))
{
	vector<ulong> ulaIndices;
	vector<LDObject*> paObjs;
	
	for (LDObject* obj : g_ForgeWindow->selection ()) {
		vertex* vaCoords = null;
		ushort uNumCoords = 0;
		
		switch (obj->getType ()) {
		case OBJ_Quad:
			uNumCoords = 4;
			vaCoords = static_cast<LDQuad*> (obj)->vaCoords;
			break;
		
		case OBJ_Triangle:
			uNumCoords = 3;
			vaCoords = static_cast<LDTriangle*> (obj)->vaCoords;
			break;
		
		case OBJ_Line:
			uNumCoords = 2;
			vaCoords = static_cast<LDLine*> (obj)->vaCoords;
			break;
		
		default:
			break;
		}
		
		ulong idx = obj->getIndex (g_CurrentFile);
		for (ushort i = 0; i < uNumCoords; ++i) {
			LDVertex* pVert = new LDVertex;
			pVert->vPosition = vaCoords[i];
			pVert->dColor = obj->dColor;
			
			g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + ++idx, pVert);
			ulaIndices.push_back (idx);
			paObjs.push_back (pVert->clone ());
		}
	}
	
	if (ulaIndices.size() > 0) {
		History::addEntry (new AddHistory (ulaIndices, paObjs));
		g_ForgeWindow->refresh ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void doMoveSelection (const bool bUp) {
	vector<LDObject*> objs = g_ForgeWindow->selection ();
	
	// Get the indices of the objects for history archival
	vector<ulong> ulaIndices;
	for (LDObject* obj : objs)
		ulaIndices.push_back (obj->getIndex (g_CurrentFile));
	
	LDObject::moveObjects (objs, bUp);
	History::addEntry (new ListMoveHistory (ulaIndices, bUp));
	g_ForgeWindow->buildObjList ();
}

ACTION (moveUp, "Move Up", "arrow-up", "Move the current selection up.", CTRL (Up)) {
	doMoveSelection (true);
}

ACTION (moveDown, "Move Down", "arrow-down", "Move the current selection down.", CTRL (Down)) {
	doMoveSelection (false);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (undo, "Undo", "undo", "Undo a step.", CTRL (Z)) {
	History::undo ();
}

ACTION (redo, "Redo", "redo", "Redo a step.", CTRL_SHIFT (Z)) {
	History::redo ();
}

ACTION (showHistory, "Show History", "history", "Show the history dialog.", (0)) {
	HistoryDialog dlg;
	dlg.exec ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void doMoveObjects (const vertex vVector) {
	vector<ulong> ulaIndices;
	
	for (LDObject* obj : g_ForgeWindow->selection ()) {
		ulaIndices.push_back (obj->getIndex (g_CurrentFile));
		obj->move (vVector);
	}
	
	History::addEntry (new MoveHistory (ulaIndices, vVector));
	g_ForgeWindow->refresh ();
}

ACTION (moveXNeg, "Move -X", "move-x-neg", "Move selected objects negative on the X axis.", KEY (Left)) {
	doMoveObjects ({-1, 0, 0});
}

ACTION (moveYNeg, "Move -Y", "move-y-neg", "Move selected objects negative on the Y axis.", KEY (PageUp)) {
	doMoveObjects ({0, -1, 0});
}

ACTION (moveZNeg, "Move -Z", "move-z-neg", "Move selected objects negative on the Z axis.", KEY (Down)) {
	doMoveObjects ({0, 0, -1});
}

ACTION (moveXPos, "Move +X", "move-x-pos", "Move selected objects positive on the X axis.", KEY (Right)) {
	doMoveObjects ({1, 0, 0});
}

ACTION (moveYPos, "Move +Y", "move-y-pos", "Move selected objects positive on the Y axis.", KEY (PageDown)) {
	doMoveObjects ({0, 1, 0});
}

ACTION (moveZPos, "Move +Z", "move-z-pos", "Move selected objects positive on the Z axis.", KEY (Up)) {
	doMoveObjects ({0, 0, 1});
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// invert - reverse winding
// 
// NOTE: History management gets a little tricky here. For lines, cond-lines,
// triangles and quads, we edit the object, thus we record an EditHistory. For
// subfiles and radials we create or delete invertnext objects. Since we have
// multiple actions of different types, we store a history entry for each of
// them and wrap them into a ComboHistory, which allows storage of multiple
// simultaneous edits with different type. This is what we ultimately store into
// History.
// =============================================================================
ACTION (invert, "Invert", "invert", "Reverse the winding of given objects.", CTRL_SHIFT (W)) {
	std::vector<LDObject*> paSelection = g_ForgeWindow->selection ();
	std::vector<HistoryEntry*> paHistory;
	
	for (LDObject* obj : paSelection) {
		// For the objects we end up editing, we store information into these
		// variables and we store them into an EditHistory after the switch
		// block. Subfile and radial management is stored into the history
		// list immediately.
		ulong ulHistoryIndex = obj->getIndex (g_CurrentFile);
		LDObject* pOldCopy, *pNewCopy;
		bool bEdited = false;
		
		switch (obj->getType ()) {
		case OBJ_Line:
		case OBJ_CondLine:
			{
				// For lines, we swap the vertices. I don't think that a
				// cond-line's control points need to be swapped, do they?
				LDLine* pLine = static_cast<LDLine*> (obj);
				vertex vTemp = pLine->vaCoords[0];
				
				pOldCopy = pLine->clone ();
				pLine->vaCoords[0] = pLine->vaCoords[1];
				pLine->vaCoords[1] = vTemp;
				pNewCopy = pLine->clone ();
				bEdited = true;
			}
			break;
		
		case OBJ_Triangle:
			{
				// Triangle goes 0 -> 1 -> 2, reversed: 0 -> 2 -> 1.
				// Thus, we swap 1 and 2.
				LDTriangle* pTri = static_cast<LDTriangle*> (obj);
				vertex vTemp = pTri->vaCoords[1];
				
				pOldCopy = pTri->clone ();
				pTri->vaCoords[1] = pTri->vaCoords[2];
				pTri->vaCoords[2] = vTemp;
				pNewCopy = pTri->clone ();
				bEdited = true;
			}
			break;
		
		case OBJ_Quad:
			{
				// Quad: 0 -> 1 -> 2 -> 3
				// rev:  0 -> 3 -> 2 -> 1
				// Thus, we swap 1 and 3.
				LDQuad* pQuad = static_cast<LDQuad*> (obj);
				vertex vTemp = pQuad->vaCoords[1];
				
				pOldCopy = pQuad->clone ();
				pQuad->vaCoords[1] = pQuad->vaCoords[3];
				pQuad->vaCoords[3] = vTemp;
				pNewCopy = pQuad->clone ();
				bEdited = true;
			}
			break;
		
		case OBJ_Subfile:
		case OBJ_Radial:
			{
				// Subfiles and radials are inverted when they're prefixed with
				// a BFC INVERTNEXT statement. Thus we need to toggle this status.
				// For flat primitives it's sufficient that the determinant is
				// flipped but I don't have a method for checking flatness yet.
				// Food for thought...
				
				bool inverted = false;
				ulong idx = obj->getIndex (g_CurrentFile);
				
				if (idx > 0) {
					LDObject* prev = g_CurrentFile->object (idx - 1);
					LDBFC* bfc = dynamic_cast<LDBFC*> (prev);
					
					if (bfc && bfc->eStatement == LDBFC::InvertNext) {
						// Object is prefixed with an invertnext, thus remove it.
						paHistory.push_back (new DelHistory ({idx - 1}, {bfc->clone ()}));
						
						inverted = true;
						g_CurrentFile->forgetObject (bfc);
						delete bfc;
					}
				}
				
				if (!inverted) {
					// Not inverted, thus prefix it with a new invertnext.
					LDBFC* bfc = new LDBFC (LDBFC::InvertNext);
					g_CurrentFile->objects.insert (g_CurrentFile->objects.begin () + idx, bfc);
					
					paHistory.push_back (new AddHistory ({idx}, {bfc->clone ()}));
				}
			}
			break;
		
		default:
			break;
		}
		
		// If we edited this object, store the EditHistory based on collected
		// information now.
		if (bEdited == true)
			paHistory.push_back (new EditHistory ({ulHistoryIndex}, {pOldCopy}, {pNewCopy}));
	}
	
	if (paHistory.size () > 0) {
		History::addEntry (new ComboHistory (paHistory));
		g_ForgeWindow->refresh ();
	}
}