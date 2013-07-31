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

#include <QApplication>
#include <QMessageBox>
#include <QAbstractButton>
#include <qfile.h>
#include <QTextStream>
#include "gui.h"
#include "file.h"
#include "misc.h"
#include "config.h"
#include "colors.h"
#include "types.h"
#include "primitives.h"

List<LDFile*> g_loadedFiles;
ForgeWindow* g_win = null; 
const QApplication* g_app = null;
File g_file_stdout (stdout, File::Write);
File g_file_stderr (stderr, File::Write);
static str g_versionString, g_fullVersionString;

const vertex g_origin (0.0f, 0.0f, 0.0f);
const matrix g_identity ({1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f});

void doPrint (File& f, initlist<StringFormatArg> args) {
	str msg = DoFormat (args);
	f.write (msg.toUtf8 ());
	f.flush ();
}

void doPrint (FILE* fp, initlist<StringFormatArg> args) {
	if (fp == stdout)
		doPrint (g_file_stdout, args);
	elif (fp == stderr)
		doPrint (g_file_stderr, args);
	else
		fatal ("unknown FILE* argument");
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
int main (int argc, char* argv[]) {
	const QApplication app (argc, argv);
	g_app = &app;
	LDFile::setCurrent (null);
	
	// Load or create the configuration
	if (!config::load ()) {
		print ("Creating configuration file...\n");
		if (config::save ())
			print ("Configuration file successfully created.\n");
		else
			print ("failed to create configuration file!\n");
	}
	
	LDPaths::initPaths ();
	initColors ();

	ForgeWindow* win = new ForgeWindow;
	
	newFile ();
	loadPrimitives ();
	
	win->show ();
	return app.exec ();
}

void doDevf (const char* func, const char* fmtstr, ...) {
	va_list va;
	
	printf ("%s: ", func);
	
	va_start (va, fmtstr);
	vprintf (fmtstr, va);
	va_end (va);
}

const char* versionString () {
	if (g_versionString.length() == 0) {
#if VERSION_PATCH == 0
		g_versionString = fmt ("%1.%2", VERSION_MAJOR, VERSION_MINOR);
#else
		g_versionString = fmt ("%1.%2.%3", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
#endif // VERSION_PATCH
	}
	
	return g_versionString.toStdString().c_str();
}

const char* versionMoniker () {
#if BUILD_ID == BUILD_INTERNAL
	return " Internal";
#elif BUILD_ID == BUILD_ALPHA
	return " Alpha";
#elif BUILD_ID == BUILD_BETA
	return " Beta";
#elif BUILD_ID == BUILD_RC
	return " RC";
#else
	return "";
#endif // BUILD_ID
}

const char* fullVersionString () {
	if (g_fullVersionString.length() == 0)
		g_fullVersionString = fmt ("v%1%2", versionString (), versionMoniker ());
	
	return g_fullVersionString.toStdString().c_str();
}

static void bombBox (str msg) {
	msg.replace ("\n", "<br />");
	
	QMessageBox box (null);
	const QMessageBox::StandardButton btn = QMessageBox::Close;
	box.setWindowTitle ("Fatal Error");
	box.setIconPixmap (getIcon ("bomb"));
	box.setWindowIcon (getIcon ("ldforge"));
	box.setText (msg);
	box.addButton (btn);
	box.button (btn)->setText ("Damn it");
	box.setDefaultButton (btn);
	box.exec ();
}

void assertionFailure (const char* file, const ulong line, const char* funcname, const char* expr) {
	str errmsg = fmt ("File: %1\nLine: %2:\nFunction %3:\n\nAssertion `%4' failed",
		file, line, funcname, expr);
	
#if BUILD_ID == BUILD_INTERNAL
	errmsg += ", aborting.";
#else
	errmsg += ".";
#endif
	
	printf ("%s\n", errmsg.toStdString().c_str());
	
#if BUILD_ID == BUILD_INTERNAL
	if (g_win)
		g_win->deleteLater ();
	
	bombBox (errmsg);
	abort ();
#endif
}

void fatalError (const char* file, const ulong line, const char* funcname, str msg) {
	str errmsg = fmt ("Aborting over a call to fatal():\nFile: %1\nLine: %2\nFunction: %3\n\n%4",
		file, line, funcname, msg);
	
	print ("%1\n", errmsg);
	
	if (g_win)
		g_win->deleteLater ();
	
	bombBox (errmsg);
	abort ();
}
