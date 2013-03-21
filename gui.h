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

#ifndef __GUI_H__
#define __GUI_H__

#include <QMainWindow>
#include <QMenu>
#include <QToolBar>
#include <QAction>
#include <QTreeWidget>
#include <QToolBar>
#include <QTextEdit>
#include "gldraw.h"

// Stuff for dialogs
#define IMPLEMENT_DIALOG_BUTTONS \
	qButtons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel); \
	connect (qButtons, SIGNAL (accepted ()), this, SLOT (accept ())); \
	connect (qButtons, SIGNAL (rejected ()), this, SLOT (reject ())); \


class ForgeWindow : public QMainWindow {
	Q_OBJECT
	
public:
	renderer* R;
	
	// Object list view
	QTreeWidget* qObjList;
	
	// Message log
	QTextEdit* qMessageLog;
	str zMessageLogHTML;
	
	// Menus
	QMenu* qFileMenu, *qEditMenu, *qInsertMenu, *qHelpMenu;
	
	// Toolbars
	QToolBar* qFileToolBar, *qEditToolBar, *qInsertToolBar;
	
	// ACTION ARMADA
	QAction* qAct_new, *qAct_open, *qAct_save, *qAct_saveAs, *qAct_exit;
	QAction* qAct_cut, *qAct_copy, *qAct_paste, *qAct_delete;
	QAction* qAct_newSubfile, *qAct_newLine, *qAct_newTriangle, *qAct_newQuad;
	QAction* qAct_newCondLine, *qAct_newComment, *qAct_newVertex;
	QAction* qAct_splitQuads, *qAct_setContents, *qAct_inline, *qAct_makeBorders;
	QAction* qAct_settings;
	QAction* qAct_help, *qAct_about, *qAct_aboutQt;
	QAction* qAct_setColor;
	
	ForgeWindow ();
	void buildObjList ();
	void setTitle ();
	void refresh ();
	std::vector<LDObject*> getSelectedObjects ();
	
	// Where would a new item be inserted into?
	ulong getInsertionPoint ();
	
private:
	void createMenuActions ();
	void createMenus ();
    void createToolbars ();
	bool copyToClipboard ();
	void deleteSelection ();

private slots:
	void slot_selectionChanged ();
	
	void slot_new ();
	void slot_open ();
	void slot_save ();
	void slot_saveAs ();
	void slot_exit ();
	
	void slot_newSubfile ();
	void slot_newLine ();
	void slot_newTriangle ();
	void slot_newQuad ();
	void slot_newCondLine ();
	void slot_newComment ();
	void slot_newVertex ();
	
	void slot_inline ();
	void slot_splitQuads ();
	void slot_setContents ();
	void slot_makeBorders ();
	
	void slot_cut ();
	void slot_copy ();
	void slot_paste ();
	void slot_delete ();
	
	void slot_settings ();
	
	void slot_help ();
	void slot_about ();
	void slot_aboutQt ();
	
	void slot_setColor ();
};

enum {
	LDOLC_Icon,
	LDOLC_Data,
	NUM_LDOL_Columns
};

#endif