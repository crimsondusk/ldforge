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

#include "common.h"
#include "config.h"
#include "misc.h"
#include "extprogs.h"
#include "gui.h"
#include "file.h"
#include <qprocess.h>
#include <qtemporaryfile.h>
#include <qeventloop.h>

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
	
	QTemporaryFile in, out, input;
	str inname, outname, inputname;
	FILE* fp;
	
	if (!mkTempFile (in, inname) || !mkTempFile (out, outname) || !mkTempFile (input, inputname))
		return;
	
	QProcess proc;
	QStringList argv ({"-p", "0", "-y", inname, outname});
	
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
	
	char line[1024];
	while (fgets (line, sizeof line, fp)) {
		printf ("%s", line);
		LDObject* obj = parseLine (str (line).strip ({'\n', '\r'}));
		g_curfile->addObject (obj);
	}
	fclose (fp);
	
	g_win->refresh ();
}