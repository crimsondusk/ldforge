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

#include <qprocess.h>
#include <qtemporaryfile.h>
#include <qeventloop.h>
#include <qdialog.h>
#include <qdialogbuttonbox.h>
#include <qspinbox.h>
#include "common.h"
#include "config.h"
#include "misc.h"
#include "extprogs.h"
#include "gui.h"
#include "file.h"
#include "radiobox.h"
#include "history.h"

// =============================================================================
cfg (str, prog_isecalc, "");
cfg (str, prog_intersector, "");
cfg (str, prog_coverer, "");
cfg (str, prog_ytruder, "");
cfg (str, prog_datheader, "");

strconfig* g_extProgPaths[] = {
	&prog_isecalc,
	&prog_intersector,
	&prog_coverer,
	&prog_ytruder,
	&prog_datheader,
};

const char* g_extProgNames[] = {
	"Isecalc",
	"Intersector",
	"Coverer",
	"Ytruder",
	"DATHeader",
};

// =============================================================================
static void noPathConfigured (const extprog prog) {
	const char* name = g_extProgNames[prog];
	
	critical (fmt ("Couldn't run %s as no path has "
		"been defined for it. Use the configuration dialog's External Programs "
		"tab to define a path for %s.", name, name));
}

// =============================================================================
static void processError (const extprog prog, QProcess& proc) {
	const char* name = g_extProgNames[prog];
	str errmsg;
	
	switch (proc.error ()) {
	case QProcess::FailedToStart:
		errmsg = fmt ("Failed to launch %s. Check that you have set the proper path "
			"to %s and that you have the proper permissions to launch it.", name, name);
		break;
	
	case QProcess::Crashed:
		errmsg = fmt ("%s crashed.", name);
		break;
	
	case QProcess::WriteError:
	case QProcess::ReadError:
		errmsg = fmt ("I/O error while interacting with %s.", name);
		break;
	
	case QProcess::UnknownError:
		errmsg = fmt ("Unknown error occurred while executing %s.", name);
		break;
	
	case QProcess::Timedout:
		errmsg = fmt ("%s timed out.", name);
		break;
	}
	
	critical (errmsg);
}

// =============================================================================
static bool mkTempFile (QTemporaryFile& tmp, str& fname) {
	if (!tmp.open ())
		return false;
	
	fname = tmp.fileName ();
	tmp.close ();
	return true;
}

// =============================================================================
void writeSelection (str fname) {
	// Write the input file
	FILE* fp = fopen (fname, "w");
	if (!fp) {
		critical (fmt ("Couldn't open temporary file %s for writing.\n", fname.chars ()));
		return;
	}
	
	for (LDObject* obj : g_win->sel ()) {
		str line = fmt ("%s\r\n", obj->getContents ().chars ());
		fwrite (line.chars(), 1, ~line, fp);
	}
	
	fclose (fp);
}

// =============================================================================
void runUtilityProcess (extprog prog, QStringList argv) {
	QTemporaryFile input, output;
	str inputname, outputname;
	
	if (!mkTempFile (input, inputname) || !mkTempFile (output, outputname))
		return;
	
	QProcess proc;
	
	// Init stdin
	FILE* stdinfp = fopen (inputname, "w");
	
	// Begin!
	proc.setStandardInputFile (inputname);
	proc.start (prog_ytruder.value, argv);
	
	// Write an enter - one is expected
	char enter[2] = "\n";
	enter[1] = '\0';
	fwrite (enter, 1, sizeof enter, stdinfp);
	fflush (stdinfp);
	
	// Wait while it runs
	proc.waitForFinished ();
	
	if (proc.exitStatus () == QProcess::CrashExit) {
		processError (prog, proc);
		return;
	}
}

