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
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qdialogbuttonbox.h>
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
cfg (str, gui_colortoolbar, "16:24:|:1:2:4:14:0:15:|:33:34:36:46");
extern_cfg (str, io_recentfiles);
extern_cfg (bool, gl_axes);
extern_cfg (str, gl_maincolor);
extern_cfg (float, gl_maincolor_alpha);
extern_cfg (bool, gl_wireframe);

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
	
	m_splitter = new QSplitter;
	m_splitter->addWidget (m_renderer);
	m_splitter->addWidget (m_objList);
	setCentralWidget (m_splitter);
	
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
	
	// Make certain actions checkable
	findAction ("gridCoarse")->setCheckable (true);
	findAction ("gridMedium")->setCheckable (true);
	findAction ("gridFine")->setCheckable (true);
	
	findAction ("axes")->setCheckable (true);
	findAction ("axes")->setChecked (gl_axes);
	
	findAction ("wireframe")->setCheckable (true);
	findAction ("wireframe")->setChecked (gl_wireframe);
	
	// things not implemented yet
	findAction ("help")->setEnabled (false);
	
	History::updateActions ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
QMenu* g_CurrentMenu;

QMenu* ForgeWindow::initMenu (const char* name) {
	return g_CurrentMenu = menuBar ()->addMenu (tr (name));
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
	addMenuAction ("setLDrawPath");		// Set LDraw Path
	menu->addSeparator ();					// -------
	addMenuAction ("exit");				// Exit
	
	// View menu
	initMenu ("&View");
	addMenuAction ("resetView");			// Reset View
	addMenuAction ("axes");				// Draw Axes
	addMenuAction ("wireframe");			// Wireframe
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
	addMenuAction ("radialResolution");	// Radial Resolution
	addMenuAction ("splitQuads");			// Split Quads
	addMenuAction ("setContents");		// Set Contents
	addMenuAction ("makeBorders");		// Make Borders
	addMenuAction ("makeCornerVerts");	// Make Corner Vertices
	addMenuAction ("roundCoords");		// Round Coordinates
	addMenuAction ("uncolorize");			// Uncolorize
	addMenuAction ("visibility");			// Toggle Visibility
	addMenuAction ("replaceCoords");		// Replace Coordinates
	addMenuAction ("flip");				// Flip
	
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
	addMenuAction ("rectifier");
	addMenuAction ("intersector");
	
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
	initSingleToolBar ("Grids");
	addToolBarAction ("gridCoarse");
	addToolBarAction ("gridMedium");
	addToolBarAction ("gridFine");
	addToolBarBreak (Qt::TopToolBarArea);
	
	// ==========================================
	initSingleToolBar ("View");
	addToolBarAction ("axes");
	addToolBarAction ("wireframe");
	
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
	addToolBarAction ("radialResolution");
	addToolBarAction ("splitQuads");
	addToolBarAction ("setContents");
	addToolBarAction ("makeBorders");
	addToolBarAction ("makeCornerVerts");
	addToolBarAction ("roundCoords");
	addToolBarAction ("screencap");
	addToolBarAction ("uncolorize");
	addToolBarAction ("visibility");
	addToolBarAction ("replaceCoords");
	addToolBarAction ("flip");
	
	initSingleToolBar ("External Programs");
	addToolBarAction ("ytruder");
	addToolBarAction ("rectifier");
	addToolBarAction ("intersector");
	updateToolBars ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
std::vector<quickColorMetaEntry> parseQuickColorMeta () {
	std::vector<quickColorMetaEntry> meta;
	
	for (str colorname : gui_colortoolbar.value / ":") {
		if (colorname == "|") {
			meta.push_back ({null, null, true});
		} else {
			color* col = getColor (atoi (colorname));
			assert (col != null);
			meta.push_back ({col, null, false});
		}
	}
	
	return meta;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::updateToolBars () {
	const QSize iconsize (gui_toolbar_iconsize, gui_toolbar_iconsize);
	
	for (QToolBar* bar : m_toolBars)
		bar->setIconSize (iconsize);
	
	// Update the quick color toolbar.
	for (QToolButton* btn : m_colorButtons)
		delete btn;
	
	m_colorButtons.clear ();
	
	// Clear the toolbar to remove separators
	m_colorToolBar->clear ();
	
	for (quickColorMetaEntry& entry : m_colorMeta) {
		if (entry.bSeparator)
			m_colorToolBar->addSeparator ();
		else {
			QToolButton* colorButton = new QToolButton;
			colorButton->setIcon (makeColorIcon (entry.col, gui_toolbar_iconsize));
			colorButton->setIconSize (iconsize);
			colorButton->setToolTip (entry.col->name);
			
			connect (colorButton, SIGNAL (clicked ()), this, SLOT (slot_quickColor ()));
			m_colorToolBar->addWidget (colorButton);
			m_colorButtons.push_back (colorButton);
			
			entry.btn = colorButton;
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
			g_curfile->m_objs[0]->getType() == LDObject::Comment)
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
	
	fullRefresh ();
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
	
	for (int i = 0; i < m_objList->count (); ++i)
		delete m_objList->item (i);
	
	m_objList->clear ();
	
	for (LDObject* obj : g_curfile->m_objs) {
		str descr;
		
		switch (obj->getType ()) {
		case LDObject::Comment:
			descr = static_cast<LDComment*> (obj)->text.chars();
			
			// Remove leading whitespace
			while (~descr && descr[0] == ' ')
				descr -= -1;
			break;
		
		case LDObject::Empty:
			break; // leave it empty
		
		case LDObject::Line:
		case LDObject::Triangle:
		case LDObject::Quad:
		case LDObject::CondLine:
			for (short i = 0; i < obj->vertices (); ++i) {
				if (i != 0)
					descr += ", ";
				
				descr += obj->coords[i].stringRep (true).chars();
			}
			break;
		
		case LDObject::Gibberish:
			descr.format ("ERROR: %s",
				static_cast<LDGibberish*> (obj)->contents.chars());
			break;
		
		case LDObject::Vertex:
			descr.format ("%s", static_cast<LDVertex*> (obj)->pos.stringRep (true).chars());
			break;
		
		case LDObject::Subfile:
			{
				LDSubfile* ref = static_cast<LDSubfile*> (obj);
				
				descr.format ("%s %s, (",
					ref->fileName.chars(), ref->pos.stringRep (true).chars());
				
				for (short i = 0; i < 9; ++i)
					descr += fmt ("%s%s",
						ftoa (ref->transform[i]).chars(),
						(i != 8) ? " " : "");
				
				descr += ')';
			}
			break;
		
		case LDObject::BFC:
			descr = LDBFC::statements[static_cast<LDBFC*> (obj)->type];
		break;
		
		case LDObject::Radial:
			{
				LDRadial* pRad = static_cast<LDRadial*> (obj);
				descr.format ("%d / %d %s", pRad->segs, pRad->divs, pRad->radialTypeName());
				
				if (pRad->radType == LDRadial::Ring || pRad->radType == LDRadial::Cone)
					descr += fmt (" %d", pRad->ringNum);
				
				descr += fmt (" %s", pRad->pos.stringRep (true).chars ());
			}
			break;
		
		default:
			descr = g_saObjTypeNames[obj->getType ()];
			break;
		}
		
		// Put it into brackets if it's hidden
		if (obj->hidden ()) {
			str copy = descr.chars ();
			descr.format ("[[ %s ]]", copy.chars ());
		}
		
		QListWidgetItem* item = new QListWidgetItem (descr.chars());
		item->setIcon (getIcon (g_saObjTypeIcons[obj->getType ()]));
		
		// Color gibberish orange on red so it stands out.
		if (obj->getType() == LDObject::Gibberish) {
			item->setBackground (QColor ("#AA0000"));
			item->setForeground (QColor ("#FFAA00"));
		} else if (lv_colorize && obj->isColored () &&
			obj->color != maincolor && obj->color != edgecolor)
		{
			// If the object isn't in the main or edge color, draw this
			// list entry in said color.
			color* col = getColor (obj->color);
			if (col)
				item->setForeground (col->faceColor);
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
		m_renderer->compileObject (obj);
	
	for (LDObject* obj : priorSelection)
		m_renderer->compileObject (obj);
	
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
	QToolButton* button = static_cast<QToolButton*> (sender ());
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
	short newColor = col->index;
	
	for (LDObject* obj : m_sel) {
		if (obj->color == -1)
			continue; // uncolored object
		
		indices.push_back (obj->getIndex (g_curfile));
		colors.push_back (obj->color);
		
		obj->color = newColor;
	}
	
	History::addEntry (new SetColorHistory (indices, colors, newColor));
	fullRefresh ();
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
void ForgeWindow::fullRefresh () {
	buildObjList ();
	m_renderer->hardRefresh ();
}

void ForgeWindow::update() {
	buildObjList ();
	m_renderer->update ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::updateSelection () {
	g_bSelectionLocked = true;
	
	for (LDObject* obj : g_curfile->m_objs)
		obj->setSelected (false);
	
	m_objList->clearSelection ();
	for (LDObject* obj : m_sel) {
		obj->qObjListEntry->setSelected (true);
		obj->setSelected (true);
	}
	
	g_bSelectionLocked = false;
	slot_selectionChanged ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool ForgeWindow::isSelected (LDObject* obj) {
	LDObject* needle = obj->topLevelParent ();
	
	for (LDObject* hay : m_sel)
		if (hay == needle)
			return true;
	
	return false;
}

short ForgeWindow::getSelectedColor() {
	short result = -1;
	
	for (LDObject* obj : m_sel) {
		if (obj->color == -1)
			continue; // doesn't use color
		
		if (result != -1 && obj->color != result)
			return -1; // No consensus in object color
		
		if (result == -1)
			result = obj->color;
	}
	
	return result;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
LDObject::Type ForgeWindow::uniformSelectedType () {
	LDObject::Type eResult = LDObject::Unidentified;
	
	for (LDObject* obj : m_sel) {
		if (eResult != LDObject::Unidentified && obj->color != eResult)
			return LDObject::Unidentified;
		
		if (eResult == LDObject::Unidentified)
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
	LDObject* singleObj = (single) ? g_win->sel ()[0] : null;
	
	QMenu* contextMenu = new QMenu;
	
	if (single && singleObj->getType () != LDObject::Empty) {
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

// ========================================================================================================================================
DelHistory* ForgeWindow::deleteObjVector (const std::vector<LDObject*> objs) {
	vector<ulong> indices;
	vector<LDObject*> cache;
	
	for (LDObject* obj : objs) {
		indices.push_back (obj->getIndex (g_curfile));
		cache.push_back (obj->clone ());
		
		g_curfile->forgetObject (obj);
		delete obj;
	}
	
	if (indices.size () > 0)
		return new DelHistory (indices, cache);
	
	return null;
}

// ========================================================================================================================================
DelHistory* ForgeWindow::deleteSelection () {
	return deleteObjVector (sel ());
}

// ========================================================================================================================================
DelHistory* ForgeWindow::deleteByColor (const short colnum) {
	vector<LDObject*> objs;
	for (LDObject* obj : g_curfile->m_objs) {
		if (!obj->isColored () || obj->color != colnum)
			continue;
		
		objs.push_back (obj);
	}
	
	return deleteObjVector (objs);
}

// ========================================================================================================================================
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
	QMessageBox::critical (g_win, "Error", msg,
		(QMessageBox::Close), QMessageBox::Close);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// Print to message log
// TODO: I don't think that the message log being a widget in the window
// is a very good idea... maybe this should log into the renderer? Or into
// another dialog that can be popped up?
void ForgeWindow::logVA (LogType type, const char* fmtstr, va_list va) {
	(void) type;
	(void) fmtstr;
	(void) va;
}

// =============================================================================
QAction* findAction (str name) {
	for (actionmeta& meta : g_ActionMeta)
		if (name == meta.name)
			return *meta.qAct;
	
	fprintf (stderr, "%s: couldn't find action named `%s'!\n", __func__, name.chars ());
	assert (false);
	return null;
}

// =============================================================================
QIcon makeColorIcon (color* colinfo, const ushort size) {
	// Create an image object and link a painter to it.
	QImage img (size, size, QImage::Format_ARGB32);
	QPainter paint (&img);
	
	QColor col = colinfo->faceColor;
	if (colinfo->index == maincolor) {
		// Use the user preferences for main color here
		col = gl_maincolor.value.chars ();
		col.setAlpha (gl_maincolor_alpha * 255.0f);
	}
	
	// Paint the icon
	paint.fillRect (QRect (0, 0, size, size), Qt::black);
	paint.drawPixmap (QRect (1, 1, size - 2, size - 2), getIcon ("checkerboard"), QRect (0, 0, 8, 8));
	paint.fillRect (QRect (1, 1, size - 2, size - 2), col);
	return QIcon (QPixmap::fromImage (img));
}

// =============================================================================
void makeColorSelector (QComboBox* box) {
	std::map<short, ulong> counts;
	
	for (LDObject* obj : g_curfile->m_objs) {
		if (!obj->isColored ())
			continue;
		
		if (counts.find (obj->color) == counts.end ())
			counts[obj->color] = 1;
		else
			counts[obj->color]++;
	}
	
	box->clear ();
	ulong row = 0;
	for (const auto& pair : counts) {
		color* col = getColor (pair.first);
		assert (col != null);
		
		QIcon ico = makeColorIcon (col, 16);
		box->addItem (ico, fmt ("[%d] %s (%lu object%s)",
			pair.first, col->name.chars (), pair.second, PLURAL (pair.second)));
		box->setItemData (row, pair.first);
		
		++row;
	}
}

// ========================================================================================================================================
QDialogButtonBox* makeButtonBox (QDialog& dlg) {
	QDialogButtonBox* bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	QWidget::connect (bbx_buttons, SIGNAL (accepted ()), &dlg, SLOT (accept ()));
	QWidget::connect (bbx_buttons, SIGNAL (rejected ()), &dlg, SLOT (reject ()));
	return bbx_buttons;
}