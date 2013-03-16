#include <QApplication>
#include "gui.h"
#include "io.h"
#include "bbox.h"
#include "misc.h"

vector<OpenFile*> g_LoadedFiles;
OpenFile* g_CurrentFile = NULL;
LDForgeWindow* g_qWindow = NULL; 
bbox g_BBox;

int main (int argc, char** argv) {
	if (g_CurrentFile) {
		printf ("bbox: (%.3f, %.3f, %.3f), (%.3f, %.3f, %.3f)\n",
			FVERTEX (g_BBox.v0), FVERTEX (g_BBox.v1));
		printf ("%u objects\n", g_CurrentFile->objects.size());
	}
	
	QApplication app (argc, argv);
	LDForgeWindow* win = new LDForgeWindow;
	g_qWindow = win;
	win->show ();
	return app.exec ();
}

vertex vertex::midpoint (vertex& other) {
	vertex mid;
	mid.x = (x + other.x);
	mid.y = (y + other.y);
	mid.z = (z + other.z);
	return mid;
}

str vertex::getStringRep (const bool bMangled) {
	const char* sFormat = (bMangled) ? "(%s, %s, %s)" : "%s %s %s";
	
	return str::mkfmt (sFormat,
		ftoa (x).chars(),
		ftoa (y).chars(),
		ftoa (z).chars());
}

// =============================================================================
// void logVA (logtype_e, const char*, va_list) [static]
//
// Common code for the two logfs
// =============================================================================
static void logVA (logtype_e eType, const char* fmt, va_list va) {
	str zText;
	char* sBuffer;
	
	// Log it to standard output
	vprintf (fmt, va);
	
	sBuffer = vdynformat (fmt, va, 128);
	zText = sBuffer;
	delete[] sBuffer;
	
	
	// Replace some things out with HTML entities
	zText.replace ("<", "&lt;");
	zText.replace (">", "&gt;");
	zText.replace ("\n", "<br />");
	
	str zHTML;
	
	switch (eType) {
	case LOG_Normal:
		zHTML = zText;
		break;
	
	case LOG_Error:
		zHTML.format ("<span style=\"color: #F8F8F8; background-color: #800\"><b>[ERROR]</b> %s</span>",
			zText.chars());
		break;
	
	case LOG_Info:
		zHTML.format ("<span style=\"color: #04F\"><b>[INFO]</b> %s</span>",
			zText.chars());
		break;
	
	case LOG_Success:
		zHTML.format ("<span style=\"color: #6A0\"><b>[SUCCESS]</b> %s</span>",
			zText.chars());
		break;
	
	case LOG_Warning:
		zHTML.format ("<span style=\"color: #C50\"><b>[WARNING]</b> %s</span>",
			zText.chars());
		break;
	}
	
	g_qWindow->zMessageLogHTML += zHTML;
	g_qWindow->qMessageLog->setHtml (g_qWindow->zMessageLogHTML);
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