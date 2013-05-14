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

#include <QFileDialog>
#include <QMessageBox>
#include <QTextEdit>
#include <QBoxLayout>

#include "gui.h"
#include "file.h"
#include "history.h"
#include "configDialog.h"
#include "addObjectDialog.h"
#include "aboutDialog.h"
#include "misc.h"
#include "gldraw.h"
#include "dialogs.h"

extern_cfg (bool, gl_wireframe);

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (newFile, "&New", "brick", "Create a new part model.", CTRL (N)) {
	NewPartDialog::StaticDialog ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (open, "&Open", "file-open", "Load a part model from a file.", CTRL (O)) {
	str name = QFileDialog::getOpenFileName (g_win, "Open File", "", "LDraw files (*.dat *.ldr)");
	
	if (~name)
		openMainFile (name);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void doSave (bool saveAs) {
	str path = g_curfile->m_filename;
	
	if (~path == 0 || saveAs) {
		path = QFileDialog::getSaveFileName (g_win, "Save As",
		"", "LDraw files (*.dat *.ldr)");
		
		if (~path == 0) {
			// User didn't give a file name. This happens if the user cancelled
			// saving in the save file dialog. Abort.
			return;
		}
	}
	
	if (g_curfile->save (path)) {
		g_curfile->m_filename = path;
		g_win->setTitle ();
		
		logf ("Saved successfully to %s\n", path.chars ());
	} else {
		setlocale (LC_ALL, "C");
		
		// Tell the user the save failed, and give the option for saving as with it.
		QMessageBox dlg (QMessageBox::Critical, "Save Failure",
			fmt ("Failed to save to %s\nReason: %s", path.chars(), strerror (g_curfile->lastError)),
			QMessageBox::Close, g_win);
		
		QPushButton* saveAsBtn = new QPushButton ("Save As");
		saveAsBtn->setIcon (getIcon ("file-save-as"));
		dlg.addButton (saveAsBtn, QMessageBox::ActionRole);
		dlg.setDefaultButton (QMessageBox::Close);
		dlg.exec ();
		
		if (dlg.clickedButton () == saveAsBtn)
			doSave (true); // yay recursion!
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (save, "&Save", "file-save", "Save the part model.", CTRL (S)) {
	doSave (false);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (saveAs, "Save &As", "file-save-as", "Save the part model to a specific file.", CTRL_SHIFT (S)) {
	doSave (true);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (settings, "Settin&gs", "settings", "Edit the settings of " APPNAME ".", (0)) {
	ConfigDialog::staticDialog ();
}

MAKE_ACTION (setLDrawPath, "Set LDraw Path", "settings", "Change the LDraw directory path.", (0)) {
	LDrawPathDialog dlg (true);
	dlg.exec ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (exit, "&Exit", "exit", "Close " APPNAME ".", CTRL (Q)) {
	exit (0);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (newSubfile, "New Subfile", "add-subfile", "Creates a new subfile reference.", 0) {
	AddObjectDialog::staticDialog (LDObject::Subfile, null);
}

MAKE_ACTION (newLine, "New Line",  "add-line", "Creates a new line.", 0) {
	AddObjectDialog::staticDialog (LDObject::Line, null);
}

MAKE_ACTION (newTriangle, "New Triangle", "add-triangle", "Creates a new triangle.", 0) {
	AddObjectDialog::staticDialog (LDObject::Triangle, null);
}

MAKE_ACTION (newQuad, "New Quadrilateral", "add-quad", "Creates a new quadrilateral.", 0) {
	AddObjectDialog::staticDialog (LDObject::Quad, null);
}

MAKE_ACTION (newCondLine, "New Conditional Line", "add-condline", "Creates a new conditional line.", 0) {
	AddObjectDialog::staticDialog (LDObject::CondLine, null);
}

MAKE_ACTION (newComment, "New Comment", "add-comment", "Creates a new comment.", 0) {
	AddObjectDialog::staticDialog (LDObject::Comment, null);
}

MAKE_ACTION (newBFC, "New BFC Statement", "add-bfc", "Creates a new BFC statement.", 0) {
	AddObjectDialog::staticDialog (LDObject::BFC, null);
}

MAKE_ACTION (newVertex, "New Vertex", "add-vertex", "Creates a new vertex.", 0) {
	AddObjectDialog::staticDialog (LDObject::Vertex, null);
}

MAKE_ACTION (newRadial, "New Radial", "add-radial", "Creates a new radial.", 0) {
	AddObjectDialog::staticDialog (LDObject::Radial, null);
}

MAKE_ACTION (editObject, "Edit Object", "edit-object", "Edits this object.", 0) {
	if (g_win->sel ().size () != 1)
		return;
	
	LDObject* obj = g_win->sel ()[0];
	AddObjectDialog::staticDialog (obj->getType (), obj);
}

MAKE_ACTION (help, "Help", "help", "Shows the " APPNAME " help manual.", KEY (F1)) {
	
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (about, "About " APPNAME, "ldforge",
	"Shows information about " APPNAME ".", (0))
{
	AboutDialog dlg;
	dlg.exec ();
}

MAKE_ACTION (aboutQt, "About Qt", "qt", "Shows information about Qt.", (0)) {
	QMessageBox::aboutQt (g_win);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (selectAll, "Select All", "select-all", "Selects all objects.", CTRL (A)) {
	g_win->sel ().clear ();
	
	for (LDObject* obj : g_curfile->m_objs)
		g_win->sel ().push_back (obj);
	
	g_win->updateSelection ();
}

// =============================================================================
MAKE_ACTION (selectByColor, "Select by Color", "select-color",
	"Select all objects by the given color.", CTRL_SHIFT (A))
{
	short dColor = g_win->getSelectedColor ();
	
	if (dColor == -1)
		return; // no consensus on color
	
	g_win->sel ().clear ();
	for (LDObject* obj : g_curfile->m_objs)
		if (obj->color == dColor)
			g_win->sel ().push_back (obj);
	
	g_win->updateSelection ();
}

// =============================================================================
MAKE_ACTION (selectByType, "Select by Type", "select-type",
	"Select all objects by the given type.", (0))
{
	if (g_win->sel ().size () == 0)
		return;
	
	LDObject::Type eType = g_win->uniformSelectedType ();
	
	if (eType == LDObject::Unidentified)
		return;
	
	// If we're selecting subfile references, the reference filename must also
	// be uniform.
	str zRefName;
	
	if (eType == LDObject::Subfile) {
		zRefName = static_cast<LDSubfile*> (g_win->sel ()[0])->fileName;
		
		for (LDObject* pObj : g_win->sel ())
			if (static_cast<LDSubfile*> (pObj)->fileName != zRefName)
				return;
	}
	
	g_win->sel ().clear ();
	for (LDObject* obj : g_curfile->m_objs) {
		if (obj->getType() != eType)
			continue;
		
		if (eType == LDObject::Subfile && static_cast<LDSubfile*> (obj)->fileName != zRefName)
			continue;
		
		g_win->sel ().push_back (obj);
	}
	
	g_win->updateSelection ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (gridCoarse, "Coarse Grid", "grid-coarse", "Set the grid to Coarse", CTRL (1)) {
	grid = Grid::Coarse;
	g_win->updateGridToolBar ();
}

MAKE_ACTION (gridMedium, "Medium Grid", "grid-medium", "Set the grid to Medium", CTRL (2)) {
	grid = Grid::Medium;
	g_win->updateGridToolBar ();
}

MAKE_ACTION (gridFine, "Fine Grid", "grid-fine", "Set the grid to Fine", CTRL (3)) {
	grid = Grid::Fine;
	g_win->updateGridToolBar ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (resetView, "Reset View", "reset-view", "Reset view angles, pan and zoom", CTRL (0)) {
	g_win->R ()->resetAngles ();
	g_win->R ()->update ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (insertFrom, "Insert from File", "insert-from", "Insert LDraw data from a file.", (0)) {
	str fname = QFileDialog::getOpenFileName ();
	ulong idx = g_win->getInsertionPoint ();
	
	if (!~fname)
		return;
	
	FILE* fp = fopen (fname, "r");
	if (!fp) {
		critical (fmt ("Couldn't open %s\n%s", fname.chars(), strerror (errno)));
		return;
	}
	
	std::vector<LDObject*> historyCopies;
	std::vector<ulong> historyIndices;
	std::vector<LDObject*> objs = loadFileContents (fp, null);
	
	g_win->sel ().clear ();
	
	for (LDObject* obj : objs) {
		historyCopies.push_back (obj->clone ());
		historyIndices.push_back (idx);
		g_curfile->insertObj (idx, obj);
		g_win->sel ().push_back (obj);
		
		idx++;
	}
	
	if (historyCopies.size() > 0) {
		History::addEntry (new AddHistory (historyIndices, historyCopies));
		g_win->fullRefresh ();
		g_win->scrollToSelection ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (insertRaw, "Insert Raw", "insert-raw", "Type in LDraw code to insert.", (0)) {
	ulong idx = g_win->getInsertionPoint ();
	
	QDialog* const dlg = new QDialog;
	QVBoxLayout* const layout = new QVBoxLayout;
	QTextEdit* const te_edit = new QTextEdit;
	QDialogButtonBox* const bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	std::vector<LDObject*> historyCopies;
	std::vector<ulong> historyIndices;
	
	layout->addWidget (te_edit);
	layout->addWidget (bbx_buttons);
	dlg->setLayout (layout);
	dlg->setWindowTitle (APPNAME ": Insert Raw");
	dlg->connect (bbx_buttons, SIGNAL (accepted ()), dlg, SLOT (accept ()));
	dlg->connect (bbx_buttons, SIGNAL (rejected ()), dlg, SLOT (reject ()));
	
	if (dlg->exec () == false)
		return;
	
	g_win->sel ().clear ();
	
	for (str line : str (te_edit->toPlainText ()).split ("\n")) {
		LDObject* obj = parseLine (line);
		
		g_curfile->insertObj (idx, obj);
		historyIndices.push_back (idx);
		historyCopies.push_back (obj->clone ());
		g_win->sel ().push_back (obj);
		idx++;
	}
	
	if (historyCopies.size () > 0) {
		History::addEntry (new AddHistory (historyIndices, historyCopies));
		g_win->fullRefresh ();
		g_win->scrollToSelection ();
	}
}

// =========================================================================================================================================
MAKE_ACTION (screencap, "Screencap Part", "screencap", "Save a picture of the model", (0)) {
	setlocale (LC_ALL, "C");
	
	ushort w, h;
	uchar* imagedata = g_win->R ()->screencap (w, h);
	
	// GL and Qt formats have R and B swapped. Also, GL flips Y - correct it as well.
	QImage img = QImage (imagedata, w, h, QImage::Format_ARGB32).rgbSwapped ().mirrored ();
	
	str root = basename (g_curfile->m_filename.chars ());
	if (root.substr (~root - 4, -1) == ".dat")
		root -= 4;
	
	str defaultname = (~root > 0) ? fmt ("%s.png", root.chars ()) : "";
	str fname = QFileDialog::getSaveFileName (g_win, "Save Screencap", defaultname,
		"PNG images (*.png);;JPG images (*.jpg);;BMP images (*.bmp);;All Files (*.*)");
	
	if (~fname > 0 && !img.save (fname))
		critical (fmt ("Couldn't open %s for writing to save screencap: %s", fname.chars(), strerror (errno)));
	
	delete[] imagedata;
}

// =========================================================================================================================================
extern_cfg (bool, gl_axes);
MAKE_ACTION (axes, "Draw Axes", "axes", "Toggles drawing of axes", (0)) {
	gl_axes = !gl_axes;
	ACTION (axes)->setChecked (gl_axes);
	g_win->R ()->update ();
}

// =========================================================================================================================================
MAKE_ACTION (draw, "Draw Mode", "draw", "Begin drawing geometry", KEY (Insert)) {
	g_win->R ()->beginPlaneDraw ();
}

// =========================================================================================================================================
MAKE_ACTION (visibility, "Toggle Visibility", "visibility", "Toggles visibility/hiding on objects.", (0)) {
	for (LDObject* obj : g_win->sel ())
		obj->setHidden (!obj->hidden ());
	
	g_win->fullRefresh ();
}

// =========================================================================================================================================
MAKE_ACTION (wireframe, "Wireframe", "wireframe", "Toggle wireframe view", (0)) {
	gl_wireframe = !gl_wireframe;
	g_win->R ()->refresh ();
}

MAKE_ACTION (setOverlay, "Set Overlay Image", "overlay", "Set an overlay image", (0)) {
	g_win->R ()->setupOverlay ();
}

MAKE_ACTION (clearOverlay, "Clear Overlay Image", "overlay-clear", "Clear the overlay image.", (0)) {
	g_win->R ()->clearOverlay ();
}