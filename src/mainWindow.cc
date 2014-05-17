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

#include <QGridLayout>
#include <QMessageBox>
#include <QEvent>
#include <QContextMenuEvent>
#include <QMenuBar>
#include <QStatusBar>
#include <QSplitter>
#include <QListWidget>
#include <QToolButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QToolBar>
#include <QProgressBar>
#include <QLabel>
#include <QFileDialog>
#include <QPushButton>
#include <QCoreApplication>
#include <QTimer>
#include <QMetaMethod>
#include "main.h"
#include "glRenderer.h"
#include "mainWindow.h"
#include "ldDocument.h"
#include "configuration.h"
#include "miscallenous.h"
#include "colors.h"
#include "editHistory.h"
#include "radioGroup.h"
#include "addObjectDialog.h"
#include "messageLog.h"
#include "configuration.h"
#include "ui_ldforge.h"

static bool g_isSelectionLocked = false;

CFGENTRY (Bool, colorizeObjectsList, true);
CFGENTRY (String, quickColorToolbar, "4:25:14:27:2:3:11:1:22:|:0:72:71:15");
CFGENTRY (Bool, listImplicitFiles, false);
EXTERN_CFGENTRY (List,		recentFiles);
EXTERN_CFGENTRY (Bool,		drawAxes);
EXTERN_CFGENTRY (String,		mainColor);
EXTERN_CFGENTRY (Float,		mainColorAlpha);
EXTERN_CFGENTRY (Bool,		drawWireframe);
EXTERN_CFGENTRY (Bool,		bfcRedGreenView);
EXTERN_CFGENTRY (Bool,		drawAngles);
EXTERN_CFGENTRY (Bool,		randomColors);

// =============================================================================
//
MainWindow::MainWindow (QWidget* parent, Qt::WindowFlags flags) :
	QMainWindow (parent, flags)
{
	g_win = this;
	ui = new Ui_LDForgeUI;
	ui->setupUi (this);
	m_updatingTabs = false;
	m_renderer = new GLRenderer (this);
	m_tabs = new QTabBar;
	ui->verticalLayout->insertWidget (0, m_tabs);

	// Stuff the renderer into its frame
	QVBoxLayout* rendererLayout = new QVBoxLayout (ui->rendererFrame);
	rendererLayout->addWidget (R());

	connect (ui->objectList, SIGNAL (itemSelectionChanged()), this, SLOT (slot_selectionChanged()));
	connect (ui->objectList, SIGNAL (itemDoubleClicked (QListWidgetItem*)), this, SLOT (slot_editObject (QListWidgetItem*)));
	connect (m_tabs, SIGNAL (currentChanged(int)), this, SLOT (changeCurrentFile()));

	m_msglog = new MessageManager;
	m_msglog->setRenderer (R());
	m_renderer->setMessageLog (m_msglog);
	m_quickColors = quickColorsFromConfig();
	slot_selectionChanged();
	setStatusBar (new QStatusBar);
	ui->actionAxes->setChecked (cfg::drawAxes);
	ui->actionWireframe->setChecked (cfg::drawWireframe);
	ui->actionBFCView->setChecked (cfg::bfcRedGreenView);
	ui->actionRandomColors->setChecked (cfg::randomColors);
	updateGridToolBar();
	updateEditModeActions();
	updateRecentFilesMenu();
	updateColorToolbar();
	updateTitle();
	updateActionShortcuts();
	setMinimumSize (300, 200);
	connect (qApp, SIGNAL (aboutToQuit()), this, SLOT (slot_lastSecondCleanup()));

	// Connect all actions
	for (QAction* act : findChildren<QAction*>())
		if (not act->objectName().isEmpty())
			connect (act, SIGNAL (triggered()), this, SLOT (slot_action()));
}

// =============================================================================
//
KeySequenceConfigEntry* MainWindow::shortcutForAction (QAction* action)
{
	String keycfgname = action->objectName() + "Shortcut";
	return KeySequenceConfigEntry::getByName (keycfgname);
}

// =============================================================================
//
void MainWindow::updateActionShortcuts()
{
	for (QAction* act : findChildren<QAction*>())
	{
		KeySequenceConfigEntry* cfg = shortcutForAction (act);

		if (cfg)
			act->setShortcut (cfg->getValue());
	}
}

