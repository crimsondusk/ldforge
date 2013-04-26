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

#include <qgridlayout.h>
#include <qmessagebox.h>
#include <qevent.h>
#include <qmenubar.h>
#include <qstatusbar.h>
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
EXTERN_ACTION (invert)
EXTERN_ACTION (rotateXNeg)
EXTERN_ACTION (rotateYNeg)
EXTERN_ACTION (rotateZNeg)
EXTERN_ACTION (rotateXPos)
EXTERN_ACTION (rotateYPos)
EXTERN_ACTION (rotateZPos)
EXTERN_ACTION (roundCoords)
EXTERN_ACTION (gridCoarse)
EXTERN_ACTION (gridMedium)
EXTERN_ACTION (gridFine)
EXTERN_ACTION (resetView)

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

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
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
	
	setStatusBar (new QStatusBar);
	
	slot_selectionChanged ();
	
	setWindowIcon (QIcon ("icons/ldforge.png"));
	setTitle ();
	setMinimumSize (320, 200);
	resize (800, 600);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::createMenuActions () {
	// Create the actions based on stored meta.
	for (actionmeta meta : g_ActionMeta) {
		QAction*& qAct = *meta.qAct;
		qAct = new QAction (getIcon (meta.sIconName), meta.sDisplayName, this);
		qAct->setStatusTip (meta.sDescription);
		qAct->setShortcut (*meta.conf);
		
		connect (qAct, SIGNAL (triggered ()), this, SLOT (slot_action ()));
	}
	
	// Grid actions are checkable
	ACTION (gridCoarse)->setCheckable (true);
	ACTION (gridMedium)->setCheckable (true);
	ACTION (gridFine)->setCheckable (true);
	
	// things not implemented yet
	QAction* const qaDisabledActions[] = {
		ACTION (help),
	};
	
	for (QAction* act : qaDisabledActions)
		act->setEnabled (false);
	
	History::updateActions ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::createMenus () {
	qRecentFilesMenu = new QMenu (tr ("Open &Recent"));
	qRecentFilesMenu->setIcon (getIcon ("open-recent"));
	updateRecentFilesMenu ();
	
	// File menu
	qFileMenu = menuBar ()->addMenu (tr ("&File"));
	qFileMenu->addAction (ACTION (newFile));			// New
	qFileMenu->addAction (ACTION (open));				// Open
	qFileMenu->addMenu (qRecentFilesMenu);					// Open Recent
	qFileMenu->addAction (ACTION (save));				// Save
	qFileMenu->addAction (ACTION (saveAs));			// Save As
	qFileMenu->addSeparator ();								// -------
	qFileMenu->addAction (ACTION (settings));			// Settings
	qFileMenu->addSeparator ();								// -------
	qFileMenu->addAction (ACTION (exit));				// Exit
	
	// View menu
	qViewMenu = menuBar ()->addMenu (tr ("&View"));
	qViewMenu->addAction (ACTION (resetView));
	
	// Insert menu
	qInsertMenu = menuBar ()->addMenu (tr ("&Insert"));
	qInsertMenu->addAction (ACTION (newSubfile));		// New Subfile
	qInsertMenu->addAction (ACTION (newLine));			// New Line
	qInsertMenu->addAction (ACTION (newTriangle));		// New Triangle
	qInsertMenu->addAction (ACTION (newQuad));			// New Quad
	qInsertMenu->addAction (ACTION (newCondLine));		// New Conditional Line
	qInsertMenu->addAction (ACTION (newComment));		// New Comment
	qInsertMenu->addAction (ACTION (newVertex));		// New Vertex
	qInsertMenu->addAction (ACTION (newRadial));		// New Radial
	
	// Edit menu
	qEditMenu = menuBar ()->addMenu (tr ("&Edit"));
	qEditMenu->addAction (ACTION (undo));				// Undo
	qEditMenu->addAction (ACTION (redo));				// Redo
	qEditMenu->addSeparator ();								// -----
	qEditMenu->addAction (ACTION (cut));				// Cut
	qEditMenu->addAction (ACTION (copy));				// Copy
	qEditMenu->addAction (ACTION (paste));				// Paste
	qEditMenu->addAction (ACTION (del));				// Delete
	qEditMenu->addSeparator ();								// -----
	qEditMenu->addAction (ACTION (selectByColor));		// Select by Color
	qEditMenu->addAction (ACTION (selectByType));		// Select by Type
	qEditMenu->addSeparator ();								// -----
	qEditMenu->addAction (ACTION (setColor));			// Set Color
	qEditMenu->addAction (ACTION (invert));			// Invert
	qEditMenu->addAction (ACTION (inlineContents));	// Inline
	qEditMenu->addAction (ACTION (deepInline));		// Deep Inline
	qEditMenu->addAction (ACTION (splitQuads));		// Split Quads
	qEditMenu->addAction (ACTION (setContents));		// Set Contents
	qEditMenu->addAction (ACTION (makeBorders));		// Make Borders
	qEditMenu->addAction (ACTION (makeCornerVerts));	// Make Corner Vertices
	qEditMenu->addAction (ACTION (roundCoords));		// Round Coordinates
	
	// Move menu
	qMoveMenu = menuBar ()->addMenu (tr ("&Move"));
	qMoveMenu->addAction (ACTION (moveUp));			// Move Up
	qMoveMenu->addAction (ACTION (moveDown));			// Move Down
	qMoveMenu->addSeparator ();								// -----
	qMoveMenu->addAction (ACTION (gridCoarse));		// Coarse Grid
	qMoveMenu->addAction (ACTION (gridMedium));		// Medium Grid
	qMoveMenu->addAction (ACTION (gridFine));			// Fine Grid
	qMoveMenu->addSeparator ();								// -----
	qMoveMenu->addAction (ACTION (moveXPos));			// Move +X
	qMoveMenu->addAction (ACTION (moveXNeg));			// Move -X
	qMoveMenu->addAction (ACTION (moveYPos));			// Move +Y
	qMoveMenu->addAction (ACTION (moveYNeg));			// Move -Y
	qMoveMenu->addAction (ACTION (moveZPos));			// Move +Z
	qMoveMenu->addAction (ACTION (moveZNeg));			// Move -Z
	qMoveMenu->addSeparator ();								// -----
	qMoveMenu->addAction (ACTION (rotateXPos));		// Rotate +X
	qMoveMenu->addAction (ACTION (rotateXNeg));		// Rotate -X
	qMoveMenu->addAction (ACTION (rotateYPos));		// Rotate +Y
	qMoveMenu->addAction (ACTION (rotateYNeg));		// Rotate -Y
	qMoveMenu->addAction (ACTION (rotateZPos));		// Rotate +Z
	qMoveMenu->addAction (ACTION (rotateZNeg));		// Rotate -Z
	
	// Control menu
	qControlMenu = menuBar ()->addMenu (tr ("&Control"));
	qControlMenu->addAction (ACTION (showHistory));	// Show History
	
#ifndef RELEASE
	// Debug menu
	qDebugMenu = menuBar ()->addMenu (tr ("&Debug"));
	qDebugMenu->addAction (ACTION (addTestQuad));		// Add Test Quad
	qDebugMenu->addAction (ACTION (addTestRadial));	// Add Test Radial
#endif // RELEASE
	
	// Help menu
	qHelpMenu = menuBar ()->addMenu (tr ("&Help"));
	qHelpMenu->addAction (ACTION (help));				// Help
	qHelpMenu->addSeparator ();								// -----
	qHelpMenu->addAction (ACTION (about));				// About
	qHelpMenu->addAction (ACTION (aboutQt));			// About Qt
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
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

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static QToolBar* g_CurrentToolBar;
static Qt::ToolBarArea g_ToolBarArea = Qt::TopToolBarArea;

void ForgeWindow::initSingleToolBar (const char* sName) {
	QToolBar* toolbar = new QToolBar (sName);
	addToolBar (g_ToolBarArea, toolbar);
	qaToolBars.push_back (toolbar);
	
	g_CurrentToolBar = toolbar;
}

void ForgeWindow::createToolbars () {
	initSingleToolBar ("File");
	g_CurrentToolBar->addAction (ACTION (newFile));
	g_CurrentToolBar->addAction (ACTION (open));
	g_CurrentToolBar->addAction (ACTION (save));
	g_CurrentToolBar->addAction (ACTION (saveAs));
	
	// ==========================================
	initSingleToolBar ("Insert");
	g_CurrentToolBar->addAction (ACTION (newSubfile));
	g_CurrentToolBar->addAction (ACTION (newLine));
	g_CurrentToolBar->addAction (ACTION (newTriangle));
	g_CurrentToolBar->addAction (ACTION (newQuad));
	g_CurrentToolBar->addAction (ACTION (newCondLine));
	g_CurrentToolBar->addAction (ACTION (newComment));
	g_CurrentToolBar->addAction (ACTION (newVertex));
	g_CurrentToolBar->addAction (ACTION (newRadial));
	
	// ==========================================
	initSingleToolBar ("Edit");
	g_CurrentToolBar->addAction (ACTION (undo));
	g_CurrentToolBar->addAction (ACTION (redo));
	g_CurrentToolBar->addAction (ACTION (cut));
	g_CurrentToolBar->addAction (ACTION (copy));
	g_CurrentToolBar->addAction (ACTION (paste));
	g_CurrentToolBar->addAction (ACTION (del));
	
	// ==========================================
	initSingleToolBar ("Select");
	g_CurrentToolBar->addAction (ACTION (selectByColor));
	g_CurrentToolBar->addAction (ACTION (selectByType));
	
	addToolBarBreak (Qt::TopToolBarArea);
	
	// ==========================================
	initSingleToolBar ("Move");
	g_CurrentToolBar->addAction (ACTION (moveUp));
	g_CurrentToolBar->addAction (ACTION (moveDown));
	g_CurrentToolBar->addAction (ACTION (moveXPos));
	g_CurrentToolBar->addAction (ACTION (moveXNeg));
	g_CurrentToolBar->addAction (ACTION (moveYPos));
	g_CurrentToolBar->addAction (ACTION (moveYNeg));
	g_CurrentToolBar->addAction (ACTION (moveZPos));
	g_CurrentToolBar->addAction (ACTION (moveZNeg));
	
	// ==========================================
	initSingleToolBar ("Rotate");
	g_CurrentToolBar->addAction (ACTION (rotateXPos));
	g_CurrentToolBar->addAction (ACTION (rotateXNeg));
	g_CurrentToolBar->addAction (ACTION (rotateYPos));
	g_CurrentToolBar->addAction (ACTION (rotateYNeg));
	g_CurrentToolBar->addAction (ACTION (rotateZPos));
	g_CurrentToolBar->addAction (ACTION (rotateZNeg));
	
	// ==========================================
	// Grid toolbar
	initSingleToolBar ("Grids");
	g_CurrentToolBar->addAction (ACTION (gridCoarse));
	g_CurrentToolBar->addAction (ACTION (gridMedium));
	g_CurrentToolBar->addAction (ACTION (gridFine));
	
	// ==========================================
	// Color toolbar
	qColorToolBar = new QToolBar ("Quick Colors");
	addToolBar (Qt::RightToolBarArea, qColorToolBar);
	
	// ==========================================
	// Left area toolbars
	g_ToolBarArea = Qt::LeftToolBarArea;
	initSingleToolBar ("Objects");
	g_CurrentToolBar->addAction (ACTION (setColor));
	g_CurrentToolBar->addAction (ACTION (invert));
	g_CurrentToolBar->addAction (ACTION (inlineContents));
	g_CurrentToolBar->addAction (ACTION (deepInline));
	g_CurrentToolBar->addAction (ACTION (splitQuads));
	g_CurrentToolBar->addAction (ACTION (setContents));
	g_CurrentToolBar->addAction (ACTION (makeBorders));
	g_CurrentToolBar->addAction (ACTION (makeCornerVerts));
	g_CurrentToolBar->addAction (ACTION (roundCoords));
	
	updateToolBars ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
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

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
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
			qColorButton->setStyleSheet (format ("background-color: %s", entry.col->zColorString.chars()));
			qColorButton->setToolTip (entry.col->zName);
			
			connect (qColorButton, SIGNAL (clicked ()), this, SLOT (slot_quickColor ()));
			qColorToolBar->addWidget (qColorButton);
			qaColorButtons.push_back (qColorButton);
			
			entry.btn = qColorButton;
		}
	}
	
	updateGridToolBar ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::updateGridToolBar () {
	// Ensure that the current grid - and only the current grid - is selected.
	ACTION (gridCoarse)->setChecked (grid == Grid::Coarse);
	ACTION (gridMedium)->setChecked (grid == Grid::Medium);
	ACTION (gridFine)->setChecked (grid == Grid::Fine);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::setTitle () {
	str title = APPNAME_DISPLAY " v" VERSION_STRING;
	
	// Append our current file if we have one
	if (g_CurrentFile) {
		title += format (": %s", basename (g_CurrentFile->zFileName.chars()));
		
		if (g_CurrentFile->objects.size() > 0 &&
			g_CurrentFile->objects[0]->getType() == OBJ_Comment)
		{
			// Append title
			LDComment* comm = static_cast<LDComment*> (g_CurrentFile->objects[0]);
			title += format (": %s", comm->zText.chars());
		}
		
		if (History::pos () != g_CurrentFile->savePos)
			title += '*';
	}
	
	setWindowTitle (title);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
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
		logf (LOG_Warning, "unknown signal sender %p!\n", qAct);
		return;
	}
	
	// We have the meta, now call the handler.
	(*pMeta->handler) ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::deleteSelection (vector<ulong>* ulapIndices, std::vector<LDObject*>* papObjects) {
	if (sel.size () == 0)
		return;
	
	std::vector<LDObject*> selCopy = sel;
	
	// Delete the objects that were being selected
	for (LDObject* obj : selCopy) {
		if (papObjects && ulapIndices) {
			papObjects->push_back (obj->clone ());
			ulapIndices->push_back (obj->getIndex (g_CurrentFile));
		}
		
		g_CurrentFile->forgetObject (obj);
		delete obj;
	}
	
	refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
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
					line->vaCoords[0].stringRep (true).chars(),
					line->vaCoords[1].stringRep (true).chars());
			}
			break;
		
		case OBJ_Triangle:
			{
				LDTriangle* triangle = static_cast<LDTriangle*> (obj);
				zText.format ("%s, %s, %s",
					triangle->vaCoords[0].stringRep (true).chars(),
					triangle->vaCoords[1].stringRep (true).chars(),
					triangle->vaCoords[2].stringRep (true).chars());
			}
			break;
		
		case OBJ_Quad:
			{
				LDQuad* quad = static_cast<LDQuad*> (obj);
				zText.format ("%s, %s, %s, %s",
					quad->vaCoords[0].stringRep (true).chars(),
					quad->vaCoords[1].stringRep (true).chars(),
					quad->vaCoords[2].stringRep (true).chars(),
					quad->vaCoords[3].stringRep (true).chars());
			}
			break;
		
		case OBJ_CondLine:
			{
				LDCondLine* line = static_cast<LDCondLine*> (obj);
				zText.format ("%s, %s, %s, %s",
					line->vaCoords[0].stringRep (true).chars(),
					line->vaCoords[1].stringRep (true).chars(),
					line->vaCoords[2].stringRep (true).chars(),
					line->vaCoords[3].stringRep (true).chars());
			}
			break;
		
		case OBJ_Gibberish:
			zText.format ("ERROR: %s",
				static_cast<LDGibberish*> (obj)->zContents.chars());
			break;
		
		case OBJ_Vertex:
			zText.format ("%s", static_cast<LDVertex*> (obj)->vPosition.stringRep (true).chars());
			break;
		
		case OBJ_Subfile:
			{
				LDSubfile* ref = static_cast<LDSubfile*> (obj);
				
				zText.format ("%s %s, (",
					ref->zFileName.chars(), ref->vPosition.stringRep (true).chars());
				
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
				
				zText.appendformat (" %s", pRad->vPosition.stringRep (true).chars ());
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

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
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
	if (R->picking == false) {
		std::vector<LDObject*> paPriorSelection = sel;
		sel = getSelectedObjects ();
		
		// Update the GL renderer
		for (LDObject* obj : sel)
			R->recompileObject (obj);
		
		for (LDObject* obj : paPriorSelection)
			R->recompileObject (obj);
		
		R->updateSelFlash ();
		R->refresh ();
	}
}

// =============================================================================
void ForgeWindow::slot_recentFile () {
	QAction* qAct = static_cast<QAction*> (sender ());
	openMainFile (qAct->text ());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::slot_quickColor () {
	QPushButton* button = static_cast<QPushButton*> (sender ());
	color* col = null;
	
	for (quickColorMetaEntry entry : quickColorMeta) {
		if (entry.btn == button) {
			col = entry.col;
			break;
		}
	}
	
	if (col == null)
		return;
	
	std::vector<ulong> indices;
	std::vector<short> colors;
	short newColor = col->index ();
	
	for (LDObject* obj : sel) {
		if (obj->dColor == -1)
			continue; // uncolored object
		
		indices.push_back (obj->getIndex (g_CurrentFile));
		colors.push_back (obj->dColor);
		
		obj->dColor = newColor;
	}
	
	History::addEntry (new SetColorHistory (indices, colors, newColor));
	refresh ();
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

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::updateSelection () {
	g_bSelectionLocked = true;
	
	for (LDObject* obj : sel)
		obj->qObjListEntry->setSelected (true);
	
	g_bSelectionLocked = false;
	slot_selectionChanged ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool ForgeWindow::isSelected (LDObject* obj) {
	LDObject* pNeedle = obj->topLevelParent ();
	
	if (pNeedle == null)
		pNeedle = obj;
	
	for (LDObject* pHay : sel)
		if (pHay == pNeedle)
			return true;
	return false;
}

short ForgeWindow::getSelectedColor() {
	short result = -1;
	
	for (LDObject* obj : sel) {
		if (obj->dColor == -1)
			continue; // doesn't use color
		
		if (result != -1 && obj->dColor != result)
			return -1; // No consensus in object color
		
		if (result == -1)
			result = obj->dColor;
	}
	
	return result;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
LDObjectType_e ForgeWindow::uniformSelectedType () {
	LDObjectType_e eResult = OBJ_Unidentified;
	
	for (LDObject* obj : sel) {
		if (eResult != OBJ_Unidentified && obj->dColor != eResult)
			return OBJ_Unidentified;
		
		if (eResult == OBJ_Unidentified)
			eResult = obj->getType ();
	}
	
	return eResult;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::closeEvent (QCloseEvent* ev) {
	// Check whether it's safe to close all files.
	for (OpenFile* f : g_LoadedFiles) {
		if (!f->safeToClose ()) {
			ev->ignore ();
			return;
		}
	}
	
	// Save the configuration before leaving so that, for instance, grid choice
	// is preserved across instances.
	config::save ();
	
	ev->accept ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
QIcon getIcon (const char* sIconName) {
	return (QIcon (format ("./icons/%s.png", sIconName)));
}

// =============================================================================
bool confirm (str msg) {
	return confirm ("Confirm", msg);
}

bool confirm (str title, str msg) {
	return QMessageBox::question (g_ForgeWindow, title, msg,
		(QMessageBox::Yes | QMessageBox::No), QMessageBox::No) == QMessageBox::Yes;
}