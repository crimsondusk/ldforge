/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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

#ifndef LDFORGE_GUI_H
#define LDFORGE_GUI_H

#include <QMainWindow>
#include <QAction>
#include <QListWidget>
#include <QRadioButton>
#include "config.h"
#include "ldtypes.h"
#include "ui_ldforge.h"

class MessageManager;
class ForgeWindow;
class LDColor;
class QToolButton;
class QDialogButtonBox;
class GLRenderer;
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
	cfg (KeySequence, key_action##NAME, DEFSHORTCUT); \
	void ForgeWindow::slot_action##NAME()

// Convenience macros for key sequences.
#define KEY(N) (Qt::Key_##N)
#define CTRL(N) (Qt::CTRL | Qt::Key_##N)
#define SHIFT(N) (Qt::SHIFT | Qt::Key_##N)
#define CTRL_SHIFT(N) (Qt::CTRL | Qt::SHIFT | Qt::Key_##N)

// =============================================================================
class LDQuickColor
{
	PROPERTY (public,	LDColor*,		Color,		NO_OPS,	STOCK_WRITE)
	PROPERTY (public,	QToolButton*,	ToolButton,	NO_OPS,	STOCK_WRITE)

	public:
		LDQuickColor (LDColor* color, QToolButton* toolButton);
		bool isSeparator() const;

		static LDQuickColor getSeparator();
};

// =============================================================================
// ObjectList
//
// Object list class for ForgeWindow
// =============================================================================
class ObjectList : public QListWidget
{
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
class ForgeWindow : public QMainWindow
{
	Q_OBJECT

	public:
		ForgeWindow();
		void buildObjList();
		void updateTitle();
		void doFullRefresh();
		void refresh();
		int getInsertionPoint();
		void updateToolBars();
		void updateRecentFilesMenu();
		void updateSelection();
		void updateGridToolBar();
		void updateEditModeActions();
		void updateDocumentList();
		void updateDocumentListItem (LDDocument* f);
		int getSelectedColor();
		LDObject::Type getUniformSelectedType();
		void scrollToSelection();
		void spawnContextMenu (const QPoint pos);
		void deleteObjects (QList<LDObject*> objs);
		int deleteSelection();
		void deleteByColor (const int colnum);
		bool save (LDDocument* f, bool saveAs);
		void updateActions();

		inline GLRenderer* R()
		{
			return m_renderer;
		}

		inline void setQuickColors (QList<LDQuickColor>& colors)
		{
			m_quickColors = colors;
		}

		void addMessage (QString msg);
		void refreshObjectList();
		void updateActionShortcuts();
		KeySequenceConfig* shortcutForAction (QAction* act);
		void endAction();

	public slots:
		void changeCurrentFile();
		void slot_action();
		void slot_actionNew();
		void slot_actionNewFile();
		void slot_actionOpen();
		void slot_actionDownloadFrom();
		void slot_actionSave();
		void slot_actionSaveAs();
		void slot_actionSaveAll();
		void slot_actionClose();
		void slot_actionCloseAll();
		void slot_actionInsertFrom();
		void slot_actionExportTo();
		void slot_actionSettings();
		void slot_actionSetLDrawPath();
		void slot_actionScanPrimitives();
		void slot_actionExit();
		void slot_actionResetView();
		void slot_actionAxes();
		void slot_actionWireframe();
		void slot_actionBFCView();
		void slot_actionSetOverlay();
		void slot_actionClearOverlay();
		void slot_actionScreenshot();
		void slot_actionInsertRaw();
		void slot_actionNewSubfile();
		void slot_actionNewLine();
		void slot_actionNewTriangle();
		void slot_actionNewQuad();
		void slot_actionNewCLine();
		void slot_actionNewComment();
		void slot_actionNewBFC();
		void slot_actionNewVertex();
		void slot_actionUndo();
		void slot_actionRedo();
		void slot_actionCut();
		void slot_actionCopy();
		void slot_actionPaste();
		void slot_actionDelete();
		void slot_actionSelectAll();
		void slot_actionSelectByColor();
		void slot_actionSelectByType();
		void slot_actionModeDraw();
		void slot_actionModeSelect();
		void slot_actionModeCircle();
		void slot_actionSetDrawDepth();
		void slot_actionSetColor();
		void slot_actionAutocolor();
		void slot_actionUncolorize();
		void slot_actionInline();
		void slot_actionInlineDeep();
		void slot_actionInvert();
		void slot_actionMakePrimitive();
		void slot_actionSplitQuads();
		void slot_actionEditRaw();
		void slot_actionBorders();
		void slot_actionCornerVerts();
		void slot_actionRoundCoordinates();
		void slot_actionVisibilityHide();
		void slot_actionVisibilityReveal();
		void slot_actionVisibilityToggle();
		void slot_actionReplaceCoords();
		void slot_actionFlip();
		void slot_actionDemote();
		void slot_actionYtruder();
		void slot_actionRectifier();
		void slot_actionIntersector();
		void slot_actionIsecalc();
		void slot_actionCoverer();
		void slot_actionEdger2();
		void slot_actionHelp();
		void slot_actionAbout();
		void slot_actionAboutQt();
		void slot_actionGridCoarse();
		void slot_actionGridMedium();
		void slot_actionGridFine();
		void slot_actionEdit();
		void slot_actionMoveUp();
		void slot_actionMoveDown();
		void slot_actionMoveXNeg();
		void slot_actionMoveXPos();
		void slot_actionMoveYNeg();
		void slot_actionMoveYPos();
		void slot_actionMoveZNeg();
		void slot_actionMoveZPos();
		void slot_actionRotateXNeg();
		void slot_actionRotateXPos();
		void slot_actionRotateYNeg();
		void slot_actionRotateYPos();
		void slot_actionRotateZNeg();
		void slot_actionRotateZPos();
		void slot_actionRotationPoint();
		void slot_actionAddHistoryLine();
		void slot_actionJumpTo();
		void slot_actionSubfileSelection();
		void slot_actionDrawAngles();

	protected:
		void closeEvent (QCloseEvent* ev);

	private:
		GLRenderer* m_renderer;
		QList<LDObject*> m_sel;
		QList<LDQuickColor> m_quickColors;
		QList<QToolButton*> m_colorButtons;
		QList<QAction*> m_recentFiles;
		MessageManager* m_msglog;
		Ui_LDForgeUI* ui;

	private slots:
		void slot_selectionChanged();
		void slot_recentFile();
		void slot_quickColor();
		void slot_lastSecondCleanup();
		void slot_editObject (QListWidgetItem* listitem);
};

// -----------------------------------------------------------------------------
// Pointer to the instance of ForgeWindow.
extern ForgeWindow* g_win;

// -----------------------------------------------------------------------------
// Other GUI-related stuff not directly part of ForgeWindow:
QPixmap getIcon (QString iconName); // Get an icon from the resource dir
QList<LDQuickColor> quickColorsFromConfig(); // Make a list of quick colors based on config
bool confirm (QString title, QString msg); // Generic confirm prompt
bool confirm (QString msg); // Generic confirm prompt
void critical (QString msg); // Generic error prompt
QIcon makeColorIcon (LDColor* colinfo, const int size); // Makes an icon for the given color
void makeColorComboBox (QComboBox* box); // Fills the given combo-box with color information
QImage imageFromScreencap (uchar* data, int w, int h);

// =============================================================================
// -----------------------------------------------------------------------------
// Takes in pairs of radio buttons and respective values and returns the value of
// the first found radio button that was checked.
// =============================================================================
template<class T>
T radioSwitch (const T& defval, QList<pair<QRadioButton*, T>> haystack)
{
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
template<class T>
void radioDefault (const T& expr, QList<pair<QRadioButton*, T>> haystack)
{
	for (pair<QRadioButton*, const T&> i : haystack)
	{
		if (i.second == expr)
		{
			i.first->setChecked (true);
			return;
		}
	}
}

#endif // LDFORGE_GUI_H