// =============================================================================
//
void MainWindow::slot_action()
{
	// Get the name of the sender object and use it to compose the slot name,
	// then invoke this slot to call the action.
	QMetaObject::invokeMethod (this,
		qPrintable (format ("slot_%1", sender()->objectName())), Qt::DirectConnection);
	endAction();
}

// =============================================================================
//
void MainWindow::endAction()
{
	// Add a step in the history now.
	getCurrentDocument()->addHistoryStep();

	// Update the list item of the current file - we may need to draw an icon
	// now that marks it as having unsaved changes.
	updateDocumentListItem (getCurrentDocument());
}

// =============================================================================
//
void MainWindow::slot_lastSecondCleanup()
{
	delete m_renderer;
	delete ui;
}

// =============================================================================
//
void MainWindow::updateRecentFilesMenu()
{
	// First, clear any items in the recent files menu
for (QAction * recent : m_recentFiles)
		delete recent;

	m_recentFiles.clear();

	QAction* first = null;

	for (const QVariant& it : cfg::recentFiles)
	{
		String file = it.toString();
		QAction* recent = new QAction (getIcon ("open-recent"), file, this);

		connect (recent, SIGNAL (triggered()), this, SLOT (slot_recentFile()));
		ui->menuOpenRecent->insertAction (first, recent);
		m_recentFiles << recent;
		first = recent;
	}
}

// =============================================================================
//
QList<LDQuickColor> quickColorsFromConfig()
{
	QList<LDQuickColor> colors;

	for (String colorname : cfg::quickColorToolbar.split (":"))
	{
		if (colorname == "|")
			colors << LDQuickColor::getSeparator();
		else
		{
			LDColor* col = getColor (colorname.toLong());

			if (col != null)
				colors << LDQuickColor (col, null);
		}
	}

	return colors;
}

// =============================================================================
//
void MainWindow::updateColorToolbar()
{
	m_colorButtons.clear();
	ui->colorToolbar->clear();
	ui->colorToolbar->addAction (ui->actionUncolor);
	ui->colorToolbar->addSeparator();

	for (LDQuickColor& entry : m_quickColors)
	{
		if (entry.isSeparator())
			ui->colorToolbar->addSeparator();
		else
		{
			QToolButton* colorButton = new QToolButton;
			colorButton->setIcon (makeColorIcon (entry.color(), 16));
			colorButton->setIconSize (QSize (16, 16));
			colorButton->setToolTip (entry.color()->name);

			connect (colorButton, SIGNAL (clicked()), this, SLOT (slot_quickColor()));
			ui->colorToolbar->addWidget (colorButton);
			m_colorButtons << colorButton;

			entry.setToolButton (colorButton);
		}
	}

	updateGridToolBar();
}

// =============================================================================
//
void MainWindow::updateGridToolBar()
{
	// Ensure that the current grid - and only the current grid - is selected.
	ui->actionGridCoarse->setChecked (cfg::grid == Grid::Coarse);
	ui->actionGridMedium->setChecked (cfg::grid == Grid::Medium);
	ui->actionGridFine->setChecked (cfg::grid == Grid::Fine);
}

// =============================================================================
//
void MainWindow::updateTitle()
{
	String title = format (APPNAME " %1", fullVersionString());

	// Append our current file if we have one
	if (getCurrentDocument())
	{
		title += ": ";
		title += getCurrentDocument()->getDisplayName();

		if (getCurrentDocument()->getObjectCount() > 0 &&
			getCurrentDocument()->getObject (0)->type() == LDObject::EComment)
		{
			// Append title
			LDCommentPtr comm = getCurrentDocument()->getObject (0).staticCast<LDComment>();
			title += format (": %1", comm->text());
		}

		if (getCurrentDocument()->hasUnsavedChanges())
			title += '*';
	}

#ifdef DEBUG
	title += " [debug build]";
#elif BUILD_ID != BUILD_RELEASE
	title += " [pre-release build]";
#endif // DEBUG

	if (compileTimeString()[0] != '\0')
		title += format (" (built %1)", compileTimeString());

	setWindowTitle (title);
}

// =============================================================================
//
int MainWindow::deleteSelection()
{
	if (selection().isEmpty())
		return 0;

	LDObjectList selCopy = selection();

	// Delete the objects that were being selected
	for (LDObjectPtr obj : selCopy)
		obj->destroy();

	refresh();
	return selCopy.size();
}

