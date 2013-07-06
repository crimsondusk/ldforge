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
#include <QAction>
#include <QListWidget>
#include <QRadioButton>
#include "config.h"
#include "ldtypes.h"

class MessageManager;
class ForgeWindow;
class color;
class QSplitter;
class QToolButton;
class QDialogButtonBox;
class GLRenderer;
class CheckBoxGroup;
class QComboBox;
class QProgressBar;

// Stuff for dialogs
#define IMPLEMENT_DIALOG_BUTTONS \
	bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel); \
	connect (bbx_buttons, SIGNAL (accepted ()), this, SLOT (accept ())); \
	connect (bbx_buttons, SIGNAL (rejected ()), this, SLOT (reject ())); \

// =============================================================================
// Metadata for actions
typedef struct {
	QAction** qAct;
	keyseqconfig* conf;
	const char* name, *sDisplayName, *sIconName, *sDescription;
	void (*handler) ();
} actionmeta;

#define MAX_ACTIONS 256
extern actionmeta g_actionMeta[256];

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
#define MAKE_ACTION(NAME, DISPLAYNAME, ICONNAME, DESCR, DEFSHORTCUT) \
	QAction* ACTION (NAME); \
	cfg (keyseq, key_##NAME, DEFSHORTCUT); \
	static void actionHandler_##NAME (); \
	static ActionAdder ActionAdderInstance_##NAME (&ACTION(NAME), DISPLAYNAME, \
		ICONNAME, DESCR, &key_##NAME, actionHandler_##NAME, #NAME); \
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
	QToolButton* btn;
	bool isSeparator;
} quickColor;

// =============================================================================
// ObjectList
// 
// Object list class for ForgeWindow
// =============================================================================
class ObjectList : public QListWidget {
	Q_OBJECT
	
protected:
	void contextMenuEvent (QContextMenuEvent* ev);
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
	ForgeWindow ();
	void buildObjList ();
	void updateTitle ();
	void fullRefresh ();
	void refresh ();
	ulong getInsertionPoint ();
	void updateToolBars ();
	void updateRecentFilesMenu ();
	void updateSelection ();
	void updateGridToolBar ();
	void updateEditModeActions ();
	bool isSelected (LDObject* obj);
	short getSelectedColor();
	LDObject::Type uniformSelectedType ();
	void scrollToSelection ();
	void spawnContextMenu (const QPoint pos);
	void deleteObjVector (vector< LDObject* > objs);
	int deleteSelection ();
	void deleteByColor (const short int colnum);
	void save( LDOpenFile* f, bool saveAs );
	GLRenderer* R () { return m_renderer; }
	vector<LDObject*>& sel () { return m_sel; }
	void setQuickColorMeta (vector<quickColor>& quickColorMeta) {
		m_colorMeta = quickColorMeta;
	}
	void setStatusBarText (str text);
	void addActionMeta (actionmeta& meta);
	void addMessage( str msg );
	
public slots:
	void primitiveLoaderStart (ulong max);
	void primitiveLoaderUpdate (ulong prog);
	void primitiveLoaderEnd ();
	
protected:
	void closeEvent (QCloseEvent* ev);
	
private:
	GLRenderer* m_renderer;
	ObjectList* m_objList;
	QMenu* m_recentFilesMenu;
	QSplitter* m_splitter;
	QToolBar* m_colorToolBar;
	QProgressBar* m_primLoaderBar;
	QWidget* m_primLoaderWidget;
	vector<QToolBar*> m_toolBars;
	vector<LDObject*> m_sel;
	vector<quickColor> m_colorMeta;
	vector<QToolButton*> m_colorButtons;
	vector<QAction*> m_recentFiles;
	MessageManager* m_msglog;
	
	void createMenuActions ();
	void createMenus ();
	void createToolbars ();
	void initSingleToolBar (const char* name);
	void addToolBarAction (const char* name);
	
	QMenu* initMenu (const char* name);
	void addMenuAction (const char* name);

private slots:
	void slot_selectionChanged ();
	void slot_action ();
	void slot_recentFile ();
	void slot_quickColor ();
	void slot_lastSecondCleanup ();
	void slot_editObject (QListWidgetItem* listitem);
};

// -----------------------------------------------------------------------------
// Pointer to the instance of ForgeWindow.
extern ForgeWindow* g_win;

// -----------------------------------------------------------------------------
// Other GUI-related stuff not directly part of ForgeWindow:
QPixmap getIcon (str iconName);
vector<quickColor> parseQuickColorMeta ();
bool confirm (str title, str msg);
bool confirm (str msg);
void critical (str msg);
QAction* findAction (str name);
QIcon makeColorIcon (color* colinfo, const ushort size);
void makeColorSelector (QComboBox* box);
QDialogButtonBox* makeButtonBox (QDialog& dlg);
CheckBoxGroup* makeAxesBox ();
QImage imageFromScreencap (uchar* data, ushort w, ushort h);

// =============================================================================
// -----------------------------------------------------------------------------
// Takes in pairs of radio buttons and respective values and returns the value of
// the first found radio button that was checked.
// =============================================================================
template<class T> T radioSwitch( const T& defval, vector<pair<QRadioButton*, T>> haystack )
{
	for( pair<QRadioButton*, const T&> i : haystack )
		if( i.first->isChecked() )
			return i.second;
	
	return defval;
}

// =============================================================================
// -----------------------------------------------------------------------------
// Takes in pairs of radio buttons and respective values and checks the first
// found radio button to have the given value.
// =============================================================================
template<class T> void radioDefault( const T& expr, vector<pair<QRadioButton*, T>> haystack )
{
	for( pair<QRadioButton*, const T&> i : haystack )
	{
		if( i.second == expr )
		{
			i.first->setChecked( true );
			return;
		}
	}
}

// =============================================================================
// ActionAdder
//
// The MAKE_ACTION macro expands into - among other stuff - into an instance
// of this. This class' constructor creates meta for the newly defined action
// and stores it in g_actionMeta. Don't use this directly!
// =============================================================================
class ActionAdder {
public:
	ActionAdder (QAction** act, const char* displayName, const char* iconName,
		const char* description, keyseqconfig* conf, void (*const handler) (),
		const char* name)
	{
		actionmeta meta = {act, conf, name, displayName, iconName, description, handler};
		g_win->addActionMeta (meta);
	}
};

#endif // GUI_H