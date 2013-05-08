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
#include "colorSelectDialog.h"
#include "historyDialog.h"
#include "setContentsDialog.h"
#include "misc.h"
#include "bbox.h"
#include "radiobox.h"
#include "extprogs.h"
#include <qspinbox.h>
#include <qcheckbox.h>

vector<LDObject*> g_Clipboard;

cfg (bool, edit_schemanticinline, false);

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static bool copyToClipboard () {
	vector<LDObject*> objs = g_win->sel ();
	
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
MAKE_ACTION (cut, "Cut", "cut", "Cut the current selection to clipboard.", CTRL (X)) {
	vector<ulong> ulaIndices;
	vector<LDObject*> copies;
	
	if (!copyToClipboard ())
		return;
	
	g_win->deleteSelection (&ulaIndices, &copies);
	History::addEntry (new DelHistory (ulaIndices, copies, DelHistory::Cut));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (copy, "Copy", "copy", "Copy the current selection to clipboard.", CTRL (C)) {
	copyToClipboard ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (paste, "Paste", "paste", "Paste clipboard contents.", CTRL (V)) {
	vector<ulong> historyIndices;
	vector<LDObject*> historyCopies;
	
	ulong idx = g_win->getInsertionPoint ();
	g_win->sel ().clear ();
	
	for (LDObject* obj : g_Clipboard) {
		historyIndices.push_back (idx);
		historyCopies.push_back (obj->clone ());
		
		LDObject* copy = obj->clone ();
		g_curfile->insertObj (idx, copy);
		g_win->sel ().push_back (copy);
	}
	
	History::addEntry (new AddHistory (historyIndices, historyCopies, AddHistory::Paste));
	g_win->refresh ();
	g_win->scrollToSelection ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (del, "Delete", "delete", "Delete the selection", KEY (Delete)) {
	vector<ulong> ulaIndices;
	vector<LDObject*> copies;
	
	g_win->deleteSelection (&ulaIndices, &copies);
	
	if (copies.size ())
		History::addEntry (new DelHistory (ulaIndices, copies));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void doInline (bool bDeep) {
	vector<LDObject*> sel = g_win->sel ();
	
	// History stuff
	vector<LDSubfile*> refs;
	vector<ulong> refIndices, bitIndices;
	
	for (LDObject* obj : sel) {
		if (obj->getType() != LDObject::Subfile)
			continue;
		
		refIndices.push_back (obj->getIndex (g_curfile));
		refs.push_back (static_cast<LDSubfile*> (obj)->clone ());
	}
	
	for (LDObject* obj : sel) {
		// Get the index of the subfile so we know where to insert the
		// inlined contents.
		long idx = obj->getIndex (g_curfile);
		if (idx == -1)
			continue;
		
		vector<LDObject*> objs;
		
		if (obj->getType() == LDObject::Subfile)
			objs = static_cast<LDSubfile*> (obj)->inlineContents (bDeep, true);
		else if (obj->getType() == LDObject::Radial)
			objs = static_cast<LDRadial*> (obj)->decompose (true);
		else
			continue;
		
		// Merge in the inlined objects
		for (LDObject* inlineobj : objs) {
			bitIndices.push_back (idx);
			
			// This object is now inlined so it has no parent anymore.
			inlineobj->parent = null;
			
			g_curfile->insertObj (idx++, inlineobj);
		}
		
		// Delete the subfile now as it's been inlined.
		g_curfile->forgetObject (obj);
		delete obj;
	}
	
	History::addEntry (new InlineHistory (bitIndices, refIndices, refs, bDeep));
	g_win->refresh ();
}

MAKE_ACTION (inlineContents, "Inline", "inline", "Inline selected subfiles.", CTRL (I)) {
	doInline (false);
}

MAKE_ACTION (deepInline, "Deep Inline", "inline-deep", "Recursively inline selected subfiles "
	"down to polygons only.", CTRL_SHIFT (I))
{
	doInline (true);
}

// =======================================================================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =======================================================================================================================================
MAKE_ACTION (radialResolution, "Radial resolution", "radial-resolve", "Resolve radials into primitives.", (0)) {
	vector<str> fails;
	vector<LDObject*> sel = g_win->sel ();
	EditHistory* history = new EditHistory;
	
	for (LDObject* obj : sel) {
		if (obj->getType() != LDObject::Radial)
			continue;
		
		LDRadial* rad = static_cast<LDRadial*> (obj);
		str name = rad->makeFileName ();
		
		OpenFile* file = loadSubfile (name);
		if (file == null) {
			fails.push_back (name);
			continue;
		}
		
		// Create the replacement primitive.
		LDSubfile* prim = new LDSubfile;
		memcpy (&prim->vPosition, &rad->vPosition, sizeof rad->vPosition); // inherit position
		memcpy (&prim->mMatrix, &rad->mMatrix, sizeof rad->mMatrix); // inherit matrix
		prim->dColor = rad->dColor; // inherit color
		prim->zFileName = name;
		prim->pFile = file;
		
		// Add the history entry - this must be done while both objects are still valid.
		history->addEntry (rad, prim);
		
		// Replace the radial with the primitive.
		rad->replace (prim);
	}
	
	// If it was not possible to replace everything, inform the user.
	if (fails.size() > 0) {
		str errmsg = fmt ("Couldn't replace %lu radials as replacement subfiles could not be loaded:<br />", (ulong)fails.size ());
		
		for (str& fail : fails) 
			errmsg += fmt ("* %s<br />", fail.chars ());
		
		critical (errmsg);
	}
	
	History::addEntry (history);
	g_win->refresh ();
}

// =======================================================================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =======================================================================================================================================
MAKE_ACTION (splitQuads, "Split Quads", "quad-split", "Split quads into triangles.", (0)) {
	vector<LDObject*> objs = g_win->sel ();
	
	vector<ulong> ulaIndices;
	vector<LDQuad*> paCopies;
	
	// Store stuff first for history archival
	for (LDObject* obj : objs) {
		if (obj->getType() != LDObject::Quad)
			continue;
		
		ulaIndices.push_back (obj->getIndex (g_curfile));
		paCopies.push_back (static_cast<LDQuad*> (obj)->clone ());
	}
	
	for (LDObject* obj : objs) {
		if (obj->getType() != LDObject::Quad)
			continue;
		
		// Find the index of this quad
		long lIndex = obj->getIndex (g_curfile);
		
		if (lIndex == -1)
			return;
		
		std::vector<LDTriangle*> triangles = static_cast<LDQuad*> (obj)->splitToTriangles ();
		
		// Replace the quad with the first triangle and add the second triangle
		// after the first one.
		g_curfile->m_objs[lIndex] = triangles[0];
		g_curfile->insertObj (lIndex + 1, triangles[1]);
		
		// Delete this quad now, it has been split.
		delete obj;
	}
	
	History::addEntry (new QuadSplitHistory (ulaIndices, paCopies));
	g_win->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (setContents, "Set Contents", "set-contents", "Set the raw code of this object.", KEY (F9)) {
	if (g_win->sel ().size() != 1)
		return;
	
	LDObject* obj = g_win->sel ()[0];
	SetContentsDialog::staticDialog (obj);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (setColor, "Set Color", "palette", "Set the color on given objects.", KEY (F10)) {
	if (g_win->sel ().size() <= 0)
		return;
	
	short dColor;
	short dDefault = -1;
	
	std::vector<LDObject*> objs = g_win->sel ();
	
	// If all selected objects have the same color, said color is our default
	// value to the color selection dialog.
	dDefault = g_win->getSelectedColor ();
	
	// Show the dialog to the user now and ask for a color.
	if (ColorSelectDialog::staticDialog (dColor, dDefault, g_win)) {
		std::vector<ulong> ulaIndices;
		std::vector<short> daColors;
		
		for (LDObject* obj : objs) {
			if (obj->dColor != -1) {
				ulaIndices.push_back (obj->getIndex (g_curfile));
				daColors.push_back (obj->dColor);
				
				obj->dColor = dColor;
			}
		}
		
		History::addEntry (new SetColorHistory (ulaIndices, daColors, dColor));
		g_win->refresh ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (makeBorders, "Make Borders", "make-borders", "Add borders around given polygons.",
	CTRL_SHIFT (B))
{
	vector<LDObject*> objs = g_win->sel ();
	
	vector<ulong> ulaIndices;
	vector<LDObject*> paObjs;
	
	for (LDObject* obj : objs) {
		if (obj->getType() != LDObject::Quad && obj->getType() != LDObject::Triangle)
			continue;
		
		short dNumLines;
		LDLine* lines[4];
		
		if (obj->getType() == LDObject::Quad) {
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
			ulong idx = obj->getIndex (g_curfile) + i + 1;
			
			lines[i]->dColor = edgecolor;
			g_curfile->insertObj (idx, lines[i]);
			
			ulaIndices.push_back (idx);
			paObjs.push_back (lines[i]->clone ());
		}
	}
	
	History::addEntry (new AddHistory (ulaIndices, paObjs));
	g_win->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (makeCornerVerts, "Make Corner Vertices", "corner-verts",
	"Adds vertex objects to the corners of the given polygons", (0))
{
	vector<ulong> ulaIndices;
	vector<LDObject*> paObjs;
	
	for (LDObject* obj : g_win->sel ()) {
		if (obj->vertices () < 2)
			continue;
		
		ulong idx = obj->getIndex (g_curfile);
		for (short i = 0; i < obj->vertices(); ++i) {
			LDVertex* vert = new LDVertex;
			vert->vPosition = obj->vaCoords[i];
			vert->dColor = obj->dColor;
			
			g_curfile->insertObj (++idx, vert);
			ulaIndices.push_back (idx);
			paObjs.push_back (vert->clone ());
		}
	}
	
	if (ulaIndices.size() > 0) {
		History::addEntry (new AddHistory (ulaIndices, paObjs));
		g_win->refresh ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void doMoveSelection (const bool bUp) {
	vector<LDObject*> objs = g_win->sel ();
	
	// Get the indices of the objects for history archival
	vector<ulong> ulaIndices;
	for (LDObject* obj : objs)
		ulaIndices.push_back (obj->getIndex (g_curfile));
	
	LDObject::moveObjects (objs, bUp);
	History::addEntry (new ListMoveHistory (ulaIndices, bUp));
	g_win->buildObjList ();
}

MAKE_ACTION (moveUp, "Move Up", "arrow-up", "Move the current selection up.", SHIFT (Up)) {
	doMoveSelection (true);
}

MAKE_ACTION (moveDown, "Move Down", "arrow-down", "Move the current selection down.", SHIFT (Down)) {
	doMoveSelection (false);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (undo, "Undo", "undo", "Undo a step.", CTRL (Z)) {
	History::undo ();
}

MAKE_ACTION (redo, "Redo", "redo", "Redo a step.", CTRL_SHIFT (Z)) {
	History::redo ();
}

MAKE_ACTION (showHistory, "Edit History", "history", "Show the history dialog.", (0)) {
	HistoryDialog dlg;
	dlg.exec ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void doMoveObjects (vertex vVector) {
	vector<ulong> ulaIndices;
	
	// Apply the grid values
	vVector[X] *= currentGrid ().confs[Grid::X]->value;
	vVector[Y] *= currentGrid ().confs[Grid::Y]->value;
	vVector[Z] *= currentGrid ().confs[Grid::Z]->value;
	
	for (LDObject* obj : g_win->sel ()) {
		ulaIndices.push_back (obj->getIndex (g_curfile));
		obj->move (vVector);
	}
	
	History::addEntry (new MoveHistory (ulaIndices, vVector));
	g_win->refresh ();
}

MAKE_ACTION (moveXNeg, "Move -X", "move-x-neg", "Move selected objects negative on the X axis.", KEY (Left)) {
	doMoveObjects ({-1, 0, 0});
}

MAKE_ACTION (moveYNeg, "Move -Y", "move-y-neg", "Move selected objects negative on the Y axis.", KEY (PageUp)) {
	doMoveObjects ({0, -1, 0});
}

MAKE_ACTION (moveZNeg, "Move -Z", "move-z-neg", "Move selected objects negative on the Z axis.", KEY (Down)) {
	doMoveObjects ({0, 0, -1});
}

MAKE_ACTION (moveXPos, "Move +X", "move-x-pos", "Move selected objects positive on the X axis.", KEY (Right)) {
	doMoveObjects ({1, 0, 0});
}

MAKE_ACTION (moveYPos, "Move +Y", "move-y-pos", "Move selected objects positive on the Y axis.", KEY (PageDown)) {
	doMoveObjects ({0, 1, 0});
}

MAKE_ACTION (moveZPos, "Move +Z", "move-z-pos", "Move selected objects positive on the Z axis.", KEY (Up)) {
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
MAKE_ACTION (invert, "Invert", "invert", "Reverse the winding of given objects.", CTRL_SHIFT (W)) {
	std::vector<LDObject*> paSelection = g_win->sel ();
	std::vector<HistoryEntry*> paHistory;
	
	for (LDObject* obj : paSelection) {
		// For the objects we end up editing, we store information into these
		// variables and we store them into an EditHistory after the switch
		// block. Subfile and radial management is stored into the history
		// list immediately.
		ulong ulHistoryIndex = obj->getIndex (g_curfile);
		LDObject* pOldCopy, *pNewCopy;
		bool bEdited = false;
		
		switch (obj->getType ()) {
		case LDObject::Line:
		case LDObject::CondLine:
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
		
		case LDObject::Triangle:
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
		
		case LDObject::Quad:
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
		
		case LDObject::Subfile:
		case LDObject::Radial:
			{
				// Subfiles and radials are inverted when they're prefixed with
				// a BFC INVERTNEXT statement. Thus we need to toggle this status.
				// For flat primitives it's sufficient that the determinant is
				// flipped but I don't have a method for checking flatness yet.
				// Food for thought...
				
				bool inverted = false;
				ulong idx = obj->getIndex (g_curfile);
				
				if (idx > 0) {
					LDObject* prev = g_curfile->object (idx - 1);
					LDBFC* bfc = dynamic_cast<LDBFC*> (prev);
					
					if (bfc && bfc->type == LDBFC::InvertNext) {
						// Object is prefixed with an invertnext, thus remove it.
						paHistory.push_back (new DelHistory ({idx - 1}, {bfc->clone ()}));
						
						inverted = true;
						g_curfile->forgetObject (bfc);
						delete bfc;
					}
				}
				
				if (!inverted) {
					// Not inverted, thus prefix it with a new invertnext.
					LDBFC* bfc = new LDBFC (LDBFC::InvertNext);
					g_curfile->insertObj (idx, bfc);
					
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
		g_win->refresh ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void doRotate (const short l, const short m, const short n) {
	std::vector<LDObject*> sel = g_win->sel ();
	bbox box;
	vertex origin;
	std::vector<vertex*> queue;
	const double angle = (pi * currentGrid ().confs[Grid::Angle]->value) / 360;
	
	// ref: http://en.wikipedia.org/wiki/Transformation_matrix#Rotation_2
	matrix<3> transform ({
		(l * l * (1 - cos (angle))) + cos (angle),
		(m * l * (1 - cos (angle))) - (n * sin (angle)),
		(n * l * (1 - cos (angle))) + (m * sin (angle)),
		
		(l * m * (1 - cos (angle))) + (n * sin (angle)),
		(m * m * (1 - cos (angle))) + cos (angle),
		(n * m * (1 - cos (angle))) - (l * sin (angle)),
		
		(l * n * (1 - cos (angle))) - (m * sin (angle)),
		(m * n * (1 - cos (angle))) + (l * sin (angle)),
		(n * n * (1 - cos (angle))) + cos (angle)
	});
	
	// Calculate center vertex
	for (LDObject* obj : sel) {
		if (obj->getType () == LDObject::Subfile)
			box << static_cast<LDSubfile*> (obj)->vPosition;
		else if (obj->getType () == LDObject::Radial)
			box << static_cast<LDRadial*> (obj)->vPosition;
		else
			box << obj;
	}
	
	origin = box.center ();
	
	// Apply the above matrix to everything
	for (LDObject* obj : sel) {
		if (obj->vertices ())
			for (short i = 0; i < obj->vertices (); ++i)
				queue.push_back (&obj->vaCoords[i]);
		else if (obj->getType () == LDObject::Subfile) {
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			
			queue.push_back (&ref->vPosition);
			ref->mMatrix = ref->mMatrix * transform;
		} else if (obj->getType () == LDObject::Radial) {
			LDRadial* rad = static_cast<LDRadial*> (obj);
			
			queue.push_back (&rad->vPosition);
			rad->mMatrix = rad->mMatrix * transform;
		} else if (obj->getType () == LDObject::Vertex)
			queue.push_back (&static_cast<LDVertex*> (obj)->vPosition);
	}
	
	for (vertex* v : queue) {
		v->move (-origin);
		v->transform (transform, g_origin);
		v->move (origin);
	}
	
	g_win->refresh ();
}

MAKE_ACTION (rotateXPos, "Rotate +X", "rotate-x-pos", "Rotate objects around X axis", CTRL (Right)) {
	doRotate (1, 0, 0);
}

MAKE_ACTION (rotateYPos, "Rotate +Y", "rotate-y-pos", "Rotate objects around Y axis", CTRL (PageDown)) {
	doRotate (0, 1, 0);
}

MAKE_ACTION (rotateZPos, "Rotate +Z", "rotate-z-pos", "Rotate objects around Z axis", CTRL (Up)) {
	doRotate (0, 0, 1);
}

MAKE_ACTION (rotateXNeg, "Rotate -X", "rotate-x-neg", "Rotate objects around X axis", CTRL (Left)) {
	doRotate (-1, 0, 0);
}

MAKE_ACTION (rotateYNeg, "Rotate -Y", "rotate-y-neg", "Rotate objects around Y axis", CTRL (PageUp)) {
	doRotate (0, -1, 0);
}

MAKE_ACTION (rotateZNeg, "Rotate -Z", "rotate-z-neg", "Rotate objects around Z axis", CTRL (Down)) {
	doRotate (0, 0, -1);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (roundCoords, "Round Coordinates", "round-coords", "Round coordinates down to 3/4 decimals", (0)) {
	setlocale (LC_ALL, "C");
	
	for (LDObject* obj : g_win->sel ())
	for (short i = 0; i < obj->vertices (); ++i)
	for (const Axis ax : g_Axes)
		obj->vaCoords[i][ax] = atof (fmt ("%.3f", obj->vaCoords[i][ax]));
	
	g_win->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (uncolorize, "Uncolorize", "uncolorize", "Reduce colors of everything selected to main and edge colors", (0)) {
	vector<LDObject*> oldCopies, newCopies;
	vector<ulong> indices;
	
	for (LDObject* obj : g_win->sel ()) {
		if (obj->isColored () == false)
			continue;
		
		indices.push_back (obj->getIndex (g_curfile));
		oldCopies.push_back (obj->clone ());
		
		obj->dColor = (obj->getType () == LDObject::Line || obj->getType () == LDObject::CondLine) ? edgecolor : maincolor;
		newCopies.push_back (obj->clone ());
	}
	
	if (indices.size () > 0) {
		History::addEntry (new EditHistory (indices, oldCopies, newCopies));
		g_win->refresh ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (ytruder, "Ytruder", "ytruder", "Extrude selected lines to a given plane", KEY (F4)) {
	runYtruder ();
}

MAKE_ACTION (rectifier, "Rectifier", "rectifier", "Optimizes quads into rect primitives.", KEY (F8)) {
	runRectifier ();
}

MAKE_ACTION (intersector, "Intersector", "intersector", "Perform clipping between two input groups.", KEY (F5)) {
	runIntersector ();
}