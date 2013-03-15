#include <QApplication>
#include "gui.h"
#include "io.h"
#include "bbox.h"

vector<OpenFile*> g_LoadedFiles;
OpenFile* g_CurrentFile = NULL;
LDForgeWindow* g_qWindow = NULL; 
bbox g_BBox;

int main (int argc, char** argv) {
	g_CurrentFile = IO_ParseLDFile ("55966.dat");
	g_BBox.calculate();
	
	if (g_CurrentFile) {
		printf ("bbox: (%.3f, %.3f, %.3f), (%.3f, %.3f, %.3f)\n",
			FVERTEX (g_BBox.v0), FVERTEX (g_BBox.v1));
		printf ("%u objects\n", g_CurrentFile->objects.size());
	}
	
	QApplication app (argc, argv);
	LDForgeWindow* win = new LDForgeWindow;
	g_qWindow = win;
	g_qWindow->buildObjList ();
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

static str getCoordinateRep (double fCoord) {
	str zRep = str::mkfmt ("%.3f", fCoord);
	
	// Remove trailing zeroes
	while (zRep[~zRep - 1] == '0')
		zRep -= 1;
	
	// If there was only zeroes in the decimal place, remove
	// the decimal point now.
	if (zRep[~zRep - 1] == '.')
		zRep -= 1;
	
	return zRep;
}

str vertex::getStringRep () {
	return str::mkfmt ("(%s, %s, %s)",
		getCoordinateRep (x).chars(),
		getCoordinateRep (y).chars(),
		getCoordinateRep (z).chars());
}