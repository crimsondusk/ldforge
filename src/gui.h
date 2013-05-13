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
#include <QToolBar>
#include <QTextEdit>
#include <qpushbutton.h>
#include <qlistwidget.h>
#include <qlabel.h>
#include <qboxlayout.h>
#include "gldraw.h"
#include "config.h"

class QComboBox;
class ForgeWindow;
class color;
class QSplitter;
class DelHistory;
class QToolButton;
class QDialogButtonBox;

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
	const char* const name, *sDisplayName, *sIconName, *sDescription;
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
		const char* sDescription, keyseqconfig* conf, void (*const handler) (),
		const char* name)
	{
		actionmeta meta = {qAct, conf, name, sDisplayName, sIconName, sDescription, handler};
		g_ActionMeta.push_back (meta);
	}
};

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
	void setTitle ();
	void fullRefresh ();
	void refresh ();
	ulong getInsertionPoint ();
	void deleteSelection (vector<ulong>* ulapIndices, std::vector<LDObject*>* papObjects);
	void updateToolBars ();
	void updateRecentFilesMenu ();
	void updateSelection ();
	void updateGridToolBar ();
	bool isSelected (LDObject* obj);
	short getSelectedColor();
	LDObject::Type uniformSelectedType ();
	void scrollToSelection ();
	void spawnContextMenu (const QPoint pos);
	DelHistory* deleteObjVector (const std::vector<LDObject*> objs);
	DelHistory* deleteSelection ();
	DelHistory* deleteByColor (const short colnum);
	GLRenderer* R () { return m_renderer; }
	std::vector<LDObject*>& sel () { return m_sel; }
	void setQuickColorMeta (std::vector<quickColorMetaEntry>& quickColorMeta) {
		m_colorMeta = quickColorMeta;
	}
	
protected:
	void closeEvent (QCloseEvent* ev);
	void logVA (LogType eType, const char* fmtstr, va_list va);
	
	friend void logf (const char* fmtstr, ...);
	friend void logf (LogType type, const char* fmtstr, ...);
	
private:
	GLRenderer* m_renderer;
	ObjectList* m_objList;
	QMenu* m_recentFilesMenu;
	QSplitter* m_splitter;
	str m_msglogHTML;
	QToolBar* m_colorToolBar;
	std::vector<QToolBar*> m_toolBars;
	std::vector<LDObject*> m_sel;
	std::vector<quickColorMetaEntry> m_colorMeta;
	std::vector<QToolButton*> m_colorButtons;
	std::vector<QAction*> m_recentFiles;
	
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
};

// =============================================================================
// LabeledWidget
//
// Convenience class for a widget with a label beside it.
// =============================================================================
template<class R> class LabeledWidget : public QWidget {
public:
	explicit LabeledWidget (const char* labelstr, QWidget* parent = null) : QWidget (parent) {
		m_widget = new R (this);
		commonInit (labelstr);
	}
	
	explicit LabeledWidget (const char* labelstr, R* widget, QWidget* parent = null) :
		QWidget (parent), m_widget (widget)
	{
		commonInit (labelstr);
	}
	
	explicit LabeledWidget (QWidget* parent = 0, Qt::WindowFlags f = 0) {
		m_widget = new R (this);
		commonInit ("");
	}
	
	R* widget () const { return m_widget; }
	R* w () const { return m_widget; }
	QLabel* label () const { return m_label; }
	QLabel* l () const { return m_label; }
	void setWidget (R* widget) { m_widget = widget; }
	void setLabel (QLabel* label) { m_label = label; }
	operator R* () { return m_widget; }
	
private:
	Q_DISABLE_COPY (LabeledWidget<R>)
	
	void commonInit (const char* labelstr) {
		m_label = new QLabel (labelstr, this);
		m_layout = new QHBoxLayout;
		m_layout->addWidget (m_label);
		m_layout->addWidget (m_widget);
		setLayout (m_layout);
	}
	
	R* m_widget;
	QLabel* m_label;
	QHBoxLayout* m_layout;
};

// -----------------------------------------------------------------------------
// Other GUI-related stuff not directly part of ForgeWindow:
QPixmap getIcon (const char* sIconName);
std::vector<quickColorMetaEntry> parseQuickColorMeta ();
bool confirm (str title, str msg);
bool confirm (str msg);
void critical (str msg);
QAction* findAction (str name);
QIcon makeColorIcon (color* colinfo, const ushort size);
void makeColorSelector (QComboBox* box);
QDialogButtonBox* makeButtonBox (QDialog& dlg);

// -----------------------------------------------------------------------------
// Pointer to the instance of ForgeWindow.
extern ForgeWindow* g_win;

#endif // GUI_H