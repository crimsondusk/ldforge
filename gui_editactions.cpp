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

#include "gui.h"
#include "common.h"
#include "zz_setContentsDialog.h"
#include "file.h"
#include "zz_colorSelectDialog.h"

vector<LDObject*> g_Clipboard;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static bool copyToClipboard () {
	vector<LDObject*> objs = g_ForgeWindow->getSelectedObjects ();
	
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
	if (!copyToClipboard ())
		return;
	
	g_ForgeWindow->deleteSelection ();
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
	for (LDObject* obj : g_Clipboard)
		g_CurrentFile->addObject (obj->clone ());
	
	g_ForgeWindow->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (del, "Delete", "delete", "Delete the selection", KEY (Delete)) {
	g_ForgeWindow->deleteSelection ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void doInline (bool bDeep) {
	vector<LDObject*> sel = g_ForgeWindow->getSelectedObjects ();
	
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
		for (LDObject* inlineobj : objs)
			g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + idx++, inlineobj);
		
		// Delete the subfile now as it's been inlined.
		g_CurrentFile->forgetObject (ref);
		delete ref;
	}
	
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
	vector<LDObject*> objs = g_ForgeWindow->getSelectedObjects ();
	
	for (LDObject* obj : objs) {
		if (obj->getType() != OBJ_Quad)
			continue;
		
		// Find the index of this quad
		long lIndex = obj->getIndex (g_CurrentFile);
		
		if (lIndex == -1) {
			// couldn't find it?
			logf (LOG_Error, "Couldn't find quad %p in "
				"current object list!!\n", obj);
			return;
		}
		
		std::vector<LDTriangle*> triangles = static_cast<LDQuad*> (obj)->splitToTriangles ();
		
		// Replace the quad with the first triangle and add the second triangle
		// after the first one.
		g_CurrentFile->objects[lIndex] = triangles[0];
		g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + lIndex + 1, triangles[1]);
		
		// Delete this quad now, it has been split.
		delete obj;
	}
	
	g_ForgeWindow->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (setContents, "Set Contents", "set-contents", "Set the raw code of this object.", KEY (F9)) {
	if (g_ForgeWindow->qObjList->selectedItems().size() != 1)
		return;
	
	LDObject* obj = g_ForgeWindow->getSelectedObjects ()[0];
	SetContentsDialog::staticDialog (obj, g_ForgeWindow);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (setColor, "Set Color", "palette", "Set the color on given objects.", KEY (F10)) {
	if (g_ForgeWindow->qObjList->selectedItems().size() <= 0)
		return;
	
	short dColor;
	short dDefault = -1;
	
	std::vector<LDObject*> objs = g_ForgeWindow->getSelectedObjects ();
	
	// If all selected objects have the same color, said color is our default
	// value to the color selection dialog.
	for (LDObject* obj : objs) {
		if (obj->dColor == -1)
			continue; // doesn't use color
		
		if (dDefault != -1 && obj->dColor != dDefault) {
			// No consensus in object color, therefore we don't have a
			// proper default value to use.
			dDefault = -1;
			break;
		}
		
		if (dDefault == -1)
			dDefault = obj->dColor;
	}
	
	// Show the dialog to the user now and ask for a color.
	if (ColorSelectDialog::staticDialog (dColor, dDefault, g_ForgeWindow)) {
		for (LDObject* obj : objs)
			if (obj->dColor != -1)
				obj->dColor = dColor;
		
		g_ForgeWindow->refresh ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (makeBorders, "Make Borders", "make-borders", "Add borders around given polygons.",
	CTRL_SHIFT (B))
{
	vector<LDObject*> objs = g_ForgeWindow->getSelectedObjects ();
	
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
			lines[i]->dColor = dEdgeColor;
			g_CurrentFile->addObject (lines[i]);
		}
	}
	
	g_ForgeWindow->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void doMoveSelection (const bool bUp) {
	vector<LDObject*> objs = g_ForgeWindow->getSelectedObjects ();
	
	// If we move down, we need to iterate the array in reverse order.
	const long start = bUp ? 0 : (objs.size() - 1);
	const long end = bUp ? objs.size() : -1;
	const long incr = bUp ? 1 : -1;
	
	for (long i = start; i != end; i += incr) {
		LDObject* obj = objs[i];
		
		const long lIndex = obj->getIndex (g_CurrentFile),
			lTarget = lIndex + (bUp ? -1 : 1);
		
		if ((bUp == true and lIndex == 0) or
			(bUp == false and lIndex == (long)(g_CurrentFile->objects.size() - 1)))
		{
			// One of the objects hit the extrema. If this happens, this should be the first
			// object to be iterated on. Thus, nothing has changed yet and it's safe to just
			// abort the entire operation.
			assert (i == start);
			return;
		}
		
		obj->swap (g_CurrentFile->objects[lTarget]);
	}
	
	g_ForgeWindow->buildObjList ();
}

ACTION (moveUp, "Move Up", "arrow-up", "Move the current selection up.", CTRL (Up)) {
	doMoveSelection (true);
}

ACTION (moveDown, "Move Down", "arrow-down", "Move the current selection down.", CTRL (Down)) {
	doMoveSelection (false);
}