#include <QtGui>

#include "common.h"
#include "gldraw.h"
#include "gui.h"
#include "file.h"

#include "zz_setContentsDialog.h"

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
	
	setTitle ();
	setMinimumSize (320, 200);
	resize (800, 600);
}

#define MAKE_ACTION(OBJECT, DISPLAYNAME, IMAGENAME, DESCR) \
	qAct_##OBJECT = new QAction (QIcon ("./icons/" IMAGENAME ".png"), tr (DISPLAYNAME), this); \
	qAct_##OBJECT->setStatusTip (tr (DESCR)); \
	connect (qAct_##OBJECT, SIGNAL (triggered ()), this, SLOT (slot_##OBJECT ()));

void ForgeWindow::createMenuActions () {
	// Long menu names go here so my cool action definition table doesn't get out of proportions
	const char* sNewCdLineText = "New Conditional Line",
		*sNewQuadText = "New Quadrilateral",
		*sAboutText = "About " APPNAME_DISPLAY;
	
	MAKE_ACTION (new,			"&New",			"file-new",		"Create a new part model.")
	MAKE_ACTION (open,			"&Open",		"file-open",	"Load a part model from a file.")
	MAKE_ACTION (save,			"&Save",		"file-save",	"Save the part model.")
	MAKE_ACTION (saveAs,		"Save &As",		"file-save-as",	"Save the part to a specific file.")
	MAKE_ACTION (exit,			"&Exit",		"exit",			"Close " APPNAME_DISPLAY ".")
	
	MAKE_ACTION (cut,			"Cut",			"cut",			"Cut the current selection to clipboard.")
	MAKE_ACTION (copy,			"Copy",			"copy",			"Copy the current selection to clipboard.")
	MAKE_ACTION (paste,			"Paste",		"paste",		"Paste clipboard contents.")
	MAKE_ACTION (about,			sAboutText,		"about",		"Shows information about " APPNAME_DISPLAY ".")
	MAKE_ACTION (aboutQt,		"About Qt",		"aboutQt",		"Shows information about Qt.")
	
	MAKE_ACTION (inline,		"Inline",		"inline",		"Inline selected subfiles.")
	MAKE_ACTION (splitQuads,	"Split Quads",	"quad-split",	"Split quads into triangles.")
	MAKE_ACTION (setContents,	"Set Contents",	"set-contents",	"Set the raw code of this object.")
	
	MAKE_ACTION (newSubfile,	"New Subfile",	"add-subfile",	"Creates a new subfile reference.")
	MAKE_ACTION (newLine,		"New Line", 	"add-line",		"Creates a new line.")
	MAKE_ACTION (newTriangle,	"New Triangle", "add-triangle",	"Creates a new triangle.")
	MAKE_ACTION (newQuad,		sNewQuadText,	"add-quad",		"Creates a new quadrilateral.")
	MAKE_ACTION (newCondLine,	sNewCdLineText,	"add-condline",	"Creates a new conditional line.");
	MAKE_ACTION (newComment,	"New Comment",	"add-comment",	"Creates a new comment.");
	MAKE_ACTION (newVector,		"New Vector",	"add-vector",	"Creates a new vector.")
	MAKE_ACTION (newVertex,		"New Vertex",	"add-vertex",	"Creates a new vertex.")
	
	// Keyboard shortcuts
	qAct_new->setShortcut (Qt::CTRL | Qt::Key_N);
	qAct_open->setShortcut (Qt::CTRL | Qt::Key_O);
	qAct_save->setShortcut (Qt::CTRL | Qt::Key_S);
	qAct_saveAs->setShortcut (Qt::CTRL | Qt::SHIFT | Qt::Key_S);
	
	qAct_cut->setShortcut (Qt::CTRL | Qt::Key_X);
	qAct_copy->setShortcut (Qt::CTRL | Qt::Key_C);
	qAct_paste->setShortcut (Qt::CTRL | Qt::Key_V);
	
	// things not implemented yet
	QAction* qaDisabledActions[] = {
		qAct_saveAs,
		qAct_newSubfile,
		qAct_newTriangle,
		qAct_newQuad,
		qAct_newCondLine,
		qAct_newComment,
		qAct_newVector,
		qAct_newVertex,
		qAct_cut,
		qAct_copy,
		qAct_paste,
		qAct_about,
		qAct_inline,
	};
	
	for (ushort i = 0; i < sizeof qaDisabledActions / sizeof *qaDisabledActions; ++i)
		qaDisabledActions[i]->setEnabled (false);
}

void ForgeWindow::createMenus () {
	// File menu
	qFileMenu = menuBar ()->addMenu (tr ("&File"));
	qFileMenu->addAction (qAct_new);			// New
	qFileMenu->addAction (qAct_open);			// Open
	qFileMenu->addAction (qAct_save);			// Save
	qFileMenu->addAction (qAct_saveAs);			// Save As
	qFileMenu->addSeparator ();					// -------
	qFileMenu->addAction (qAct_exit);			// Exit
	
	// Edit menu
	qInsertMenu = menuBar ()->addMenu (tr ("&Insert"));
	qInsertMenu->addAction (qAct_newSubfile);	// New Subfile
	qInsertMenu->addAction (qAct_newLine);		// New Line
	qInsertMenu->addAction (qAct_newTriangle);	// New Triangle
	qInsertMenu->addAction (qAct_newQuad);		// New Quad
	qInsertMenu->addAction (qAct_newCondLine);	// New Conditional Line
	qInsertMenu->addAction (qAct_newComment);	// New Comment
	qInsertMenu->addAction (qAct_newVector);	// New Vector
	qInsertMenu->addAction (qAct_newVertex);	// New Vertex
	
	qEditMenu = menuBar ()->addMenu (tr ("&Edit"));
	qEditMenu->addAction (qAct_cut);			// Cut
	qEditMenu->addAction (qAct_copy);			// Copy
	qEditMenu->addAction (qAct_paste);			// Paste
	qEditMenu->addSeparator ();					// -----
	qEditMenu->addAction (qAct_inline);			// Inline
	qEditMenu->addAction (qAct_splitQuads);		// Split Quads
	qEditMenu->addAction (qAct_setContents);	// Set Contents
	
	// Help menu
	qHelpMenu = menuBar ()->addMenu (tr ("&Help"));
	qHelpMenu->addAction (qAct_about);			// About
	qHelpMenu->addAction (qAct_aboutQt);		// About Qt
}

void ForgeWindow::createToolbars () {
	qFileToolBar = new QToolBar ("File");
	qFileToolBar->addAction (qAct_new);
	qFileToolBar->addAction (qAct_open);
	qFileToolBar->addAction (qAct_save);
	qFileToolBar->addAction (qAct_saveAs);
	addToolBar (qFileToolBar);
	
	qInsertToolBar = new QToolBar ("Insert");
	qInsertToolBar->addAction (qAct_newSubfile);
	qInsertToolBar->addAction (qAct_newLine);
	qInsertToolBar->addAction (qAct_newTriangle);
	qInsertToolBar->addAction (qAct_newQuad);
	qInsertToolBar->addAction (qAct_newCondLine);
	qInsertToolBar->addAction (qAct_newComment);
	qInsertToolBar->addAction (qAct_newVector);
	qInsertToolBar->addAction (qAct_newVertex);
	addToolBar (qInsertToolBar);
	
	qEditToolBar = new QToolBar ("Edit");
	qEditToolBar->addAction (qAct_cut);
	qEditToolBar->addAction (qAct_copy);
	qEditToolBar->addAction (qAct_paste);
	qEditToolBar->addAction (qAct_inline);
	qEditToolBar->addAction (qAct_splitQuads);
	qEditToolBar->addAction (qAct_setContents);
	addToolBar (qEditToolBar);
}

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

void ForgeWindow::slot_new () {
	newFile ();
}

void ForgeWindow::slot_open () {
	str name = QFileDialog::getOpenFileName (this, "Open File",
		"", "LDraw files (*.dat *.ldr)").toStdString().c_str();
	
	openMainFile (name);
}

void ForgeWindow::slot_save () {
	g_CurrentFile->save ();
}

void ForgeWindow::slot_saveAs () {
	
}

void ForgeWindow::slot_exit () {
	exit (0);
}

void ForgeWindow::slot_newSubfile () {
	
}

void ForgeWindow::slot_newLine () {
	LDLine* line = new LDLine;
	const ulong ulSpot = getInsertionPoint ();
	
	memset (line->vaCoords, 0, sizeof line->vaCoords);
	line->dColor = 24;
	
	g_CurrentFile->objects.insert (g_CurrentFile->objects.begin() + ulSpot, line);
	
	buildObjList ();
	R->hardRefresh ();
}

void ForgeWindow::slot_newTriangle () {
	
}

void ForgeWindow::slot_newQuad () {
	
}

void ForgeWindow::slot_newCondLine () {

}

void ForgeWindow::slot_newComment () {

}

void ForgeWindow::slot_about () {
	
}

void ForgeWindow::slot_aboutQt () {
	QMessageBox::aboutQt (this);
}

void ForgeWindow::slot_cut () {

}

void ForgeWindow::slot_copy () {

}

void ForgeWindow::slot_paste () {
	
}

void ForgeWindow::slot_newVector () {
	
}

void ForgeWindow::slot_newVertex () {
	
}

void ForgeWindow::slot_inline () {

}

void ForgeWindow::slot_splitQuads () {
	if (qObjList->selectedItems().size() == 0)
		return;
	
	const QList<QTreeWidgetItem*> qaItems = qObjList->selectedItems();
	
	for (ulong i = 0; i < g_CurrentFile->objects.size(); ++i) {
		LDObject* obj = g_CurrentFile->objects[i];
		
		// Don't even consider non-quads
		if (obj->getType() != OBJ_Quad)
			continue;
		
		bool bIsSelected = false;
		
		for (long j = 0; j < qaItems.size(); ++j) {
			if (qaItems[j] == obj->qObjListEntry) {
				bIsSelected = true;
				break;
			}
		}
		
		if (!bIsSelected)
			continue; // Was not selected
		
		static_cast<LDQuad*> (obj)->splitToTriangles ();
		i++;// Skip past the second triangle
	}
	
	printf ("build obj list\n");
	buildObjList ();
	
	printf ("refresh teh renderer\n");
	R->hardRefresh ();
}

void ForgeWindow::slot_setContents () {
	if (qObjList->selectedItems().size() != 1)
		return;
	
	ulong ulIndex;
	LDObject* obj = nullptr;
	
	QTreeWidgetItem* item = qObjList->selectedItems()[0];
	for (ulIndex = 0; ulIndex < g_CurrentFile->objects.size(); ++ulIndex) {
		obj = g_CurrentFile->objects[ulIndex];
		
		if (obj->qObjListEntry == item)
			break;
	}
	
	if (ulIndex >= g_CurrentFile->objects.size())
		return;
	
	Dialog_SetContents::staticDialog (obj, this);
}

static QIcon IconForObjectType (LDObject* obj) {
	switch (obj->getType ()) {
	case OBJ_Empty:
		return QIcon ("icons/empty.png");
	
	case OBJ_Line:
		return QIcon ("icons/line.png");
	
	case OBJ_Quad:
		return QIcon ("icons/quad.png");
	
	case OBJ_Subfile:
		return QIcon ("icons/subfile.png");
	
	case OBJ_Triangle:
		return QIcon ("icons/triangle.png");
	
	case OBJ_CondLine:
		return QIcon ("icons/condline.png");
	
	case OBJ_Comment:
		return QIcon ("icons/comment.png");
	
	case OBJ_Vector:
		return QIcon ("icons/vector.png");
	
	case OBJ_Vertex:
		return QIcon ("icons/vertex.png");
	
	case OBJ_Gibberish:
	case OBJ_Unidentified:
		return QIcon ("icons/error.png");
	}
	
	return QIcon ();
}

void ForgeWindow::buildObjList () {
	if (!g_CurrentFile)
		return;
	
	QList<QTreeWidgetItem*> qaItems;
	
	qObjList->clear ();
	
	for (ulong i = 0; i < g_CurrentFile->objects.size(); ++i) {
		LDObject* obj = g_CurrentFile->objects[i];
		
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
		case OBJ_CondLine:
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
		
		case OBJ_Gibberish:
			zText.format ("ERROR: %s",
				static_cast<LDGibberish*> (obj)->zContents.chars());
			break;
		
		case OBJ_Vector:
			zText.format ("%s", static_cast<LDVector*> (obj)->vPos.getStringRep(true).chars());
			break;
		
		case OBJ_Vertex:
			zText.format ("%s", static_cast<LDVertex*> (obj)->vPosition.getStringRep(true).chars());
			break;
		
		default:
			zText = g_saObjTypeNames[obj->getType ()];
			break;
		}
		
		QTreeWidgetItem* item = new QTreeWidgetItem ((QTreeWidget*) (nullptr),
			QStringList (zText.chars()), 0);
		item->setIcon (0, IconForObjectType (obj));
		
		// Color gibberish red
		if (obj->getType() == OBJ_Gibberish) {
			item->setBackgroundColor (0, "#AA0000");
			item->setForeground (0, QColor ("#FFAA00"));
		}
		
		obj->qObjListEntry = item;
		
		qaItems.append (item);
	}
	
	qObjList->insertTopLevelItems (0, qaItems);
}

void ForgeWindow::slot_selectionChanged () {
	// If the selection isn't 1 exact, disable setting contents
	qAct_setContents->setEnabled (qObjList->selectedItems().size() == 1);
	
	// If we have no selection, disable splitting quads
	qAct_splitQuads->setEnabled (qObjList->selectedItems().size() > 0);
}

// =============================================================================
// ulong getInsertionPoint ()
// 
// Returns the index of where a new item should be placed at.
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