// =============================================================================
//
void MainWindow::buildObjList()
{
	if (not getCurrentDocument())
		return;

	// Lock the selection while we do this so that refreshing the object list
	// doesn't trigger selection updating so that the selection doesn't get lost
	// while this is done.
	g_isSelectionLocked = true;

	for (int i = 0; i < ui->objectList->count(); ++i)
		delete ui->objectList->item (i);

	ui->objectList->clear();

	for (LDObjectPtr obj : getCurrentDocument()->objects())
	{
		String descr;

		switch (obj->type())
		{
			case LDObject::EComment:
			{
				descr = obj.staticCast<LDComment>()->text();

				// Remove leading whitespace
				while (descr[0] == ' ')
					descr.remove (0, 1);

				break;
			}

			case LDObject::EEmpty:
				break; // leave it empty

			case LDObject::ELine:
			case LDObject::ETriangle:
			case LDObject::EQuad:
			case LDObject::ECondLine:
			{
				for (int i = 0; i < obj->numVertices(); ++i)
				{
					if (i != 0)
						descr += ", ";

					descr += obj->vertex (i).toString (true);
				}
				break;
			}

			case LDObject::EError:
			{
				descr = format ("ERROR: %1", obj->asText());
				break;
			}

			case LDObject::EVertex:
			{
				descr = obj.staticCast<LDVertex>()->pos.toString (true);
				break;
			}

			case LDObject::ESubfile:
			{
				LDSubfilePtr ref = obj.staticCast<LDSubfile>();

				descr = format ("%1 %2, (", ref->fileInfo()->getDisplayName(), ref->position().toString (true));

				for (int i = 0; i < 9; ++i)
					descr += format ("%1%2", ref->transform()[i], (i != 8) ? " " : "");

				descr += ')';
				break;
			}

			case LDObject::EBFC:
			{
				descr = LDBFC::k_statementStrings[obj.staticCast<LDBFC>()->statement()];
				break;
			}

			case LDObject::EOverlay:
			{
				LDOverlayPtr ovl = obj.staticCast<LDOverlay>();
				descr = format ("[%1] %2 (%3, %4), %5 x %6", g_CameraNames[ovl->camera()],
					basename (ovl->fileName()), ovl->x(), ovl->y(),
					ovl->width(), ovl->height());
				break;
			}

			default:
			{
				descr = obj->typeName();
				break;
			}
		}

		QListWidgetItem* item = new QListWidgetItem (descr);
		item->setIcon (getIcon (obj->typeName()));

		// Use italic font if hidden
		if (obj->isHidden())
		{
			QFont font = item->font();
			font.setItalic (true);
			item->setFont (font);
		}

		// Color gibberish orange on red so it stands out.
		if (obj->type() == LDObject::EError)
		{
			item->setBackground (QColor ("#AA0000"));
			item->setForeground (QColor ("#FFAA00"));
		}
		elif (cfg::colorizeObjectsList && obj->isColored() &&
			obj->color() != maincolor && obj->color() != edgecolor)
		{
			// If the object isn't in the main or edge color, draw this
			// list entry in said color.
			LDColor* col = getColor (obj->color());

			if (col)
				item->setForeground (col->faceColor);
		}

		obj->qObjListEntry = item;
		ui->objectList->insertItem (ui->objectList->count(), item);
	}

	g_isSelectionLocked = false;
	updateSelection();
	scrollToSelection();
}

// =============================================================================
//
void MainWindow::scrollToSelection()
{
	if (selection().isEmpty())
		return;

	LDObjectPtr obj = selection().last();
	ui->objectList->scrollToItem (obj->qObjListEntry);
}

// =============================================================================
//
void MainWindow::slot_selectionChanged()
{
	if (g_isSelectionLocked == true || getCurrentDocument() == null)
		return;

	LDObjectList priorSelection = selection();

	// Get the objects from the object list selection
	getCurrentDocument()->clearSelection();
	const QList<QListWidgetItem*> items = ui->objectList->selectedItems();

	for (LDObjectPtr obj : getCurrentDocument()->objects())
	{
		for (QListWidgetItem* item : items)
		{
			if (item == obj->qObjListEntry)
			{
				obj->select();
				break;
			}
		}
	}

	// Update the GL renderer
	LDObjectList compound = priorSelection + selection();
	removeDuplicates (compound);

	for (LDObjectPtr obj : compound)
		R()->compileObject (obj);

	R()->update();
}

