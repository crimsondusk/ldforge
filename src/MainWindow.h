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

#pragma once
#include <QMainWindow>
#include <QAction>
#include <QListWidget>
#include <QRadioButton>
#include "Configuration.h"
#include "LDObject.h"
#include "ui_ldforge.h"

class MessageManager;
class MainWindow;
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
	void MainWindow::slot_action##NAME()

// Convenience macros for key sequences.
#define KEY(N) (Qt::Key_##N)
#define CTRL(N) (Qt::CTRL | Qt::Key_##N)
#define SHIFT(N) (Qt::SHIFT | Qt::Key_##N)
#define CTRL_SHIFT(N) (Qt::CTRL | Qt::SHIFT | Qt::Key_##N)

// =============================================================================
class LDQuickColor
{
	PROPERTY (public,	LDColor*,		color,		setColor,		STOCK_WRITE)
	PROPERTY (public,	QToolButton*,	toolButton,	setToolButton,	STOCK_WRITE)

	public:
		LDQuickColor (LDColor* color, QToolButton* toolButton);
		bool isSeparator() const;

		static LDQuickColor getSeparator();
};

//!
//! Object list class for MainWindow
//!
class ObjectList : public QListWidget
{
	Q_OBJECT

	protected:
		void contextMenuEvent (QContextMenuEvent* ev);
};

//!
//! \brief The main window class.
//!
//! The MainWindow is LDForge's main GUI. It hosts the renderer, the object list,
//! the message log, etc. Contains \c slot_action(), which is what all actions
//! connect to.
//!
class MainWindow : public QMainWindow
{
	Q_OBJECT

	public:
		//! Constructs the main window
		explicit MainWindow (QWidget* parent = null, Qt::WindowFlags flags = 0);

		//! Rebuilds the object list, located to the right of the GUI.
		void buildObjList();

		//! Updates the window title.
		void updateTitle();

		//! Builds the object list and tells the GL renderer to init a full
		//! refresh.
		void doFullRefresh();

		//! Builds the object list and tells the GL renderer to do a soft update.
		void refresh();

		//! \returns the suggested position to place a new object at.
		int getInsertionPoint();

		//! Updates the quick color toolbar
		void updateColorToolbar();

		//! Rebuilds the recent files submenu
		void updateRecentFilesMenu();

		//! Sets the selection based on what's selected in the object list.
		void updateSelection();

		//! Updates the grids, selects the selected grid and deselects others.
		void updateGridToolBar();

		//! Updates the edit modes, current one is selected and others are deselected.
		void updateEditModeActions();

		//! Rebuilds the document tab list.
		void updateDocumentList();

		//! Updates the document tab for \c doc. If no such tab exists, the
		//! document list is rebuilt instead.
		void updateDocumentListItem (LDDocument* doc);

		//! \returns the uniform selected color (i.e. 4 if everything selected is
		//! red), -1 if there is no such consensus.
		int getSelectedColor();

		//! \returns the uniform selected type (i.e. \c LDObject::ELine if everything
		//! selected is a line), \c LDObject::EUnidentified if there is no such
		//! consensus.
		LDObject::Type getUniformSelectedType();

		//! Automatically scrolls the object list so that it points to the first
		//! selected object.
		void scrollToSelection();

		//! Spawns the context menu at the given position.
		void spawnContextMenu (const QPoint pos);

		//! Deletes all selected objects.
		//! \returns the count of deleted objects.
		int deleteSelection();

		//! Deletes all objects by the given color number.
		void deleteByColor (int colnum);

		//! Tries to save the given document.
		//! \param doc the document to save
		//! \param saveAs if true, always ask for a file path
		//! \returns whether the save was successful
		bool save (LDDocument* doc, bool saveAs);

		//! Updates various actions, undo/redo are set enabled/disabled where
		//! appropriate, togglable actions are updated based on configuration,
		//! etc.
		void updateActions();

		//! \returns a pointer to the renderer
		inline GLRenderer* R()
		{
			return m_renderer;
		}

		//! Sets the quick color list to the given list of colors.
		inline void setQuickColors (const QList<LDQuickColor>& colors)
		{
			m_quickColors = colors;
			updateColorToolbar();
		}

		//! Adds a message to the renderer's message manager.
		void addMessage (QString msg);

		//! Updates the object list. Right now this just rebuilds it.
		void refreshObjectList();

		//! Updates all actions to ensure they have the correct shortcut as
		//! defined in the configuration entries.
		void updateActionShortcuts();

		//! Gets the shortcut configuration for the given \c action
		KeySequenceConfig* shortcutForAction (QAction* action);

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
		GLRenderer*			m_renderer;
		LDObjectList		m_sel;
		QList<LDQuickColor>	m_quickColors;
		QList<QToolButton*>	m_colorButtons;
		QList<QAction*>		m_recentFiles;
		MessageManager*		m_msglog;
		Ui_LDForgeUI*		ui;
		QTabBar*			m_tabs;
		bool				m_updatingTabs;

		void endAction();

	private slots:
		void slot_selectionChanged();
		void slot_recentFile();
		void slot_quickColor();
		void slot_lastSecondCleanup();
		void slot_editObject (QListWidgetItem* listitem);
};

//! Pointer to the instance of MainWindow.
extern MainWindow* g_win;

//! Get an icon by name from the resources directory.
QPixmap getIcon (QString iconName);

//! \returns a list of quick colors based on the configuration entry.
QList<LDQuickColor> quickColorsFromConfig();

//! Asks the user a yes/no question with the given \c message and the given
//! window \c title.
//! \returns true if the user answered yes, false if no.
bool confirm (QString title, QString message); // Generic confirm prompt

//! An overload of \c confirm(), this asks the user a yes/no question with the
//! given \c message.
//! \returns true if the user answered yes, false if no.
bool confirm (QString message);

//! Displays an error prompt with the given \c message
void critical (QString message);

//! Makes an icon of \c size x \c size pixels to represent \c colinfo
QIcon makeColorIcon (LDColor* colinfo, const int size);

//! Fills the given combo-box with color information
void makeColorComboBox (QComboBox* box);

//! \returns a QImage from the given raw GL \c data
QImage imageFromScreencap (uchar* data, int w, int h);

//!
//! Takes in pairs of radio buttons and respective values and finds the first
//! selected one.
//! \returns returns the value of the first found radio button that was checked
//! \returns by the user.
//!
template<class T>
T radioSwitch (const T& defval, QList<pair<QRadioButton*, T>> haystack)
{
	for (pair<QRadioButton*, const T&> i : haystack)
		if (i.first->isChecked())
			return i.second;

	return defval;
}

//!
//! Takes in pairs of radio buttons and respective values and checks the first
//! found radio button whose respsective value matches \c expr have the given value.
//!
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
