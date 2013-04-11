/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri `arezey` Piippo
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
OpenFile* g_CurrentFile = nullptr;
ForgeWindow* g_ForgeWindow = nullptr; 
bbox g_BBox;
QApplication* g_qMainApp = nullptr;

const vertex g_Origin (0.0f, 0.0f, 0.0f);

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
	
	QApplication app (dArgc, saArgv);
	ForgeWindow* win = new ForgeWindow;
	
	g_qMainApp = &app;
	g_ForgeWindow = win;
	
	newFile ();
	
	win->show ();
	return app.exec ();
}

// =============================================================================
// void logVA (logtype_e, const char*, va_list) [static]
//
// Common code for the two logfs
// =============================================================================
static void logVA (logtype_e eType, const char* fmt, va_list va) {
	// Log it to standard output
	vprintf (fmt, va);
	
	return;
	
	str zText;
	char* sBuffer;
	
	sBuffer = vdynformat (fmt, va, 128);
	zText = sBuffer;
	delete[] sBuffer;
	
	// Replace some things out with HTML entities
	zText.replace ("<", "&lt;");
	zText.replace (">", "&gt;");
	zText.replace ("\n", "<br />");
	
	str* zpHTML = &g_ForgeWindow->zMessageLogHTML;
	
	switch (eType) {
	case LOG_Normal:
		zpHTML->append (zText);
		break;
	
	case LOG_Error:
		zpHTML->appendformat ("<span style=\"color: #F8F8F8; background-color: #800\"><b>[ERROR]</b> %s</span>",
			zText.chars());
		break;
	
	case LOG_Info:
		zpHTML->appendformat ("<span style=\"color: #04F\"><b>[INFO]</b> %s</span>",
			zText.chars());
		break;
	
	case LOG_Success:
		zpHTML->appendformat ("<span style=\"color: #6A0\"><b>[SUCCESS]</b> %s</span>",
			zText.chars());
		break;
	
	case LOG_Warning:
		zpHTML->appendformat ("<span style=\"color: #C50\"><b>[WARNING]</b> %s</span>",
			zText.chars());
		break;
	}
	
	g_ForgeWindow->qMessageLog->setHtml (*zpHTML);
}


// =============================================================================
// logf (const char*, ...)
// logf (logtype_e eType, const char*, ...)
//
// Outputs a message into the message log
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