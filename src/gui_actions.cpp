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
#include "misc.h"
#include "gldraw.h"
#include "dialogs.h"
#include "primitives.h"
#include "ui_newpart.h"
#include "widgets.h"

extern_cfg (Bool, gl_wireframe);
extern_cfg (Bool, gl_colorbfc);
extern_cfg (String, ld_defaultname);
extern_cfg (String, ld_defaultuser);
extern_cfg (Int, ld_defaultlicense);

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (New, CTRL_SHIFT (N))
{	QDialog* dlg = new QDialog (g_win);
	Ui::NewPartUI ui;
	ui.setupUi (dlg);

	str authortext = ld_defaultname;

	if (!ld_defaultuser.value.isEmpty())
		authortext.append (fmt (" [%1]", ld_defaultuser));

	ui.le_author->setText (authortext);

	switch (ld_defaultlicense)
	{	case 0:
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
								  fmt ("Unknown ld_defaultlicense value %1!", ld_defaultlicense));
			break;
	}

	if (dlg->exec() == false)
		return;

	newFile();

	const LDBFC::Type BFCType =
		ui.rb_bfc_ccw->isChecked() ? LDBFC::CertifyCCW :
		ui.rb_bfc_cw->isChecked()  ? LDBFC::CertifyCW : LDBFC::NoCertify;

	const str license =
		ui.rb_license_ca->isChecked()    ? CALicense :
		ui.rb_license_nonca->isChecked() ? NonCALicense : "";

	LDFile::current()->addObjects (
	{	new LDComment (ui.le_title->text()),
		new LDComment ("Name: <untitled>.dat"),
		new LDComment (fmt ("Author: %1", ui.le_author->text())),
		new LDComment (fmt ("!LDRAW_ORG Unofficial_Part")),
		(license != "" ?
		new LDComment (license) :
		null),
		new LDEmpty,
		new LDBFC (BFCType),
		new LDEmpty,
	});

	g_win->doFullRefresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (NewFile, CTRL (N))
{	newFile();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Open, CTRL (O))
{	str name = QFileDialog::getOpenFileName (g_win, "Open File", "", "LDraw files (*.dat *.ldr)");

	if (name.length() == 0)
		return;

	openMainFile (name);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Save, CTRL (S))
{	g_win->save (LDFile::current(), false);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (SaveAs, CTRL_SHIFT (S))
{	g_win->save (LDFile::current(), true);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (SaveAll, CTRL (L))
{	for (LDFile* file : g_loadedFiles)
	{	if (file->isImplicit())
			continue;

		g_win->save (file, false);
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Close, CTRL (W))
{	if (!LDFile::current()->isSafeToClose())
		return;

	delete LDFile::current();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (CloseAll, 0)
{	if (!safeToCloseAll())
		return;

	closeAll();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Settings, 0)
{	(new ConfigDialog)->exec();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (SetLDrawPath, 0)
{	(new LDrawPathDialog (true))->exec();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Exit, CTRL (Q))
{	exit (0);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (NewSubfile, 0)
{	AddObjectDialog::staticDialog (LDObject::Subfile, null);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (NewLine, 0)
{	AddObjectDialog::staticDialog (LDObject::Line, null);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (NewTriangle, 0)
{	AddObjectDialog::staticDialog (LDObject::Triangle, null);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (NewQuad, 0)
{	AddObjectDialog::staticDialog (LDObject::Quad, null);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (NewCLine, 0)
{	AddObjectDialog::staticDialog (LDObject::CndLine, null);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (NewComment, 0)
{	AddObjectDialog::staticDialog (LDObject::Comment, null);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (NewBFC, 0)
{	AddObjectDialog::staticDialog (LDObject::BFC, null);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (NewVertex, 0)
{	AddObjectDialog::staticDialog (LDObject::Vertex, null);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Edit, 0)
{	if (selection().size() != 1)
		return;

	LDObject* obj = selection() [0];
	AddObjectDialog::staticDialog (obj->getType(), obj);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Help, KEY (F1))
{
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (About, 0)
{	AboutDialog().exec();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (AboutQt, 0)
{	QMessageBox::aboutQt (g_win);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (SelectAll, CTRL (A))
{	for (LDObject* obj : LDFile::current()->getObjects())
		obj->select();

	g_win->updateSelection();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (SelectByColor, CTRL_SHIFT (A))
{	int colnum = g_win->getSelectedColor();

	if (colnum == -1)
		return; // no consensus on color

	LDFile::current()->clearSelection();

	for (LDObject* obj : LDFile::current()->getObjects())
		if (obj->getColor() == colnum)
			obj->select();

	g_win->updateSelection();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (SelectByType, 0)
{	if (selection().isEmpty())
		return;

	LDObject::Type type = g_win->getUniformSelectedType();

	if (type == LDObject::Unidentified)
		return;

	// If we're selecting subfile references, the reference filename must also
	// be uniform.
	str refName;

	if (type == LDObject::Subfile)
	{	refName = static_cast<LDSubfile*> (selection()[0])->getFileInfo()->getName();

		for (LDObject* obj : selection())
			if (static_cast<LDSubfile*> (obj)->getFileInfo()->getName() != refName)
				return;
	}

	LDFile::current()->clearSelection();

	for (LDObject* obj : LDFile::current()->getObjects())
	{	if (obj->getType() != type)
			continue;

		if (type == LDObject::Subfile && static_cast<LDSubfile*> (obj)->getFileInfo()->getName() != refName)
			continue;

		obj->select();
	}

	g_win->updateSelection();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (GridCoarse, 0)
{	grid = Grid::Coarse;
	g_win->updateGridToolBar();
}

DEFINE_ACTION (GridMedium, 0)
{	grid = Grid::Medium;
	g_win->updateGridToolBar();
}

DEFINE_ACTION (GridFine, 0)
{	grid = Grid::Fine;
	g_win->updateGridToolBar();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (ResetView, CTRL (0))
{	g_win->R()->resetAngles();
	g_win->R()->update();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (InsertFrom, 0)
{	str fname = QFileDialog::getOpenFileName();
	int idx = g_win->getInsertionPoint();

	if (!fname.length())
		return;

	File f (fname, File::Read);

	if (!f)
	{	critical (fmt ("Couldn't open %1 (%2)", fname, strerror (errno)));
		return;
	}

	QList<LDObject*> objs = loadFileContents (&f, null);

	LDFile::current()->clearSelection();

	for (LDObject* obj : objs)
	{	LDFile::current()->insertObj (idx, obj);
		obj->select();
		g_win->R()->compileObject (obj);

		idx++;
	}

	g_win->refresh();
	g_win->scrollToSelection();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (ExportTo, 0)
{	if (selection().isEmpty())
		return;

	str fname = QFileDialog::getSaveFileName();

	if (fname.length() == 0)
		return;

	QFile file (fname);

	if (!file.open (QIODevice::WriteOnly | QIODevice::Text))
	{	critical (fmt ("Unable to open %1 for writing (%2)", fname, strerror (errno)));
		return;
	}

	for (LDObject* obj : selection())
	{	str contents = obj->raw();
		QByteArray data = contents.toUtf8();
		file.write (data, data.size());
		file.write ("\r\n", 2);
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (InsertRaw, 0)
{	int idx = g_win->getInsertionPoint();

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

	LDFile::current()->clearSelection();

	for (str line : str (te_edit->toPlainText()).split ("\n"))
	{	LDObject* obj = parseLine (line);

		LDFile::current()->insertObj (idx, obj);
		obj->select();
		g_win->R()->compileObject (obj);
		idx++;
	}

	g_win->refresh();
	g_win->scrollToSelection();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Screenshot, 0)
{	setlocale (LC_ALL, "C");

	int w, h;
	uchar* imgdata = g_win->R()->getScreencap (w, h);
	QImage img = imageFromScreencap (imgdata, w, h);

	str root = basename (LDFile::current()->getName());

	if (root.right (4) == ".dat")
		root.chop (4);

	str defaultname = (root.length() > 0) ? fmt ("%1.png", root) : "";
	str fname = QFileDialog::getSaveFileName (g_win, "Save Screencap", defaultname,
				"PNG images (*.png);;JPG images (*.jpg);;BMP images (*.bmp);;All Files (*.*)");

	if (fname.length() > 0 && !img.save (fname))
		critical (fmt ("Couldn't open %1 for writing to save screencap: %2", fname, strerror (errno)));

	delete[] imgdata;
}

// =============================================================================
// -----------------------------------------------------------------------------
extern_cfg (Bool, gl_axes);
DEFINE_ACTION (Axes, 0)
{	gl_axes = !gl_axes;
	ACTION (Axes)->setChecked (gl_axes);
	g_win->R()->update();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (VisibilityToggle, 0)
{	for (LDObject* obj : selection())
		obj->toggleHidden();

	g_win->refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (VisibilityHide, 0)
{	for (LDObject* obj : selection())
		obj->setHidden (true);
	
	g_win->refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (VisibilityReveal, 0)
{	for (LDObject* obj : selection())
	obj->setHidden (false);
	
	g_win->refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (Wireframe, 0)
{	gl_wireframe = !gl_wireframe;
	g_win->R()->refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (SetOverlay,  0)
{	OverlayDialog dlg;

	if (!dlg.exec())
		return;

	g_win->R()->setupOverlay ( (GL::Camera) dlg.camera(), dlg.fpath(), dlg.ofsx(),
							   dlg.ofsy(), dlg.lwidth(), dlg.lheight());
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (ClearOverlay, 0)
{	g_win->R()->clearOverlay();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (ModeSelect, CTRL (1))
{	g_win->R()->setEditMode (Select);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (ModeDraw, CTRL (2))
{	g_win->R()->setEditMode (Draw);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (ModeCircle, CTRL (3))
{	g_win->R()->setEditMode (CircleMode);
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (SetDrawDepth, 0)
{	if (g_win->R()->camera() == GL::Free)
		return;

	bool ok;
	double depth = QInputDialog::getDouble (g_win, "Set Draw Depth",
											fmt ("Depth value for %1 Camera:", g_win->R()->getCameraName()),
											g_win->R()->getDepthValue(), -10000.0f, 10000.0f, 3, &ok);

	if (ok)
		g_win->R()->setDepthValue (depth);
}

#if 0
// This is a test to draw a dummy axle. Meant to be used as a primitive gallery,
// but I can't figure how to generate these pictures properly. Multi-threading
// these is an immense pain.
DEFINE_ACTION (testpic, "Test picture", "", "", (0))
{	LDFile* file = getFile ("axle.dat");
	setlocale (LC_ALL, "C");

	if (!file)
	{	critical ("couldn't load axle.dat");
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
	{	critical ("Failed to create the image!\n");
	}
	else
	{	QLabel* label = new QLabel;
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
// -----------------------------------------------------------------------------
DEFINE_ACTION (ScanPrimitives, 0)
{	PrimitiveLister::start();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (BFCView, SHIFT (B))
{	gl_colorbfc = !gl_colorbfc;
	ACTION (BFCView)->setChecked (gl_colorbfc);
	g_win->R()->refresh();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (JumpTo, CTRL (G))
{	bool ok;
	int defval = 0;
	LDObject* obj;

	if (selection().size() == 1)
		defval = selection()[0]->getIndex();

	int idx = QInputDialog::getInt (null, "Go to line", "Go to line:", defval,
		1, LDFile::current()->getObjectCount(), 1, &ok);

	if (!ok || (obj = LDFile::current()->getObject (idx - 1)) == null)
		return;

	LDFile::current()->clearSelection();
	obj->select();
	g_win->updateSelection();
}
