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
#include <qcheckbox.h>
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
cfg (str, prog_rectifier, "");

const char* g_extProgNames[] = {
	"Isecalc",
	"Intersector",
	"Coverer",
	"Ytruder",
	"Rectifier",
	"DATHeader",
};

// =============================================================================
static bool checkProgPath (str path, const extprog prog) {
	if (~path)
		return true;
	
	const char* name = g_extProgNames[prog];
	
	critical (fmt ("Couldn't run %s as no path has "
		"been defined for it. Use the configuration dialog's External Programs "
		"tab to define a path for %s.", name, name));
	return false;
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
void runUtilityProcess (extprog prog, str path, QString argvstr) {
	QTemporaryFile input, output;
	str inputname, outputname;
	QStringList argv = argvstr.split (" ", QString::SkipEmptyParts);
	
	printf ("cmdline: %s %s\n", path.chars (), qchars (argvstr));
	
	if (!mkTempFile (input, inputname) || !mkTempFile (output, outputname))
		return;
	
	QProcess proc;
	
	// Init stdin
	FILE* stdinfp = fopen (inputname, "w");
	
	// Begin!
	proc.setStandardInputFile (inputname);
	proc.start (path, argv);
	
	// Write an enter - one is expected
	char enter[2] = "\n";
	enter[1] = '\0';
	fwrite (enter, 1, sizeof enter, stdinfp);
	fflush (stdinfp);
	
	// Wait while it runs
	proc.waitForFinished ();
	
#ifndef RELASE
	printf ("%s", qchars (QString (proc.readAllStandardOutput ())));
#endif
	
	if (proc.exitStatus () == QProcess::CrashExit) {
		processError (prog, proc);
		return;
	}
}

// =============================================================================
static void insertOutput (str fname, bool replace) {
#ifndef RELEASE
	QFile::copy (fname, "./output.dat");
#endif
	
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
	
	// If we replace the objects, delete the selection now.
	if (replace)
		*cmb << g_win->deleteSelection ();
	
	// Insert the new objects
	g_win->sel ().clear ();
	for (LDObject* obj : objs) {
		if (!obj->isSchemantic ()) {
			delete obj;
			continue;
		}
		
		ulong idx = g_curfile->addObject (obj);
		indices.push_back (idx);
		copies.push_back (obj->clone ());
		g_win->sel ().push_back (obj);
	}
	
	if (indices.size() > 0)
		*cmb << new AddHistory ({indices, copies});
	
	if (cmb->paEntries.size () > 0)
		History::addEntry (cmb);
	else
		delete cmb;
	
	fclose (fp);
	g_win->refresh ();
}

QDialogButtonBox* makeButtonBox (QDialog* dlg) {
	QDialogButtonBox* bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	QWidget::connect (bbx_buttons, SIGNAL (accepted ()), dlg, SLOT (accept ()));
	QWidget::connect (bbx_buttons, SIGNAL (rejected ()), dlg, SLOT (reject ()));
	return bbx_buttons;
}

// =============================================================================
// Interface for Ytruder
MAKE_ACTION (ytruder, "Ytruder", "ytruder", "Extrude selected lines to a given plane", KEY (F8)) {
	setlocale (LC_ALL, "C");
	
	if (!checkProgPath (prog_ytruder, Ytruder))
		return;
	
	QDialog* dlg = new QDialog (g_win);
	
	RadioBox* rb_mode = new RadioBox ("Extrusion mode", {"Distance", "Symmetry", "Projection", "Radial"}, 0, Qt::Horizontal, dlg);
	RadioBox* rb_axis = new RadioBox ("Axis", {"X", "Y", "Z"}, 0, Qt::Horizontal, dlg);
	LabeledWidget<QDoubleSpinBox>* dsb_depth = new LabeledWidget<QDoubleSpinBox> ("Plane depth", dlg),
		*dsb_condAngle = new LabeledWidget<QDoubleSpinBox> ("Conditional line threshold", dlg);
	
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
	layout->addWidget (makeButtonBox (dlg));
	
	dlg->setWindowIcon (getIcon ("extrude"));
	
	if (!dlg->exec ())
		return;
	
	// Read the user's choices
	const enum modetype { Distance, Symmetry, Projection, Radial } mode = (modetype) rb_mode->value ();
	const Axis axis = (Axis) rb_axis->value ();
	const double depth = dsb_depth->w ()->value (),
		condAngle = dsb_condAngle->w ()->value ();
	
	QTemporaryFile indat, outdat;
	str inDATName, outDATName;
	
	// Make temp files for the input and output files
	if (!mkTempFile (indat, inDATName) || !mkTempFile (outdat, outDATName))
		return;
	
	// Compose the command-line arguments
	str argv = fmt ("%s %s %f -a %f %s %s",
		(axis == X) ? "-x" : (axis == Y) ? "-y" : "-z",
		(mode == Distance) ? "-d" : (mode == Symmetry) ? "-s" : (mode == Projection) ? "-p" : "-r",
		depth, condAngle, inDATName.chars (), outDATName.chars ());
	
	writeSelection (inDATName);
	runUtilityProcess (Ytruder, prog_ytruder, argv);
	insertOutput (outDATName, false);
}

// =============================================================================
// Rectifier interface
MAKE_ACTION (rectifier, "Rectifier", "rectifier", "Optimizes quads into rect primitives.", (0)) {
	setlocale (LC_ALL, "C");
	
	if (!checkProgPath (prog_rectifier, Rectifier))
		return;
	
	QDialog* dlg = new QDialog (g_win);
	QCheckBox* cb_condense = new QCheckBox ("Condense triangles to quads"),
		*cb_subst = new QCheckBox ("Substitute rect primitives"),
		*cb_condlineCheck = new QCheckBox ("Don't replace quads with adj. condlines"),
		*cb_colorize = new QCheckBox ("Colorize resulting objects");
	LabeledWidget<QDoubleSpinBox>* dsb_coplthres = new LabeledWidget<QDoubleSpinBox> ("Coplanarity threshold", dlg);
	
	dsb_coplthres->w ()->setMinimum (0.0f);
	dsb_coplthres->w ()->setMaximum (360.0f);
	dsb_coplthres->w ()->setDecimals (3);
	dsb_coplthres->w ()->setValue (0.95f);
	cb_condense->setChecked (true);
	cb_subst->setChecked (true);
	
	QVBoxLayout* layout = new QVBoxLayout (dlg);
	layout->addWidget (cb_condense);
	layout->addWidget (cb_subst);
	layout->addWidget (cb_condlineCheck);
	layout->addWidget (cb_colorize);
	layout->addWidget (dsb_coplthres);
	layout->addWidget (makeButtonBox (dlg));
	
	if (!dlg->exec ())
		return;
	
	const bool condense = cb_condense->isChecked (),
		subst = cb_subst->isChecked (),
		condlineCheck = cb_condlineCheck->isChecked (),
		colorize = cb_colorize->isChecked ();
	const double coplthres = dsb_coplthres->w ()->value ();
	
	QTemporaryFile indat, outdat;
	str inDATName, outDATName;
	
	// Make temp files for the input and output files
	if (!mkTempFile (indat, inDATName) || !mkTempFile (outdat, outDATName))
		return;
	
	// Compose arguments
	str argv = fmt ("%s %s %s %s -t %f %s %s",
		(condense == false) ? "-q" : "",
		(subst == false) ? "-r" : "",
		(condlineCheck) ? "-a" : "",
		(colorize) ? "-c" : "",
		coplthres, inDATName.chars (), outDATName.chars ());
	
	writeSelection (inDATName);
	runUtilityProcess (Rectifier, prog_rectifier, argv);
	insertOutput (outDATName, true);
}