// =============================================================================
//
void MainWindow::slot_recentFile()
{
	QAction* qAct = static_cast<QAction*> (sender());
	openMainFile (qAct->text());
}

// =============================================================================
//
void MainWindow::slot_quickColor()
{
	QToolButton* button = static_cast<QToolButton*> (sender());
	LDColor* col = null;

	for (const LDQuickColor& entry : m_quickColors)
	{
		if (entry.toolButton() == button)
		{
			col = entry.color();
			break;
		}
	}

	if (col == null)
		return;

	int newColor = col->index;

	for (LDObjectPtr obj : selection())
	{
		if (not obj->isColored())
			continue; // uncolored object

		obj->setColor (newColor);
		R()->compileObject (obj);
	}

	endAction();
	refresh();
}

// =============================================================================
//
int MainWindow::getInsertionPoint()
{
	// If we have a selection, put the item after it.
	if (not selection().isEmpty())
		return selection().last()->lineNumber() + 1;

	// Otherwise place the object at the end.
	return getCurrentDocument()->getObjectCount();
}

// =============================================================================
//
void MainWindow::doFullRefresh()
{
	buildObjList();
	m_renderer->hardRefresh();
}

// =============================================================================
//
void MainWindow::refresh()
{
	buildObjList();
	m_renderer->update();
}

// =============================================================================
//
void MainWindow::updateSelection()
{
	g_isSelectionLocked = true;

	ui->objectList->clearSelection();

	for (LDObjectPtr obj : selection())
	{
		if (obj->qObjListEntry == null)
			continue;

		obj->qObjListEntry->setSelected (true);
	}

	g_isSelectionLocked = false;
}

// =============================================================================
//
int MainWindow::getSelectedColor()
{
	int result = -1;

	for (LDObjectPtr obj : selection())
	{
		if (not obj->isColored())
			continue; // doesn't use color

		if (result != -1 && obj->color() != result)
			return -1; // No consensus in object color

		if (result == -1)
			result = obj->color();
	}

	return result;
}

// =============================================================================
//
LDObject::Type MainWindow::getUniformSelectedType()
{
	LDObject::Type result = LDObject::EUnidentified;

	for (LDObjectPtr obj : selection())
	{
		if (result != LDObject::EUnidentified && obj->color() != result)
			return LDObject::EUnidentified;

		if (result == LDObject::EUnidentified)
			result = obj->type();
	}

	return result;
}

// =============================================================================
//
void MainWindow::closeEvent (QCloseEvent* ev)
{
	// Check whether it's safe to close all files.
	if (not safeToCloseAll())
	{
		ev->ignore();
		return;
	}

	// Save the configuration before leaving so that, for instance, grid choice
	// is preserved across instances.
	Config::save();

	ev->accept();
}

// =============================================================================
//
void MainWindow::spawnContextMenu (const QPoint pos)
{
	const bool single = (selection().size() == 1);
	LDObjectPtr singleObj = single ? selection().first() : LDObjectPtr();

	QMenu* contextMenu = new QMenu;

	if (single && singleObj->type() != LDObject::EEmpty)
	{
		contextMenu->addAction (ui->actionEdit);
		contextMenu->addSeparator();
	}

	contextMenu->addAction (ui->actionCut);
	contextMenu->addAction (ui->actionCopy);
	contextMenu->addAction (ui->actionPaste);
	contextMenu->addAction (ui->actionDelete);
	contextMenu->addSeparator();
	contextMenu->addAction (ui->actionSetColor);

	if (single)
		contextMenu->addAction (ui->actionEditRaw);

	contextMenu->addAction (ui->actionBorders);
	contextMenu->addAction (ui->actionSetOverlay);
	contextMenu->addAction (ui->actionClearOverlay);
	contextMenu->addAction (ui->actionModeSelect);
	contextMenu->addAction (ui->actionModeDraw);
	contextMenu->addAction (ui->actionModeCircle);

	if (not selection().isEmpty())
	{
		contextMenu->addSeparator();
		contextMenu->addAction (ui->actionSubfileSelection);
	}

	if (R()->camera() != EFreeCamera)
	{
		contextMenu->addSeparator();
		contextMenu->addAction (ui->actionSetDrawDepth);
	}

	contextMenu->exec (pos);
}

// =============================================================================
//
void MainWindow::deleteByColor (int colnum)
{
	LDObjectList objs;

	for (LDObjectPtr obj : getCurrentDocument()->objects())
	{
		if (not obj->isColored() || obj->color() != colnum)
			continue;

		objs << obj;
	}

	for (LDObjectPtr obj : objs)
		obj->destroy();
}

