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

#include <QSpinBox>
#include <QCheckBox>
#include <QBoxLayout>

#include "gui.h"
#include "common.h"
#include "file.h"
#include "colorSelectDialog.h"
#include "historyDialog.h"
#include "misc.h"
#include "widgets.h"
#include "extprogs.h"
#include "gldraw.h"
#include "dialogs.h"
#include "colors.h"

vector<str> g_Clipboard;

cfg (bool, edit_schemanticinline, false);

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void copyToClipboard () {
	vector<LDObject*> objs = g_win->sel ();
	
	// Clear the clipboard first.
	g_Clipboard.clear ();
	
	// Now, copy the contents into the clipboard.
	for (LDObject* obj : objs)
		g_Clipboard << obj->raw ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (cut, "Cut", "cut", "Cut the current selection to clipboard.", CTRL (X)) {
	copyToClipboard ();
	g_win->deleteSelection ();
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
	ulong idx = g_win->getInsertionPoint ();
	g_win->sel ().clear ();
	
	for (str line : g_Clipboard) {
		LDObject* pasted = parseLine (line);
		g_curfile->insertObj (idx++, pasted);
		g_win->sel () << pasted;
		g_win->R ()->compileObject (pasted);
	}
	
	g_win->refresh ();
	g_win->scrollToSelection ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (del, "Delete", "delete", "Delete the selection", KEY (Delete)) {
	g_win->deleteSelection ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void doInline (bool deep) {
	vector<LDObject*> sel = g_win->sel ();
	
	for (LDObject* obj : sel) {
		// Get the index of the subfile so we know where to insert the
		// inlined contents.
		long idx = obj->getIndex (g_curfile);
		if (idx == -1)
			continue;
		
		vector<LDObject*> objs;
		
		if (obj->getType() == LDObject::Subfile)
			objs = static_cast<LDSubfile*> (obj)->inlineContents (deep, true);
		else
			continue;
		
		// Merge in the inlined objects
		for (LDObject* inlineobj : objs) {
			// This object is now inlined so it has no parent anymore.
			inlineobj->setParent (null);
			g_curfile->insertObj (idx++, inlineobj);
		}
		
		// Delete the subfile now as it's been inlined.
		g_curfile->forgetObject (obj);
		delete obj;
	}
	
	g_win->fullRefresh ();
}

MAKE_ACTION (inlineContents, "Inline", "inline", "Inline selected subfiles.", CTRL (I)) {
	doInline (false);
}

MAKE_ACTION (deepInline, "Deep Inline", "inline-deep", "Recursively inline selected subfiles "
	"down to polygons only.", CTRL_SHIFT (I))
{
	doInline (true);
}

// ===============================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// ===============================================================================================
MAKE_ACTION (splitQuads, "Split Quads", "quad-split", "Split quads into triangles.", (0)) {
	vector<LDObject*> objs = g_win->sel ();
	
	for (LDObject* obj : objs) {
		if (obj->getType() != LDObject::Quad)
			continue;
		
		// Find the index of this quad
		long index = obj->getIndex (g_curfile);
		
		if (index == -1)
			return;
		
		vector<LDTriangle*> triangles = static_cast<LDQuad*> (obj)->splitToTriangles ();
		
		// Replace the quad with the first triangle and add the second triangle
		// after the first one.
		g_curfile->setObject (index, triangles[0]);
		g_curfile->insertObj (index + 1, triangles[1]);
		
		// Delete this quad now, it has been split.
		delete obj;
	}
	
	g_win->fullRefresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (setContents, "Edit LDraw Code", "set-contents", "Edit the LDraw code of this object.", KEY (F9)) {
	if (g_win->sel ().size() != 1)
		return;
	
	LDObject* obj = g_win->sel ()[0];
	
	SetContentsDialog dlg;
	dlg.setObject (obj);
	if (!dlg.exec ())
		return;
	
	LDObject* oldobj = obj;
	
	// Reinterpret it from the text of the input field
	obj = parseLine (dlg.text ());
	
	oldobj->replace (obj);
	
	// Refresh
	g_win->R ()->compileObject (obj);
	g_win->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (setColor, "Set Color", "palette", "Set the color on given objects.", KEY (C)) {
	if (g_win->sel ().size () <= 0)
		return;
	
	short colnum;
	short defcol = -1;
	
	vector<LDObject*> objs = g_win->sel ();
	
	// If all selected objects have the same color, said color is our default
	// value to the color selection dialog.
	defcol = g_win->getSelectedColor ();
	
	// Show the dialog to the user now and ask for a color.
	if (ColorSelectDialog::staticDialog (colnum, defcol, g_win)) {
		for (LDObject* obj : objs) {
			if (obj->isColored () == false)
				continue;
			
			obj->setColor (colnum);
			g_win->R ()->compileObject (obj);
		}
		
		g_win->refresh ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (makeBorders, "Make Borders", "make-borders", "Add borders around given polygons.", CTRL_SHIFT (B)) {
	vector<LDObject*> objs = g_win->sel ();
	int num = 0;
	
	for (LDObject* obj : objs) {
		if (obj->getType() != LDObject::Quad && obj->getType() != LDObject::Triangle)
			continue;
		
		short numLines;
		LDLine* lines[4];
		
		if (obj->getType() == LDObject::Quad) {
			numLines = 4;
			
			LDQuad* quad = static_cast<LDQuad*> (obj);
			lines[0] = new LDLine (quad->getVertex (0), quad->getVertex (1));
			lines[1] = new LDLine (quad->getVertex (1), quad->getVertex (2));
			lines[2] = new LDLine (quad->getVertex (2), quad->getVertex (3));
			lines[3] = new LDLine (quad->getVertex (3), quad->getVertex (0));
		} else {
			numLines = 3;
			
			LDTriangle* tri = static_cast<LDTriangle*> (obj);
			lines[0] = new LDLine (tri->getVertex (0), tri->getVertex (1));
			lines[1] = new LDLine (tri->getVertex (1), tri->getVertex (2));
			lines[2] = new LDLine (tri->getVertex (2), tri->getVertex (0)); 
		}
		
		for (short i = 0; i < numLines; ++i) {
			ulong idx = obj->getIndex (g_curfile) + i + 1;
			
			lines[i]->setColor (edgecolor);
			g_curfile->insertObj (idx, lines[i]);
			g_win->R ()->compileObject (lines[i]);
		}
		
		num += numLines;
	}
	
	log( ForgeWindow::tr( "Added %1 border lines" ), num );
	g_win->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (makeCornerVerts, "Make Corner Vertices", "corner-verts",
	"Adds vertex objects to the corners of the given polygons", (0))
{
	int num = 0;
	
	for (LDObject* obj : g_win->sel ()) {
		if (obj->vertices () < 2)
			continue;
		
		ulong idx = obj->getIndex (g_curfile);
		for (short i = 0; i < obj->vertices(); ++i) {
			LDVertex* vert = new LDVertex;
			vert->pos = obj->getVertex (i);
			vert->setColor (obj->color ());
			
			g_curfile->insertObj (++idx, vert);
			g_win->R ()->compileObject (vert);
			++num;
		}
	}
	
	log( ForgeWindow::tr( "Added %1 vertices" ), num );
	g_win->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void doMoveSelection (const bool up) {
	vector<LDObject*> objs = g_win->sel ();
	LDObject::moveObjects (objs, up);
	g_win->buildObjList ();
}

MAKE_ACTION (moveUp, "Move Up", "arrow-up", "Move the current selection up.", KEY (PageUp)) {
	doMoveSelection (true);
}

MAKE_ACTION (moveDown, "Move Down", "arrow-down", "Move the current selection down.", KEY (PageDown)) {
	doMoveSelection (false);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (undo, "Undo", "undo", "Undo a step.", CTRL (Z)) {
	g_curfile->undo ();
}

MAKE_ACTION (redo, "Redo", "redo", "Redo a step.", CTRL_SHIFT (Z)) {
	g_curfile->redo ();
}

MAKE_ACTION (showHistory, "Edit History", "history", "Show the history dialog.", (0)) {
	HistoryDialog dlg;
	dlg.exec ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void doMoveObjects (vertex vect) {
	// Apply the grid values
	vect[X] *= currentGrid ().confs[Grid::X]->value;
	vect[Y] *= currentGrid ().confs[Grid::Y]->value;
	vect[Z] *= currentGrid ().confs[Grid::Z]->value;
	
	for (LDObject* obj : g_win->sel ()) {
		obj->move (vect);
		g_win->R ()->compileObject (obj);
	}
	
	g_win->refresh ();
}

MAKE_ACTION (moveXNeg, "Move -X", "move-x-neg", "Move selected objects negative on the X axis.", KEY (Left)) {
	doMoveObjects ({-1, 0, 0});
}

MAKE_ACTION (moveYNeg, "Move -Y", "move-y-neg", "Move selected objects negative on the Y axis.", KEY (Home)) {
	doMoveObjects ({0, -1, 0});
}

MAKE_ACTION (moveZNeg, "Move -Z", "move-z-neg", "Move selected objects negative on the Z axis.", KEY (Down)) {
	doMoveObjects ({0, 0, -1});
}

MAKE_ACTION (moveXPos, "Move +X", "move-x-pos", "Move selected objects positive on the X axis.", KEY (Right)) {
	doMoveObjects ({1, 0, 0});
}

MAKE_ACTION (moveYPos, "Move +Y", "move-y-pos", "Move selected objects positive on the Y axis.", KEY (End)) {
	doMoveObjects ({0, 1, 0});
}

MAKE_ACTION (moveZPos, "Move +Z", "move-z-pos", "Move selected objects positive on the Z axis.", KEY (Up)) {
	doMoveObjects ({0, 0, 1});
}

// =============================================================================
MAKE_ACTION (invert, "Invert", "invert", "Reverse the winding of given objects.", CTRL_SHIFT (W)) {
	vector<LDObject*> sel = g_win->sel ();
	
	for (LDObject* obj : sel) {
		obj->invert ();
		g_win->R ()->compileObject (obj);
	}
	
	g_win->refresh ();
}

// =============================================================================
static void rotateVertex (vertex& v, const vertex& rotpoint, const matrix& transform) {
	v.move (-rotpoint);
	v.transform (transform, g_origin);
	v.move (rotpoint);
}

static void doRotate (const short l, const short m, const short n) {
	vector<LDObject*> sel = g_win->sel ();
	vector<vertex*> queue;
	const vertex rotpoint = rotPoint (sel);
	const double angle = (pi * currentGrid ().confs[Grid::Angle]->value) / 180;
	
	// ref: http://en.wikipedia.org/wiki/Transformation_matrix#Rotation_2
	const double cosangle = cos (angle),
		sinangle = sin (angle);
	
	matrix transform ({
		(l * l * (1 - cosangle)) + cosangle,
		(m * l * (1 - cosangle)) - (n * sinangle),
		(n * l * (1 - cosangle)) + (m * sinangle),
		
		(l * m * (1 - cosangle)) + (n * sinangle),
		(m * m * (1 - cosangle)) + cosangle,
		(n * m * (1 - cosangle)) - (l * sinangle),
		
		(l * n * (1 - cosangle)) - (m * sinangle),
		(m * n * (1 - cosangle)) + (l * sinangle),
		(n * n * (1 - cosangle)) + cosangle
	});
	
	// Apply the above matrix to everything
	for (LDObject* obj : sel) {
		if (obj->vertices ()) {
			for (short i = 0; i < obj->vertices (); ++i) {
				vertex v = obj->getVertex (i);
				rotateVertex (v, rotpoint, transform);
				obj->setVertex (i, v);
			}
		} elif (obj->hasMatrix ()) {
			LDMatrixObject* mo = dynamic_cast<LDMatrixObject*> (obj);
			vertex v = mo->position ();
			rotateVertex (v, rotpoint, transform);
			mo->setPosition (v);
			mo->setTransform (mo->transform () * transform);
		} elif (obj->getType () == LDObject::Vertex) {
			LDVertex* vert = static_cast<LDVertex*> (obj);
			vertex v = vert->pos;
			rotateVertex (v, rotpoint, transform);
			vert->pos = v;
		}
		
		g_win->R ()->compileObject (obj);
	}
	
	g_win->refresh ();
}

MAKE_ACTION (rotateXPos, "Rotate +X", "rotate-x-pos", "Rotate objects around X axis", CTRL (Right)) {
	doRotate (1, 0, 0);
}

MAKE_ACTION (rotateYPos, "Rotate +Y", "rotate-y-pos", "Rotate objects around Y axis", CTRL (End)) {
	doRotate (0, 1, 0);
}

MAKE_ACTION (rotateZPos, "Rotate +Z", "rotate-z-pos", "Rotate objects around Z axis", CTRL (Up)) {
	doRotate (0, 0, 1);
}

MAKE_ACTION (rotateXNeg, "Rotate -X", "rotate-x-neg", "Rotate objects around X axis", CTRL (Left)) {
	doRotate (-1, 0, 0);
}

MAKE_ACTION (rotateYNeg, "Rotate -Y", "rotate-y-neg", "Rotate objects around Y axis", CTRL (Home)) {
	doRotate (0, -1, 0);
}

MAKE_ACTION (rotateZNeg, "Rotate -Z", "rotate-z-neg", "Rotate objects around Z axis", CTRL (Down)) {
	doRotate (0, 0, -1);
}

// =========================================================================================================================================
MAKE_ACTION (rotpoint, "Set Rotation Point", "rotpoint", "Configure the rotation point.", (0)) {
	configRotationPoint ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (roundCoords, "Round Coordinates", "round-coords", "Round coordinates down to 3/4 decimals", (0)) {
	setlocale (LC_ALL, "C");
	int num = 0;
	
	for (LDObject* obj : g_win->sel ())
	for (short i = 0; i < obj->vertices (); ++i) {
		vertex v = obj->getVertex (i);
		
		for (const Axis ax : g_Axes) {
			// HACK: :p
			char valstr[64];
			sprintf (valstr, "%.3f", v[ax]);
			v[ax] = atof (valstr);
		}
		
		obj->setVertex (i, v);
		num += 3;
	}
	
	log( ForgeWindow::tr( "Rounded %1 coordinates" ), num );
	g_win->fullRefresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (uncolorize, "Uncolorize", "uncolorize", "Reduce colors of everything selected to main and edge colors", (0)) {
	for (LDObject* obj : g_win->sel ()) {
		if (obj->isColored () == false)
			continue;
		
		obj->setColor ((obj->getType () == LDObject::Line || obj->getType () == LDObject::CondLine) ? edgecolor : maincolor);
	}
	
	g_win->fullRefresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (ytruder, "Ytruder", "ytruder", "Extrude selected lines to a given plane", (0)) {
	runYtruder ();
}

MAKE_ACTION (rectifier, "Rectifier", "rectifier", "Optimizes quads into rect primitives.", (0)) {
	runRectifier ();
}

MAKE_ACTION (intersector, "Intersector", "intersector", "Perform clipping between two input groups.", (0)) {
	runIntersector ();
}

MAKE_ACTION (coverer, "Coverer", "coverer", "Fill the space between two line shapes", (0)) {
	runCoverer ();
}

MAKE_ACTION (isecalc, "Isecalc", "isecalc", "Compute intersection between objects", (0)) {
	runIsecalc ();
}

// =============================================================================
MAKE_ACTION (replaceCoords, "Replace Coordinates", "replace-coords", "Find and replace coordinate values", CTRL (R)) {
	ReplaceCoordsDialog dlg;
	
	if (!dlg.exec ())
		return;
	
	const double search = dlg.searchValue (),
		replacement = dlg.replacementValue ();
	const bool any = dlg.any (),
		rel = dlg.rel ();
	
	vector<int> sel = dlg.axes ();
	
	for (LDObject* obj : g_win->sel ())
	for (short i = 0; i < obj->vertices (); ++i) {
		vertex v = obj->getVertex (i);
		for (int ax : sel) {
			double& coord = v[(Axis) ax];
			
			if (any || coord == search) {
				if (!rel)
					coord = 0;
				
				coord += replacement;
			}
		}
		
		obj->setVertex (i, v);
	}
	
	g_win->fullRefresh ();
}

// =================================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =================================================================================================
class FlipDialog : public QDialog {
public:
	explicit FlipDialog (QWidget* parent = 0, Qt::WindowFlags f = 0) : QDialog (parent, f) {
		cbg_axes = makeAxesBox ();
		
		QVBoxLayout* layout = new QVBoxLayout;
		layout->addWidget (cbg_axes);
		layout->addWidget (makeButtonBox (*this));
		setLayout (layout);
	}
	
	vector<int> axes () { return cbg_axes->checkedValues (); }
	
private:
	CheckBoxGroup* cbg_axes;
};

// ================================================================================================
MAKE_ACTION (flip, "Flip", "flip", "Flip coordinates", CTRL_SHIFT (F)) {
	FlipDialog dlg;
	
	if (!dlg.exec ())
		return;
	
	vector<int> sel = dlg.axes ();
	
	for (LDObject* obj : g_win->sel ())
	for (short i = 0; i < obj->vertices (); ++i) {
		vertex v = obj->getVertex (i);
		
		for (int ax : sel)
			v[(Axis) ax] *= -1;
		
		obj->setVertex (i, v);
	}
	
	g_win->fullRefresh ();
}

// ================================================================================================
MAKE_ACTION (demote, "Demote conditional lines", "demote", "Demote conditional lines down to normal lines.", (0)) {
	vector<LDObject*> sel = g_win->sel ();
	int num = 0;
	
	for (LDObject* obj : sel) {
		if (obj->getType () != LDObject::CondLine)
			continue;
		
		LDLine* repl = static_cast<LDCondLine*> (obj)->demote ();
		g_win->R ()->compileObject (repl);
		++num;
	}
	
	log( ForgeWindow::tr( "Demoted %1 conditional lines" ), num );
	g_win->refresh ();
}

// =================================================================================================
static bool isColorUsed (short colnum) {
	for (LDObject* obj : g_curfile->objs ())
		if (obj->isColored () && obj->color () == colnum)
			return true;
	
	return false;
}

MAKE_ACTION (autoColor, "Autocolor", "autocolor", "Set the color of the given object to the first found unused color.", (0)) {
	short colnum = 0;
	
	while (colnum < 512 && (getColor (colnum) == null || isColorUsed (colnum)))
		colnum++;
	
	if (colnum >= 512) {
		//: Auto-colorer error message
		critical( ForgeWindow::tr( "Out of unused colors! What are you doing?!" ));
		return;
	}
	
	for (LDObject* obj : g_win->sel ()) {
		if (obj->isColored () == false)
			continue;
		
		obj->setColor (colnum);
		g_win->R ()->compileObject (obj);
	}
	
	log( ForgeWindow::tr( "Auto-colored: new color is [%1] %2" ), colnum, getColor( colnum )->name );
	
	g_win->refresh ();
}