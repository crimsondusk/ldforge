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

vector<OpenFile*> g_LoadedFiles;
OpenFile* g_CurrentFile = null;
ForgeWindow* g_ForgeWindow = null; 
bbox g_BBox;
QApplication* g_qMainApp = null;

const vertex g_Origin (0.0f, 0.0f, 0.0f);
const matrix g_mIdentity (1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
int main (int dArgc, char* saArgv[]) {
	// Load or create the configuration
	if (!config::load()) {
		printf ("Creating configuration file...\n");
		if (config::save ())
			printf ("Configuration file successfully created.\n");
		else
			printf ("failed to create configuration file!\n");
	}
	
	initColors ();
	initPartList ();
	
	QApplication app (dArgc, saArgv);
	ForgeWindow* win = new ForgeWindow;
	
	g_qMainApp = &app;
	
	newFile ();
	
	win->show ();
	return app.exec ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// Common code for the two logfs
static void logVA (logtype_e eType, const char* fmt, va_list va) {
	return;
	
	char* sBuffer = vdynformat (fmt, va, 128);
	printf ("buffer: %s\n", sBuffer);
	str zText (sBuffer);
	delete[] sBuffer;
	
	// Log it to standard output
	printf ("%s", zText.chars ());
	
	// Replace some things out with HTML entities
	zText.replace ("<", "&lt;");
	zText.replace (">", "&gt;");
	zText.replace ("\n", "<br />");
	
	str& zLog = g_ForgeWindow->zMessageLogHTML;
	
	switch (eType) {
	case LOG_Normal:
		printf ("appending \"%s\"\n", zText.chars ());
		zLog.append (zText);
		break;
	
	case LOG_Error:
		zLog.appendformat ("<span style=\"color: #F8F8F8; background-color: #800\"><b>[ERROR]</b> %s</span>",
			zText.chars());
		break;
	
	case LOG_Info:
		zLog.appendformat ("<span style=\"color: #04F\"><b>[INFO]</b> %s</span>",
			zText.chars());
		break;
	
	case LOG_Success:
		zLog.appendformat ("<span style=\"color: #6A0\"><b>[SUCCESS]</b> %s</span>",
			zText.chars());
		break;
	
	case LOG_Warning:
		zLog.appendformat ("<span style=\"color: #C50\"><b>[WARNING]</b> %s</span>",
			zText.chars());
		break;
	}
	
	g_ForgeWindow->qMessageLog->setHtml (zLog);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void logf (const char* fmt, ...) {
	va_list va;
	
	va_start (va, fmt);
	logVA (LOG_Normal, fmt, va);
	va_end (va);
}

void logf (logtype_e eType, const char* fmt, ...) {
	va_list va;
	
	va_start (va, fmt);
	logVA (eType, fmt, va);
	va_end (va);
}