// =============================================================================
//
void MainWindow::updateEditModeActions()
{
	const EditMode mode = R()->editMode();
	ui->actionModeSelect->setChecked (mode == ESelectMode);
	ui->actionModeDraw->setChecked (mode == EDrawMode);
	ui->actionModeCircle->setChecked (mode == ECircleMode);
}

// =============================================================================
//
void MainWindow::slot_editObject (QListWidgetItem* listitem)
{
	for (LDObjectPtr it : getCurrentDocument()->objects())
	{
		if (it->qObjListEntry == listitem)
		{
			AddObjectDialog::staticDialog (it->type(), it);
			break;
		}
	}
}

// =============================================================================
//
bool MainWindow::save (LDDocument* doc, bool saveAs)
{
	String path = doc->fullPath();

	if (saveAs || path.isEmpty())
	{
		String name = doc->defaultName();

		if (not doc->fullPath().isEmpty()) 
			name = doc->fullPath();
		elif (not doc->name().isEmpty())
			name = doc->name();

		name.replace ("\\", "/");
		path = QFileDialog::getSaveFileName (g_win, tr ("Save As"),
			name, tr ("LDraw files (*.dat *.ldr)"));

		if (path.isEmpty())
		{
			// User didn't give a file name, abort.
			return false;
		}
	}

	if (doc->save (path))
	{
		if (doc == getCurrentDocument())
			updateTitle();

		print ("Saved to %1.", path);

		// Add it to recent files
		addRecentFile (path);
		return true;
	}

	String message = format (tr ("Failed to save to %1: %2"), path, strerror (errno));

	// Tell the user the save failed, and give the option for saving as with it.
	QMessageBox dlg (QMessageBox::Critical, tr ("Save Failure"), message, QMessageBox::Close, g_win);

	// Add a save-as button
	QPushButton* saveAsBtn = new QPushButton (tr ("Save As"));
	saveAsBtn->setIcon (getIcon ("file-save-as"));
	dlg.addButton (saveAsBtn, QMessageBox::ActionRole);
	dlg.setDefaultButton (QMessageBox::Close);
	dlg.exec();

	if (dlg.clickedButton() == saveAsBtn)
		return save (doc, true); // yay recursion!

	return false;
}

void MainWindow::addMessage (String msg)
{
	m_msglog->addLine (msg);
}

// ============================================================================
void ObjectList::contextMenuEvent (QContextMenuEvent* ev)
{
	g_win->spawnContextMenu (ev->globalPos());
}

// =============================================================================
//
QPixmap getIcon (String iconName)
{
	return (QPixmap (format (":/icons/%1.png", iconName)));
}

// =============================================================================
//
bool confirm (const String& message)
{
	return confirm (MainWindow::tr ("Confirm"), message);
}

// =============================================================================
//
bool confirm (const String& title, const String& message)
{
	return QMessageBox::question (g_win, title, message,
		(QMessageBox::Yes | QMessageBox::No), QMessageBox::No) == QMessageBox::Yes;
}

// =============================================================================
//
void critical (const String& message)
{
	QMessageBox::critical (g_win, MainWindow::tr ("Error"), message,
		(QMessageBox::Close), QMessageBox::Close);
}

// =============================================================================
//
QIcon makeColorIcon (LDColor* colinfo, const int size)
{
	// Create an image object and link a painter to it.
	QImage img (size, size, QImage::Format_ARGB32);
	QPainter paint (&img);
	QColor col = colinfo->faceColor;

	if (colinfo->index == maincolor)
	{
		// Use the user preferences for main color here
		col = cfg::mainColor;
		col.setAlphaF (cfg::mainColorAlpha);
	}

	// Paint the icon border
	paint.fillRect (QRect (0, 0, size, size), colinfo->edgeColor);

	// Paint the checkerboard background, visible with translucent icons
	paint.drawPixmap (QRect (1, 1, size - 2, size - 2), getIcon ("checkerboard"), QRect (0, 0, 8, 8));

	// Paint the color above the checkerboard
	paint.fillRect (QRect (1, 1, size - 2, size - 2), col);
	return QIcon (QPixmap::fromImage (img));
}

