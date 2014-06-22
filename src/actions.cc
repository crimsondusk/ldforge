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

#include <QFileDialog>
#include <QMessageBox>
#include <QTextEdit>
#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QInputDialog>

#include "mainWindow.h"
#include "ldDocument.h"
#include "editHistory.h"
#include "configDialog.h"
#include "addObjectDialog.h"
#include "miscallenous.h"
#include "glRenderer.h"
#include "dialogs.h"
#include "primitives.h"
#include "radioGroup.h"
#include "colors.h"
#include "glCompiler.h"
#include "ui_newpart.h"

EXTERN_CFGENTRY (Bool,		drawWireframe);
EXTERN_CFGENTRY (Bool,		bfcRedGreenView);
EXTERN_CFGENTRY (String,	defaultName);
EXTERN_CFGENTRY (String,	defaultUser);
EXTERN_CFGENTRY (Int,		defaultLicense);
EXTERN_CFGENTRY (Bool,		drawAngles);
EXTERN_CFGENTRY (Bool,		randomColors)

// =============================================================================
//
void MainWindow::slot_actionNew()
{
	QDialog* dlg = new QDialog (g_win);
	Ui::NewPartUI ui;
	ui.setupUi (dlg);

	QString authortext = cfg::defaultName;

	if (not cfg::defaultUser.isEmpty())
		authortext.append (format (" [%1]", cfg::defaultUser));

	ui.le_author->setText (authortext);

	switch (cfg::defaultLicense)
	{
		case 0:
			ui.rb_license_ca->setChecked (true);
			break;

		case 1:
			ui.rb_license_nonca->setChecked (true);
			break;

		case 2:
			ui.rb_license_none->setChecked (true);
			break;

		default:
			QMessageBox::warning (null, "Warning",
				format ("Unknown ld_defaultlicense value %1!", cfg::defaultLicense));
			break;
	}

	if (dlg->exec() == QDialog::Rejected)
		return;

	newFile();

	const LDBFC::Statement bfctype =
		ui.rb_bfc_ccw->isChecked() ? LDBFC::CertifyCCW :
		ui.rb_bfc_cw->isChecked()  ? LDBFC::CertifyCW : LDBFC::NoCertify;

	const QString license =
		ui.rb_license_ca->isChecked()    ? g_CALicense :
		ui.rb_license_nonca->isChecked() ? g_nonCALicense : "";

	LDObjectList objs;
	objs << spawn<LDComment> (ui.le_title->text());
	objs << spawn<LDComment> ("Name: <untitled>.dat");
	objs << spawn<LDComment> (format ("Author: %1", ui.le_author->text()));
	objs << spawn<LDComment> (format ("!LDRAW_ORG Unofficial_Part"));

	if (not license.isEmpty())
		objs << spawn<LDComment> (license);

	objs << spawn<LDEmpty>();
	objs << spawn<LDBFC> (bfctype);
	objs << spawn<LDEmpty>();

	getCurrentDocument()->addObjects (objs);

	doFullRefresh();
}

// =============================================================================
//
void MainWindow::slot_actionNewFile()
{
	newFile();
}

// =============================================================================
//
void MainWindow::slot_actionOpen()
{
	QString name = QFileDialog::getOpenFileName (g_win, "Open File", "", "LDraw files (*.dat *.ldr)");

	if (name.isEmpty())
		return;

	openMainFile (name);
}

// =============================================================================
//
void MainWindow::slot_actionSave()
{
	save (getCurrentDocument(), false);
}

// =============================================================================
//
void MainWindow::slot_actionSaveAs()
{
	save (getCurrentDocument(), true);
}

// =============================================================================
//
void MainWindow::slot_actionSaveAll()
{
	for (LDDocumentPtr file : LDDocument::explicitDocuments())
		save (file, false);
}

// =============================================================================
//
void MainWindow::slot_actionClose()
{
	if (not getCurrentDocument()->isSafeToClose())
		return;

	getCurrentDocument()->dismiss();
}

// =============================================================================
//
void MainWindow::slot_actionCloseAll()
{
	if (not safeToCloseAll())
		return;

	closeAll();
}

// =============================================================================
//
void MainWindow::slot_actionSettings()
{
	(new ConfigDialog)->exec();
}

// =============================================================================
//
void MainWindow::slot_actionSetLDrawPath()
{
	(new LDrawPathDialog (true))->exec();
}

