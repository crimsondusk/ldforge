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

#include <qfiledialog.h>
#include <qmessagebox.h>
#include "gui.h"
#include "file.h"
#include "zz_newPartDialog.h"
#include "zz_configDialog.h"
#include "zz_addObjectDialog.h"
#include "history.h"

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (newFile, "&New", "brick", "Create a new part model.", CTRL (N)) {
	NewPartDialog::StaticDialog ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (open, "&Open", "file-open", "Load a part model from a file.", CTRL (O)) {
	str zName;
	zName += QFileDialog::getOpenFileName (g_ForgeWindow, "Open File",
		"", "LDraw files (*.dat *.ldr)");
	
	if (~zName)
		openMainFile (zName);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void doSaveAs () {
	str zName;
	zName += QFileDialog::getSaveFileName (g_ForgeWindow, "Save As",
		"", "LDraw files (*.dat *.ldr)");
	
	if (~zName && g_CurrentFile->save (zName))
		g_CurrentFile->zFileName = zName;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (save, "&Save", "file-save", "Save the part model.", CTRL (S)) {
	if (!~g_CurrentFile->zFileName) {
		// If we don't have a file name, this is an anonymous file created
		// with the new file command. We cannot save without a name so ask
		// the user for one.
		doSaveAs ();
		return;
	}
	
	g_CurrentFile->save ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (saveAs, "Save &As", "file-save-as", "Save the part model to a specific file.", CTRL_SHIFT (S))
{
	doSaveAs ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (settings, "Settin&gs", "settings", "Edit the settings of " APPNAME_DISPLAY ".", (0)) {
	ConfigDialog::staticDialog (g_ForgeWindow);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (exit, "&Exit", "exit", "Close " APPNAME_DISPLAY ".", CTRL (Q)) {
	exit (0);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (newSubfile, "New Subfile", "add-subfile", "Creates a new subfile reference.", 0) {
	
}

ACTION (newLine, "New Line",  "add-line", "Creates a new line.", 0) {
	AddObjectDialog::staticDialog (OBJ_Line, g_ForgeWindow);
}

ACTION (newTriangle, "New Triangle", "add-triangle", "Creates a new triangle.", 0) {
	AddObjectDialog::staticDialog (OBJ_Triangle, g_ForgeWindow);
}

ACTION (newQuad, "New Quadrilateral", "add-quad", "Creates a new quadrilateral.", 0) {
	AddObjectDialog::staticDialog (OBJ_Quad, g_ForgeWindow);
}

ACTION (newCondLine, "New Conditional Line", "add-condline", "Creates a new conditional line.", 0) {
	AddObjectDialog::staticDialog (OBJ_CondLine, g_ForgeWindow);
}

ACTION (newComment, "New Comment", "add-comment", "Creates a new comment.", 0) {
	AddObjectDialog::staticDialog (OBJ_Comment, g_ForgeWindow);
}
ACTION (newVertex, "New Vertex", "add-vertex", "Creates a new vertex.", 0) {
	AddObjectDialog::staticDialog (OBJ_Vertex, g_ForgeWindow);
}

ACTION (help, "Help", "help", "Shows the " APPNAME_DISPLAY " help manual.", KEY (F1)) {
	
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ACTION (about, "About " APPNAME_DISPLAY, "ldforge",
	"Shows information about " APPNAME_DISPLAY ".", CTRL (F1))
{
	
}

ACTION (aboutQt, "About Qt", "qt", "Shows information about Qt.", CTRL_SHIFT (F1)) {
	QMessageBox::aboutQt (g_ForgeWindow);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// Debug things
#ifndef RELEASE
ACTION (addTestQuad, "Add Test Quad", "add-quad", "Adds a test quad.", CTRL_SHIFT (Q)) {
	LDQuad* pQuad = new LDQuad;
	pQuad->dColor = rand () % 16;
	pQuad->vaCoords[0] = { 1.0f, 0.0f,  1.0f};
	pQuad->vaCoords[1] = {-1.0f, 0.0f,  1.0f};
	pQuad->vaCoords[2] = {-1.0f, 0.0f, -1.0f};
	pQuad->vaCoords[3] = { 1.0f, 0.0f, -1.0f};
	
	g_CurrentFile->addObject (pQuad);
	History::addEntry (new AddHistory ({(ulong)pQuad->getIndex (g_CurrentFile)}, {pQuad->clone ()}));
	g_ForgeWindow->refresh ();
}
#endif // RELEASE