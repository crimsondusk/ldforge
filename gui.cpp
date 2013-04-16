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

#include <QtGui>

#include "common.h"
#include "gldraw.h"
#include "gui.h"
#include "file.h"
#include "config.h"
#include "misc.h"
#include "colors.h"
#include "history.h"
#include "config.h"

EXTERN_ACTION (newFile)
EXTERN_ACTION (open)
EXTERN_ACTION (save)
EXTERN_ACTION (saveAs)
EXTERN_ACTION (settings)
EXTERN_ACTION (exit)
EXTERN_ACTION (cut)
EXTERN_ACTION (copy)
EXTERN_ACTION (paste)
EXTERN_ACTION (del)
EXTERN_ACTION (setColor)
EXTERN_ACTION (inlineContents)
EXTERN_ACTION (deepInline)
EXTERN_ACTION (splitQuads)
EXTERN_ACTION (setContents)
EXTERN_ACTION (makeBorders)
EXTERN_ACTION (makeCornerVerts)
EXTERN_ACTION (moveUp)
EXTERN_ACTION (moveDown)
EXTERN_ACTION (newSubfile)
EXTERN_ACTION (newLine)
EXTERN_ACTION (newCondLine)
EXTERN_ACTION (newTriangle)
EXTERN_ACTION (newQuad)
EXTERN_ACTION (newVertex)
EXTERN_ACTION (newComment)
EXTERN_ACTION (newRadial)
EXTERN_ACTION (help)
EXTERN_ACTION (about)
EXTERN_ACTION (aboutQt)
EXTERN_ACTION (undo)
EXTERN_ACTION (redo)
EXTERN_ACTION (showHistory)
EXTERN_ACTION (selectByColor)
EXTERN_ACTION (selectByType)
EXTERN_ACTION (moveXNeg)
EXTERN_ACTION (moveYNeg)
EXTERN_ACTION (moveZNeg)
EXTERN_ACTION (moveXPos)
EXTERN_ACTION (moveYPos)
EXTERN_ACTION (moveZPos)

#ifndef RELEASE
EXTERN_ACTION (addTestQuad)
EXTERN_ACTION (addTestRadial)
#endif // RELEASE

vector<actionmeta> g_ActionMeta;

static bool g_bSelectionLocked = false;

