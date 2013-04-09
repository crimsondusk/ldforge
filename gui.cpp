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

#include <QtGui>

#include "common.h"
#include "gldraw.h"
#include "gui.h"
#include "file.h"
#include "config.h"
#include "misc.h"
#include "colors.h"
#include "config.h"
#include "zz_addObjectDialog.h"
#include "zz_colorSelectDialog.h"
#include "zz_configDialog.h"
#include "zz_newPartDialog.h"
#include "zz_setContentsDialog.h"

// =============================================================================
// ACTION ARMADA
namespace Actions {
	QAction* newFile, *open, *save, *saveAs, *exit;
	QAction* cut, *copy, *paste, *del;
	QAction* newSubfile, *newLine, *newTriangle, *newQuad;
	QAction* newCondLine, *newComment, *newVertex;
	QAction* splitQuads, *setContents, *setColor;
	QAction* inlineContents, *deepInline, *makeBorders;
	QAction* settings;
	QAction* help, *about, *aboutQt;
};

// Key-binding configurations
cfg (keyseq, key_newFile,			(Qt::CTRL | Qt::Key_N));
cfg (keyseq, key_open,				(Qt::CTRL | Qt::Key_O));
cfg (keyseq, key_save,				(Qt::CTRL | Qt::Key_S));
cfg (keyseq, key_saveAs,			(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
cfg (keyseq, key_exit,				(Qt::CTRL | Qt::Key_Q));
cfg (keyseq, key_cut,				(Qt::CTRL | Qt::Key_X));
cfg (keyseq, key_copy,				(Qt::CTRL | Qt::Key_C));
cfg (keyseq, key_paste,				(Qt::CTRL | Qt::Key_V));
cfg (keyseq, key_del,				(Qt::Key_Delete));
cfg (keyseq, key_newSubfile,		0);
cfg (keyseq, key_newLine,			0);
cfg (keyseq, key_newTriangle,		0);
cfg (keyseq, key_newQuad,			0);
cfg (keyseq, key_newCondLine,		0);
cfg (keyseq, key_newComment,		0);
cfg (keyseq, key_newVertex,			0);
cfg (keyseq, key_splitQuads,		0);
cfg (keyseq, key_setContents,		0);
cfg (keyseq, key_inlineContents,	(Qt::CTRL | Qt::Key_I));
cfg (keyseq, key_deepInline,		(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
cfg (keyseq, key_makeBorders,		(Qt::CTRL | Qt::SHIFT | Qt::Key_B));
cfg (keyseq, key_settings,			0);
cfg (keyseq, key_help,				(Qt::Key_F1));
cfg (keyseq, key_about,				0);
cfg (keyseq, key_aboutQt,			0);
cfg (keyseq, key_setColor,			0);

vector<LDObject*> g_Clipboard;
vector<actionmeta> g_ActionMeta;

cfg (bool, lv_colorize, true);

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ForgeWindow::ForgeWindow () {
	R = new renderer;
	
	qObjList = new QTreeWidget;
	qObjList->setHeaderHidden (true);
	qObjList->setMaximumWidth (256);
	qObjList->setSelectionMode (QTreeWidget::MultiSelection);
	connect (qObjList, SIGNAL (itemSelectionChanged ()), this, SLOT (slot_selectionChanged ()));
	
	qMessageLog = new QTextEdit;
	qMessageLog->setReadOnly (true);
	qMessageLog->setMaximumHeight (96);
	
	QWidget* w = new QWidget;
	QGridLayout* layout = new QGridLayout;
	layout->setColumnMinimumWidth (0, 192);
	layout->setColumnStretch (0, 1);
	layout->addWidget (R,			0, 0);
	layout->addWidget (qObjList,	0, 1);
	layout->addWidget (qMessageLog,	1, 0, 1, 2);
	w->setLayout (layout);
	setCentralWidget (w);
	
	createMenuActions ();
	createMenus ();
	createToolbars ();
	
	slot_selectionChanged ();
	
	setWindowIcon (QIcon ("icons/ldforge.png"));
	setTitle ();
	setMinimumSize (320, 200);
	resize (800, 600);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
#define ACTION(N) (Actions::N)
#define MAKE_ACTION(OBJECT, DISPLAYNAME, IMAGENAME, DESCR) \
	ACTION (OBJECT) = new QAction (QIcon ("./icons/" IMAGENAME ".png"), tr (DISPLAYNAME), this); \
	ACTION (OBJECT)->setStatusTip (tr (DESCR)); \
	connect (ACTION (OBJECT), SIGNAL (triggered ()), this, SLOT (slot_##OBJECT ())); \
	{ \
		actionmeta meta = {&ACTION (OBJECT), &key_##OBJECT}; \
		g_ActionMeta.push_back (meta); \
	}

void ForgeWindow::createMenuActions () {
	// Long menu names go here so my cool action definition table doesn't get out of proportions
	char const* sNewCdLineText = "New Conditional Line",
		*sNewQuadText = "New Quadrilateral",
		*sAboutText = "About " APPNAME_DISPLAY;
	
	MAKE_ACTION (newFile,		"&New",			"brick",		"Create a new part model.")
	MAKE_ACTION (open,			"&Open",		"file-open",	"Load a part model from a file.")
	MAKE_ACTION (save,			"&Save",		"file-save",	"Save the part model.")
	MAKE_ACTION (saveAs,		"Save &As",		"file-save-as",	"Save the part to a specific file.")
	MAKE_ACTION (exit,			"&Exit",		"exit",			"Close " APPNAME_DISPLAY ".")
	
	MAKE_ACTION (cut,			"Cut",			"cut",			"Cut the current selection to clipboard.")
	MAKE_ACTION (copy,			"Copy",			"copy",			"Copy the current selection to clipboard.")
	MAKE_ACTION (paste,			"Paste",		"paste",		"Paste clipboard contents.")
	MAKE_ACTION (del,			"Delete",		"delete",		"Delete the selection")
	
	MAKE_ACTION (setColor,		"Set Color",	"palette",		"Set the color on given objects.")
	MAKE_ACTION (inlineContents,"Inline",		"inline",		"Inline selected subfiles.")
	MAKE_ACTION (deepInline,	"Deep Inline",	"inline-deep",	"Recursively inline selected subfiles down to polygons only.")
	MAKE_ACTION (splitQuads,	"Split Quads",	"quad-split",	"Split quads into triangles.")
	MAKE_ACTION (setContents,	"Set Contents",	"set-contents",	"Set the raw code of this object.")
	MAKE_ACTION (makeBorders,	"Make Borders",	"make-borders",	"Add borders around given polygons.")
	
	MAKE_ACTION (newSubfile,	"New Subfile",	"add-subfile",	"Creates a new subfile reference.")
	MAKE_ACTION (newLine,		"New Line", 	"add-line",		"Creates a new line.")
	MAKE_ACTION (newTriangle,	"New Triangle", "add-triangle",	"Creates a new triangle.")
	MAKE_ACTION (newQuad,		sNewQuadText,	"add-quad",		"Creates a new quadrilateral.")
	MAKE_ACTION (newCondLine,	sNewCdLineText,	"add-condline",	"Creates a new conditional line.")
	MAKE_ACTION (newComment,	"New Comment",	"add-comment",	"Creates a new comment.")
	MAKE_ACTION (newVertex,		"New Vertex",	"add-vertex",	"Creates a new vertex.")
	
	MAKE_ACTION (settings,		"Settings",		"settings",		"Edit the settings of " APPNAME_DISPLAY ".")
	
	MAKE_ACTION (help,			"Help",			"help",			"Shows the " APPNAME_DISPLAY " help manual.")
	MAKE_ACTION (about,			sAboutText,		"ldforge",		"Shows information about " APPNAME_DISPLAY ".")
	MAKE_ACTION (aboutQt,		"About Qt",		"aboutQt",		"Shows information about Qt.")
	
	// Apply the shortcuts now
	for (actionmeta meta : g_ActionMeta)
		(*meta.qAct)->setShortcut (*meta.conf);
	
	// things not implemented yet
	QAction* const qaDisabledActions[] = {
		ACTION (newSubfile),
		ACTION (about),
		ACTION (help),
	};
	
	for (QAction* act : qaDisabledActions)
		act->setEnabled (false);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
#define ADD_MENU_ITEM(MENU, ACT) q##MENU##Menu->addAction (ACTION (ACT));

void ForgeWindow::createMenus () {
	// File menu
	qFileMenu = menuBar ()->addMenu (tr ("&File"));
	ADD_MENU_ITEM (File, newFile)			// New
	ADD_MENU_ITEM (File, open)				// Open
	ADD_MENU_ITEM (File, save)				// Save
	ADD_MENU_ITEM (File, saveAs)			// Save As
	qFileMenu->addSeparator ();				// -------
	ADD_MENU_ITEM (File, settings)			// Settings
	qFileMenu->addSeparator ();				// -------
	ADD_MENU_ITEM (File, exit)				// Exit
	
	// Edit menu
	qInsertMenu = menuBar ()->addMenu (tr ("&Insert"));
	ADD_MENU_ITEM (Insert, newSubfile)		// New Subfile
	ADD_MENU_ITEM (Insert, newLine)			// New Line
	ADD_MENU_ITEM (Insert, newTriangle)		// New Triangle
	ADD_MENU_ITEM (Insert, newQuad)			// New Quad
	ADD_MENU_ITEM (Insert, newCondLine)		// New Conditional Line
	ADD_MENU_ITEM (Insert, newComment)		// New Comment
	ADD_MENU_ITEM (Insert, newVertex)		// New Vertex
	
	qEditMenu = menuBar ()->addMenu (tr ("&Edit"));
	ADD_MENU_ITEM (Edit, cut)				// Cut
	ADD_MENU_ITEM (Edit, copy)				// Copy
	ADD_MENU_ITEM (Edit, paste)				// Paste
	ADD_MENU_ITEM (Edit, del)				// Delete
	qEditMenu->addSeparator ();				// -----
	ADD_MENU_ITEM (Edit, setColor)			// Set Color
	qEditMenu->addSeparator ();				// -----
	ADD_MENU_ITEM (Edit, inlineContents)	// Inline
	ADD_MENU_ITEM (Edit, deepInline)		// Deep Inline
	ADD_MENU_ITEM (Edit, splitQuads)		// Split Quads
	ADD_MENU_ITEM (Edit, setContents)		// Set Contents
	ADD_MENU_ITEM (Edit, makeBorders)		// Make Borders
	
	// Help menu
	qHelpMenu = menuBar ()->addMenu (tr ("&Help"));
	ADD_MENU_ITEM (Help, help)				// Help
	qHelpMenu->addSeparator ();				// -----
	ADD_MENU_ITEM (Help, about)				// About
	ADD_MENU_ITEM (Help, aboutQt)			// About Qt
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
#define ADD_TOOLBAR_ITEM(BAR, ACT) q##BAR##ToolBar->addAction (ACTION (ACT));

void ForgeWindow::createToolbars () {
	qFileToolBar = new QToolBar ("File");
	ADD_TOOLBAR_ITEM (File, newFile)
	ADD_TOOLBAR_ITEM (File, open)
	ADD_TOOLBAR_ITEM (File, save)
	ADD_TOOLBAR_ITEM (File, saveAs)
	addToolBar (qFileToolBar);
	
	qInsertToolBar = new QToolBar ("Insert");
	ADD_TOOLBAR_ITEM (Insert, newSubfile)
	ADD_TOOLBAR_ITEM (Insert, newLine)
	ADD_TOOLBAR_ITEM (Insert, newTriangle)
	ADD_TOOLBAR_ITEM (Insert, newQuad)
	ADD_TOOLBAR_ITEM (Insert, newCondLine)
	ADD_TOOLBAR_ITEM (Insert, newComment)
	ADD_TOOLBAR_ITEM (Insert, newVertex)
	addToolBar (qInsertToolBar);
	
	qEditToolBar = new QToolBar ("Edit");
	ADD_TOOLBAR_ITEM (Edit, cut)
	ADD_TOOLBAR_ITEM (Edit, copy)
	ADD_TOOLBAR_ITEM (Edit, paste)
	ADD_TOOLBAR_ITEM (Edit, del)
	ADD_TOOLBAR_ITEM (Edit, setColor)
	ADD_TOOLBAR_ITEM (Edit, inlineContents)
	ADD_TOOLBAR_ITEM (Edit, splitQuads)
	ADD_TOOLBAR_ITEM (Edit, setContents)
	ADD_TOOLBAR_ITEM (Edit, makeBorders)
	addToolBar (qEditToolBar);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::setTitle () {
	str zTitle = APPNAME_DISPLAY " v" VERSION_STRING;
	
	// Append our current file if we have one
	if (g_CurrentFile) {
		zTitle.appendformat (": %s", basename (g_CurrentFile->zFileName.chars()));
		
		if (g_CurrentFile->objects.size() > 0 &&
			g_CurrentFile->objects[0]->getType() == OBJ_Comment)
		{
			// Append title
			LDComment* comm = static_cast<LDComment*> (g_CurrentFile->objects[0]);
			zTitle.appendformat (":%s", comm->zText.chars());
		}
	}
	
	setWindowTitle (zTitle.chars());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::slot_newFile () {
	// newFile ();
	NewPartDialog::StaticDialog ();
}

void ForgeWindow::slot_open () {
	str zName;
	zName += QFileDialog::getOpenFileName (this, "Open File",
		"", "LDraw files (*.dat *.ldr)");
	
	if (~zName)
		openMainFile (zName);
}

void ForgeWindow::slot_save () {
	if (!~g_CurrentFile->zFileName) {
		// If we don't have a file name, this is an anonymous file created
		// with the new file command. We cannot save without a name so ask
		// the user for one.
		slot_saveAs ();
		return;
	}
	
	g_CurrentFile->save ();
}

void ForgeWindow::slot_saveAs () {
	str zName;
	zName += QFileDialog::getSaveFileName (this, "Save As",
		"", "LDraw files (*.dat *.ldr)");
	
	if (~zName && g_CurrentFile->save (zName))
		g_CurrentFile->zFileName = zName;
}

void ForgeWindow::slot_settings () {
	ConfigDialog::staticDialog (this);
}

void ForgeWindow::slot_exit () {
	exit (0);
}

void ForgeWindow::slot_newSubfile () {
	
}

void ForgeWindow::slot_newLine () {
	AddObjectDialog::staticDialog (OBJ_Line, this);
}

void ForgeWindow::slot_newTriangle () {
	AddObjectDialog::staticDialog (OBJ_Triangle, this);
}

void ForgeWindow::slot_newQuad () {
	AddObjectDialog::staticDialog (OBJ_Quad, this);
}

void ForgeWindow::slot_newCondLine () {
	AddObjectDialog::staticDialog (OBJ_CondLine, this);
}

void ForgeWindow::slot_newComment () {
	AddObjectDialog::staticDialog (OBJ_Comment, this);
}

void ForgeWindow::slot_help () {
	
}

void ForgeWindow::slot_about () {
	
}

void ForgeWindow::slot_aboutQt () {
	QMessageBox::aboutQt (this);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool ForgeWindow::copyToClipboard () {
	vector<LDObject*> objs = getSelectedObjects ();
	
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
void ForgeWindow::slot_cut () {
	if (!copyToClipboard ())
		return;
	
	deleteSelection ();
	
	ACTION (paste)->setEnabled (true);
	refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::slot_copy () {
	if (copyToClipboard ())
		ACTION (paste)->setEnabled (true);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::slot_paste () {
	for (LDObject* obj : g_Clipboard)
		g_CurrentFile->addObject (obj->clone ());
	
	refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::slot_del () {
	deleteSelection ();
	refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::slot_newVertex () {
	AddObjectDialog::staticDialog (OBJ_Vertex, this);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::doInline (bool bDeep) {
	vector<LDObject*> sel = getSelectedObjects ();
	
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
	
	refresh ();
}

void ForgeWindow::slot_inlineContents () {
	doInline (false);
}

void ForgeWindow::slot_deepInline () {
	doInline (true);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::slot_splitQuads () {
	vector<LDObject*> objs = getSelectedObjects ();
	
	for (LDObject* obj : objs) {
		if (obj->getType() != OBJ_Quad)
			continue;
		
		// Find the index of this quad
		long lIndex = obj->getIndex (g_CurrentFile);
		
		if (lIndex == -1) {
			// couldn't find it?
			logf (LOG_Error, "Couldn't find quad %p in "
				"current object list!!\n", this);
			return;
		}
		
		std::vector<LDTriangle*> triangles = static_cast<LDQuad*> (obj)->splitToTriangles ();
		
		// Replace the quad with the first triangle and add the second triangle
		// after the first one.
		g_CurrentFile->objects[lIndex] = triangles[0];
		g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + lIndex + 1, triangles[1]);
		
		// Delete this quad now, it has been split.
		delete this;
	}
	
	refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::slot_setContents () {
	if (qObjList->selectedItems().size() != 1)
		return;
	
	LDObject* obj = getSelectedObjects ()[0];
	SetContentsDialog::staticDialog (obj, this);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::slot_setColor () {
	if (qObjList->selectedItems().size() <= 0)
		return;
	
	short dColor;
	short dDefault = -1;
	
	std::vector<LDObject*> objs = getSelectedObjects ();
	
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
	if (ColorSelectDialog::staticDialog (dColor, dDefault, this)) {
		for (LDObject* obj : objs)
			if (obj->dColor != -1)
				obj->dColor = dColor;
		
		refresh ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::slot_makeBorders () {
	vector<LDObject*> objs = getSelectedObjects ();
	
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
	
	refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::deleteSelection () {
	vector<LDObject*> objs = getSelectedObjects ();
	
	// Delete the objects that were being selected
	for (LDObject* obj : objs) {
		g_CurrentFile->forgetObject (obj);
		delete obj;
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::buildObjList () {
	if (!g_CurrentFile)
		return;
	
	QList<QTreeWidgetItem*> qaItems;
	
	qObjList->clear ();
	
	for (LDObject* obj : g_CurrentFile->objects) {
		str zText;
		switch (obj->getType ()) {
		case OBJ_Comment:
			zText = static_cast<LDComment*> (obj)->zText.chars();
			
			// Remove leading whitespace
			while (~zText && zText[0] == ' ')
				zText -= -1;
			break;
		
		case OBJ_Empty:
			break; // leave it empty
		
		case OBJ_Line:
			{
				LDLine* line = static_cast<LDLine*> (obj);
				zText.format ("%s, %s",
					line->vaCoords[0].getStringRep (true).chars(),
					line->vaCoords[1].getStringRep (true).chars());
			}
			break;
		
		case OBJ_Triangle:
			{
				LDTriangle* triangle = static_cast<LDTriangle*> (obj);
				zText.format ("%s, %s, %s",
					triangle->vaCoords[0].getStringRep (true).chars(),
					triangle->vaCoords[1].getStringRep (true).chars(),
					triangle->vaCoords[2].getStringRep (true).chars());
			}
			break;
		
		case OBJ_Quad:
			{
				LDQuad* quad = static_cast<LDQuad*> (obj);
				zText.format ("%s, %s, %s, %s",
					quad->vaCoords[0].getStringRep (true).chars(),
					quad->vaCoords[1].getStringRep (true).chars(),
					quad->vaCoords[2].getStringRep (true).chars(),
					quad->vaCoords[3].getStringRep (true).chars());
			}
			break;
		
		case OBJ_CondLine:
			{
				LDCondLine* line = static_cast<LDCondLine*> (obj);
				zText.format ("%s, %s, %s, %s",
					line->vaCoords[0].getStringRep (true).chars(),
					line->vaCoords[1].getStringRep (true).chars(),
					line->vaCoords[2].getStringRep (true).chars(),
					line->vaCoords[3].getStringRep (true).chars());
			}
			break;
		
		case OBJ_Gibberish:
			zText.format ("ERROR: %s",
				static_cast<LDGibberish*> (obj)->zContents.chars());
			break;
		
		case OBJ_Vertex:
			zText.format ("%s", static_cast<LDVertex*> (obj)->vPosition.getStringRep (true).chars());
			break;
		
		case OBJ_Subfile:
			{
				LDSubfile* ref = static_cast<LDSubfile*> (obj);
				
				zText.format ("%s %s, (",
					ref->zFileName.chars(), ref->vPosition.getStringRep (true).chars());
				
				for (short i = 0; i < 9; ++i)
					zText.appendformat ("%s%s",
						ftoa (ref->mMatrix[i]).chars(),
						(i != 8) ? " " : "");
				
				zText += ')';
			}
			break;
		
		case OBJ_BFC:
			{
				LDBFC* bfc = static_cast<LDBFC*> (obj);
				zText = LDBFC::saStatements[bfc->dStatement];
			}
			break;
		
		default:
			zText = g_saObjTypeNames[obj->getType ()];
			break;
		}
		
		QTreeWidgetItem* item = new QTreeWidgetItem ((QTreeWidget*) (nullptr),
			QStringList (zText.chars()), 0);
		item->setIcon (0, QIcon (str::mkfmt ("icons/%s.png", g_saObjTypeIcons[obj->getType ()]).chars()));
		
		// Color gibberish red
		if (obj->getType() == OBJ_Gibberish) {
			item->setBackground (0, QColor ("#AA0000"));
			item->setForeground (0, QColor ("#FFAA00"));
		} else if (lv_colorize &&
			obj->dColor != -1 &&
			obj->dColor != dMainColor &&
			obj->dColor != dEdgeColor)
		{
			// If the object isn't in the main or edge color, draw this
			// list entry in said color.
			color* col = getColor (obj->dColor);
			if (col)
				item->setForeground (0, col->qColor);
		}
		
		obj->qObjListEntry = item;
		
		qaItems.append (item);
	}
	
	qObjList->insertTopLevelItems (0, qaItems);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::slot_selectionChanged () {
	// If the selection isn't 1 exact, disable setting contents
	ACTION (setContents)->setEnabled (qObjList->selectedItems().size() == 1);
	
	// If we have no selection, disable splitting quads
	ACTION (splitQuads)->setEnabled (qObjList->selectedItems().size() > 0);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ulong ForgeWindow::getInsertionPoint () {
	ulong ulIndex;
	
	if (qObjList->selectedItems().size() == 1) {
		// If we have a selection, put the item after it.
		for (ulIndex = 0; ulIndex < g_CurrentFile->objects.size(); ++ulIndex)
			if (g_CurrentFile->objects[ulIndex]->qObjListEntry == qObjList->selectedItems()[0])
				break;
		
		if (ulIndex >= g_CurrentFile->objects.size())
			return ulIndex + 1;
	}
	
	// Otherwise place the object at the end.
	return g_CurrentFile->objects.size();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::refresh () {
	buildObjList ();
	R->hardRefresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
std::vector<LDObject*> ForgeWindow::getSelectedObjects () {
	std::vector<LDObject*> objs;
	
	QList<QTreeWidgetItem*> const qaItems = qObjList->selectedItems();
	for (LDObject* obj : g_CurrentFile->objects)
	for (QTreeWidgetItem* qItem : qaItems) {
		if (qItem == obj->qObjListEntry) {
			objs.push_back (obj);
			break;
		}
	}
	
	return objs;
}