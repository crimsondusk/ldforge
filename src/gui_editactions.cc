/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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
#include "main.h"
#include "document.h"
#include "colorSelectDialog.h"
#include "misc.h"
#include "widgets.h"
#include "gldraw.h"
#include "dialogs.h"
#include "colors.h"
#include "ui_replcoords.h"
#include "ui_editraw.h"
#include "ui_flip.h"
#include "ui_addhistoryline.h"

cfg (Bool, edit_schemanticinline, false);
extern_cfg (String, ld_defaultuser);

// =============================================================================
// -----------------------------------------------------------------------------
static int copyToClipboard()
{	QList<LDObject*> objs = selection();
	int num = 0;

	// Clear the clipboard first.
	qApp->clipboard()->clear();

	// Now, copy the contents into the clipboard.
	str data;

	for (LDObject* obj : objs)
	{	if (data.length() > 0)
			data += "\n";

		data += obj->raw();
		++num;
	}

	qApp->clipboard()->setText (data);
	return num;
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Cut, CTRL (X))
{	int num = copyToClipboard();
	deleteSelection();
	log (tr ("%1 objects cut"), num);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Copy, CTRL (C))
{	int num = copyToClipboard();
	log (tr ("%1 objects copied"), num);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Paste, CTRL (V))
{	const str clipboardText = qApp->clipboard()->text();
	int idx = getInsertionPoint();
	getCurrentDocument()->clearSelection();
	int num = 0;

	for (str line : clipboardText.split ("\n"))
	{	LDObject* pasted = parseLine (line);
		getCurrentDocument()->insertObj (idx++, pasted);
		pasted->select();
		R()->compileObject (pasted);
		++num;
	}

	log (tr ("%1 objects pasted"), num);
	refresh();
	scrollToSelection();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Delete, KEY (Delete))
{	int num = deleteSelection();
	log (tr ("%1 objects deleted"), num);
}

// =============================================================================
// -----------------------------------------------------------------------------
static void doInline (bool deep)
{	QList<LDObject*> sel = selection();

	for (LDObject* obj : sel)
	{	// Get the index of the subfile so we know where to insert the
		// inlined contents.
		long idx = obj->getIndex();

		if (idx == -1)
			continue;

		QList<LDObject*> objs;

		if (obj->getType() == LDObject::Subfile)
			objs = static_cast<LDSubfile*> (obj)->inlineContents (
					   (LDSubfile::InlineFlags)
					   ( (deep) ? LDSubfile::DeepInline : 0) |
					   LDSubfile::CacheInline
				   );
		else
			continue;

		// Merge in the inlined objects
		for (LDObject * inlineobj : objs)
		{	str line = inlineobj->raw();
			inlineobj->deleteSelf();
			LDObject* newobj = parseLine (line);
			getCurrentDocument()->insertObj (idx++, newobj);
			newobj->select();
			g_win->R()->compileObject (newobj);
		}

		// Delete the subfile now as it's been inlined.
		obj->deleteSelf();
	}

	g_win->refresh();
}

DEFINE_ACTION (Inline, CTRL (I))
{	doInline (false);
}

