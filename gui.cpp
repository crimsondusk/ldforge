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
EXTERN_ACTION (moveUp)
EXTERN_ACTION (moveDown)
EXTERN_ACTION (newSubfile)
EXTERN_ACTION (newLine)
EXTERN_ACTION (newCondLine)
EXTERN_ACTION (newTriangle)
EXTERN_ACTION (newQuad)
EXTERN_ACTION (newVertex)
EXTERN_ACTION (newComment)
EXTERN_ACTION (help)
EXTERN_ACTION (about)
EXTERN_ACTION (aboutQt)

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
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
#define ADD_MENU_ITEM(MENU, ACT) q##MENU##Menu->addAction (ACTION_NAME (ACT));

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
	
	// Insert menu
	qInsertMenu = menuBar ()->addMenu (tr ("&Insert"));
	ADD_MENU_ITEM (Insert, newSubfile)		// New Subfile
	ADD_MENU_ITEM (Insert, newLine)			// New Line
	ADD_MENU_ITEM (Insert, newTriangle)		// New Triangle
	ADD_MENU_ITEM (Insert, newQuad)			// New Quad
	ADD_MENU_ITEM (Insert, newCondLine)		// New Conditional Line
	ADD_MENU_ITEM (Insert, newComment)		// New Comment
	ADD_MENU_ITEM (Insert, newVertex)		// New Vertex
	
	// Edit menu
	qEditMenu = menuBar ()->addMenu (tr ("&Edit"));
	ADD_MENU_ITEM (Edit, cut)				// Cut
	ADD_MENU_ITEM (Edit, copy)				// Copy
	ADD_MENU_ITEM (Edit, paste)				// Paste
	ADD_MENU_ITEM (Edit, del)				// Delete
	qEditMenu->addSeparator ();				// -----
	ADD_MENU_ITEM (Edit, moveUp)			// Move Up
	ADD_MENU_ITEM (Edit, moveDown)			// Move Down
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
#define ADD_TOOLBAR_ITEM(BAR, ACT) q##BAR##ToolBar->addAction (ACTION_NAME (ACT));

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
	ADD_TOOLBAR_ITEM (Edit, moveUp)
	ADD_TOOLBAR_ITEM (Edit, moveDown)
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
void ForgeWindow::slot_action () {
	// Get the action that triggered this slot.
	QAction* qAct = static_cast<QAction*> (sender ());
	
	// Find the meta for the action.
	actionmeta* pMeta = nullptr;
	
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
	
	if (objs.size() > 0)
		refresh ();
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
	/*
	// If the selection isn't 1 exact, disable setting contents
	ACTION (setContents)->setEnabled (qObjList->selectedItems().size() == 1);
	
	// If we have no selection, disable splitting quads
	ACTION (splitQuads)->setEnabled (qObjList->selectedItems().size() > 0);
	*/
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

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
QIcon getIcon (const char* sIconName) {
	return (QIcon (str::mkfmt ("./icons/%s.png", sIconName)));
}