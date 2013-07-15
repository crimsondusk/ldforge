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
class LDColor;
class QToolButton;
class QDialogButtonBox;
class GLRenderer;
class CheckBoxGroup;
class QComboBox;
class QProgressBar;
class Ui_LDForgeUI;

// Stuff for dialogs
#define IMPLEMENT_DIALOG_BUTTONS \
	bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel); \
	connect (bbx_buttons, SIGNAL (accepted()), this, SLOT (accept())); \
	connect (bbx_buttons, SIGNAL (rejected()), this, SLOT (reject())); \

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
#define DEFINE_ACTION(NAME, DEFSHORTCUT) \
	cfg (keyseq, key_##NAME, DEFSHORTCUT); \
	void actiondef_##NAME()

#define ACTION(N) g_win->action##N()

// Convenience macros for key sequences.
#define KEY(N) (Qt::Key_##N)
#define CTRL(N) (Qt::CTRL | Qt::Key_##N)
#define SHIFT(N) (Qt::SHIFT | Qt::Key_##N)
#define CTRL_SHIFT(N) (Qt::CTRL | Qt::SHIFT | Qt::Key_##N)

// =============================================================================
typedef struct {
	LDColor* col;
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
	ForgeWindow();
	void buildObjList();
	void updateTitle();
	void fullRefresh();
	void refresh();
	ulong getInsertionPoint();
	void updateToolBars();
	void updateRecentFilesMenu();
	void updateSelection();
	void updateGridToolBar();
	void updateEditModeActions();
	void updateFileList();
	void updateFileListItem (LDOpenFile* f);
	bool isSelected (LDObject* obj);
	short getSelectedColor();
	LDObject::Type uniformSelectedType();
	void scrollToSelection();
	void spawnContextMenu (const QPoint pos);
	void deleteObjVector (vector< LDObject* > objs);
	int deleteSelection();
	void deleteByColor (const short int colnum);
	void save (LDOpenFile* f, bool saveAs);
	GLRenderer* R() { return m_renderer; }
	vector<LDObject*>& sel() { return m_sel; }
	void setQuickColorMeta (vector<quickColor>& quickColorMeta) {
		m_colorMeta = quickColorMeta;
	}
	void setStatusBarText (str text);
	void addMessage (str msg);
	Ui_LDForgeUI* interface() const;
	
	void beginAction(QAction* act);
	void endAction();
	
#define act(N) QAction* action##N();
#include "actions.h"
	
public slots:
	void primitiveLoaderStart (ulong max);
	void primitiveLoaderUpdate (ulong prog);
	void primitiveLoaderEnd();
	void clearSelection();
	void slot_action();
	
protected:
	void closeEvent (QCloseEvent* ev);
	
private:
	GLRenderer* m_renderer;
	QProgressBar* m_primLoaderBar;
	QWidget* m_primLoaderWidget;
	vector<LDObject*> m_sel;
	vector<quickColor> m_colorMeta;
	vector<QToolButton*> m_colorButtons;
	vector<QAction*> m_recentFiles;
	MessageManager* m_msglog;
	Ui_LDForgeUI* ui;
	
	void invokeAction (QAction* act, void (*func)());
	

private slots:
	void slot_selectionChanged();
	void slot_recentFile();
	void slot_quickColor();
	void slot_lastSecondCleanup();
	void slot_editObject (QListWidgetItem* listitem);
};

#define INVOKE_ACTION(N) actiondef_##N();
#define act(N) void actiondef_##N();
#include "actions.h"

// -----------------------------------------------------------------------------
// Pointer to the instance of ForgeWindow.
extern ForgeWindow* g_win;

// -----------------------------------------------------------------------------
// Other GUI-related stuff not directly part of ForgeWindow:
QPixmap getIcon (str iconName);
vector<quickColor> parseQuickColorMeta();
bool confirm (str title, str msg);
bool confirm (str msg);
void critical (str msg);
QIcon makeColorIcon (LDColor* colinfo, const ushort size);
void makeColorSelector (QComboBox* box);
CheckBoxGroup* makeAxesBox();
QImage imageFromScreencap (uchar* data, ushort w, ushort h);

// =============================================================================
// -----------------------------------------------------------------------------
// Takes in pairs of radio buttons and respective values and returns the value of
// the first found radio button that was checked.
// =============================================================================
template<class T> T radioSwitch (const T& defval, vector<pair<QRadioButton*, T>> haystack) {
	for (pair<QRadioButton*, const T&> i : haystack)
		if (i.first->isChecked())
			return i.second;
	
	return defval;
}

// =============================================================================
// -----------------------------------------------------------------------------
// Takes in pairs of radio buttons and respective values and checks the first
// found radio button to have the given value.
// =============================================================================
template<class T> void radioDefault (const T& expr, vector<pair<QRadioButton*, T>> haystack) {
	for (pair<QRadioButton*, const T&> i : haystack) {
		if (i.second == expr) {
			i.first->setChecked (true);
			return;
		}
	}
}

#endif // GUI_H