cfg (bool, lv_colorize, true);
cfg (int, gui_toolbar_iconsize, 24);
cfg (str, gui_colortoolbar, "16:24:|:0:1:2:3:4:5:6:7");
extern_cfg (str, io_recentfiles);

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
ForgeWindow::ForgeWindow () {
	g_ForgeWindow = this;
	R = new GLRenderer;
	
	qObjList = new QTreeWidget;
	qObjList->setHeaderHidden (true);
	qObjList->setMaximumWidth (256);
	qObjList->setSelectionMode (QTreeWidget::ExtendedSelection);
	qObjList->setAlternatingRowColors (true);
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
	
	quickColorMeta = parseQuickColorMeta ();
	
	createMenuActions ();
	createMenus ();
	createToolbars ();
	
	slot_selectionChanged ();
	
	setWindowIcon (QIcon ("icons/ldforge.png"));
	setTitle ();
	setMinimumSize (320, 200);
	resize (800, 600);
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void ForgeWindow::createMenuActions () {
	// Create the actions based on stored meta.
	for (actionmeta meta : g_ActionMeta) {
		QAction*& qAct = *meta.qAct;
		qAct = new QAction (getIcon (meta.sIconName), meta.sDisplayName, this);
		qAct->setStatusTip (meta.sDescription);
		qAct->setShortcut (*meta.conf);
		
		connect (qAct, SIGNAL (triggered ()), this, SLOT (slot_action ()));
	}
	
	// things not implemented yet
	QAction* const qaDisabledActions[] = {
		ACTION_NAME (newSubfile),
		ACTION_NAME (about),
		ACTION_NAME (help),
	};
	
	for (QAction* act : qaDisabledActions)
		act->setEnabled (false);
	
	History::updateActions ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void ForgeWindow::createMenus () {
	qRecentFilesMenu = new QMenu (tr ("Open &Recent"));
	qRecentFilesMenu->setIcon (getIcon ("open-recent"));
	updateRecentFilesMenu ();
	
	// File menu
	qFileMenu = menuBar ()->addMenu (tr ("&File"));
	qFileMenu->addAction (ACTION_NAME (newFile));			// New
	qFileMenu->addAction (ACTION_NAME (open));				// Open
	qFileMenu->addMenu (qRecentFilesMenu);					// Open Recent
	qFileMenu->addAction (ACTION_NAME (save));				// Save
	qFileMenu->addAction (ACTION_NAME (saveAs));			// Save As
	qFileMenu->addSeparator ();								// -------
	qFileMenu->addAction (ACTION_NAME (settings));			// Settings
	qFileMenu->addSeparator ();								// -------
	qFileMenu->addAction (ACTION_NAME (exit));				// Exit
	
	// Insert menu
	qInsertMenu = menuBar ()->addMenu (tr ("&Insert"));
	qInsertMenu->addAction (ACTION_NAME (newSubfile));		// New Subfile
	qInsertMenu->addAction (ACTION_NAME (newLine));			// New Line
	qInsertMenu->addAction (ACTION_NAME (newTriangle));		// New Triangle
	qInsertMenu->addAction (ACTION_NAME (newQuad));			// New Quad
	qInsertMenu->addAction (ACTION_NAME (newCondLine));		// New Conditional Line
	qInsertMenu->addAction (ACTION_NAME (newComment));		// New Comment
	qInsertMenu->addAction (ACTION_NAME (newVertex));		// New Vertex
	qInsertMenu->addAction (ACTION_NAME (newRadial));		// New Radial
	
	// Edit menu
	qEditMenu = menuBar ()->addMenu (tr ("&Edit"));
	qEditMenu->addAction (ACTION_NAME (undo));				// Undo
	qEditMenu->addAction (ACTION_NAME (redo));				// Redo
	qEditMenu->addSeparator ();								// -----
	qEditMenu->addAction (ACTION_NAME (cut));				// Cut
	qEditMenu->addAction (ACTION_NAME (copy));				// Copy
	qEditMenu->addAction (ACTION_NAME (paste));				// Paste
	qEditMenu->addAction (ACTION_NAME (del));				// Delete
	qEditMenu->addSeparator ();								// -----
	qEditMenu->addAction (ACTION_NAME (selectByColor));		// Select by Color
	qEditMenu->addAction (ACTION_NAME (selectByType));		// Select by Type
	qEditMenu->addSeparator ();								// -----
	qEditMenu->addAction (ACTION_NAME (setColor));			// Set Color
	qEditMenu->addAction (ACTION_NAME (inlineContents));	// Inline
	qEditMenu->addAction (ACTION_NAME (deepInline));		// Deep Inline
	qEditMenu->addAction (ACTION_NAME (splitQuads));		// Split Quads
	qEditMenu->addAction (ACTION_NAME (setContents));		// Set Contents
	qEditMenu->addAction (ACTION_NAME (makeBorders));		// Make Borders
	qEditMenu->addAction (ACTION_NAME (makeCornerVerts));	// Make Corner Vertices
	
	// Move menu
	qMoveMenu = menuBar ()->addMenu (tr ("&Move"));
	qMoveMenu->addAction (ACTION_NAME (moveUp));			// Move Up
	qMoveMenu->addAction (ACTION_NAME (moveDown));			// Move Down
	qMoveMenu->addSeparator ();								// -----
	qMoveMenu->addAction (ACTION_NAME (moveXPos));			// Move +X
	qMoveMenu->addAction (ACTION_NAME (moveXNeg));			// Move -X
	qMoveMenu->addAction (ACTION_NAME (moveYPos));			// Move +Y
	qMoveMenu->addAction (ACTION_NAME (moveYNeg));			// Move -Y
	qMoveMenu->addAction (ACTION_NAME (moveZPos));			// Move +Z
	qMoveMenu->addAction (ACTION_NAME (moveZNeg));			// Move -Z
	
	// Control menu
	qControlMenu = menuBar ()->addMenu (tr ("&Control"));
	qControlMenu->addAction (ACTION_NAME (showHistory));	// Show History
	
#ifndef RELEASE
	// Debug menu
	qDebugMenu = menuBar ()->addMenu (tr ("&Debug"));
	qDebugMenu->addAction (ACTION_NAME (addTestQuad));		// Add Test Quad
	qDebugMenu->addAction (ACTION_NAME (addTestRadial));	// Add Test Radial
#endif // RELEASE
	
	// Help menu
	qHelpMenu = menuBar ()->addMenu (tr ("&Help"));
	qHelpMenu->addAction (ACTION_NAME (help));				// Help
	qHelpMenu->addSeparator ();								// -----
	qHelpMenu->addAction (ACTION_NAME (about));				// About
	qHelpMenu->addAction (ACTION_NAME (aboutQt));			// About Qt
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void ForgeWindow::updateRecentFilesMenu () {
	// First, clear any items in the recent files menu
	for (QAction* qRecent : qaRecentFiles)
		delete qRecent;
	qaRecentFiles.clear ();
	
	std::vector<str> zaFiles = io_recentfiles.value / "@";
	for (long i = zaFiles.size() - 1; i >= 0; --i) {
		str zFile = zaFiles[i];
		
		QAction* qRecent = new QAction (getIcon ("open-recent"), zFile, this);
		
		connect (qRecent, SIGNAL (triggered ()), this, SLOT (slot_recentFile ()));
		qRecentFilesMenu->addAction (qRecent);
		qaRecentFiles.push_back (qRecent);
	}
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
#define ADD_TOOLBAR_ITEM(ACT) g_CurrentToolBar->addAction (ACTION_NAME (ACT));
static QToolBar* g_CurrentToolBar;
static Qt::ToolBarArea g_ToolBarArea = Qt::TopToolBarArea;

void ForgeWindow::initSingleToolBar (const char* sName) {
	QToolBar* qBar = new QToolBar (sName);
	addToolBar (g_ToolBarArea, qBar);
	qaToolBars.push_back (qBar);
	
	g_CurrentToolBar = qBar;
}

void ForgeWindow::createToolbars () {
	initSingleToolBar ("File");
	ADD_TOOLBAR_ITEM (newFile)
	ADD_TOOLBAR_ITEM (open)
	ADD_TOOLBAR_ITEM (save)
	ADD_TOOLBAR_ITEM (saveAs)
	
	initSingleToolBar ("Insert");
	ADD_TOOLBAR_ITEM (newSubfile)
	ADD_TOOLBAR_ITEM (newLine)
	ADD_TOOLBAR_ITEM (newTriangle)
	ADD_TOOLBAR_ITEM (newQuad)
	ADD_TOOLBAR_ITEM (newCondLine)
	ADD_TOOLBAR_ITEM (newComment)
	ADD_TOOLBAR_ITEM (newVertex)
	ADD_TOOLBAR_ITEM (newRadial)
	
	initSingleToolBar ("Edit");
	ADD_TOOLBAR_ITEM (undo)
	ADD_TOOLBAR_ITEM (redo)
	ADD_TOOLBAR_ITEM (cut)
	ADD_TOOLBAR_ITEM (copy)
	ADD_TOOLBAR_ITEM (paste)
	ADD_TOOLBAR_ITEM (del)
	
	initSingleToolBar ("Move");
	ADD_TOOLBAR_ITEM (moveUp)
	ADD_TOOLBAR_ITEM (moveDown)
	ADD_TOOLBAR_ITEM (moveXPos)
	ADD_TOOLBAR_ITEM (moveXNeg)
	ADD_TOOLBAR_ITEM (moveYPos)
	ADD_TOOLBAR_ITEM (moveYNeg)
	ADD_TOOLBAR_ITEM (moveZPos)
	ADD_TOOLBAR_ITEM (moveZNeg)
	
	addToolBarBreak (Qt::TopToolBarArea);
	
	initSingleToolBar ("Select");
	ADD_TOOLBAR_ITEM (selectByColor)
	ADD_TOOLBAR_ITEM (selectByType)
	
	// ==========================================
	// Color toolbar
	qColorToolBar = new QToolBar ("Quick Colors");
	addToolBar (Qt::RightToolBarArea, qColorToolBar);
	
	// ==========================================
	// Left area toolbars
	g_ToolBarArea = Qt::LeftToolBarArea;
	initSingleToolBar ("Objects");
	ADD_TOOLBAR_ITEM (setColor)
	ADD_TOOLBAR_ITEM (inlineContents)
	ADD_TOOLBAR_ITEM (deepInline)
	ADD_TOOLBAR_ITEM (splitQuads)
	ADD_TOOLBAR_ITEM (setContents)
	ADD_TOOLBAR_ITEM (makeBorders)
	ADD_TOOLBAR_ITEM (makeCornerVerts)
	
	updateToolBars ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
std::vector<quickColorMetaEntry> parseQuickColorMeta () {
	std::vector<quickColorMetaEntry> meta;
	
	for (str zColor : gui_colortoolbar.value / ":") {
		if (zColor == "|") {
			meta.push_back ({null, null, true});
		} else {
			color* col = getColor (atoi (zColor));
			meta.push_back ({col, null, false});
		}
	}
	
	return meta;
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void ForgeWindow::updateToolBars () {
	for (QToolBar* qBar : qaToolBars)
		qBar->setIconSize (QSize (gui_toolbar_iconsize, gui_toolbar_iconsize));
	
	// Update the quick color toolbar.
	for (QPushButton* qButton : qaColorButtons)
		delete qButton;
	
	qaColorButtons.clear ();
	
	// Clear the toolbar to remove separators
	qColorToolBar->clear ();
	
	for (quickColorMetaEntry& entry : quickColorMeta) {
		if (entry.bSeparator)
			qColorToolBar->addSeparator ();
		else {
			QPushButton* qColorButton = new QPushButton;
			qColorButton->setAutoFillBackground (true);
			qColorButton->setStyleSheet (str::mkfmt ("background-color: %s", entry.col->zColorString.chars()));
			qColorButton->setToolTip (entry.col->zName);
			
			connect (qColorButton, SIGNAL (clicked ()), this, SLOT (slot_quickColor ()));
			qColorToolBar->addWidget (qColorButton);
			qaColorButtons.push_back (qColorButton);
			
			entry.btn = qColorButton;
		}
	}
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
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
			zTitle.appendformat (": %s", comm->zText.chars());
		}
	}
	
	setWindowTitle (zTitle.chars());
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void ForgeWindow::slot_action () {
	// Get the action that triggered this slot.
	QAction* qAct = static_cast<QAction*> (sender ());
	
	// Find the meta for the action.
	actionmeta* pMeta = null;
	
	for (actionmeta meta : g_ActionMeta) {
		if (*meta.qAct == qAct) {
			pMeta = &meta;
			break;
		}
	}
	
	if (!pMeta) {
		fprintf (stderr, "unknown signal sender %p!\n", qAct);
		return;
	}
	
	// We have the meta, now call the handler.
	(*pMeta->handler) ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void ForgeWindow::deleteSelection (vector<ulong>* ulapIndices, std::vector<LDObject*>* papObjects) {
	if (selection ().size () == 0)
		return;
	
	std::vector<LDObject*> sel = selection ();
	
	// Delete the objects that were being selected
	for (LDObject* obj : sel) {
		if (papObjects && ulapIndices) {
			papObjects->push_back (obj->clone ());
			ulapIndices->push_back (obj->getIndex (g_CurrentFile));
		}
		
		g_CurrentFile->forgetObject (obj);
		delete obj;
	}
	
	refresh ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void ForgeWindow::buildObjList () {
	if (!g_CurrentFile)
		return;
	
	// Lock the selection while we do this so that refreshing the object list
	// doesn't trigger selection updating so that the selection doesn't get lost
	// while this is done
	g_bSelectionLocked = true;
	
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
				zText = LDBFC::saStatements[bfc->eStatement];
			}
			break;
		
		case OBJ_Radial:
			{
				LDRadial* pRad = static_cast<LDRadial*> (obj);
				zText.format ("%s: %d / %d %s", pRad->makeFileName ().chars (),
					pRad->dSegments, pRad->dDivisions, pRad->radialTypeName());
				
				if (pRad->eRadialType == LDRadial::Ring || pRad->eRadialType == LDRadial::Cone)
					zText.appendformat (" %d", pRad->dRingNum);
				
				zText.appendformat (" %s", pRad->vPosition.getStringRep (true).chars ());
			}
			break;
		
		default:
			zText = g_saObjTypeNames[obj->getType ()];
			break;
		}
		
		QTreeWidgetItem* item = new QTreeWidgetItem ((QTreeWidget*) (null),
			QStringList (zText.chars()), 0);
		item->setIcon (0, getIcon (g_saObjTypeIcons[obj->getType ()]));
		
		// Color gibberish orange on red so it stands out.
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
	
	g_bSelectionLocked = false;
	updateSelection ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void ForgeWindow::slot_selectionChanged () {
	if (g_bSelectionLocked == true)
		return;
	
	/*
	// If the selection isn't 1 exact, disable setting contents
	ACTION (setContents)->setEnabled (qObjList->selectedItems().size() == 1);
	
	// If we have no selection, disable splitting quads
	ACTION (splitQuads)->setEnabled (qObjList->selectedItems().size() > 0);
	*/
	
	// Update the shared selection array, unless this was called during GL picking,
	// in which case the GL renderer takes care of the selection.
	if (R->bPicking == false) {
		std::vector<LDObject*> paPriorSelection = paSelection;
		paSelection = getSelectedObjects ();
		
		// Update the GL renderer
		for (LDObject* obj : paSelection)
			R->recompileObject (obj);
		
		for (LDObject* obj : paPriorSelection)
			R->recompileObject (obj);
		
		R->updateSelFlash ();
		R->refresh ();
	}
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void ForgeWindow::slot_recentFile () {
	QAction* qAct = static_cast<QAction*> (sender ());
	openMainFile (qAct->text ());
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void ForgeWindow::slot_quickColor () {
	QPushButton* qBtn = static_cast<QPushButton*> (sender ());
	color* col = null;
	
	for (quickColorMetaEntry entry : quickColorMeta) {
		if (entry.btn == qBtn) {
			col = entry.col;
			break;
		}
	}
	
	if (col == null)
		return;
	
	std::vector<ulong> ulaIndices;
	std::vector<short> daColors;
	short dNewColor = col->index ();
	
	for (LDObject* obj : paSelection) {
		if (obj->dColor == -1)
			continue; // uncolored object
		
		ulaIndices.push_back (obj->getIndex (g_CurrentFile));
		daColors.push_back (obj->dColor);
		
		obj->dColor = dNewColor;
	}
	
	History::addEntry (new SetColorHistory (ulaIndices, daColors, dNewColor));
	refresh ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
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

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void ForgeWindow::refresh () {
	printf ("refreshing.. (%lu)\n", paSelection.size ());
	buildObjList ();
	R->hardRefresh ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
std::vector<LDObject*> ForgeWindow::getSelectedObjects () {
	std::vector<LDObject*> objs;
	
	if (g_CurrentFile == nullptr)
		return objs;
	
	QList<QTreeWidgetItem*> const qaItems = qObjList->selectedItems ();
	
	for (LDObject* obj : g_CurrentFile->objects)
	for (QTreeWidgetItem* qItem : qaItems) {
		if (qItem == obj->qObjListEntry) {
			objs.push_back (obj);
			break;
		}
	}
	
	return objs;
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
void ForgeWindow::updateSelection () {
	g_bSelectionLocked = true;
	
	for (LDObject* obj : paSelection)
		obj->qObjListEntry->setSelected (true);
	
	printf ("updateSelection: %lu objects selected\n", paSelection.size ());
	
	g_bSelectionLocked = false;
	slot_selectionChanged ();
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
bool ForgeWindow::isSelected (LDObject* obj) {
	LDObject* pNeedle = obj->topLevelParent ();
	
	if (pNeedle == null)
		pNeedle = obj;
	
	for (LDObject* pHay : paSelection)
		if (pHay == pNeedle)
			return true;
	return false;
}

short ForgeWindow::getSelectedColor() {
	short dResult = -1;
	
	for (LDObject* obj : paSelection) {
		if (obj->dColor == -1)
			continue; // doesn't use color
		
		if (dResult != -1 && obj->dColor != dResult)
			return -1; // No consensus in object color
		
		if (dResult == -1)
			dResult = obj->dColor;
	}
	
	return dResult;
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
LDObjectType_e ForgeWindow::getSelectedType () {
	LDObjectType_e eResult = OBJ_Unidentified;
	
	for (LDObject* obj : paSelection) {
		if (eResult != OBJ_Unidentified && obj->dColor != eResult)
			return OBJ_Unidentified;
		
		if (eResult == OBJ_Unidentified)
			eResult = obj->getType ();
	}
	
	return eResult;
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
std::vector<LDObject*>& ForgeWindow::selection () {
	return paSelection;
}

// ========================================================================= //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// ========================================================================= //
QIcon getIcon (const char* sIconName) {
	return (QIcon (str::mkfmt ("./icons/%s.png", sIconName)));
}

// ========================================================================= //
bool confirm (str zMessage) {
	return QMessageBox::question (g_ForgeWindow, "Confirm", zMessage,
		(QMessageBox::Yes | QMessageBox::No), QMessageBox::No) == QMessageBox::Yes;
}