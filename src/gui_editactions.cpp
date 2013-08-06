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
#include <QClipboard>

#include "gui.h"
#include "common.h"
#include "file.h"
#include "colorSelectDialog.h"
#include "misc.h"
#include "widgets.h"
#include "gldraw.h"
#include "dialogs.h"
#include "colors.h"
#include "ui_replcoords.h"
#include "ui_editraw.h"
#include "ui_flip.h"

cfg (bool, edit_schemanticinline, false);

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static int copyToClipboard() {
	List<LDObject*> objs = g_win->sel();
	int num = 0;
	
	// Clear the clipboard first.
	qApp->clipboard()->clear();
	
	// Now, copy the contents into the clipboard.
	str data;
	
	for (LDObject* obj : objs) {
		if (data.length() > 0)
			data += "\n";
		
		data += obj->raw();
		++num;
	}
	
	qApp->clipboard()->setText (data);
	return num;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
DEFINE_ACTION (Cut, CTRL (X)) {
	int num = copyToClipboard();
	g_win->deleteSelection();
	log (ForgeWindow::tr ("%1 objects cut"), num);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
DEFINE_ACTION (Copy, CTRL (C)) {
	int num = copyToClipboard();
	log (ForgeWindow::tr ("%1 objects copied"), num);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
DEFINE_ACTION (Paste, CTRL (V)) {
	const str clipboardText = qApp->clipboard()->text();
	ulong idx = g_win->getInsertionPoint();
	g_win->sel().clear();
	int num = 0;
	
	for (str line : clipboardText.split ("\n")) {
		LDObject* pasted = parseLine (line);
		LDFile::current()->insertObj (idx++, pasted);
		g_win->sel() << pasted;
		g_win->R()->compileObject (pasted);
		++num;
	}
	
	log (ForgeWindow::tr ("%1 objects pasted"), num);
	g_win->refresh();
	g_win->scrollToSelection();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
DEFINE_ACTION (Delete, KEY (Delete)) {
	int num = g_win->deleteSelection();
	log (ForgeWindow::tr ("%1 objects deleted"), num);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void doInline (bool deep) {
	List<LDObject*> sel = g_win->sel();
	
	for (LDObject* obj : sel) {
		// Get the index of the subfile so we know where to insert the
		// inlined contents.
		long idx = obj->getIndex();
		
		if (idx == -1)
			continue;
		
		List<LDObject*> objs;
		
		if (obj->getType() == LDObject::Subfile)
			objs = static_cast<LDSubfileObject*> (obj)->inlineContents (deep, true);
		else
			continue;
		
		// Merge in the inlined objects
		for (LDObject* inlineobj : objs) {
			str line = inlineobj->raw();
			delete inlineobj;
			
			LDObject* newobj = parseLine (line);
			LDFile::current()->insertObj (idx++, newobj);
			g_win->sel() << newobj;
			g_win->R()->compileObject (newobj);
		}
		
		// Delete the subfile now as it's been inlined.
		LDFile::current()->forgetObject (obj);
		delete obj;
	}
	
	g_win->refresh();
}

DEFINE_ACTION (Inline, CTRL (I)) {
	doInline (false);
}

DEFINE_ACTION (InlineDeep, CTRL_SHIFT (I)) {
	doInline (true);
}

// ===============================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// ===============================================================================================
DEFINE_ACTION (SplitQuads, 0) {
	List<LDObject*> objs = g_win->sel();
	int num = 0;
	
	for (LDObject* obj : objs) {
		if (obj->getType() != LDObject::Quad)
			continue;
		
		// Find the index of this quad
		long index = obj->getIndex();
		
		if (index == -1)
			return;
		
		List<LDTriangleObject*> triangles = static_cast<LDQuadObject*> (obj)->splitToTriangles();
		
		// Replace the quad with the first triangle and add the second triangle
		// after the first one.
		LDFile::current()->setObject (index, triangles[0]);
		LDFile::current()->insertObj (index + 1, triangles[1]);
		
		for (LDTriangleObject * t : triangles)
			g_win->R()->compileObject (t);
		
		// Delete this quad now, it has been split.
		delete obj;
		
		num++;
	}
	
	log ("%1 quadrilaterals split", num);
	g_win->refresh();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
DEFINE_ACTION (EditRaw, KEY (F9)) {
	if (g_win->sel().size() != 1)
		return;
	
	LDObject* obj = g_win->sel() [0];
	QDialog* dlg = new QDialog;
	Ui::EditRawUI ui;
	
	ui.setupUi (dlg);
	ui.code->setText (obj->raw());
	
	if (obj->getType() == LDObject::Error)
		ui.errorDescription->setText (static_cast<LDErrorObject*> (obj)->reason);
	else {
		ui.errorDescription->hide();
		ui.errorIcon->hide();
	}
	
	if (!dlg->exec())
		return;
	
	LDObject* oldobj = obj;
	
	// Reinterpret it from the text of the input field
	obj = parseLine (ui.code->text());
	oldobj->replace (obj);
	
	// Refresh
	g_win->R()->compileObject (obj);
	g_win->refresh();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
DEFINE_ACTION (SetColor, KEY (C)) {
	if (g_win->sel().size() <= 0)
		return;
	
	short colnum;
	short defcol = -1;
	
	List<LDObject*> objs = g_win->sel();
	
	// If all selected objects have the same color, said color is our default
	// value to the color selection dialog.
	defcol = g_win->getSelectedColor();
	
	// Show the dialog to the user now and ask for a color.
	if (ColorSelector::getColor (colnum, defcol, g_win)) {
		for (LDObject* obj : objs) {
			if (obj->isColored() == false)
				continue;
			
			obj->setColor (colnum);
			g_win->R()->compileObject (obj);
		}
		
		g_win->refresh();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
DEFINE_ACTION (Borders, CTRL_SHIFT (B)) {
	List<LDObject*> objs = g_win->sel();
	int num = 0;
	
	for (LDObject* obj : objs) {
		if (obj->getType() != LDObject::Quad && obj->getType() != LDObject::Triangle)
			continue;
		
		short numLines;
		LDLineObject* lines[4];
		
		if (obj->getType() == LDObject::Quad) {
			numLines = 4;
			
			LDQuadObject* quad = static_cast<LDQuadObject*> (obj);
			lines[0] = new LDLineObject (quad->getVertex (0), quad->getVertex (1));
			lines[1] = new LDLineObject (quad->getVertex (1), quad->getVertex (2));
			lines[2] = new LDLineObject (quad->getVertex (2), quad->getVertex (3));
			lines[3] = new LDLineObject (quad->getVertex (3), quad->getVertex (0));
		} else {
			numLines = 3;
			
			LDTriangleObject* tri = static_cast<LDTriangleObject*> (obj);
			lines[0] = new LDLineObject (tri->getVertex (0), tri->getVertex (1));
			lines[1] = new LDLineObject (tri->getVertex (1), tri->getVertex (2));
			lines[2] = new LDLineObject (tri->getVertex (2), tri->getVertex (0));
		}
		
		for (short i = 0; i < numLines; ++i) {
			ulong idx = obj->getIndex() + i + 1;
			
			lines[i]->setColor (edgecolor);
			LDFile::current()->insertObj (idx, lines[i]);
			g_win->R()->compileObject (lines[i]);
		}
		
		num += numLines;
	}
	
	log (ForgeWindow::tr ("Added %1 border lines"), num);
	g_win->refresh();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
DEFINE_ACTION (CornerVerts, 0) {
	int num = 0;
	
	for (LDObject* obj : g_win->sel()) {
		if (obj->vertices() < 2)
			continue;
		
		ulong idx = obj->getIndex();
		
		for (short i = 0; i < obj->vertices(); ++i) {
			LDVertexObject* vert = new LDVertexObject;
			vert->pos = obj->getVertex (i);
			vert->setColor (obj->color());
			
			LDFile::current()->insertObj (++idx, vert);
			g_win->R()->compileObject (vert);
			++num;
		}
	}
	
	log (ForgeWindow::tr ("Added %1 vertices"), num);
	g_win->refresh();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void doMoveSelection (const bool up) {
	List<LDObject*> objs = g_win->sel();
	LDObject::moveObjects (objs, up);
	g_win->buildObjList();
}

DEFINE_ACTION (MoveUp, KEY (PageUp)) {
	doMoveSelection (true);
}

DEFINE_ACTION (MoveDown, KEY (PageDown)) {
	doMoveSelection (false);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
DEFINE_ACTION (Undo, CTRL (Z)) {
	LDFile::current()->undo();
}

DEFINE_ACTION (Redo, CTRL_SHIFT (Z)) {
	LDFile::current()->redo();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void doMoveObjects (vertex vect) {
	// Apply the grid values
	vect[X] *= currentGrid().confs[Grid::X]->value;
	vect[Y] *= currentGrid().confs[Grid::Y]->value;
	vect[Z] *= currentGrid().confs[Grid::Z]->value;
	
	for (LDObject* obj : g_win->sel()) {
		obj->move (vect);
		g_win->R()->compileObject (obj);
	}
	
	g_win->refresh();
}

DEFINE_ACTION (MoveXNeg, KEY (Left)) {
	doMoveObjects ({ -1, 0, 0});
}

DEFINE_ACTION (MoveYNeg, KEY (Home)) {
	doMoveObjects ({0, -1, 0});
}

DEFINE_ACTION (MoveZNeg, KEY (Down)) {
	doMoveObjects ({0, 0, -1});
}

DEFINE_ACTION (MoveXPos, KEY (Right)) {
	doMoveObjects ({1, 0, 0});
}

DEFINE_ACTION (MoveYPos, KEY (End)) {
	doMoveObjects ({0, 1, 0});
}

DEFINE_ACTION (MoveZPos, KEY (Up)) {
	doMoveObjects ({0, 0, 1});
}

// =============================================================================
DEFINE_ACTION (Invert, CTRL_SHIFT (W)) {
	List<LDObject*> sel = g_win->sel();
	
	for (LDObject* obj : sel) {
		obj->invert();
		g_win->R()->compileObject (obj);
	}
	
	g_win->refresh();
}

// =============================================================================
static void rotateVertex (vertex& v, const vertex& rotpoint, const matrix& transform) {
	v.move (-rotpoint);
	v.transform (transform, g_origin);
	v.move (rotpoint);
}

static void doRotate (const short l, const short m, const short n) {
	List<LDObject*> sel = g_win->sel();
	List<vertex*> queue;
	const vertex rotpoint = rotPoint (sel);
	const double angle = (pi * currentGrid().confs[Grid::Angle]->value) / 180;
	
	// ref: http://en.wikipedia.org/wiki/Transformation_matrix#Rotation_2
	const double cosangle = cos (angle),
		sinangle = sin (angle);
	
	matrix transform ({
		(l* l * (1 - cosangle)) + cosangle,
		(m* l * (1 - cosangle)) - (n* sinangle),
		(n* l * (1 - cosangle)) + (m* sinangle),
		
		(l* m * (1 - cosangle)) + (n* sinangle),
		(m* m * (1 - cosangle)) + cosangle,
		(n* m * (1 - cosangle)) - (l* sinangle),
		
		(l* n * (1 - cosangle)) - (m* sinangle),
		(m* n * (1 - cosangle)) + (l* sinangle),
		(n* n * (1 - cosangle)) + cosangle
	});
	
	// Apply the above matrix to everything
	for (LDObject* obj : sel) {
		if (obj->vertices()) {
			for (short i = 0; i < obj->vertices(); ++i) {
				vertex v = obj->getVertex (i);
				rotateVertex (v, rotpoint, transform);
				obj->setVertex (i, v);
			}
		} elif (obj->hasMatrix()) {
			LDMatrixObject* mo = dynamic_cast<LDMatrixObject*> (obj);
			vertex v = mo->position();
			rotateVertex (v, rotpoint, transform);
			mo->setPosition (v);
			mo->setTransform (mo->transform() * transform);
		} elif (obj->getType() == LDObject::Vertex) {
			LDVertexObject* vert = static_cast<LDVertexObject*> (obj);
			vertex v = vert->pos;
			rotateVertex (v, rotpoint, transform);
			vert->pos = v;
		}
		
		g_win->R()->compileObject (obj);
	}
	
	g_win->refresh();
}

DEFINE_ACTION (RotateXPos, CTRL (Right)) { doRotate (1, 0, 0); }
DEFINE_ACTION (RotateYPos, CTRL (End))   { doRotate (0, 1, 0); }
DEFINE_ACTION (RotateZPos, CTRL (Up))    { doRotate (0, 0, 1); }
DEFINE_ACTION (RotateXNeg, CTRL (Left))  { doRotate (-1, 0, 0); }
DEFINE_ACTION (RotateYNeg, CTRL (Home))  { doRotate (0, -1, 0); }
DEFINE_ACTION (RotateZNeg, CTRL (Down))  { doRotate (0, 0, -1); }

DEFINE_ACTION (RotationPoint, (0)) {
	configRotationPoint();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
DEFINE_ACTION (RoundCoordinates, 0) {
	setlocale (LC_ALL, "C");
	int num = 0;
	
	for (LDObject* obj : g_win->sel())
	for (short i = 0; i < obj->vertices(); ++i) {
		vertex v = obj->getVertex (i);
		
		for (const Axis ax : g_Axes) {
			// HACK: .. should find a better way to do this
			char valstr[64];
			sprintf (valstr, "%.3f", v[ax]);
			v[ax] = atof (valstr);
		}
		
		obj->setVertex (i, v);
		g_win->R()->compileObject (obj);
		num += 3;
	}
	
	log (ForgeWindow::tr ("Rounded %1 coordinates"), num);
	g_win->refresh();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
DEFINE_ACTION (Uncolorize, 0) {
	int num = 0;
	
	for (LDObject* obj : g_win->sel()) {
		if (obj->isColored() == false)
			continue;
		
		int col = maincolor;
		
		if (obj->getType() == LDObject::Line || obj->getType() == LDObject::CondLine)
			col = edgecolor;
		
		obj->setColor (col);
		g_win->R()->compileObject (obj);
		num++;
	}
	
	log (ForgeWindow::tr ("%1 objects uncolored"), num);
	g_win->refresh();
}

// =============================================================================
DEFINE_ACTION (ReplaceCoords, CTRL (R)) {
	QDialog* dlg = new QDialog (g_win);
	Ui::ReplaceCoordsUI ui;
	ui.setupUi (dlg);
	
	if (!dlg->exec())
		return;
	
	const double search = ui.search->value(),
		replacement = ui.replacement->value();
	const bool any = ui.any->isChecked(),
		rel = ui.relative->isChecked();
	
	List<Axis> sel;
	int num = 0;
	
	if (ui.x->isChecked()) sel << X;
	if (ui.y->isChecked()) sel << Y;
	if (ui.z->isChecked()) sel << Z;

	for (LDObject* obj : g_win->sel())
	for (short i = 0; i < obj->vertices(); ++i) {
		vertex v = obj->getVertex (i);
		
		for (Axis ax : sel) {
			double& coord = v[ax];
			
			if (any || coord == search) {
				if (!rel)
					coord = 0;
				
				coord += replacement;
				num++;
			}
		}
		
		obj->setVertex (i, v);
		g_win->R()->compileObject (obj);
	}
	
	log (ForgeWindow::tr ("Altered %1 values"), num);
	g_win->refresh();
}

// ================================================================================================
DEFINE_ACTION (Flip, CTRL_SHIFT (F)) {
	QDialog* dlg = new QDialog;
	Ui::FlipUI ui;
	ui.setupUi (dlg);
	
	if (!dlg->exec())
		return;
	
	List<Axis> sel;
	if (ui.x->isChecked()) sel << X;
	if (ui.y->isChecked()) sel << Y;
	if (ui.z->isChecked()) sel << Z;
	
	for (LDObject* obj : g_win->sel())
	for (short i = 0; i < obj->vertices(); ++i) {
		vertex v = obj->getVertex (i);
		
		for (Axis ax : sel)
			v[ax] *= -1;

		obj->setVertex (i, v);
		g_win->R()->compileObject (obj);
	}
	
	g_win->refresh();
}

// ================================================================================================
DEFINE_ACTION (Demote, 0) {
	List<LDObject*> sel = g_win->sel();
	int num = 0;
	
	for (LDObject* obj : sel) {
		if (obj->getType() != LDObject::CondLine)
			continue;
		
		LDLineObject* repl = static_cast<LDCondLineObject*> (obj)->demote();
		g_win->R()->compileObject (repl);
		++num;
	}
	
	log (ForgeWindow::tr ("Demoted %1 conditional lines"), num);
	g_win->refresh();
}

// =================================================================================================
static bool isColorUsed (short colnum) {
	for (LDObject* obj : LDFile::current()->objs())
		if (obj->isColored() && obj->color() == colnum)
			return true;
	
	return false;
}

DEFINE_ACTION (Autocolor, 0) {
	short colnum = 0;
	
	while (colnum < MAX_COLORS && (getColor (colnum) == null || isColorUsed (colnum)))
		colnum++;
	
	if (colnum >= MAX_COLORS) {
		//: Auto-colorer error message
		critical (ForgeWindow::tr ("Out of unused colors! What are you doing?!"));
		return;
	}
	
	for (LDObject* obj : g_win->sel()) {
		if (obj->isColored() == false)
			continue;
		
		obj->setColor (colnum);
		g_win->R()->compileObject (obj);
	}
	
	log (ForgeWindow::tr ("Auto-colored: new color is [%1] %2"), colnum, getColor (colnum)->name);
	g_win->refresh();
}