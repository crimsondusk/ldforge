#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "io.h"
#include "gui.h"
#include "draw.h"

// Clear everything from the model
void closeModel () {
	// Remove all loaded files and the objects they contain
	for (ushort i = 0; i < g_LoadedFiles.size(); i++) {
		OpenFile* f = g_LoadedFiles[i];
		
		for (ushort j = 0; j < f->objects.size(); ++j)
			delete (LDObject*)f->objects[j];
		
		delete f;
	}
	
	// Clear the array
	g_LoadedFiles.clear();
	g_CurrentFile = NULL;
	
	g_qWindow->R->hardRefresh();
}

void newModel () {
	// Create a new anonymous file and set it to our current
	if (g_LoadedFiles.size())
		closeModel (); // Close any open file first, though
	
	OpenFile* f = new OpenFile;
	f->zFileName = "";
	g_LoadedFiles.push_back (f);
	g_CurrentFile = f;
	
	g_qWindow->R->hardRefresh();
}

void saveModel () {
	if (!g_CurrentFile)
		return;
	
	FILE* fp = fopen (g_CurrentFile->zFileName, "w");
	if (!fp)
		return;
	
	// Write all entries now
	for (ulong i = 0; i < g_CurrentFile->objects.size(); ++i) {
		LDObject* obj = g_CurrentFile->objects[i];
		
		// LDraw requires lines to have DOS line endings
		str zLine = str::mkfmt ("%s\r\n",obj->getContents ().chars ());
		
		fwrite (zLine.chars(), 1, ~zLine, fp);
	}
	
	fclose (fp);
}