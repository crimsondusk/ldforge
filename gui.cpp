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
	findAction ("gridCoarse")->setCheckable (true);
	findAction ("gridMedium")->setCheckable (true);
	findAction ("gridFine")->setCheckable (true);
	
	findAction ("axes")->setCheckable (true);
	findAction ("axes")->setChecked (gl_axes);
	
	// things not implemented yet
	findAction ("help")->setEnabled (false);
	
	History::updateActions ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
QMenu* g_CurrentMenu;

void ForgeWindow::initMenu (const char* name) {
	g_CurrentMenu = menuBar ()->addMenu (tr (name));
}

void ForgeWindow::addMenuAction (const char* name) {
	g_CurrentMenu->addAction (findAction (name));
}


// =============================================================================
void ForgeWindow::createMenus () {
	m_recentFilesMenu = new QMenu (tr ("Open &Recent"));
	m_recentFilesMenu->setIcon (getIcon ("open-recent"));
	updateRecentFilesMenu ();
	
	QMenu*& menu = g_CurrentMenu;
	
	// File menu
	initMenu ("&File");
	addMenuAction ("newFile");				// New
	addMenuAction ("open");				// Open
	menu->addMenu (m_recentFilesMenu);	// Open Recent
	addMenuAction ("save");				// Save
	addMenuAction ("saveAs");				// Save As
	menu->addSeparator ();					// -------
	addMenuAction ("settings");			// Settings
	menu->addSeparator ();					// -------
	addMenuAction ("exit");				// Exit
	
	// View menu
	initMenu ("&View");
	addMenuAction ("resetView");			// Reset View
	addMenuAction ("axes");				// Draw Axes
	menu->addSeparator ();					// -----
	addMenuAction ("screencap");			// Screencap Part
	addMenuAction ("showHistory");		// Edit History
	
	// Insert menu
	initMenu ("&Insert");
	addMenuAction ("insertFrom");			// Insert from File
	addMenuAction ("insertRaw");			// Insert Raw
	menu->addSeparator ();					// -------
	addMenuAction ("newSubfile");			// New Subfile
	addMenuAction ("newLine");			// New Line
	addMenuAction ("newTriangle");		// New Triangle
	addMenuAction ("newQuad");				// New Quad
	addMenuAction ("newCondLine");		// New Conditional Line
	addMenuAction ("newComment");			// New Comment
	addMenuAction ("newBFC");				// New BFC Statment
	addMenuAction ("newVertex");			// New Vertex
	addMenuAction ("newRadial");			// New Radial
	menu->addSeparator ();					// -----
	addMenuAction ("beginDraw");			// Begin Drawing
	addMenuAction ("doneDraw");			// Cancel Drawing
	addMenuAction ("cancelDraw");			// Done Drawing
	
	// Edit menu
	initMenu ("&Edit");
	addMenuAction ("undo");				// Undo
	addMenuAction ("redo");				// Redo
	menu->addSeparator ();					// -----
	addMenuAction ("cut");					// Cut
	addMenuAction ("copy");				// Copy
	addMenuAction ("paste");				// Paste
	addMenuAction ("del");					// Delete
	menu->addSeparator ();					// -----
	addMenuAction ("selectAll");			// Select All
	addMenuAction ("selectByColor");		// Select by Color
	addMenuAction ("selectByType");		// Select by Type
	
	initMenu ("&Tools");
	addMenuAction ("setColor");			// Set Color
	addMenuAction ("invert");				// Invert
	addMenuAction ("inlineContents");		// Inline
	addMenuAction ("deepInline");			// Deep Inline
	addMenuAction ("splitQuads");			// Split Quads
	addMenuAction ("setContents");		// Set Contents
	addMenuAction ("makeBorders");		// Make Borders
	addMenuAction ("makeCornerVerts");	// Make Corner Vertices
	addMenuAction ("roundCoords");		// Round Coordinates
	addMenuAction ("uncolorize");			// Uncolorize
	
	// Move menu
	initMenu ("&Move");
	addMenuAction ("moveUp");				// Move Up
	addMenuAction ("moveDown");			// Move Down
	menu->addSeparator ();					// -----
	addMenuAction ("gridCoarse");			// Coarse Grid
	addMenuAction ("gridMedium");			// Medium Grid
	addMenuAction ("gridFine");			// Fine Grid
	menu->addSeparator ();					// -----
	addMenuAction ("moveXPos");			// Move +X
	addMenuAction ("moveXNeg");			// Move -X
	addMenuAction ("moveYPos");			// Move +Y
	addMenuAction ("moveYNeg");			// Move -Y
	addMenuAction ("moveZPos");			// Move +Z
	addMenuAction ("moveZNeg");			// Move -Z
	menu->addSeparator ();					// -----
	addMenuAction ("rotateXPos");			// Rotate +X
	addMenuAction ("rotateXNeg");			// Rotate -X
	addMenuAction ("rotateYPos");			// Rotate +Y
	addMenuAction ("rotateYNeg");			// Rotate -Y
	addMenuAction ("rotateZPos");			// Rotate +Z
	addMenuAction ("rotateZNeg");			// Rotate -Z
	
	initMenu ("E&xternal Programs");
	addMenuAction ("ytruder");
	
#ifndef RELEASE
	// Debug menu
	initMenu ("&Debug");
	addMenuAction ("addTestQuad");		// Add Test Quad
	addMenuAction ("addTestRadial");		// Add Test Radial
#endif // RELEASE
	
	// Help menu
	initMenu ("&Help");
	addMenuAction ("help");				// Help
	menu->addSeparator ();					// -----
	addMenuAction ("about");				// About
	addMenuAction ("aboutQt");				// About Qt
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::updateRecentFilesMenu () {
	// First, clear any items in the recent files menu
	for (QAction* recent : m_recentFiles)
		delete recent;
	m_recentFiles.clear ();
	
	std::vector<str> files = io_recentfiles.value / "@";
	for (long i = files.size() - 1; i >= 0; --i) {
		str file = files[i];
		
		QAction* recent = new QAction (getIcon ("open-recent"), file, this);
		
		connect (recent, SIGNAL (triggered ()), this, SLOT (slot_recentFile ()));
		m_recentFilesMenu->addAction (recent);
		m_recentFiles.push_back (recent);
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static QToolBar* g_CurrentToolBar;
static Qt::ToolBarArea g_ToolBarArea = Qt::TopToolBarArea;

void ForgeWindow::initSingleToolBar (const char* name) {
	QToolBar* toolbar = new QToolBar (name);
	addToolBar (g_ToolBarArea, toolbar);
	m_toolBars.push_back (toolbar);
	
	g_CurrentToolBar = toolbar;
}

// =============================================================================
void ForgeWindow::addToolBarAction (const char* name) {
	g_CurrentToolBar->addAction (findAction (name));
}

// =============================================================================
void ForgeWindow::createToolbars () {
	initSingleToolBar ("File");
	addToolBarAction ("newFile");
	addToolBarAction ("open");
	addToolBarAction ("save");
	addToolBarAction ("saveAs");
	
	// ==========================================
	initSingleToolBar ("Insert");
	addToolBarAction ("newSubfile");
	addToolBarAction ("newLine");
	addToolBarAction ("newTriangle");
	addToolBarAction ("newQuad");
	addToolBarAction ("newCondLine");
	addToolBarAction ("newComment");
	addToolBarAction ("newBFC");
	addToolBarAction ("newVertex");
	addToolBarAction ("newRadial");
	
	// ==========================================
	initSingleToolBar ("Edit");
	addToolBarAction ("undo");
	addToolBarAction ("redo");
	addToolBarAction ("cut");
	addToolBarAction ("copy");
	addToolBarAction ("paste");
	addToolBarAction ("del");
	
	// ==========================================
	initSingleToolBar ("Select");
	addToolBarAction ("selectAll");
	addToolBarAction ("selectByColor");
	addToolBarAction ("selectByType");
	
	addToolBarBreak (Qt::TopToolBarArea);
	
	// ==========================================
	initSingleToolBar ("Move");
	addToolBarAction ("moveUp");
	addToolBarAction ("moveDown");
	addToolBarAction ("moveXPos");
	addToolBarAction ("moveXNeg");
	addToolBarAction ("moveYPos");
	addToolBarAction ("moveYNeg");
	addToolBarAction ("moveZPos");
	addToolBarAction ("moveZNeg");
	
	// ==========================================
	initSingleToolBar ("Rotate");
	addToolBarAction ("rotateXPos");
	addToolBarAction ("rotateXNeg");
	addToolBarAction ("rotateYPos");
	addToolBarAction ("rotateYNeg");
	addToolBarAction ("rotateZPos");
	addToolBarAction ("rotateZNeg");
	
	// ==========================================
	// Grid toolbar
	initSingleToolBar ("Grids");
	addToolBarAction ("gridCoarse");
	addToolBarAction ("gridMedium");
	addToolBarAction ("gridFine");
	addToolBarBreak (Qt::TopToolBarArea);
	
	// ==========================================
	initSingleToolBar ("View");
	addToolBarAction ("axes");
	
	// ==========================================
	// Color toolbar
	m_colorToolBar = new QToolBar ("Quick Colors");
	addToolBar (Qt::RightToolBarArea, m_colorToolBar);
	
	// ==========================================
	// Left area toolbars
	//g_ToolBarArea = Qt::LeftToolBarArea;
	initSingleToolBar ("Tools");
	addToolBarAction ("setColor");
	addToolBarAction ("invert");
	addToolBarAction ("inlineContents");
	addToolBarAction ("deepInline");
	addToolBarAction ("splitQuads");
	addToolBarAction ("setContents");
	addToolBarAction ("makeBorders");
	addToolBarAction ("makeCornerVerts");
	addToolBarAction ("roundCoords");
	addToolBarAction ("screencap");
	addToolBarAction ("uncolorize");
	
	initSingleToolBar ("External Programs");
	addToolBarAction ("ytruder");
	
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
	findAction ("gridCoarse")->setChecked (grid == Grid::Coarse);
	findAction ("gridMedium")->setChecked (grid == Grid::Medium);
	findAction ("gridFine")->setChecked (grid == Grid::Fine);
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
			title += fmt (": %s", comm->text.chars());
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
			zText = static_cast<LDComment*> (obj)->text.chars();
			
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
				zText = LDBFC::statements[bfc->type];
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
		} else if (lv_colorize && obj->isColored () &&
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
	findAction ("setContents")->setEnabled (qObjList->selectedItems().size() == 1);
	
	// If we have no selection, disable splitting quads
	findAction ("splitQuads")->setEnabled (qObjList->selectedItems().size() > 0);
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
		contextMenu->addAction (findAction ("editObject"));
		contextMenu->addSeparator ();
	}
	
	contextMenu->addAction (findAction ("cut"));
	contextMenu->addAction (findAction ("copy"));
	contextMenu->addAction (findAction ("paste"));
	contextMenu->addAction (findAction ("del"));
	contextMenu->addSeparator ();
	contextMenu->addAction (findAction ("setColor"));
	if (single)
		contextMenu->addAction (findAction ("setContents"));
	contextMenu->addAction (findAction ("makeBorders"));
	
	contextMenu->exec (pos);
}

// =============================================================================
void ObjectList::contextMenuEvent (QContextMenuEvent* ev) {
	g_win->spawnContextMenu (ev->globalPos ());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
QPixmap getIcon (const char* iconName) {
	return (QPixmap (fmt (":/icons/%s.png", iconName)));
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

// =============================================================================
QAction* findAction (str name) {
	for (actionmeta& meta : g_ActionMeta)
		if (name == meta.name)
			return *meta.qAct;
	
	return null;
}