// =============================================================================
void insertOutput (str fname, bool replace) {
	// Read the output file
	FILE* fp = fopen (fname, "r");
	if (!fp) {
		critical (fmt ("Couldn't open temporary file %s for reading.\n", fname.chars ()));
		return;
	}
	
	ComboHistory* cmb = new ComboHistory ({});
	std::vector<LDObject*> objs = loadFileContents (fp, null),
		copies;
	std::vector<ulong> indices;
	
	ulong idx = g_win->getInsertionPoint ();
	
	// If we replace the objects, delete the selection now.
	if (replace) {
		vector<ulong> indices;
		vector<LDObject*> cache,
			sel = g_win->sel ();
		
		for (LDObject* obj : sel) {
			indices.push_back (obj->getIndex (g_curfile));
			cache.push_back (obj->clone ());
			
			g_curfile->forgetObject (obj);
			delete obj;
		}
	}
	
	// Insert the new objects
	g_win->sel ().clear ();
	for (LDObject* obj : objs) {
		if (!obj->isSchemantic ()) {
			delete obj;
			continue;
		}
		
		g_curfile->insertObj (idx, obj);
		indices.push_back (idx);
		copies.push_back (obj->clone ());
		g_win->sel ().push_back (obj);
		
		++idx;
	}
	
	if (indices.size() > 0)
		cmb->paEntries.push_back (new AddHistory ({indices, copies}));
	
	if (cmb->paEntries.size () > 0)
		History::addEntry (cmb);
	else
		delete cmb;
	
	fclose (fp);
	g_win->refresh ();
}

// =============================================================================
MAKE_ACTION (ytruder, "Ytruder", "ytruder", "Extrude selected lines to a given plane", KEY (F8)) {
	if (prog_ytruder.value.len () == 0) {
		noPathConfigured (Ytruder);
		return;
	}
	
	QDialog* dlg = new QDialog (g_win);
	
	RadioBox* rb_mode = new RadioBox ("Extrusion mode", {"Distance", "Symmetry", "Projection", "Radial"}, 0, Qt::Horizontal, dlg);
	RadioBox* rb_axis = new RadioBox ("Axis", {"X", "Y", "Z"}, 0, Qt::Horizontal, dlg);
	LabeledWidget<QDoubleSpinBox>* dsb_depth = new LabeledWidget<QDoubleSpinBox> ("Plane depth", dlg),
		*dsb_condAngle = new LabeledWidget<QDoubleSpinBox> ("Conditional line threshold", dlg);
	QDialogButtonBox* bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	
	QWidget::connect (bbx_buttons, SIGNAL (accepted ()), dlg, SLOT (accept ()));
	QWidget::connect (bbx_buttons, SIGNAL (rejected ()), dlg, SLOT (reject ()));
	
	rb_axis->setValue (Y);
	dsb_depth->w ()->setMinimum (-10000.0);
	dsb_depth->w ()->setMaximum (10000.0);
	dsb_depth->w ()->setDecimals (3);
	dsb_condAngle->w ()->setValue (30.0f);
	
	QVBoxLayout* layout = new QVBoxLayout (dlg);
	layout->addWidget (rb_mode);
	layout->addWidget (rb_axis);
	layout->addWidget (dsb_depth);
	layout->addWidget (dsb_condAngle);
	layout->addWidget (bbx_buttons);
	
	dlg->setWindowIcon (getIcon ("extrude"));
	
	if (!dlg->exec ())
		return;
	
	const enum modetype { Distance, Symmetry, Projection, Radial } mode = (modetype) rb_mode->value ();
	const Axis axis = (Axis) rb_axis->value ();
	const double depth = dsb_depth->w ()->value (),
		condAngle = dsb_condAngle->w ()->value ();
	
	QTemporaryFile indat, outdat;
	str inDATName, outDATName;
	
	if (!mkTempFile (indat, inDATName) || !mkTempFile (outdat, outDATName))
		return;
	
	QStringList argv ({(axis == X) ? "-x" : (axis == Y) ? "-y" : "-z",
		(mode == Distance) ? "-d" : (mode == Symmetry) ? "-s" : (mode == Projection) ? "-p" : "-r",
		fmt ("%f", depth), "-a", fmt ("%f", condAngle), inDATName, outDATName
	});
	
	writeSelection (inDATName);
	runUtilityProcess (Ytruder, argv);
	insertOutput (outDATName, false);
}