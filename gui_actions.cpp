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

#include <qfiledialog.h>
#include <qmessagebox.h>
#include "gui.h"
#include "file.h"
#include "history.h"
#include "zz_newPartDialog.h"
#include "zz_configDialog.h"
#include "zz_addObjectDialog.h"
#include "zz_aboutDialog.h"
#include "misc.h"

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
	str zName;
	zName += QFileDialog::getOpenFileName (g_ForgeWindow, "Open File",
		"", "LDraw files (*.dat *.ldr)");
	
	if (~zName)
		openMainFile (zName);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void doSave (bool saveAs) {
	str path = g_CurrentFile->zFileName;
	
	if (~path == 0 || saveAs) {
		path = QFileDialog::getSaveFileName (g_ForgeWindow, "Save As",
		"", "LDraw files (*.dat *.ldr)");
		
		if (~path == 0) {
			// User didn't give a file name. This happens if the user cancelled
			// saving in the save file dialog. Abort.
			return;
		}
	}
	
	if (g_CurrentFile->save (path)) {
		g_CurrentFile->zFileName = path;
		g_ForgeWindow->setTitle ();
		
		logf ("Saved successfully to %s\n", path.chars ());
	} else {
		setlocale (LC_ALL, "C");
		
		// Tell the user the save failed, and give the option for saving as with it.
		QMessageBox dlg (QMessageBox::Critical, "Save Failure",
			format ("Failed to save to %s\nReason: %s", path.chars(), strerror (g_CurrentFile->lastError)),
			QMessageBox::Close, g_ForgeWindow);
		
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
MAKE_ACTION (settings, "Settin&gs", "settings", "Edit the settings of " APPNAME_DISPLAY ".", (0)) {
	ConfigDialog::staticDialog ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (exit, "&Exit", "exit", "Close " APPNAME_DISPLAY ".", CTRL (Q)) {
	exit (0);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (newSubfile, "New Subfile", "add-subfile", "Creates a new subfile reference.", 0) {
	AddObjectDialog::staticDialog (OBJ_Subfile, g_ForgeWindow);
}

MAKE_ACTION (newLine, "New Line",  "add-line", "Creates a new line.", 0) {
	AddObjectDialog::staticDialog (OBJ_Line, g_ForgeWindow);
}

MAKE_ACTION (newTriangle, "New Triangle", "add-triangle", "Creates a new triangle.", 0) {
	AddObjectDialog::staticDialog (OBJ_Triangle, g_ForgeWindow);
}

MAKE_ACTION (newQuad, "New Quadrilateral", "add-quad", "Creates a new quadrilateral.", 0) {
	AddObjectDialog::staticDialog (OBJ_Quad, g_ForgeWindow);
}

MAKE_ACTION (newCondLine, "New Conditional Line", "add-condline", "Creates a new conditional line.", 0) {
	AddObjectDialog::staticDialog (OBJ_CondLine, g_ForgeWindow);
}

MAKE_ACTION (newComment, "New Comment", "add-comment", "Creates a new comment.", 0) {
	AddObjectDialog::staticDialog (OBJ_Comment, g_ForgeWindow);
}

MAKE_ACTION (newVertex, "New Vertex", "add-vertex", "Creates a new vertex.", 0) {
	AddObjectDialog::staticDialog (OBJ_Vertex, g_ForgeWindow);
}

MAKE_ACTION (newRadial, "New Radial", "add-radial", "Creates a new radial.", 0) {
	AddObjectDialog::staticDialog (OBJ_Radial, g_ForgeWindow);
}

MAKE_ACTION (help, "Help", "help", "Shows the " APPNAME_DISPLAY " help manual.", KEY (F1)) {
	
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (about, "About " APPNAME_DISPLAY, "ldforge",
	"Shows information about " APPNAME_DISPLAY ".", CTRL (F1))
{
	AboutDialog dlg;
	dlg.exec ();
}

MAKE_ACTION (aboutQt, "About Qt", "qt", "Shows information about Qt.", CTRL_SHIFT (F1)) {
	QMessageBox::aboutQt (g_ForgeWindow);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (selectByColor, "Select by Color", "select-color",
	"Select all objects by the given color.", CTRL_SHIFT (A))
{
	short dColor = g_ForgeWindow->getSelectedColor ();
	
	if (dColor == -1)
		return; // no consensus on color
	
	g_ForgeWindow->sel.clear ();
	for (LDObject* obj : g_CurrentFile->objects)
		if (obj->dColor == dColor)
			g_ForgeWindow->sel.push_back (obj);
	
	g_ForgeWindow->updateSelection ();
}

MAKE_ACTION (selectByType, "Select by Type", "select-type",
	"Select all objects by the given type.", (0))
{
	if (g_ForgeWindow->sel.size () == 0)
		return;
	
	LDObjectType_e eType = g_ForgeWindow->uniformSelectedType ();
	
	if (eType == OBJ_Unidentified)
		return;
	
	// If we're selecting subfile references, the reference filename must also
	// be uniform.
	str zRefName;
	
	if (eType == OBJ_Subfile) {
		zRefName = static_cast<LDSubfile*> (g_ForgeWindow->sel[0])->zFileName;
		
		for (LDObject* pObj : g_ForgeWindow->sel)
			if (static_cast<LDSubfile*> (pObj)->zFileName != zRefName)
				return;
	}
	
	g_ForgeWindow->sel.clear ();
	for (LDObject* obj : g_CurrentFile->objects) {
		if (obj->getType() != eType)
			continue;
		
		if (eType == OBJ_Subfile && static_cast<LDSubfile*> (obj)->zFileName != zRefName)
			continue;
		
		g_ForgeWindow->sel.push_back (obj);
	}
	
	g_ForgeWindow->updateSelection ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (gridCoarse, "Coarse Grid", "grid-coarse", "Set the grid to Coarse", CTRL (1)) {
	grid = Grid::Coarse;
	g_ForgeWindow->updateGridToolBar ();
}

MAKE_ACTION (gridMedium, "Medium Grid", "grid-medium", "Set the grid to Medium", CTRL (2)) {
	grid = Grid::Medium;
	g_ForgeWindow->updateGridToolBar ();
}

MAKE_ACTION (gridFine, "Fine Grid", "grid-fine", "Set the grid to Fine", CTRL (3)) {
	grid = Grid::Fine;
	g_ForgeWindow->updateGridToolBar ();
}

// =============================================================================
MAKE_ACTION (resetView, "Reset View", "reset-view", "Reset view angles, pan and zoom", CTRL (0)) {
	g_ForgeWindow->R->resetAngles ();
	g_ForgeWindow->R->updateGL ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// Debug things
#ifndef RELEASE
MAKE_ACTION (addTestQuad, "Add Test Quad", "add-quad", "Adds a test quad.", (0)) {
	LDQuad* pQuad = new LDQuad;
	pQuad->dColor = rand () % 24;
	pQuad->vaCoords[0] = { 1.0f, 0.0f,  1.0f};
	pQuad->vaCoords[1] = {-1.0f, 0.0f,  1.0f};
	pQuad->vaCoords[2] = {-1.0f, 0.0f, -1.0f};
	pQuad->vaCoords[3] = { 1.0f, 0.0f, -1.0f};
	
	g_CurrentFile->addObject (pQuad);
	History::addEntry (new AddHistory ({(ulong)pQuad->getIndex (g_CurrentFile)}, {pQuad->clone ()}));
	g_ForgeWindow->refresh ();
}

MAKE_ACTION (addTestRadial, "Add Test Radial", "add-radial", "Adds a test radial.", (0)) {
	LDRadial* pRad = new LDRadial;
	pRad->eRadialType = LDRadial::Cone;
	pRad->mMatrix = g_mIdentity;
	pRad->vPosition = vertex (0, 0, 0);
	pRad->dColor = rand () % 24;
	pRad->dDivisions = 16;
	pRad->dRingNum = 2;
	pRad->dSegments = 16;
	
	g_CurrentFile->addObject (pRad);
	History::addEntry (new AddHistory ({(ulong)pRad->getIndex (g_CurrentFile)}, {pRad->clone ()}));
	g_ForgeWindow->refresh ();
}

#endif // RELEASE