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

#include <QGridLayout>
#include <QMessageBox>
#include <QEvent>
#include <QContextMenuEvent>
#include <QMenuBar>
#include <QStatusBar>
#include <QSplitter>
#include <QListWidget>
#include <QToolButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QToolBar>
#include <QProgressBar>
#include <QLabel>
#include <QFileDialog>
#include <QPushButton>
#include <QCoreApplication>
#include <QTimer>

#include "common.h"
#include "gldraw.h"
#include "gui.h"
#include "file.h"
#include "config.h"
#include "misc.h"
#include "colors.h"
#include "history.h"
#include "widgets.h"
#include "addObjectDialog.h"
#include "messagelog.h"
#include "config.h"

actionmeta g_actionMeta[MAX_ACTIONS];
static ushort g_metacursor = 0;

static bool g_bSelectionLocked = false;

cfg (bool, lv_colorize, true);
cfg (int, gui_toolbar_iconsize, 24);
cfg (str, gui_colortoolbar, "16:24:|:1:2:4:14:0:15:|:33:34:36:46");
extern_cfg (str, io_recentfiles);
extern_cfg (bool, gl_axes);
extern_cfg (str, gl_maincolor);
extern_cfg (float, gl_maincolor_alpha);
extern_cfg (bool, gl_wireframe);
extern_cfg (bool, gl_colorbfc);

const char* g_modeActionNames[] = {
	"modeSelect",
	"modeDraw",
};

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
	connect (m_objList, SIGNAL (itemDoubleClicked (QListWidgetItem*)), this, SLOT (slot_editObject (QListWidgetItem*)));
	
	m_msglog = new MessageManager;
	m_msglog->setRenderer( R() );
	m_renderer->setMessageLog( m_msglog );
	
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
	
	m_primLoaderBar = new QProgressBar;
	m_primLoaderWidget = new QWidget;
	QHBoxLayout* primLoaderLayout = new QHBoxLayout (m_primLoaderWidget);
	primLoaderLayout->addWidget (new QLabel ("Loading primitives:"));
	primLoaderLayout->addWidget (m_primLoaderBar);
	statusBar ()->addPermanentWidget (m_primLoaderWidget);
	m_primLoaderWidget->hide ();
	
	setWindowIcon (getIcon ("ldforge"));
	updateTitle ();
	setMinimumSize (320, 200);
	resize (800, 600);
	
	connect (qApp, SIGNAL (aboutToQuit ()), this, SLOT (slot_lastSecondCleanup ()));
}

