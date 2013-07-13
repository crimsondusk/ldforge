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

#include <QFileDialog>
#include <QMessageBox>
#include <QTextEdit>
#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QInputDialog>

#include "gui.h"
#include "file.h"
#include "history.h"
#include "configDialog.h"
#include "addObjectDialog.h"
#include "aboutDialog.h"
#include "misc.h"
#include "gldraw.h"
#include "dialogs.h"
#include "primitives.h"
#include "ui_newpart.h"
#include "widgets.h"

extern_cfg (bool, gl_wireframe);
extern_cfg (bool, gl_colorbfc);

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (newFile, "&New", "brick", "Create a new part model.", CTRL (N)) {
	if (safeToCloseAll() == false)
		return;
	
	QDialog* dlg = new QDialog (g_win);
	Ui::NewPartUI ui;
	ui.setupUi (dlg);
	
	if (dlg->exec() == false)
		return;
	
	newFile();
	
	const LDBFCObject::Type BFCType =
		ui.rb_bfc_ccw->isChecked() ? LDBFCObject::CertifyCCW :
		ui.rb_bfc_cw->isChecked()  ? LDBFCObject::CertifyCW :
		                             LDBFCObject::NoCertify;
	
	const str license =
		ui.rb_license_ca->isChecked()    ? CALicense :
		ui.rb_license_nonca->isChecked() ? NonCALicense :
		                                   "";
	
	*g_curfile << new LDCommentObject (ui.le_title->text());
	*g_curfile << new LDCommentObject ("Name: <untitled>.dat" );
	*g_curfile << new LDCommentObject (fmt ("Author: %1", ui.le_author->text()));
	*g_curfile << new LDCommentObject (fmt ("!LDRAW_ORG Unofficial_Part"));
	
	if (license != "")
		*g_curfile << new LDCommentObject (license);
	
	*g_curfile << new LDEmptyObject;
	*g_curfile << new LDBFCObject (BFCType);
	*g_curfile << new LDEmptyObject;
	
	g_win->fullRefresh();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (open, "&Open", "file-open", "Load a part model from a file.", CTRL (O)) {
	if (safeToCloseAll() == false)
		return;
	
	str name = QFileDialog::getOpenFileName (g_win, "Open File", "", "LDraw files (*.dat *.ldr)");
	
	if (name.length() == 0)
		return;
	
	closeAll();
	openMainFile (name);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (save, "&Save", "file-save", "Save the part model.", CTRL (S))
{
	g_win->save (g_curfile, false);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (saveAs, "Save &As", "file-save-as", "Save the part model to a specific file.", CTRL_SHIFT (S))
{
	g_win->save (g_curfile, true);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (settings, "Settin&gs", "settings", "Edit the settings of " APPNAME ".", (0)) {
	ConfigDialog::staticDialog();
}

MAKE_ACTION (setLDrawPath, "Set LDraw Path", "settings", "Change the LDraw directory path.", (0)) {
	LDrawPathDialog dlg (true);
	dlg.exec();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (exit, "&Exit", "exit", "Close " APPNAME ".", CTRL (Q)) {
	exit (0);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (newSubfile, "New Subfile", "add-subfile", "Creates a new subfile reference.", 0) {
	AddObjectDialog::staticDialog (LDObject::Subfile, null);
}

MAKE_ACTION (newLine, "New Line",  "add-line", "Creates a new line.", 0) {
	AddObjectDialog::staticDialog (LDObject::Line, null);
}

MAKE_ACTION (newTriangle, "New Triangle", "add-triangle", "Creates a new triangle.", 0) {
	AddObjectDialog::staticDialog (LDObject::Triangle, null);
}

MAKE_ACTION (newQuad, "New Quadrilateral", "add-quad", "Creates a new quadrilateral.", 0) {
	AddObjectDialog::staticDialog (LDObject::Quad, null);
}

MAKE_ACTION (newCondLine, "New Conditional Line", "add-condline", "Creates a new conditional line.", 0) {
	AddObjectDialog::staticDialog (LDObject::CondLine, null);
}

MAKE_ACTION (newComment, "New Comment", "add-comment", "Creates a new comment.", 0) {
	AddObjectDialog::staticDialog (LDObject::Comment, null);
}

MAKE_ACTION (newBFC, "New BFC Statement", "add-bfc", "Creates a new BFC statement.", 0) {
	AddObjectDialog::staticDialog (LDObject::BFC, null);
}

MAKE_ACTION (newVertex, "New Vertex", "add-vertex", "Creates a new vertex.", 0) {
	AddObjectDialog::staticDialog (LDObject::Vertex, null);
}

MAKE_ACTION (makePrimitive, "Make a Primitive", "radial", "Generate a new circular primitive.", 0)
{
	generatePrimitive();
}

MAKE_ACTION (editObject, "Edit Object", "edit-object", "Edits this object.", 0) {
	if (g_win->sel().size() != 1)
		return;
	
	LDObject* obj = g_win->sel()[0];
	AddObjectDialog::staticDialog (obj->getType(), obj);
}

MAKE_ACTION (help, "Help", "help", "Shows the " APPNAME " help manual.", KEY (F1)) {
	
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (about, "About " APPNAME, "ldforge",
	"Shows information about " APPNAME ".", (0))
{
	AboutDialog dlg;
	dlg.exec();
}

MAKE_ACTION (aboutQt, "About Qt", "qt", "Shows information about Qt.", (0)) {
	QMessageBox::aboutQt (g_win);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (selectAll, "Select All", "select-all", "Selects all objects.", CTRL (A)) {
	g_win->sel().clear();
	
	for (LDObject* obj : g_curfile->objs())
		g_win->sel() << obj;
	
	g_win->updateSelection();
}

// =============================================================================
MAKE_ACTION (selectByColor, "Select by Color", "select-color",
	"Select all objects by the given color.", CTRL_SHIFT (A))
{
	short colnum = g_win->getSelectedColor();
	
	if (colnum == -1)
		return; // no consensus on color
	
	g_win->sel().clear();
	for (LDObject* obj : g_curfile->objs())
		if (obj->color() == colnum)
			g_win->sel() << obj;
	
	g_win->updateSelection();
}

// =============================================================================
MAKE_ACTION (selectByType, "Select by Type", "select-type",
	"Select all objects by the given type.", (0))
{
	if (g_win->sel().size() == 0)
		return;
	
	LDObject::Type type = g_win->uniformSelectedType();
	
	if (type == LDObject::Unidentified)
		return;
	
	// If we're selecting subfile references, the reference filename must also
	// be uniform.
	str refName;
	
	if (type == LDObject::Subfile) {
		refName = static_cast<LDSubfileObject*> (g_win->sel()[0])->fileInfo()->name();
		
		for (LDObject* obj : g_win->sel())
			if (static_cast<LDSubfileObject*> (obj)->fileInfo()->name() != refName)
				return;
	}
	
	g_win->sel().clear();
	for (LDObject* obj : g_curfile->objs()) {
		if (obj->getType() != type)
			continue;
		
		if (type == LDObject::Subfile && static_cast<LDSubfileObject*> (obj)->fileInfo()->name() != refName)
			continue;
		
		g_win->sel() << obj;
	}
	
	g_win->updateSelection();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (gridCoarse, "Coarse Grid", "grid-coarse", "Set the grid to Coarse", (0)) {
	grid = Grid::Coarse;
	g_win->updateGridToolBar();
}

MAKE_ACTION (gridMedium, "Medium Grid", "grid-medium", "Set the grid to Medium", (0)) {
	grid = Grid::Medium;
	g_win->updateGridToolBar();
}

MAKE_ACTION (gridFine, "Fine Grid", "grid-fine", "Set the grid to Fine", (0)) {
	grid = Grid::Fine;
	g_win->updateGridToolBar();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (resetView, "Reset View", "reset-view", "Reset view angles, pan and zoom", CTRL (0)) {
	g_win->R()->resetAngles();
	g_win->R()->update();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (insertFrom, "Insert from File", "file-import", "Insert LDraw data from a file.", (0)) {
	str fname = QFileDialog::getOpenFileName();
	ulong idx = g_win->getInsertionPoint();
	
	if (!fname.length())
		return;
	
	File f (fname, File::Read);
	if (!f) {
		critical (fmt ("Couldn't open %1 (%2)", fname, strerror (errno)));
		return;
	}
	
	vector<LDObject*> objs = loadFileContents (&f, null);
	
	g_win->sel().clear();
	
	for (LDObject* obj : objs) {
		g_curfile->insertObj (idx, obj);
		g_win->sel() << obj;
		
		idx++;
	}
	
	g_win->fullRefresh();
	g_win->scrollToSelection();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (exportTo, "Export To File", "file-export", "Export current selection to file", (0)) {
	if (g_win->sel().size() == 0)
		return;
	
	str fname = QFileDialog::getSaveFileName();
	if (fname.length() == 0)
		return;
	
	QFile file (fname);
	if (!file.open (QIODevice::WriteOnly | QIODevice::Text)) {
		critical (fmt ("Unable to open %1 for writing (%2)", fname, strerror (errno)));
		return;
	}
	
	for (LDObject* obj : g_win->sel()) {
		str contents = obj->raw();
		QByteArray data = contents.toUtf8();
		file.write (data, data.size());
		file.write ("\r\n", 2);
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
MAKE_ACTION (insertRaw, "Insert Raw", "insert-raw", "Type in LDraw code to insert.", (0)) {
	ulong idx = g_win->getInsertionPoint();
	
	QDialog* const dlg = new QDialog;
	QVBoxLayout* const layout = new QVBoxLayout;
	QTextEdit* const te_edit = new QTextEdit;
	QDialogButtonBox* const bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	
	layout->addWidget (te_edit);
	layout->addWidget (bbx_buttons);
	dlg->setLayout (layout);
	dlg->setWindowTitle (APPNAME ": Insert Raw");
	dlg->connect (bbx_buttons, SIGNAL (accepted()), dlg, SLOT (accept()));
	dlg->connect (bbx_buttons, SIGNAL (rejected()), dlg, SLOT (reject()));
	
	if (dlg->exec() == false)
		return;
	
	g_win->sel().clear();
	
	for (str line : str (te_edit->toPlainText()).split ("\n")) {
		LDObject* obj = parseLine (line);
		
		g_curfile->insertObj (idx, obj);
		g_win->sel() << obj;
		idx++;
	}
	
	g_win->fullRefresh();
	g_win->scrollToSelection();
}

// =========================================================================================================================================
MAKE_ACTION (screencap, "Screencap Part", "screencap", "Save a picture of the model", (0)) {
	setlocale (LC_ALL, "C");
	
	ushort w, h;
	uchar* imgdata = g_win->R()->screencap (w, h);
	QImage img = imageFromScreencap (imgdata, w, h);
	
	str root = basename (g_curfile->name());
	if (root.right (4) == ".dat")
		root.chop (4);
	
	str defaultname = (root.length() > 0) ? fmt ("%1.png", root) : "";
	str fname = QFileDialog::getSaveFileName (g_win, "Save Screencap", defaultname,
		"PNG images (*.png);;JPG images (*.jpg);;BMP images (*.bmp);;All Files (*.*)");
	
	if (fname.length() > 0 && !img.save (fname))
		critical (fmt ("Couldn't open %1 for writing to save screencap: %2", fname, strerror (errno)));
	
	delete[] imgdata;
}

// =========================================================================================================================================
extern_cfg (bool, gl_axes);
MAKE_ACTION (axes, "Draw Axes", "axes", "Toggles drawing of axes", (0)) {
	gl_axes = !gl_axes;
	ACTION (axes)->setChecked (gl_axes);
	g_win->R()->update();
}

// =========================================================================================================================================
MAKE_ACTION (visibility, "Toggle Visibility", "visibility", "Toggles visibility/hiding on objects.", (0)) {
	for (LDObject* obj : g_win->sel())
		obj->setHidden (!obj->hidden());
	
	g_win->fullRefresh();
}

MAKE_ACTION (wireframe, "Wireframe", "wireframe", "Toggle wireframe view", (0)) {
	gl_wireframe = !gl_wireframe;
	g_win->R()->refresh();
}

MAKE_ACTION (setOverlay, "Set Overlay Image", "overlay", "Set an overlay image", 0)
{
	OverlayDialog dlg;
	
	if (!dlg.exec())
		return;
	
	g_win->R()->setupOverlay ((GL::Camera) dlg.camera(), dlg.fpath(), dlg.ofsx(),
		dlg.ofsy(), dlg.lwidth(), dlg.lheight() );
}

MAKE_ACTION (clearOverlay, "Clear Overlay Image", "overlay-clear", "Clear the overlay image.", (0)) {
	g_win->R()->clearOverlay();
}

MAKE_ACTION (modeSelect, "Select Mode", "mode-select", "Select objects from the camera view.", CTRL (1)) {
	g_win->R()->setEditMode (Select);
}

MAKE_ACTION (modeDraw, "Draw Mode", "mode-draw", "Draw objects into the camera view.", CTRL (2)) {
	g_win->R()->setEditMode (Draw);
}

MAKE_ACTION (setDrawDepth, "Set Depth Value", "depth-value", "Set the depth coordinate of the current camera.", (0)) {
	if (g_win->R()->camera() == GL::Free)
		return;
	
	bool ok;
	double depth = QInputDialog::getDouble (g_win, "Set Draw Depth",
		fmt ("Depth value for %1 Camera:", g_win->R()->cameraName()),
		g_win->R()->depthValue(), -10000.0f, 10000.0f, 3, &ok);
	
	if (ok)
		g_win->R()->setDepthValue (depth);
}

#ifndef RELEASE
// This is a test to draw a dummy axle. Meant to be used as a primitive gallery,
// but I can't figure how to generate these pictures properly. Multi-threading
// these is an immense pain.
MAKE_ACTION (testpic, "Test picture", "", "", (0)) {
	LDOpenFile* file = getFile ("axle.dat");
	setlocale (LC_ALL, "C");
	
	if (!file) {
		critical ("couldn't load axle.dat");
		return;
	}
	
	ushort w, h;
	
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
	
	if (img.isNull()) {
		critical ("Failed to create the image!\n");
	} else {
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

MAKE_ACTION (reloadPrimitives, "Scan Primitives", "", "", (0)) {
	PrimitiveLister::start();
}

MAKE_ACTION (colorbfc, "BFC Red/Green View", "bfc-view", "", SHIFT (B))
{
	gl_colorbfc = !gl_colorbfc;
	ACTION (colorbfc)->setChecked (gl_colorbfc);
	g_win->R()->refresh();
}