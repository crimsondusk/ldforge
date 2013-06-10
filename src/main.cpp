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
#include "gui.h"
#include "file.h"
#include "bbox.h"
#include "misc.h"
#include "config.h"
#include "colors.h"
#include "types.h"

vector<LDOpenFile*> g_loadedFiles;
LDOpenFile* g_curfile = null;
ForgeWindow* g_win = null; 
bbox g_BBox;
const QApplication* g_app = null;

const vertex g_origin (0.0f, 0.0f, 0.0f);
const matrix g_identity ({1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f});

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
int main (int argc, char* argv[]) {
	const QApplication app (argc, argv);
	g_app = &app;
	g_curfile = NULL;
	
	// Load or create the configuration
	if (!config::load ()) {
		printf ("Creating configuration file...\n");
		if (config::save ())
			printf ("Configuration file successfully created.\n");
		else
			printf ("failed to create configuration file!\n");
	}
	
	LDPaths::initPaths ();
	
	initColors ();
	initPartList ();
	
	ForgeWindow* win = new ForgeWindow;
	newFile ();
	
	win->show ();
	return app.exec ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void logf (const char* fmtstr, ...) {
	va_list va;
	va_start (va, fmtstr);
	g_win->logVA (LOG_Normal, fmtstr, va);
	va_end (va);
}

void logf (LogType type, const char* fmtstr, ...) {
	va_list va;
	va_start (va, fmtstr);
	g_win->logVA (type, fmtstr, va);
	va_end (va);
}

void doDevf (const char* func, const char* fmtstr, ...) {
	va_list va;
	
	printf ("%s: ", func);
	
	va_start (va, fmtstr);
	vprintf (fmtstr, va);
	va_end (va);
}

str versionString () {
#if VERSION_PATCH == 0
	return fmt ("%d.%d", VERSION_MAJOR, VERSION_MINOR);
#else
	return fmt ("%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
#endif // VERSION_PATCH
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

str fullVersionString () {
	return fmt ("v%s%s", versionString ().chars (), versionMoniker ());
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
	str errmsg = fmt ("File %s\nLine: %lu:\nFunction %s:\n\nAssertion `%s' failed",
		file, line, funcname, expr);
	
#if BUILD_ID == BUILD_INTERNAL
	errmsg += ", aborting.";
#else
	errmsg += ".";
#endif
	
	printf ("%s\n", errmsg.chars ());
	
#if BUILD_ID == BUILD_INTERNAL
	if (g_win)
		g_win->deleteLater ();
	
	bombBox (errmsg);
	abort ();
#endif
}

void fatalError (const char* file, const ulong line, const char* funcname, str msg) {
	str errmsg = fmt ("Aborting over a call to fatal():\nFile: %s\nLine: %lu\nFunction: %s\n\n%s",
		file, line, funcname, msg.chars ());
	
	printf ("%s\n", errmsg.chars ());
	
	if (g_win)
		g_win->deleteLater ();
	
	bombBox (errmsg);
	abort ();
}