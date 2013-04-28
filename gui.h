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

#ifndef GUI_H
#define GUI_H

#include <QMainWindow>
#include <QMenu>
#include <QToolBar>
#include <QAction>
#include <QTreeWidget>
#include <QToolBar>
#include <QTextEdit>
#include <qpushbutton.h>
#include "gldraw.h"
#include "config.h"

class ForgeWindow;
class color;
class QSplitter;

// Stuff for dialogs
#define IMPLEMENT_DIALOG_BUTTONS \
	bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel); \
	connect (bbx_buttons, SIGNAL (accepted ()), this, SLOT (accept ())); \
	connect (bbx_buttons, SIGNAL (rejected ()), this, SLOT (reject ())); \

// =============================================================================
// Metadata for actions
typedef struct {
	QAction** const qAct;
	keyseqconfig* const conf;
	const char* const sDisplayName, *sIconName, *sDescription;
	void (*const handler) ();
} actionmeta;

extern vector<actionmeta> g_ActionMeta;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
#define MAKE_ACTION(NAME, DISPLAYNAME, ICONNAME, DESCR, DEFSHORTCUT) \
	QAction* ACTION (NAME); \
	cfg (keyseq, key_##NAME, DEFSHORTCUT); \
	static void actionHandler_##NAME (); \
	static ActionAdder ActionAdderInstance_##NAME (&ACTION(NAME), DISPLAYNAME, \
		ICONNAME, DESCR, &key_##NAME, actionHandler_##NAME); \
	static void actionHandler_##NAME ()

#define EXTERN_ACTION(NAME) extern QAction* ACTION (NAME);
#define ACTION(N) LDForgeAction_##N

// Convenience macros for key sequences.
#define KEY(N) (Qt::Key_##N)
#define CTRL(N) (Qt::CTRL | Qt::Key_##N)
#define SHIFT(N) (Qt::SHIFT | Qt::Key_##N)
#define CTRL_SHIFT(N) (Qt::CTRL | Qt::SHIFT | Qt::Key_##N)

// =============================================================================
typedef struct {
	color* col;
	QPushButton* btn;
	bool bSeparator;
} quickColorMetaEntry;

// =============================================================================
// ActionAdder
//
// The ACTION macro expands into - among other stuff - into an instance of this.
// This class' constructor creates meta for the newly defined action and stores
// it in g_ActionMeta. It is not supposed to be used directly!
// =============================================================================
class ActionAdder {
public:
	ActionAdder (QAction** qAct, const char* sDisplayName, const char* sIconName,
		const char* sDescription, keyseqconfig* conf, void (*const handler) ())
	{
		actionmeta meta = {qAct, conf, sDisplayName, sIconName, sDescription, handler};
		g_ActionMeta.push_back (meta);
	}
};

// =============================================================================
// ForgeWindow
// 
// The one main GUI class. Hosts the renderer, object list, message log. Contains
// slot_action, which is what all actions connect to. Manages menus and toolbars.
// Large and in charge.
// =============================================================================
class ForgeWindow : public QMainWindow {
	Q_OBJECT
	
public:
	GLRenderer* R;
	
	// Object list view
	QTreeWidget* qObjList;
	QTextEdit* qMessageLog;
	QMenu* qFileMenu, *qEditMenu, *qViewMenu, *qInsertMenu, *qMoveMenu,
		*qHelpMenu, *qControlMenu;
	QMenu* qRecentFilesMenu;
	std::vector<QAction*> qaRecentFiles;
	QSplitter* hsplit, *vsplit;
	
#ifndef RELEASE
	QMenu* qDebugMenu;
	QToolBar* qDebugToolBar;
#endif // RELEASE
	
	std::vector<QToolBar*> qaToolBars;
	
	// Quick color buttons
	std::vector<QPushButton*> qaColorButtons;
	QToolBar* qColorToolBar;
	std::vector<quickColorMetaEntry> quickColorMeta;
	
	// Selected objects
	std::vector<LDObject*> sel;
	
	str zMessageLogHTML;
	
	ForgeWindow ();
	void buildObjList ();
	void setTitle ();
	void refresh ();
	std::vector<LDObject*> getSelectedObjects ();
	ulong getInsertionPoint ();
	void deleteSelection (vector<ulong>* ulapIndices, std::vector<LDObject*>* papObjects);
	void updateToolBars ();
	void updateRecentFilesMenu ();
	void updateSelection ();
	void updateGridToolBar ();
	bool isSelected (LDObject* obj);
	short getSelectedColor();
	LDObjectType_e uniformSelectedType ();
	void scrollToSelection ();
	
protected:
	void closeEvent (QCloseEvent* ev);
	
private:
	void createMenuActions ();
	void createMenus ();
	void createToolbars ();
	void initSingleToolBar (const char* sName);

private slots:
	void slot_selectionChanged ();
	void slot_action ();
	void slot_recentFile ();
	void slot_quickColor ();
	void slot_lastSecondCleanup ();
};

// -----------------------------------------------------------------------------
// Other GUI-related stuff not directly part of ForgeWindow:
QIcon getIcon (const char* sIconName);
std::vector<quickColorMetaEntry> parseQuickColorMeta ();
bool confirm (str title, str msg);
bool confirm (str msg);
void critical (str msg);

// -----------------------------------------------------------------------------
// Pointer to the instance of ForgeWindow.
extern ForgeWindow* g_ForgeWindow;

// Is this still needed?
enum {
	LDOLC_Icon,
	LDOLC_Data,
	NUM_LDOL_Columns
};

#endif // GUI_H