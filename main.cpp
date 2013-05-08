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
#include "gui.h"
#include "file.h"
#include "bbox.h"
#include "misc.h"
#include "config.h"
#include "colors.h"
#include "types.h"

vector<OpenFile*> g_loadedFiles;
OpenFile* g_curfile = null;
ForgeWindow* g_win = null; 
bbox g_BBox;
const QApplication* g_app = null;

const vertex g_origin (0.0f, 0.0f, 0.0f);
const matrix<3> g_identity ({1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f});

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
int main (int argc, char* argv[]) {
	// Load or create the configuration
	if (!config::load()) {
		printf ("Creating configuration file...\n");
		if (config::save ())
			printf ("Configuration file successfully created.\n");
		else
			printf ("failed to create configuration file!\n");
	}
	
	const QApplication app (argc, argv);
	LDPaths::initPaths ();
	
	initColors ();
	initPartList ();
	
	ForgeWindow* win = new ForgeWindow;
	
	g_app = &app;
	
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

void warnf (const char* fmtstr, ...) {
	va_list va;
	va_start (va, fmtstr);
	g_win->logVA (LOG_Warning, fmtstr, va);
	va_end (va);
}

void errf (const char* fmtstr, ...) {
	va_list va;
	va_start (va, fmtstr);
	g_win->logVA (LOG_Error, fmtstr, va);
	va_end (va);
}

#ifndef RELEASE
void devf (const char* fmtstr, ...) {
	va_list va;
	va_start (va, fmtstr);
	g_win->logVA (LOG_Dev, fmtstr, va);
	va_end (va);
}
#endif // RELEASE