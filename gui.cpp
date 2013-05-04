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
#include <qsplitter.h>
#include <qlistwidget.h>
#include <qcoreapplication.h>
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
EXTERN_ACTION (newComment)
EXTERN_ACTION (newBFC)
EXTERN_ACTION (newVertex)
EXTERN_ACTION (newRadial)
EXTERN_ACTION (help)
EXTERN_ACTION (about)
EXTERN_ACTION (aboutQt)
EXTERN_ACTION (undo)
EXTERN_ACTION (redo)
EXTERN_ACTION (showHistory)
EXTERN_ACTION (selectAll)
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
EXTERN_ACTION (insertFrom)
EXTERN_ACTION (insertRaw)
EXTERN_ACTION (screencap)
EXTERN_ACTION (editObject)
EXTERN_ACTION (uncolorize)
EXTERN_ACTION (axes)

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
extern_cfg (bool, gl_axes);

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ForgeWindow::ForgeWindow () {
	g_win = this;
	m_renderer = new GLRenderer;
	
	m_objList = new ObjectList;
	m_objList->setSelectionMode (QListWidget::ExtendedSelection);
	m_objList->setAlternatingRowColors (true);
	connect (m_objList, SIGNAL (itemSelectionChanged ()), this, SLOT (slot_selectionChanged ()));
	
	m_msglog = new QTextEdit;
	m_msglog->setReadOnly (true);
	m_msglog->setMaximumHeight (96);
	
	m_hsplit = new QSplitter;
	m_hsplit->addWidget (m_renderer);
	m_hsplit->addWidget (m_objList);
	
	m_vsplit = new QSplitter (Qt::Vertical);
	m_vsplit->addWidget (m_hsplit);
	m_vsplit->addWidget (m_msglog);
	
	setCentralWidget (m_vsplit);
	
	m_colorMeta = parseQuickColorMeta ();
	
	createMenuActions ();
	createMenus ();
	createToolbars ();
	
	slot_selectionChanged ();
	
	setStatusBar (new QStatusBar);
	setWindowIcon (getIcon ("ldforge"));
	setTitle ();
	setMinimumSize (320, 200);
	resize (800, 600);
	
	connect (QCoreApplication::instance (), SIGNAL (aboutToQuit ()), this, SLOT (slot_lastSecondCleanup ()));
}

