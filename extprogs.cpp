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

bool g_processDone = false;

// =============================================================================
void runYtruder () {
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
	
	QTemporaryFile in, out, input;
	str inname, outname, inputname;
	FILE* fp;
	
	if (!mkTempFile (in, inname) || !mkTempFile (out, outname) || !mkTempFile (input, inputname))
		return;
	
	QProcess proc;
	QStringList argv ({(axis == X) ? "-x" : (axis == Y) ? "-y" : "-z",
		(mode == Distance) ? "-d" : (mode == Symmetry) ? "-s" : (mode == Projection) ? "-p" : "-r",
		fmt ("%f", depth), "-a", fmt ("%f", condAngle), inname, outname});
	
	printf ("cmdline: %s ", prog_ytruder.value.chars ());
	for (QString& arg : argv)
		printf ("%s ", qchars (arg));
	printf ("\n");
	
	// Write the input file
	fp = fopen (inname, "w");
	if (!fp)
		return;
	
	for (LDObject* obj : g_win->sel ()) {
		str line = fmt ("%s\r\n", obj->getContents ().chars ());
		fwrite (line.chars(), 1, ~line, fp);
	}
	fclose (fp);
	
	// Init stdin
	FILE* stdinfp = fopen (inputname, "w");
	
	// Begin!
	proc.setStandardInputFile (inputname);
	proc.setStandardOutputFile ("blarg");
	proc.start (prog_ytruder.value, argv);
	
	// Write an enter - one is expected
	char enter[2] = "\n";
	enter[1] = '\0';
	fwrite (enter, 1, sizeof enter, stdinfp);
	fflush (stdinfp);
	
	// Wait while it runs
	proc.waitForFinished ();
	
	if (proc.exitStatus () == QProcess::CrashExit) {
		processError (Ytruder, proc);
		return;
	}
	
	// Read the output file
	fp = fopen (outname, "r");
	if (!fp)
		fprintf (stderr, "couldn't read %s\n", outname.chars ());
	
	std::vector<LDObject*> objs = loadFileContents (fp, null),
		copies;
	std::vector<ulong> indices;
	
	g_win->sel ().clear ();
	for (LDObject* obj : objs) {
		ulong idx = g_curfile->addObject (obj);
		indices.push_back (idx);
		copies.push_back (obj->clone ());
	}
	
	History::addEntry (new AddHistory ({indices, copies}));
	
	fclose (fp);
	g_win->sel () = objs;
	g_win->refresh ();
}