DEFINE_ACTION (InlineDeep, CTRL_SHIFT (I))
{	doInline (true);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (SplitQuads, 0)
{	QList<LDObject*> objs = selection();
	int num = 0;

	for (LDObject* obj : objs)
	{	if (obj->getType() != LDObject::Quad)
			continue;

		// Find the index of this quad
		long index = obj->getIndex();

		if (index == -1)
			return;

		QList<LDTriangle*> triangles = static_cast<LDQuad*> (obj)->splitToTriangles();

		// Replace the quad with the first triangle and add the second triangle
		// after the first one.
		getCurrentDocument()->setObject (index, triangles[0]);
		getCurrentDocument()->insertObj (index + 1, triangles[1]);

		for (LDTriangle* t : triangles)
			R()->compileObject (t);

		// Delete this quad now, it has been split.
		obj->deleteSelf();

		num++;
	}

	log ("%1 quadrilaterals split", num);
	refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (EditRaw, KEY (F9))
{	if (selection().size() != 1)
		return;

	LDObject* obj = selection()[0];
	QDialog* dlg = new QDialog;
	Ui::EditRawUI ui;

	ui.setupUi (dlg);
	ui.code->setText (obj->raw());

	if (obj->getType() == LDObject::Error)
		ui.errorDescription->setText (static_cast<LDError*> (obj)->reason);
	else
	{	ui.errorDescription->hide();
		ui.errorIcon->hide();
	}

	if (!dlg->exec())
		return;

	LDObject* oldobj = obj;

	// Reinterpret it from the text of the input field
	obj = parseLine (ui.code->text());
	oldobj->replace (obj);

	// Refresh
	R()->compileObject (obj);
	refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (SetColor, KEY (C))
{	if (selection().isEmpty())
		return;

	int colnum;
	int defcol = -1;

	QList<LDObject*> objs = selection();

	// If all selected objects have the same color, said color is our default
	// value to the color selection dialog.
	defcol = getSelectedColor();

	// Show the dialog to the user now and ask for a color.
	if (ColorSelector::selectColor (colnum, defcol, g_win))
	{	for (LDObject* obj : objs)
		{	if (obj->isColored() == false)
				continue;

			obj->setColor (colnum);
			R()->compileObject (obj);
		}

		refresh();
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Borders, CTRL_SHIFT (B))
{	QList<LDObject*> objs = selection();
	int num = 0;

	for (LDObject* obj : objs)
	{	const LDObject::Type type = obj->getType();
		if (type != LDObject::Quad && type != LDObject::Triangle)
			continue;

		int numLines;
		LDLine* lines[4];

		if (type == LDObject::Quad)
		{	numLines = 4;

			LDQuad* quad = static_cast<LDQuad*> (obj);
			lines[0] = new LDLine (quad->getVertex (0), quad->getVertex (1));
			lines[1] = new LDLine (quad->getVertex (1), quad->getVertex (2));
			lines[2] = new LDLine (quad->getVertex (2), quad->getVertex (3));
			lines[3] = new LDLine (quad->getVertex (3), quad->getVertex (0));
		}
		else
		{	numLines = 3;

			LDTriangle* tri = static_cast<LDTriangle*> (obj);
			lines[0] = new LDLine (tri->getVertex (0), tri->getVertex (1));
			lines[1] = new LDLine (tri->getVertex (1), tri->getVertex (2));
			lines[2] = new LDLine (tri->getVertex (2), tri->getVertex (0));
		}

		for (int i = 0; i < numLines; ++i)
		{	long idx = obj->getIndex() + i + 1;

			lines[i]->setColor (edgecolor);
			getCurrentDocument()->insertObj (idx, lines[i]);
			R()->compileObject (lines[i]);
		}

		num += numLines;
	}

	log (tr ("Added %1 border lines"), num);
	refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (CornerVerts, 0)
{	int num = 0;

	for (LDObject* obj : selection())
	{	if (obj->vertices() < 2)
			continue;

		int idx = obj->getIndex();

		for (int i = 0; i < obj->vertices(); ++i)
		{	LDVertex* vert = new LDVertex;
			vert->pos = obj->getVertex (i);
			vert->setColor (obj->getColor());

			getCurrentDocument()->insertObj (++idx, vert);
			R()->compileObject (vert);
			++num;
		}
	}

	log (tr ("Added %1 vertices"), num);
	refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
static void doMoveSelection (const bool up)
{	QList<LDObject*> objs = selection();
	LDObject::moveObjects (objs, up);
	g_win->buildObjList();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (MoveUp, KEY (PageUp))
{	doMoveSelection (true);
}

DEFINE_ACTION (MoveDown, KEY (PageDown))
{	doMoveSelection (false);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Undo, CTRL (Z))
{	getCurrentDocument()->undo();
}

DEFINE_ACTION (Redo, CTRL_SHIFT (Z))
{	getCurrentDocument()->redo();
}

// =============================================================================
// -----------------------------------------------------------------------------
void doMoveObjects (vertex vect)
{	// Apply the grid values
	vect[X] *= *currentGrid().confs[Grid::X];
	vect[Y] *= *currentGrid().confs[Grid::Y];
	vect[Z] *= *currentGrid().confs[Grid::Z];

	for (LDObject* obj : selection())
	{	obj->move (vect);
		g_win->R()->compileObject (obj);
	}

	g_win->refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (MoveXNeg, KEY (Left))
{	doMoveObjects ({ -1, 0, 0});
}

DEFINE_ACTION (MoveYNeg, KEY (Home))
{	doMoveObjects ({0, -1, 0});
}

DEFINE_ACTION (MoveZNeg, KEY (Down))
{	doMoveObjects ({0, 0, -1});
}

DEFINE_ACTION (MoveXPos, KEY (Right))
{	doMoveObjects ({1, 0, 0});
}

DEFINE_ACTION (MoveYPos, KEY (End))
{	doMoveObjects ({0, 1, 0});
}

DEFINE_ACTION (MoveZPos, KEY (Up))
{	doMoveObjects ({0, 0, 1});
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Invert, CTRL_SHIFT (W))
{	QList<LDObject*> sel = selection();

	for (LDObject* obj : sel)
	{	obj->invert();
		R()->compileObject (obj);
	}

	refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
static void rotateVertex (vertex& v, const vertex& rotpoint, const matrix& transform)
{	v.move (-rotpoint);
	v.transform (transform, g_origin);
	v.move (rotpoint);
}

// =============================================================================
// -----------------------------------------------------------------------------
static void doRotate (const int l, const int m, const int n)
{	QList<LDObject*> sel = selection();
	QList<vertex*> queue;
	const vertex rotpoint = rotPoint (sel);
	const double angle = (pi * *currentGrid().confs[Grid::Angle]) / 180,
				 cosangle = cos (angle),
				 sinangle = sin (angle);

	// ref: http://en.wikipedia.org/wiki/Transformation_matrix#Rotation_2
	matrix transform (
	{	(l* l * (1 - cosangle)) + cosangle,
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
	for (LDObject* obj : sel)
	{	if (obj->vertices())
		{	for (int i = 0; i < obj->vertices(); ++i)
			{	vertex v = obj->getVertex (i);
				rotateVertex (v, rotpoint, transform);
				obj->setVertex (i, v);
			}
		} elif (obj->hasMatrix())
		{	LDMatrixObject* mo = dynamic_cast<LDMatrixObject*> (obj);

			// Transform the position
			/*
			vertex v = mo->getPosition();
			rotateVertex (v, rotpoint, transform);
			mo->setPosition (v);
			*/

			// Transform the matrix
			mo->setTransform (transform * mo->getTransform());
		} elif (obj->getType() == LDObject::Vertex)
		{	LDVertex* vert = static_cast<LDVertex*> (obj);
			vertex v = vert->pos;
			rotateVertex (v, rotpoint, transform);
			vert->pos = v;
		}

		g_win->R()->compileObject (obj);
	}

	g_win->refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (RotateXPos, CTRL (Right))
{	doRotate (1, 0, 0);
}
DEFINE_ACTION (RotateYPos, CTRL (End))
{	doRotate (0, 1, 0);
}
DEFINE_ACTION (RotateZPos, CTRL (Up))
{	doRotate (0, 0, 1);
}
DEFINE_ACTION (RotateXNeg, CTRL (Left))
{	doRotate (-1, 0, 0);
}
DEFINE_ACTION (RotateYNeg, CTRL (Home))
{	doRotate (0, -1, 0);
}
DEFINE_ACTION (RotateZNeg, CTRL (Down))
{	doRotate (0, 0, -1);
}

DEFINE_ACTION (RotationPoint, (0))
{	configRotationPoint();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (RoundCoordinates, 0)
{	setlocale (LC_ALL, "C");
	int num = 0;

	for (LDObject* obj : selection())
	{	LDMatrixObject* mo = dynamic_cast<LDMatrixObject*> (obj);

		if (mo != null)
		{	vertex v = mo->getPosition();
			matrix t = mo->getTransform();

			for_axes (ax)
				roundToDecimals (v[ax], 3);

			// Let matrix values be rounded to 4 decimals,
			// they need that extra precision
			for (int i = 0; i < 9; ++i)
				roundToDecimals (t[i], 4);

			mo->setPosition (v);
			mo->setTransform (t);
			num += 10;
		}
		else
		{	for (int i = 0; i < obj->vertices(); ++i)
			{	vertex v = obj->getVertex (i);

				for_axes (ax)
					roundToDecimals (v[ax], 3);

				obj->setVertex (i, v);
				R()->compileObject (obj);
				num += 3;
			}
		}
	}

	log (tr ("Rounded %1 values"), num);
	refreshObjectList();
	refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Uncolorize, 0)
{	int num = 0;

	for (LDObject* obj : selection())
	{	if (obj->isColored() == false)
			continue;

		int col = maincolor;

		if (obj->getType() == LDObject::Line || obj->getType() == LDObject::CondLine)
			col = edgecolor;

		obj->setColor (col);
		R()->compileObject (obj);
		num++;
	}

	log (tr ("%1 objects uncolored"), num);
	refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (ReplaceCoords, CTRL (R))
{	QDialog* dlg = new QDialog (g_win);
	Ui::ReplaceCoordsUI ui;
	ui.setupUi (dlg);

	if (!dlg->exec())
		return;

	const double search = ui.search->value(),
		replacement = ui.replacement->value();
	const bool any = ui.any->isChecked(),
		rel = ui.relative->isChecked();

	QList<Axis> sel;
	int num = 0;

	if (ui.x->isChecked()) sel << X;
	if (ui.y->isChecked()) sel << Y;
	if (ui.z->isChecked()) sel << Z;

	for (LDObject* obj : selection())
	{	for (int i = 0; i < obj->vertices(); ++i)
		{	vertex v = obj->getVertex (i);

			for (Axis ax : sel)
			{	double& coord = v[ax];

				if (any || coord == search)
				{	if (!rel)
						coord = 0;

					coord += replacement;
					num++;
				}
			}

			obj->setVertex (i, v);
			R()->compileObject (obj);
		}
	}

	log (tr ("Altered %1 values"), num);
	refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Flip, CTRL_SHIFT (F))
{	QDialog* dlg = new QDialog;
	Ui::FlipUI ui;
	ui.setupUi (dlg);

	if (!dlg->exec())
		return;

	QList<Axis> sel;

	if (ui.x->isChecked()) sel << X;
	if (ui.y->isChecked()) sel << Y;
	if (ui.z->isChecked()) sel << Z;

	for (LDObject* obj : selection())
	{	for (int i = 0; i < obj->vertices(); ++i)
		{	vertex v = obj->getVertex (i);

			for (Axis ax : sel)
				v[ax] *= -1;

			obj->setVertex (i, v);
			R()->compileObject (obj);
		}
	}

	refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Demote, 0)
{	QList<LDObject*> sel = selection();
	int num = 0;

	for (LDObject* obj : sel)
	{	if (obj->getType() != LDObject::CondLine)
			continue;

		LDLine* repl = static_cast<LDCondLine*> (obj)->demote();
		R()->compileObject (repl);
		++num;
	}

	log (tr ("Demoted %1 conditional lines"), num);
	refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
static bool isColorUsed (int colnum)
{	for (LDObject* obj : getCurrentDocument()->getObjects())
		if (obj->isColored() && obj->getColor() == colnum)
			return true;

	return false;
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Autocolor, 0)
{	int colnum = 0;

	while (colnum < MAX_COLORS && (getColor (colnum) == null || isColorUsed (colnum)))
		colnum++;

	if (colnum >= MAX_COLORS)
	{	log (tr ("Cannot auto-color: all colors are in use!"));
		return;
	}

	for (LDObject* obj : selection())
	{	if (obj->isColored() == false)
			continue;

		obj->setColor (colnum);
		R()->compileObject (obj);
	}

	log (tr ("Auto-colored: new color is [%1] %2"), colnum, getColor (colnum)->name);
	refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (AddHistoryLine, 0)
{	LDObject* obj;
	bool ishistory = false,
		 prevIsHistory = false;

	QDialog* dlg = new QDialog;
	Ui_AddHistoryLine* ui = new Ui_AddHistoryLine;
	ui->setupUi (dlg);
	ui->m_username->setText (ld_defaultuser);
	ui->m_date->setDate (QDate::currentDate());
	ui->m_comment->setFocus();

	if (!dlg->exec())
		return;

	// Create the comment object based on input
	str commentText = fmt ("!HISTORY %1 [%2] %3",
		ui->m_date->date().toString ("yyyy-MM-dd"),
		ui->m_username->text(),
		ui->m_comment->text());

	LDComment* comm = new LDComment (commentText);

	// Find a spot to place the new comment
	for (
		obj = getCurrentDocument()->getObject (0);
		obj && obj->next() && !obj->next()->isScemantic();
		obj = obj->next()
	)
	{	LDComment* comm = dynamic_cast<LDComment*> (obj);

		if (comm && comm->text.startsWith ("!HISTORY "))
			ishistory = true;

		if (prevIsHistory && !ishistory)
		{	// Last line was history, this isn't, thus insert the new history
			// line here.
			break;
		}

		prevIsHistory = ishistory;
	}

	int idx = obj ? obj->getIndex() : 0;
	getCurrentDocument()->insertObj (idx++, comm);

	// If we're adding a history line right before a scemantic object, pad it
	// an empty line
	if (obj && obj->next() && obj->next()->isScemantic())
		getCurrentDocument()->insertObj (idx, new LDEmpty);

	buildObjList();
	delete ui;
}