// =============================================================================
void ForgeWindow::slot_lastSecondCleanup () {
	m_renderer->setParent (null);
	delete m_renderer;
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
	
	// Grid actions and axes are checkable
	ACTION (gridCoarse)->setCheckable (true);
	ACTION (gridMedium)->setCheckable (true);
	ACTION (gridFine)->setCheckable (true);
	
	ACTION (axes)->setCheckable (true);
	ACTION (axes)->setChecked (gl_axes);
	
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
	m_recentFilesMenu = new QMenu (tr ("Open &Recent"));
	m_recentFilesMenu->setIcon (getIcon ("open-recent"));
	updateRecentFilesMenu ();
	
	// File menu
	QMenu* qFileMenu = menuBar ()->addMenu (tr ("&File"));
	qFileMenu->addAction (ACTION (newFile));				// New
	qFileMenu->addAction (ACTION (open));				// Open
	qFileMenu->addMenu (m_recentFilesMenu);				// Open Recent
	qFileMenu->addAction (ACTION (save));					// Save
	qFileMenu->addAction (ACTION (saveAs));				// Save As
	qFileMenu->addSeparator ();							// -------
	qFileMenu->addAction (ACTION (settings));				// Settings
	qFileMenu->addSeparator ();							// -------
	qFileMenu->addAction (ACTION (exit));					// Exit
	
	// View menu
	QMenu* qViewMenu = menuBar ()->addMenu (tr ("&View"));
	qViewMenu->addAction (ACTION (resetView));			// Reset View
	qViewMenu->addAction (ACTION (axes));					// Draw Axes
	qViewMenu->addSeparator ();							// -----
	qViewMenu->addAction (ACTION (screencap));			// Screencap Part
	qViewMenu->addAction (ACTION (showHistory));			// Edit History
	
	// Insert menu
	QMenu* qInsertMenu = menuBar ()->addMenu (tr ("&Insert"));
	qInsertMenu->addAction (ACTION (insertFrom));		// Insert from File
	qInsertMenu->addAction (ACTION (insertRaw));			// Insert Raw
	qInsertMenu->addSeparator ();							// -------
	qInsertMenu->addAction (ACTION (newSubfile));		// New Subfile
	qInsertMenu->addAction (ACTION (newLine));			// New Line
	qInsertMenu->addAction (ACTION (newTriangle));		// New Triangle
	qInsertMenu->addAction (ACTION (newQuad));			// New Quad
	qInsertMenu->addAction (ACTION (newCondLine));		// New Conditional Line
	qInsertMenu->addAction (ACTION (newComment));		// New Comment
	qInsertMenu->addAction (ACTION (newBFC));			// New BFC Statment
	qInsertMenu->addAction (ACTION (newVertex));			// New Vertex
	qInsertMenu->addAction (ACTION (newRadial));			// New Radial
	
	// Edit menu
	QMenu* qEditMenu = menuBar ()->addMenu (tr ("&Edit"));
	qEditMenu->addAction (ACTION (undo));					// Undo
	qEditMenu->addAction (ACTION (redo));				// Redo
	qEditMenu->addSeparator ();							// -----
	qEditMenu->addAction (ACTION (cut));					// Cut
	qEditMenu->addAction (ACTION (copy));					// Copy
	qEditMenu->addAction (ACTION (paste));				// Paste
	qEditMenu->addAction (ACTION (del));					// Delete
	qEditMenu->addSeparator ();							// -----
	qEditMenu->addAction (ACTION (selectAll));			// Select All
	qEditMenu->addAction (ACTION (selectByColor));		// Select by Color
	qEditMenu->addAction (ACTION (selectByType));		// Select by Type
	qEditMenu->addSeparator ();							// -----
	
	QMenu* toolsMenu = menuBar ()->addMenu (tr ("&Tools"));
	toolsMenu->addAction (ACTION (setColor));			// Set Color
	toolsMenu->addAction (ACTION (invert));				// Invert
	toolsMenu->addAction (ACTION (inlineContents));		// Inline
	toolsMenu->addAction (ACTION (deepInline));			// Deep Inline
	toolsMenu->addAction (ACTION (splitQuads));			// Split Quads
	toolsMenu->addAction (ACTION (setContents));			// Set Contents
	toolsMenu->addAction (ACTION (makeBorders));			// Make Borders
	toolsMenu->addAction (ACTION (makeCornerVerts));		// Make Corner Vertices
	toolsMenu->addAction (ACTION (roundCoords));			// Round Coordinates
	toolsMenu->addAction (ACTION (uncolorize));			// Uncolorize
	
	// Move menu
	QMenu* qMoveMenu = menuBar ()->addMenu (tr ("&Move"));
	qMoveMenu->addAction (ACTION (moveUp));				// Move Up
	qMoveMenu->addAction (ACTION (moveDown));			// Move Down
	qMoveMenu->addSeparator ();							// -----
	qMoveMenu->addAction (ACTION (gridCoarse));			// Coarse Grid
	qMoveMenu->addAction (ACTION (gridMedium));			// Medium Grid
	qMoveMenu->addAction (ACTION (gridFine));			// Fine Grid
	qMoveMenu->addSeparator ();							// -----
	qMoveMenu->addAction (ACTION (moveXPos));			// Move +X
	qMoveMenu->addAction (ACTION (moveXNeg));			// Move -X
	qMoveMenu->addAction (ACTION (moveYPos));			// Move +Y
	qMoveMenu->addAction (ACTION (moveYNeg));			// Move -Y
	qMoveMenu->addAction (ACTION (moveZPos));			// Move +Z
	qMoveMenu->addAction (ACTION (moveZNeg));			// Move -Z
	qMoveMenu->addSeparator ();							// -----
	qMoveMenu->addAction (ACTION (rotateXPos));			// Rotate +X
	qMoveMenu->addAction (ACTION (rotateXNeg));			// Rotate -X
	qMoveMenu->addAction (ACTION (rotateYPos));			// Rotate +Y
	qMoveMenu->addAction (ACTION (rotateYNeg));			// Rotate -Y
	qMoveMenu->addAction (ACTION (rotateZPos));			// Rotate +Z
	qMoveMenu->addAction (ACTION (rotateZNeg));			// Rotate -Z
	
#ifndef RELEASE
	// Debug menu
	QMenu* qDebugMenu = menuBar ()->addMenu (tr ("&Debug"));
	qDebugMenu->addAction (ACTION (addTestQuad));		// Add Test Quad
	qDebugMenu->addAction (ACTION (addTestRadial));		// Add Test Radial
#endif // RELEASE
	
	// Help menu
	QMenu* qHelpMenu = menuBar ()->addMenu (tr ("&Help"));
	qHelpMenu->addAction (ACTION (help));				// Help
	qHelpMenu->addSeparator ();							// -----
	qHelpMenu->addAction (ACTION (about));				// About
	qHelpMenu->addAction (ACTION (aboutQt));				// About Qt
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::updateRecentFilesMenu () {
	// First, clear any items in the recent files menu
	for (QAction* qRecent : m_recentFiles)
		delete qRecent;
	m_recentFiles.clear ();
	
	std::vector<str> zaFiles = io_recentfiles.value / "@";
	for (long i = zaFiles.size() - 1; i >= 0; --i) {
		str zFile = zaFiles[i];
		
		QAction* qRecent = new QAction (getIcon ("open-recent"), zFile, this);
		
		connect (qRecent, SIGNAL (triggered ()), this, SLOT (slot_recentFile ()));
		m_recentFilesMenu->addAction (qRecent);
		m_recentFiles.push_back (qRecent);
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
	m_toolBars.push_back (toolbar);
	
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
	g_CurrentToolBar->addAction (ACTION (newBFC));
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
	g_CurrentToolBar->addAction (ACTION (selectAll));
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
	addToolBarBreak (Qt::TopToolBarArea);
	
	// ==========================================
	initSingleToolBar ("View");
	g_CurrentToolBar->addAction (ACTION (axes));
	
	// ==========================================
	// Color toolbar
	m_colorToolBar = new QToolBar ("Quick Colors");
	addToolBar (Qt::RightToolBarArea, m_colorToolBar);
	
	// ==========================================
	// Left area toolbars
	//g_ToolBarArea = Qt::LeftToolBarArea;
	initSingleToolBar ("Tools");
	g_CurrentToolBar->addAction (ACTION (setColor));
	g_CurrentToolBar->addAction (ACTION (invert));
	g_CurrentToolBar->addAction (ACTION (inlineContents));
	g_CurrentToolBar->addAction (ACTION (deepInline));
	g_CurrentToolBar->addAction (ACTION (splitQuads));
	g_CurrentToolBar->addAction (ACTION (setContents));
	g_CurrentToolBar->addAction (ACTION (makeBorders));
	g_CurrentToolBar->addAction (ACTION (makeCornerVerts));
	g_CurrentToolBar->addAction (ACTION (roundCoords));
	g_CurrentToolBar->addAction (ACTION (screencap));
	g_CurrentToolBar->addAction (ACTION (uncolorize));
	
	
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
	for (QToolBar* qBar : m_toolBars)
		qBar->setIconSize (QSize (gui_toolbar_iconsize, gui_toolbar_iconsize));
	
	// Update the quick color toolbar.
	for (QPushButton* qButton : m_colorButtons)
		delete qButton;
	
	m_colorButtons.clear ();
	
	// Clear the toolbar to remove separators
	m_colorToolBar->clear ();
	
	for (quickColorMetaEntry& entry : m_colorMeta) {
		if (entry.bSeparator)
			m_colorToolBar->addSeparator ();
		else {
			QPushButton* qColorButton = new QPushButton;
			qColorButton->setAutoFillBackground (true);
			qColorButton->setStyleSheet (fmt ("background-color: %s", entry.col->zColorString.chars()));
			qColorButton->setToolTip (entry.col->zName);
			
			connect (qColorButton, SIGNAL (clicked ()), this, SLOT (slot_quickColor ()));
			m_colorToolBar->addWidget (qColorButton);
			m_colorButtons.push_back (qColorButton);
			
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
	str title = APPNAME " v";
	title += versionString;
	
	// Append our current file if we have one
	if (g_curfile) {
		title += fmt (": %s", basename (g_curfile->m_filename.chars()));
		
		if (g_curfile->m_objs.size() > 0 &&
			g_curfile->m_objs[0]->getType() == OBJ_Comment)
		{
			// Append title
			LDComment* comm = static_cast<LDComment*> (g_curfile->m_objs[0]);
			title += fmt (": %s", comm->zText.chars());
		}
		
		if (History::pos () != g_curfile->savePos)
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
	if (m_sel.size () == 0)
		return;
	
	std::vector<LDObject*> selCopy = m_sel;
	
	// Delete the objects that were being selected
	for (LDObject* obj : selCopy) {
		if (papObjects && ulapIndices) {
			papObjects->push_back (obj->clone ());
			ulapIndices->push_back (obj->getIndex (g_curfile));
		}
		
		g_curfile->forgetObject (obj);
		delete obj;
	}
	
	refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::buildObjList () {
	if (!g_curfile)
		return;
	
	// Lock the selection while we do this so that refreshing the object list
	// doesn't trigger selection updating so that the selection doesn't get lost
	// while this is done.
	g_bSelectionLocked = true;
	
	m_objList->clear ();
	
	for (LDObject* obj : g_curfile->m_objs) {
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
				zText.format ("%d / %d %s", pRad->dSegments, pRad->dDivisions, pRad->radialTypeName());
				
				if (pRad->eRadialType == LDRadial::Ring || pRad->eRadialType == LDRadial::Cone)
					zText.appendformat (" %d", pRad->dRingNum);
				
				zText.appendformat (" %s", pRad->vPosition.stringRep (true).chars ());
			}
			break;
		
		default:
			zText = g_saObjTypeNames[obj->getType ()];
			break;
		}
		
		QListWidgetItem* item = new QListWidgetItem (zText.chars());
		item->setIcon (getIcon (g_saObjTypeIcons[obj->getType ()]));
		
		// Color gibberish orange on red so it stands out.
		if (obj->getType() == OBJ_Gibberish) {
			item->setBackground (QColor ("#AA0000"));
			item->setForeground (QColor ("#FFAA00"));
		} else if (lv_colorize && obj->dColor != -1 &&
			obj->dColor != maincolor && obj->dColor != edgecolor)
		{
			// If the object isn't in the main or edge color, draw this
			// list entry in said color.
			color* col = getColor (obj->dColor);
			if (col)
				item->setForeground (col->qColor);
		}
		
		obj->qObjListEntry = item;
		m_objList->insertItem (m_objList->count (), item);
	}
	
	g_bSelectionLocked = false;
	updateSelection ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::scrollToSelection () {
	if (m_sel.size() == 0)
		return;
	
	LDObject* obj = m_sel[m_sel.size () - 1];
	m_objList->scrollToItem (obj->qObjListEntry);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::slot_selectionChanged () {
	if (g_bSelectionLocked == true || g_curfile == null)
		return;
	
	/*
	// If the selection isn't 1 exact, disable setting contents
	ACTION (setContents)->setEnabled (qObjList->selectedItems().size() == 1);
	
	// If we have no selection, disable splitting quads
	ACTION (splitQuads)->setEnabled (qObjList->selectedItems().size() > 0);
	*/
	
	// Update the shared selection array, though don't do this if this was
	// called during GL picking, in which case the GL renderer takes care
	// of the selection.
	if (m_renderer->picking ())
		return;
	
	std::vector<LDObject*> priorSelection = m_sel;
	
	// Get the objects from the object list selection
	m_sel.clear ();	
	const QList<QListWidgetItem*> items = m_objList->selectedItems ();
	
	for (LDObject* obj : g_curfile->m_objs)
	for (QListWidgetItem* qItem : items) {
		if (qItem == obj->qObjListEntry) {
			m_sel.push_back (obj);
			break;
		}
	}
	
	// Update the GL renderer
	for (LDObject* obj : m_sel)
		m_renderer->recompileObject (obj);
	
	for (LDObject* obj : priorSelection)
		m_renderer->recompileObject (obj);
	
	m_renderer->updateSelFlash ();
	m_renderer->refresh ();
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
	
	for (quickColorMetaEntry entry : m_colorMeta) {
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
	
	for (LDObject* obj : m_sel) {
		if (obj->dColor == -1)
			continue; // uncolored object
		
		indices.push_back (obj->getIndex (g_curfile));
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
	if (m_sel.size () > 0) {
		// If we have a selection, put the item after it.
		return (m_sel[m_sel.size() - 1]->getIndex (g_curfile)) + 1;
	}
	
	// Otherwise place the object at the end.
	return g_curfile->m_objs.size();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::refresh () {
	buildObjList ();
	m_renderer->hardRefresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::updateSelection () {
	g_bSelectionLocked = true;
	
	m_objList->clearSelection ();
	for (LDObject* obj : m_sel)
		obj->qObjListEntry->setSelected (true);
	
	g_bSelectionLocked = false;
	slot_selectionChanged ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool ForgeWindow::isSelected (LDObject* obj) {
	LDObject* needle = obj->topLevelParent ();
	
	if (needle == null)
		needle = obj;
	
	for (LDObject* hay : m_sel)
		if (hay == needle)
			return true;
	
	return false;
}

short ForgeWindow::getSelectedColor() {
	short result = -1;
	
	for (LDObject* obj : m_sel) {
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
	
	for (LDObject* obj : m_sel) {
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
	for (OpenFile* f : g_loadedFiles) {
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
void ForgeWindow::spawnContextMenu (const QPoint pos) {
	const bool single = (g_win->sel ().size () == 1);
	
	QMenu* contextMenu = new QMenu;
	
	if (single) {
		contextMenu->addAction (ACTION (editObject));
		contextMenu->addSeparator ();
	}
	
	contextMenu->addAction (ACTION (cut));
	contextMenu->addAction (ACTION (copy));
	contextMenu->addAction (ACTION (paste));
	contextMenu->addAction (ACTION (del));
	contextMenu->addSeparator ();
	contextMenu->addAction (ACTION (setColor));
	if (single)
		contextMenu->addAction (ACTION (setContents));
	contextMenu->addAction (ACTION (makeBorders));
	
	contextMenu->exec (pos);
}

// =============================================================================
void ObjectList::contextMenuEvent (QContextMenuEvent* ev) {
	g_win->spawnContextMenu (ev->globalPos ());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
QPixmap getIcon (const char* sIconName) {
	return (QPixmap (fmt (":/icons/%s.png", sIconName)));
}

// =============================================================================
bool confirm (str msg) {
	return confirm ("Confirm", msg);
}

bool confirm (str title, str msg) {
	return QMessageBox::question (g_win, title, msg,
		(QMessageBox::Yes | QMessageBox::No), QMessageBox::No) == QMessageBox::Yes;
}

// =============================================================================
void critical (str msg) {
	QMessageBox::critical (g_win, APPNAME ": Critical Error", msg,
		(QMessageBox::Close), QMessageBox::Close);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// Print to message log
void ForgeWindow::logVA (LogType type, const char* fmtstr, va_list va) {
	return; // FIXME: crashes for some reason o_O
	
	char* buf = vdynformat (fmtstr, va, 128);
	str zText (buf);
	delete[] buf;
	
	// Log it to standard output
	printf ("%s", zText.chars ());
	
	// Replace some things out with HTML entities
	zText.replace ("<", "&lt;");
	zText.replace (">", "&gt;");
	zText.replace ("\n", "<br />");
	
	str& log = m_msglogHTML;
	
	switch (type) {
	case LOG_Normal:
		log.append (zText);
		break;
	
	case LOG_Error:
		log.appendformat ("<span style=\"color: #F8F8F8; background-color: #800\"><b>[ERROR]</b> %s</span>",
			zText.chars());
		break;
	
	case LOG_Info:
		log.appendformat ("<span style=\"color: #0AC\"><b>[INFO]</b> %s</span>",
			zText.chars());
		break;
	
	case LOG_Success:
		log.appendformat ("<span style=\"color: #6A0\"><b>[SUCCESS]</b> %s</span>",
			zText.chars());
		break;
	
	case LOG_Warning:
		log.appendformat ("<span style=\"color: #C50\"><b>[WARNING]</b> %s</span>",
			zText.chars());
		break;
		
	case LOG_Dev:
#ifndef RELEASE
		log.appendformat ("<span style=\"color: #0AC\"><b>[DEV]</b> %s</span>",
			zText.chars());
#endif // RELEASE
		break;
	}
	
	m_msglog->setHtml (log);
}