// =============================================================================
//
void makeColorComboBox (QComboBox* box)
{
	std::map<int, int> counts;

	for (LDObjectPtr obj : getCurrentDocument()->objects())
	{
		if (not obj->isColored())
			continue;

		if (counts.find (obj->color()) == counts.end())
			counts[obj->color()] = 1;
		else
			counts[obj->color()]++;
	}

	box->clear();
	int row = 0;

	for (const auto& pair : counts)
	{
		LDColor* col = getColor (pair.first);
		assert (col != null);

		QIcon ico = makeColorIcon (col, 16);
		box->addItem (ico, format ("[%1] %2 (%3 object%4)",
			pair.first, col->name, pair.second, plural (pair.second)));
		box->setItemData (row, pair.first);

		++row;
	}
}

// =============================================================================
//
void MainWindow::updateDocumentList()
{
	m_updatingTabs = true;

	while (m_tabs->count() > 0)
		m_tabs->removeTab (0);

	for (LDDocument* f : g_loadedFiles)
	{
		// Don't list implicit files unless explicitly desired.
		if (f->isImplicit() && not cfg::listImplicitFiles)
			continue;

		// Add an item to the list for this file and store the tab index
		// in the document so we can find documents by tab index.
		f->setTabIndex (m_tabs->addTab (""));
		updateDocumentListItem (f);
	}

	m_updatingTabs = false;
}

// =============================================================================
//
void MainWindow::updateDocumentListItem (LDDocument* doc)
{
	bool oldUpdatingTabs = m_updatingTabs;
	m_updatingTabs = true;

	if (doc->tabIndex() == -1)
	{
		// We don't have a list item for this file, so the list either doesn't
		// exist yet or is out of date. Build the list now.
		updateDocumentList();
		return;
	}

	// If this is the current file, it also needs to be the selected item on
	// the list.
	if (doc == getCurrentDocument())
		m_tabs->setCurrentIndex (doc->tabIndex());

	m_tabs->setTabText (doc->tabIndex(), doc->getDisplayName());

	// If the document.has unsaved changes, draw a little icon next to it to mark that.
	m_tabs->setTabIcon (doc->tabIndex(), doc->hasUnsavedChanges() ? getIcon ("file-save") : QIcon());
	m_updatingTabs = oldUpdatingTabs;
}

// =============================================================================
//
// A file is selected from the list of files on the left of the screen. Find out
// which file was picked and change to it.
//
void MainWindow::changeCurrentFile()
{
	if (m_updatingTabs)
		return;

	LDDocument* f = null;
	int tabIndex = m_tabs->currentIndex();

	// Find the file pointer of the item that was selected.
	for (LDDocument* it : g_loadedFiles)
	{
		if (it->tabIndex() == tabIndex)
		{
			f = it;
			break;
		}
	}

	// If we picked the same file we're currently on, we don't need to do
	// anything.
	if (f == null || f == getCurrentDocument())
		return;

	LDDocument::setCurrent (f);
}

// =============================================================================
//
void MainWindow::refreshObjectList()
{
#if 0
	ui->objectList->clear();
	LDDocument* f = getCurrentDocument();

for (LDObjectPtr obj : *f)
		ui->objectList->addItem (obj->qObjListEntry);

#endif

	buildObjList();
}

// =============================================================================
//
void MainWindow::updateActions()
{
	History* his = getCurrentDocument()->history();
	int pos = his->position();
	ui->actionUndo->setEnabled (pos != -1);
	ui->actionRedo->setEnabled (pos < (long) his->getSize() - 1);
	ui->actionAxes->setChecked (cfg::drawAxes);
	ui->actionBFCView->setChecked (cfg::bfcRedGreenView);
	ui->actionRandomColors->setChecked (cfg::randomColors);
	ui->actionDrawAngles->setChecked (cfg::drawAngles);
}

// =============================================================================
//
QImage imageFromScreencap (uchar* data, int w, int h)
{
	// GL and Qt formats have R and B swapped. Also, GL flips Y - correct it as well.
	return QImage (data, w, h, QImage::Format_ARGB32).rgbSwapped().mirrored();
}

// =============================================================================
//
LDQuickColor::LDQuickColor (LDColor* color, QToolButton* toolButton) :
	m_color (color),
	m_toolButton (toolButton) {}

// =============================================================================
//
LDQuickColor LDQuickColor::getSeparator()
{
	return LDQuickColor (null, null);
}

// =============================================================================
//
bool LDQuickColor::isSeparator() const
{
	return color() == null;
}