// =============================================================================
void ForgeWindow::slot_lastSecondCleanup () {
	delete m_renderer;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::createMenuActions () {
	// Create the actions based on stored meta.
	for (actionmeta meta : g_actionMeta) {
		if (meta.qAct == null)
			break;
		
		QAction*& act = *meta.qAct;
		act = new QAction (getIcon (meta.sIconName), meta.sDisplayName, this);
		act->setStatusTip (meta.sDescription);
		act->setShortcut (*meta.conf);
		
		connect (act, SIGNAL (triggered ()), this, SLOT (slot_action ()));
	}
	
	// Make certain actions checkable
	findAction ("gridCoarse")->setCheckable (true);
	findAction ("gridMedium")->setCheckable (true);
	findAction ("gridFine")->setCheckable (true);
	
	findAction ("axes")->setCheckable (true);
	findAction ("axes")->setChecked (gl_axes);

	findAction ("wireframe")->setCheckable (true);
	findAction ("wireframe")->setChecked (gl_wireframe);
	
	findAction ("colorbfc")->setCheckable (true);
	findAction ("colorbfc")->setChecked (gl_colorbfc);
	
	updateEditModeActions ();
	
	// things not implemented yet
	findAction ("help")->setEnabled (false);
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
	addMenuAction ("newFile");
	addMenuAction ("open");
	menu->addMenu (m_recentFilesMenu);
	addMenuAction ("save");
	addMenuAction ("saveAs");
	menu->addSeparator ();
	addMenuAction ("insertFrom");
	addMenuAction ("exportTo");
	menu->addSeparator ();
	addMenuAction ("settings");
	addMenuAction ("setLDrawPath");
	menu->addSeparator ();
#ifndef RELEASE
	addMenuAction ("testpic");
#endif
	addMenuAction ("reloadPrimitives");
	menu->addSeparator ();
	addMenuAction ("exit");
	
	// View menu
	initMenu ("&View");
	addMenuAction ("resetView");
	addMenuAction ("axes");
	addMenuAction ("wireframe");
	addMenuAction ("colorbfc");
	menu->addSeparator ();
	addMenuAction ("setOverlay");
	addMenuAction ("clearOverlay");
	menu->addSeparator ();
	addMenuAction ("screencap");
	
	// Insert menu
	initMenu ("&Insert");
	addMenuAction ("insertRaw");
	menu->addSeparator ();
	addMenuAction ("newSubfile");
	addMenuAction ("newLine");
	addMenuAction ("newTriangle");
	addMenuAction ("newQuad");
	addMenuAction ("newCondLine");
	addMenuAction ("newComment");
	addMenuAction ("newBFC");
	addMenuAction ("newVertex");
	
	// Edit menu
	initMenu ("&Edit");
	addMenuAction ("undo");
	addMenuAction ("redo");
	menu->addSeparator ();
	addMenuAction ("cut");
	addMenuAction ("copy");
	addMenuAction ("paste");
	addMenuAction ("del");
	menu->addSeparator ();
	addMenuAction ("selectAll");
	addMenuAction ("selectByColor");
	addMenuAction ("selectByType");
	menu->addSeparator ();
	addMenuAction ("modeSelect");
	addMenuAction ("modeDraw");
	menu->addSeparator ();
	addMenuAction ("setDrawDepth");
	
	// Move menu
	initMenu ("&Move");
	addMenuAction ("moveUp");
	addMenuAction ("moveDown");
	menu->addSeparator ();
	addMenuAction ("gridCoarse");
	addMenuAction ("gridMedium");
	addMenuAction ("gridFine");
	menu->addSeparator ();
	addMenuAction ("moveXPos");
	addMenuAction ("moveXNeg");
	addMenuAction ("moveYPos");
	addMenuAction ("moveYNeg");
	addMenuAction ("moveZPos");
	addMenuAction ("moveZNeg");
	menu->addSeparator ();
	addMenuAction ("rotateXPos");
	addMenuAction ("rotateXNeg");
	addMenuAction ("rotateYPos");
	addMenuAction ("rotateYNeg");
	addMenuAction ("rotateZPos");
	addMenuAction ("rotateZNeg");
	addMenuAction ("rotpoint");
	
	initMenu ("&Tools");
	addMenuAction ("setColor");
	addMenuAction ("autoColor");
	addMenuAction ("uncolorize");
	menu->addSeparator ();
	addMenuAction ("invert");
	addMenuAction ("inlineContents");
	addMenuAction ("deepInline");
	addMenuAction ("makePrimitive");
	menu->addSeparator ();
	addMenuAction ("splitQuads");
	addMenuAction ("setContents");
	addMenuAction ("makeBorders");
	addMenuAction ("makeCornerVerts");
	addMenuAction ("roundCoords");
	addMenuAction ("visibility");
	addMenuAction ("replaceCoords");
	addMenuAction ("flip");
	addMenuAction ("demote");
	
	initMenu ("E&xternal Programs");
	addMenuAction ("ytruder");
	addMenuAction ("rectifier");
	addMenuAction ("intersector");
	addMenuAction ("isecalc");
	addMenuAction ("coverer");
	addMenuAction ("edger2");
	
	// Help menu
	initMenu ("&Help");
	addMenuAction ("help");
	menu->addSeparator ();
	addMenuAction ("about");
	addMenuAction ("aboutQt");
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::updateRecentFilesMenu () {
	// First, clear any items in the recent files menu
	for (QAction* recent : m_recentFiles)
		delete recent;
	m_recentFiles.clear ();
	
	vector<str> files = container_cast<QStringList, vector<str>> (io_recentfiles.value.split ("@"));
	for (str file : c_rev<str> (files)) {
		QAction* recent = new QAction (getIcon ("open-recent"), file, this);
		
		connect (recent, SIGNAL (triggered ()), this, SLOT (slot_recentFile ()));
		m_recentFilesMenu->addAction (recent);
		m_recentFiles << recent;
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
	m_toolBars << toolbar;
	
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
	
	// ==========================================
	initSingleToolBar ("Edit");
	addToolBarAction ("undo");
	addToolBarAction ("redo");
	addToolBarAction ("cut");
	addToolBarAction ("copy");
	addToolBarAction ("paste");
	addToolBarAction ("del");
	addToolBarBreak (Qt::TopToolBarArea);
	
	// ==========================================
	initSingleToolBar ("Select");
	addToolBarAction ("selectAll");
	addToolBarAction ("selectByColor");
	addToolBarAction ("selectByType");
	
	// ==========================================
	initSingleToolBar ("Grids");
	addToolBarAction ("gridCoarse");
	addToolBarAction ("gridMedium");
	addToolBarAction ("gridFine");
	
	// ==========================================
	initSingleToolBar ("View");
	addToolBarAction ("axes");
	addToolBarAction ("wireframe");
	addToolBarAction ("colorbfc");
	
	// ==========================================
	// Color toolbar
	m_colorToolBar = new QToolBar ("Quick Colors");
	addToolBar (Qt::RightToolBarArea, m_colorToolBar);
	
	// ==========================================
	initSingleToolBar ("Tools");
	addToolBarAction ("setColor");
	addToolBarAction ("autoColor");
	addToolBarAction ("splitQuads");
	addToolBarAction ("setContents");
	addToolBarAction ("makeBorders");
	addToolBarAction ("replaceCoords");
	addToolBarAction ("roundCoords");
	addToolBarAction ("visibility");
	
	// ==========================================
	g_ToolBarArea = Qt::LeftToolBarArea;
	initSingleToolBar ("Modes");
	addToolBarAction ("modeSelect");
	addToolBarAction ("modeDraw");
	
	updateToolBars ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
vector<quickColor> parseQuickColorMeta () {
	vector<quickColor> meta;
	
	for (str colorname : gui_colortoolbar.value.split (":")) {
		if (colorname == "|") {
			meta << quickColor ({null, null, true});
		} else {
			color* col = getColor (colorname.toLong ());
			assert (col != null);
			meta << quickColor ({col, null, false});
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
	
	for (quickColor& entry : m_colorMeta) {
		if (entry.isSeparator)
			m_colorToolBar->addSeparator ();
		else {
			QToolButton* colorButton = new QToolButton;
			colorButton->setIcon (makeColorIcon (entry.col, gui_toolbar_iconsize));
			colorButton->setIconSize (iconsize);
			colorButton->setToolTip (entry.col->name);
			
			connect (colorButton, SIGNAL (clicked ()), this, SLOT (slot_quickColor ()));
			m_colorToolBar->addWidget (colorButton);
			m_colorButtons << colorButton;
			
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
void ForgeWindow::updateTitle () {
	str title = fmt (APPNAME " %1", fullVersionString());
	
	// Append our current file if we have one
	if (g_curfile) {
		if (g_curfile->name ().length () > 0)
			title += fmt (": %1", basename (g_curfile->name ()));
		else
			title += fmt (": <anonymous>");
		
		if (g_curfile->numObjs () > 0 &&
			g_curfile->obj (0)->getType () == LDObject::Comment)
		{
			// Append title
			LDComment* comm = static_cast<LDComment*> (g_curfile->obj (0));
			title += fmt (": %1", comm->text);
		}
		
		if (g_curfile->history ().pos () != g_curfile->savePos ())
			title += '*';
	}
	
	setWindowTitle (title);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
EXTERN_ACTION( undo );
EXTERN_ACTION( redo );
EXTERN_ACTION( open );
void ForgeWindow::slot_action () {
	// Open the history so we can record the edits done during this action.
	if( sender() != ACTION( undo ) && sender() != ACTION( redo ) && sender() != ACTION( open ))
		g_curfile->openHistory ();
	
	// Get the action that triggered this slot.
	QAction* qAct = static_cast<QAction*> (sender ());
	
	// Find the meta for the action.
	actionmeta* meta = null;
	
	for (actionmeta& it : g_actionMeta) {
		if (it.qAct == null)
			break;
		
		if (*it.qAct == qAct) {
			meta = &it;
			break;
		}
	}
	
	if (!meta) {
		log ("Warning: unknown signal sender %p!\n", qAct);
		return;
	}
	
	// We have the meta, now call the handler.
	(*meta->handler) ();
	
	g_curfile->closeHistory ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
int ForgeWindow::deleteSelection()
{
	if( m_sel.size() == 0 )
		return 0;
	
	vector<LDObject*> selCopy = m_sel;
	int num = 0;
	
	// Delete the objects that were being selected
	for( LDObject * obj : selCopy )
	{
		g_curfile->forgetObject( obj );
		++num;
		delete obj;
	}
	
	refresh();
	return num;
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
	
	for (LDObject* obj : g_curfile->objs ()) {
		str descr;
		
		switch (obj->getType ()) {
		case LDObject::Comment:
			descr = static_cast<LDComment*> (obj)->text;
			
			// Remove leading whitespace
			while (descr[0] == ' ')
				descr.remove (0, 1);
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
				
				descr += obj->getVertex (i).stringRep (true);
			}
			break;
		
		case LDObject::Gibberish:
			descr = fmt ("ERROR: %1", static_cast<LDGibberish*> (obj)->contents);
			break;
		
		case LDObject::Vertex:
			descr = static_cast<LDVertex*> (obj)->pos.stringRep (true);
			break;
		
		case LDObject::Subfile:
			{
				LDSubfile* ref = static_cast<LDSubfile*> (obj);
				
				descr = fmt ("%1 %2, (", ref->fileInfo ()->name (),
					ref->position ().stringRep (true));
				
				for (short i = 0; i < 9; ++i)
					descr += fmt ("%1%2", ftoa (ref->transform ()[i]),
						(i != 8) ? " " : "");
				
				descr += ')';
			}
			break;
		
		case LDObject::BFC:
			descr = LDBFC::statements[static_cast<LDBFC*> (obj)->type];
		break;
		
		case LDObject::Overlay:
			{
				LDOverlay* ovl = static_cast<LDOverlay*>( obj );
				descr = fmt( "[%1] %2 (%3, %4), %5 x %6", g_CameraNames[ovl->camera()],
					basename( ovl->filename() ), ovl->x(), ovl->y(), ovl->width(), ovl->height() );
			}
			break;
		
		default:
			descr = g_saObjTypeNames[obj->getType ()];
			break;
		}
		
		// Put it into brackets if it's hidden
		if (obj->hidden ()) {
			descr = fmt ("[[ %1 ]]", descr);
		}
		
		QListWidgetItem* item = new QListWidgetItem (descr);
		item->setIcon (getIcon (g_saObjTypeIcons[obj->getType ()]));
		
		// Color gibberish orange on red so it stands out.
		if (obj->getType() == LDObject::Gibberish) {
			item->setBackground (QColor ("#AA0000"));
			item->setForeground (QColor ("#FFAA00"));
		} elif (lv_colorize && obj->isColored () &&
			obj->color () != maincolor && obj->color () != edgecolor)
		{
			// If the object isn't in the main or edge color, draw this
			// list entry in said color.
			color* col = getColor (obj->color ());
			if (col)
				item->setForeground (col->faceColor);
		}
		
		obj->qObjListEntry = item;
		m_objList->insertItem (m_objList->count (), item);
	}
	
	g_bSelectionLocked = false;
	updateSelection ();
	scrollToSelection ();
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
	
	vector<LDObject*> priorSelection = m_sel;
	
	// Get the objects from the object list selection
	m_sel.clear ();	
	const QList<QListWidgetItem*> items = m_objList->selectedItems ();
	
	for (LDObject* obj : g_curfile->objs ())
	for (QListWidgetItem* item : items) {
		if (item == obj->qObjListEntry) {
			m_sel << obj;
			break;
		}
	}
	
	// Update the GL renderer
	for (LDObject* obj : priorSelection) {
		obj->setSelected (false);
		m_renderer->compileObject (obj);
	}
	
	for (LDObject* obj : m_sel) {
		obj->setSelected (true);
		m_renderer->compileObject (obj);
	}
	
	m_renderer->update ();
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
	g_curfile->openHistory ();
	QToolButton* button = static_cast<QToolButton*> (sender ());
	color* col = null;
	
	for (quickColor entry : m_colorMeta) {
		if (entry.btn == button) {
			col = entry.col;
			break;
		}
	}
	
	if (col == null)
		return;
	
	short newColor = col->index;
	
	for (LDObject* obj : m_sel) {
		if (obj->isColored () == false)
			continue; // uncolored object
		
		obj->setColor (newColor);
	}
	
	fullRefresh ();
	g_curfile->closeHistory ();
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
	return g_curfile->numObjs ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::fullRefresh () {
	buildObjList ();
	m_renderer->hardRefresh ();
}

void ForgeWindow::refresh () {
	buildObjList ();
	m_renderer->update ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::updateSelection () {
	g_bSelectionLocked = true;
	
	for (LDObject* obj : g_curfile->objs ())
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

short ForgeWindow::getSelectedColor () {
	short result = -1;
	
	for (LDObject* obj : m_sel) {
		if (obj->isColored () == false)
			continue; // doesn't use color
		
		if (result != -1 && obj->color () != result)
			return -1; // No consensus in object color
		
		if (result == -1)
			result = obj->color ();
	}
	
	return result;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
LDObject::Type ForgeWindow::uniformSelectedType () {
	LDObject::Type result = LDObject::Unidentified;
	
	for (LDObject* obj : m_sel) {
		if (result != LDObject::Unidentified && obj->color () != result)
			return LDObject::Unidentified;
		
		if (result == LDObject::Unidentified)
			result = obj->getType ();
	}
	
	return result;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::closeEvent (QCloseEvent* ev) {
	// Check whether it's safe to close all files.
	if (!safeToCloseAll ()) {
		ev->ignore ();
		return;
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
	contextMenu->addAction (findAction ("setOverlay"));
	contextMenu->addAction (findAction ("clearOverlay"));
	
	for (const char* mode : g_modeActionNames)
		contextMenu->addAction (findAction (mode));
	
	if (R ()->camera () != GL::Free) {
		contextMenu->addSeparator ();
		contextMenu->addAction (findAction ("setDrawDepth"));
	}
	
	contextMenu->exec (pos);
}

// =============================================================================
void ForgeWindow::deleteObjVector (vector<LDObject*> objs) {
	for (LDObject* obj : objs) {
		g_curfile->forgetObject (obj);
		delete obj;
	}
}

// =============================================================================
void ForgeWindow::deleteByColor (const short colnum) {
	vector<LDObject*> objs;
	for (LDObject* obj : g_curfile->objs ()) {
		if (!obj->isColored () || obj->color () != colnum)
			continue;
		
		objs << obj;
	}
	
	deleteObjVector (objs);
}

// =============================================================================
void ForgeWindow::updateEditModeActions () {
	const EditMode mode = R ()->editMode ();
	const size_t numModeActions = (sizeof g_modeActionNames / sizeof *g_modeActionNames);
	assert ((size_t) mode < numModeActions);
	
	for (size_t i = 0; i < numModeActions; ++i) {
		QAction* act = findAction (g_modeActionNames[i]);
		
		act->setCheckable (true);
		act->setChecked (i == (size_t) mode);
		
		if (i != Select)
			act->setEnabled (R ()->camera () != GL::Free);
	}
}

void ForgeWindow::slot_editObject (QListWidgetItem* listitem) {
	LDObject* obj = null;
	for (LDObject* it : *g_curfile) {
		if (it->qObjListEntry == listitem) {
			obj = it;
			break;
		}
	}
	
	AddObjectDialog::staticDialog (obj->getType (), obj);
}

void ForgeWindow::primitiveLoaderStart (ulong max) {
	m_primLoaderWidget->show ();
	m_primLoaderBar->setRange (0, max);
	m_primLoaderBar->setValue (0);
	m_primLoaderBar->setFormat ("%p%");
}

void ForgeWindow::primitiveLoaderUpdate (ulong prog) {
	m_primLoaderBar->setValue (prog);
}

void ForgeWindow::primitiveLoaderEnd () {
	QTimer* hidetimer = new QTimer;
	connect (hidetimer, SIGNAL (timeout ()), m_primLoaderWidget, SLOT (hide ()));
	hidetimer->setSingleShot (true);
	hidetimer->start (1500);
	m_primLoaderBar->setFormat( tr( "Done" ));
	log( tr( "Primitives scanned: %1 primitives listed" ), m_primLoaderBar->value() );
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ForgeWindow::save( LDOpenFile* f, bool saveAs )
{
	str path = f->name ();
	
	if (path.length () == 0 || saveAs) {
		path = QFileDialog::getSaveFileName (g_win, tr( "Save As" ),
			g_curfile->name (), tr( "LDraw files (*.dat *.ldr)" ));
		
		if (path.length () == 0) {
			// User didn't give a file name. This happens if the user cancelled
			// saving in the save file dialog. Abort.
			return;
		}
	}
	
	if( f->save( path ))
	{
		f->setName (path);
		
		if( f == g_curfile )
			g_win->updateTitle ();
		
		log( "Saved to %1.", path );
	}
	else
	{
		setlocale( LC_ALL, "C" );
		
		str message = fmt( tr( "Failed to save to %1: %2" ), path, strerror (errno));
		
		// Tell the user the save failed, and give the option for saving as with it.
		QMessageBox dlg( QMessageBox::Critical, tr( "Save Failure" ), message,
			QMessageBox::Close, g_win );
		
		// Add a save-as button
		QPushButton* saveAsBtn = new QPushButton( tr( "Save As" ));
		saveAsBtn->setIcon( getIcon( "file-save-as" ));
		dlg.addButton( saveAsBtn, QMessageBox::ActionRole );
		dlg.setDefaultButton( QMessageBox::Close );
		dlg.exec();
		
		if( dlg.clickedButton () == saveAsBtn )
			save (f, true); // yay recursion!
	}
}

void ForgeWindow::addMessage( str msg )
{
	m_msglog->addLine( msg );
}

// ============================================================================
void ObjectList::contextMenuEvent (QContextMenuEvent* ev) {
	g_win->spawnContextMenu (ev->globalPos ());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
QPixmap getIcon (str iconName) {
	return (QPixmap (fmt (":/icons/%1.png", iconName)));
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
	QMessageBox::critical (g_win, QObject::tr( "Error" ), msg,
		(QMessageBox::Close), QMessageBox::Close);
}

// =============================================================================
QAction* findAction (str name) {
	for (actionmeta& meta : g_actionMeta) {
		if (meta.qAct == null)
			break;
		
		if (name == meta.name)
			return *meta.qAct;
	}
	
	fatal (fmt ("Couldn't find action named `%2'!", name));
	abort ();
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
		col = gl_maincolor.value;
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
	
	for (LDObject* obj : g_curfile->objs ()) {
		if (!obj->isColored ())
			continue;
		
		if (counts.find (obj->color ()) == counts.end ())
			counts[obj->color ()] = 1;
		else
			counts[obj->color ()]++;
	}
	
	box->clear ();
	ulong row = 0;
	for (const auto& pair : counts) {
		color* col = getColor (pair.first);
		assert (col != null);
		
		QIcon ico = makeColorIcon (col, 16);
		box->addItem (ico, fmt ("[%1] %2 (%3 object%4)",
			pair.first, col->name, pair.second, plural (pair.second)));
		box->setItemData (row, pair.first);
		
		++row;
	}
}

// =============================================================================
QDialogButtonBox* makeButtonBox (QDialog& dlg) {
	QDialogButtonBox* bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	QWidget::connect (bbx_buttons, SIGNAL (accepted ()), &dlg, SLOT (accept ()));
	QWidget::connect (bbx_buttons, SIGNAL (rejected ()), &dlg, SLOT (reject ()));
	return bbx_buttons;
}

CheckBoxGroup* makeAxesBox () {
	CheckBoxGroup* cbg_axes = new CheckBoxGroup ("Axes", Qt::Horizontal);
	cbg_axes->addCheckBox ("X", X);
	cbg_axes->addCheckBox ("Y", Y);
	cbg_axes->addCheckBox ("Z", Z);
	return cbg_axes;
}

void ForgeWindow::setStatusBarText (str text) {
	statusBar ()->showMessage (text);
}

void ForgeWindow::addActionMeta (actionmeta& meta) {
	if (g_metacursor == 0)
		memset (g_actionMeta, 0, sizeof g_actionMeta);
	
	assert (g_metacursor < MAX_ACTIONS);
	g_actionMeta[g_metacursor++] = meta;
}

QImage imageFromScreencap (uchar* data, ushort w, ushort h) {
	// GL and Qt formats have R and B swapped. Also, GL flips Y - correct it as well.
	return QImage (data, w, h, QImage::Format_ARGB32).rgbSwapped ().mirrored ();
}