// =============================================================================
//
void MainWindow::slot_actionExit()
{
	exit (0);
}

// =============================================================================
//
void MainWindow::slot_actionNewSubfile()
{
	AddObjectDialog::staticDialog (OBJ_Subfile, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::slot_actionNewLine()
{
	AddObjectDialog::staticDialog (OBJ_Line, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::slot_actionNewTriangle()
{
	AddObjectDialog::staticDialog (OBJ_Triangle, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::slot_actionNewQuad()
{
	AddObjectDialog::staticDialog (OBJ_Quad, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::slot_actionNewCLine()
{
	AddObjectDialog::staticDialog (OBJ_CondLine, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::slot_actionNewComment()
{
	AddObjectDialog::staticDialog (OBJ_Comment, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::slot_actionNewBFC()
{
	AddObjectDialog::staticDialog (OBJ_BFC, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::slot_actionNewVertex()
{
	AddObjectDialog::staticDialog (OBJ_Vertex, LDObjectPtr());
}

// =============================================================================
//
void MainWindow::slot_actionEdit()
{
	if (selection().size() != 1)
		return;

	LDObjectPtr obj = selection() [0];
	AddObjectDialog::staticDialog (obj->type(), obj);
}

// =============================================================================
//
void MainWindow::slot_actionHelp()
{
}

// =============================================================================
//
void MainWindow::slot_actionAbout()
{
	AboutDialog().exec();
}

// =============================================================================
//
void MainWindow::slot_actionAboutQt()
{
	QMessageBox::aboutQt (g_win);
}

// =============================================================================
//
void MainWindow::slot_actionSelectAll()
{
	for (LDObjectPtr obj : getCurrentDocument()->objects())
		obj->select();

	ui->objectList->selectAll();
	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionSelectByColor()
{
	if (selection().isEmpty())
		return;

	QList<LDColor> colors;

	for (LDObjectPtr obj : selection())
	{
		if (obj->isColored())
			colors << obj->color();
	}

	removeDuplicates (colors);
	getCurrentDocument()->clearSelection();

	for (LDObjectPtr obj : getCurrentDocument()->objects())
	{
		if (colors.contains (obj->color()))
			obj->select();
	}

	updateSelection();
	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionSelectByType()
{
	if (selection().isEmpty())
		return;

	QList<LDObjectType> types;
	QStringList subfilenames;

	for (LDObjectPtr obj : selection())
	{
		types << obj->type();

		if (types.last() == OBJ_Subfile)
			subfilenames << obj.staticCast<LDSubfile>()->fileInfo()->name();
	}

	removeDuplicates (types);
	removeDuplicates (subfilenames);
	getCurrentDocument()->clearSelection();

	for (LDObjectPtr obj : getCurrentDocument()->objects())
	{
		LDObjectType type = obj->type();

		if (not types.contains (type))
			continue;

		// For subfiles, type check is not enough, we check the name of the document as well.
		if (type == OBJ_Subfile && not subfilenames.contains (obj.staticCast<LDSubfile>()->fileInfo()->name()))
			continue;

		obj->select();
	}

	updateSelection();
	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionGridCoarse()
{
	cfg::grid = Grid::Coarse;
	updateGridToolBar();
}

void MainWindow::slot_actionGridMedium()
{
	cfg::grid = Grid::Medium;
	updateGridToolBar();
}

void MainWindow::slot_actionGridFine()
{
	cfg::grid = Grid::Fine;
	updateGridToolBar();
}

// =============================================================================
//
void MainWindow::slot_actionResetView()
{
	R()->resetAngles();
	R()->update();
}

// =============================================================================
//
void MainWindow::slot_actionInsertFrom()
{
	QString fname = QFileDialog::getOpenFileName();
	int idx = getInsertionPoint();

	if (not fname.length())
		return;

	QFile f (fname);

	if (not f.open (QIODevice::ReadOnly))
	{
		critical (format ("Couldn't open %1 (%2)", fname, f.errorString()));
		return;
	}

	LDObjectList objs = loadFileContents (&f, null);

	getCurrentDocument()->clearSelection();

	for (LDObjectPtr obj : objs)
	{
		getCurrentDocument()->insertObj (idx, obj);
		obj->select();
		R()->compileObject (obj);

		idx++;
	}

	refresh();
	scrollToSelection();
}

// =============================================================================
//
void MainWindow::slot_actionExportTo()
{
	if (selection().isEmpty())
		return;

	QString fname = QFileDialog::getSaveFileName();

	if (fname.length() == 0)
		return;

	QFile file (fname);

	if (not file.open (QIODevice::WriteOnly | QIODevice::Text))
	{
		critical (format ("Unable to open %1 for writing (%2)", fname, file.errorString()));
		return;
	}

	for (LDObjectPtr obj : selection())
	{
		QString contents = obj->asText();
		QByteArray data = contents.toUtf8();
		file.write (data, data.size());
		file.write ("\r\n", 2);
	}
}

// =============================================================================
//
void MainWindow::slot_actionInsertRaw()
{
	int idx = getInsertionPoint();

	QDialog* const dlg = new QDialog;
	QVBoxLayout* const layout = new QVBoxLayout;
	QTextEdit* const te_edit = new QTextEdit;
	QDialogButtonBox* const bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	layout->addWidget (te_edit);
	layout->addWidget (bbx_buttons);
	dlg->setLayout (layout);
	dlg->setWindowTitle (APPNAME " - Insert Raw");
	dlg->connect (bbx_buttons, SIGNAL (accepted()), dlg, SLOT (accept()));
	dlg->connect (bbx_buttons, SIGNAL (rejected()), dlg, SLOT (reject()));

	if (dlg->exec() == QDialog::Rejected)
		return;

	getCurrentDocument()->clearSelection();

	for (QString line : QString (te_edit->toPlainText()).split ("\n"))
	{
		LDObjectPtr obj = parseLine (line);

		getCurrentDocument()->insertObj (idx, obj);
		obj->select();
		idx++;
	}

	refresh();
	scrollToSelection();
}

// =============================================================================
//
void MainWindow::slot_actionScreenshot()
{
	setlocale (LC_ALL, "C");

	int w, h;
	uchar* imgdata = R()->getScreencap (w, h);
	QImage img = imageFromScreencap (imgdata, w, h);

	QString root = basename (getCurrentDocument()->name());

	if (root.right (4) == ".dat")
		root.chop (4);

	QString defaultname = (root.length() > 0) ? format ("%1.png", root) : "";
	QString fname = QFileDialog::getSaveFileName (g_win, "Save Screencap", defaultname,
				"PNG images (*.png);;JPG images (*.jpg);;BMP images (*.bmp);;All Files (*.*)");

	if (not fname.isEmpty() && not img.save (fname))
		critical (format ("Couldn't open %1 for writing to save screencap: %2", fname, strerror (errno)));

	delete[] imgdata;
}

// =============================================================================
//
EXTERN_CFGENTRY (Bool, drawAxes);
void MainWindow::slot_actionAxes()
{
	cfg::drawAxes = not cfg::drawAxes;
	updateActions();
	R()->update();
}

// =============================================================================
//
void MainWindow::slot_actionVisibilityToggle()
{
	for (LDObjectPtr obj : selection())
		obj->setHidden (not obj->isHidden());

	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionVisibilityHide()
{
	for (LDObjectPtr obj : selection())
		obj->setHidden (true);

	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionVisibilityReveal()
{
	for (LDObjectPtr obj : selection())
	obj->setHidden (false);
	refresh();
}

// =============================================================================
//
void MainWindow::slot_actionWireframe()
{
	cfg::drawWireframe = not cfg::drawWireframe;
	R()->refresh();
}

// =============================================================================
//
void MainWindow::slot_actionSetOverlay()
{
	OverlayDialog dlg;

	if (not dlg.exec())
		return;

	R()->setupOverlay ((ECamera) dlg.camera(), dlg.fpath(), dlg.ofsx(),
		dlg.ofsy(), dlg.lwidth(), dlg.lheight());
}

// =============================================================================
//
void MainWindow::slot_actionClearOverlay()
{
	R()->clearOverlay();
}

// =============================================================================
//
void MainWindow::slot_actionModeSelect()
{
	R()->setEditMode (ESelectMode);
}

// =============================================================================
//
void MainWindow::slot_actionModeDraw()
{
	R()->setEditMode (EDrawMode);
}

// =============================================================================
//
void MainWindow::slot_actionModeCircle()
{
	R()->setEditMode (ECircleMode);
}

// =============================================================================
//
void MainWindow::slot_actionDrawAngles()
{
	cfg::drawAngles = not cfg::drawAngles;
	R()->refresh();
}

// =============================================================================
//
void MainWindow::slot_actionSetDrawDepth()
{
	if (R()->camera() == EFreeCamera)
		return;

	bool ok;
	double depth = QInputDialog::getDouble (g_win, "Set Draw Depth",
											format ("Depth value for %1 Camera:", R()->getCameraName()),
											R()->getDepthValue(), -10000.0f, 10000.0f, 3, &ok);

	if (ok)
		R()->setDepthValue (depth);
}

#if 0
// This is a test to draw a dummy axle. Meant to be used as a primitive gallery,
// but I can't figure how to generate these pictures properly. Multi-threading
// these is an immense pain.
void MainWindow::slot_actiontestpic()
{
	LDDocumentPtr file = getFile ("axle.dat");
	setlocale (LC_ALL, "C");

	if (not file)
	{
		critical ("couldn't load axle.dat");
		return;
	}

	int w, h;

	GLRenderer* rend = new GLRenderer;
	rend->resize (64, 64);
	rend->setAttribute (Qt::WA_DontShowOnScreen);
	rend->show();
	rend->setFile (file);
	rend->setDrawOnly (true);
	rend->compileAllObjects();
	rend->initGLData();
	rend->drawGLScene();

	uchar* imgdata = rend->screencap (w, h);
	QImage img = imageFromScreencap (imgdata, w, h);

	if (img.isNull())
	{
		critical ("Failed to create the image!\n");
	}
	else
	{
		QLabel* label = new QLabel;
		QDialog* dlg = new QDialog;
		label->setPixmap (QPixmap::fromImage (img));
		QVBoxLayout* layout = new QVBoxLayout (dlg);
		layout->addWidget (label);
		dlg->exec();
	}

	delete[] imgdata;
	rend->deleteLater();
}
#endif

// =============================================================================
//
void MainWindow::slot_actionScanPrimitives()
{
	PrimitiveScanner::start();
}

// =============================================================================
//
void MainWindow::slot_actionBFCView()
{
	cfg::bfcRedGreenView = not cfg::bfcRedGreenView;

	if (cfg::bfcRedGreenView)
		cfg::randomColors = false;

	updateActions();
	R()->refresh();
}

// =============================================================================
//
void MainWindow::slot_actionJumpTo()
{
	bool ok;
	int defval = 0;
	LDObjectPtr obj;

	if (selection().size() == 1)
		defval = selection()[0]->lineNumber();

	int idx = QInputDialog::getInt (null, "Go to line", "Go to line:", defval,
		1, getCurrentDocument()->getObjectCount(), 1, &ok);

	if (not ok || (obj = getCurrentDocument()->getObject (idx - 1)) == null)
		return;

	getCurrentDocument()->clearSelection();
	obj->select();
	updateSelection();
}

// =============================================================================
//
void MainWindow::slot_actionSubfileSelection()
{
	if (selection().size() == 0)
		return;

	QString			parentpath = getCurrentDocument()->fullPath();

	// BFC type of the new subfile - it shall inherit the BFC type of the parent document
	LDBFC::Statement		bfctype = LDBFC::NoCertify;

	// Dirname of the new subfile
	QString			subdirname = dirname (parentpath);

	// Title of the new subfile
	QString			subtitle;

	// Comment containing the title of the parent document
	LDCommentPtr	titleobj = getCurrentDocument()->getObject (0).dynamicCast<LDComment>();

	// License text for the subfile
	QString			license = getLicenseText (cfg::defaultLicense);

	// LDraw code body of the new subfile (i.e. code of the selection)
	QStringList		code;

	// Full path of the subfile to be
	QString			fullsubname;

	// Where to insert the subfile reference?
	int				refidx = selection()[0]->lineNumber();

	// Determine title of subfile
	if (titleobj != null)
		subtitle = "~" + titleobj->text();
	else
		subtitle = "~subfile";

	// Remove duplicate tildes
	while (subtitle.startsWith ("~~"))
		subtitle.remove (0, 1);

	// If this the parent document isn't already in s/, we need to stuff it into
	// a subdirectory named s/. Ensure it exists!
	QString topdirname = basename (dirname (getCurrentDocument()->fullPath()));

	if (topdirname != "s")
	{
		QString desiredPath = subdirname + "/s";
		QString title = tr ("Create subfile directory?");
		QString text = format (tr ("The directory <b>%1</b> is suggested for "
			"subfiles. This directory does not exist, create it?"), desiredPath);

		if (QDir (desiredPath).exists() || confirm (title, text))
		{
			subdirname = desiredPath;
			QDir().mkpath (subdirname);
		}
		else
			return;
	}

	// Determine the body of the name of the subfile
	if (not parentpath.isEmpty())
	{
		// Chop existing '.dat' suffix
		if (parentpath.endsWith (".dat"))
			parentpath.chop (4);

		// Remove the s?? suffix if it's there, otherwise we'll get filenames
		// like s01s01.dat when subfiling subfiles.
		QRegExp subfilesuffix ("s[0-9][0-9]$");
		if (subfilesuffix.indexIn (parentpath) != -1)
			parentpath.chop (subfilesuffix.matchedLength());

		int subidx = 1;
		QString digits;
		QFile f;
		QString testfname;

		// Now find the appropriate filename. Increase the number of the subfile
		// until we find a name which isn't already taken.
		do
		{
			digits.setNum (subidx++);

			// pad it with a zero
			if (digits.length() == 1)
				digits.prepend ("0");

			fullsubname = subdirname + "/" + basename (parentpath) + "s" + digits + ".dat";
		} while (findDocument ("s\\" + basename (fullsubname)) != null || QFile (fullsubname).exists());
	}

	// Determine the BFC winding type used in the main document - it is to
	// be carried over to the subfile.
	for (LDObjectPtr obj : getCurrentDocument()->objects())
	{
		if (obj->type() != OBJ_BFC)
			continue;

		LDBFC::Statement a = obj.staticCast<LDBFC>()->statement();

		if (a == LDBFC::CertifyCCW || a == LDBFC::CertifyCW || a == LDBFC::NoCertify)
		{
			bfctype = a;
			break;
		}
	}

	// Get the body of the document in LDraw code
	for (LDObjectPtr obj : selection())
		code << obj->asText();

	// Create the new subfile document
	LDDocumentPtr doc = LDDocument::createNew();
	doc->setImplicit (false);
	doc->setFullPath (fullsubname);
	doc->setName (LDDocument::shortenName (fullsubname));

	LDObjectList objs;
	objs << spawn<LDComment> (subtitle);
	objs << spawn<LDComment> ("Name: "); // This gets filled in when the subfile is saved
	objs << spawn<LDComment> (format ("Author: %1 [%2]", cfg::defaultName, cfg::defaultUser));
	objs << spawn<LDComment> ("!LDRAW_ORG Unofficial_Subpart");

	if (not license.isEmpty())
		objs << spawn<LDComment> (license);

	objs << spawn<LDEmpty>();
	objs << spawn<LDBFC> (bfctype);
	objs << spawn<LDEmpty>();

	doc->addObjects (objs);

	// Add the actual subfile code to the new document
	for (QString line : code)
	{
		LDObjectPtr obj = parseLine (line);
		doc->addObject (obj);
	}

	// Try save it
	if (save (doc, true))
	{
		// Save was successful. Delete the original selection now from the
		// main document.
		for (LDObjectPtr obj : selection())
			obj->destroy();

		// Add a reference to the new subfile to where the selection was
		LDSubfilePtr ref (spawn<LDSubfile>());
		ref->setColor (maincolor());
		ref->setFileInfo (doc);
		ref->setPosition (g_origin);
		ref->setTransform (g_identity);
		getCurrentDocument()->insertObj (refidx, ref);

		// Refresh stuff
		updateDocumentList();
		doFullRefresh();
	}
	else
	{
		// Failed to save.
		doc->dismiss();
	}
}

void MainWindow::slot_actionRandomColors()
{
	cfg::randomColors = not cfg::randomColors;

	if (cfg::randomColors)
		cfg::bfcRedGreenView = false;

	updateActions();
	R()->refresh();
}

void MainWindow::slot_actionOpenSubfiles()
{
	for (LDObjectPtr obj : selection())
	{
		LDSubfilePtr ref = obj.dynamicCast<LDSubfile>();

		if (ref == null || not ref->fileInfo()->isImplicit())
			continue;

		ref->fileInfo()->setImplicit (false);
	}
}
