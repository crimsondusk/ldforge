#ifndef __GUI_H__
#define __GUI_H__

#include <QMainWindow>
#include <QMenu>
#include <QToolBar>
#include <QAction>
#include <QTreeWidget>
#include <QToolBar>
#include <QTextEdit>
#include "draw.h"

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
	QAction* qAct_cut, *qAct_copy, *qAct_paste;
	QAction* qAct_newSubfile, *qAct_newLine, *qAct_newTriangle, *qAct_newQuad;
	QAction* qAct_newCondLine, *qAct_newComment, *qAct_newVector, *qAct_newVertex;
	QAction* qAct_splitQuads, *qAct_setContents, *qAct_inline;
	QAction* qAct_about, *qAct_aboutQt;
	
	ForgeWindow ();
	void buildObjList ();
	void setTitle ();
	
	// Where would a new item be inserted into?
	ulong getInsertionPoint ();
	
private:
	void createMenuActions ();
	void createMenus ();
    void createToolbars ();

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
	void slot_newVector ();
	void slot_newVertex ();
	
	void slot_inline ();
	void slot_splitQuads ();
	void slot_setContents ();
	
	void slot_cut ();
	void slot_copy ();
	void slot_paste ();
	
	void slot_about ();
	void slot_aboutQt ();
};

enum {
	LDOLC_Icon,
	LDOLC_Data,
	NUM_LDOL_Columns
};

#endif