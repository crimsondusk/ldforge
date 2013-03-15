#include <vector>

#include "common.h"
#include "io.h"
#include "ldtypes.h"
#include "misc.h"
#include "gui.h"
#include "bbox.h"

// =============================================================================
// IO_FindLoadedFile (str)
//
// Returns a pointer to the first found open file with the given name.
// =============================================================================
OpenFile* IO_FindLoadedFile (str name) {
	OpenFile* file;
	
	for (uint i = 0; i < g_LoadedFiles.size(); i++) {
		file = g_LoadedFiles[i];
		if (!file->filename.icompare (name))
			return file;
	}
	
	return NULL;
}

// =============================================================================
// IO_OpenLDrawFile (str)
//
// Opens the given file and parses the LDraw code within.
// =============================================================================
OpenFile* IO_OpenLDrawFile (str path) {
	FILE* fp = fopen (path.chars (), "r");
	
	if (!fp) {
		printf ("Couldn't open %s!\n", path.chars ());
		return NULL;
	}
	
	OpenFile* load = new OpenFile;
	load->filename = path;
	
	vector<str> lines;
	
	{
		char line[1024];
		while (fgets (line, sizeof line, fp)) {
			// Trim the trailing newline
			str zLine = line;
			while (zLine[~zLine - 1] == '\n' || zLine[~zLine - 1] == '\r')
				zLine -= 1;
			
			lines.push_back (zLine);
		}
	}
	
	fclose (fp);
	
	for (ulong i = 0; i < lines.size(); ++i)
		load->objects.push_back (ParseLine (lines[i]));
	
	g_LoadedFiles.push_back (load);
	g_CurrentFile = g_LoadedFiles[g_LoadedFiles.size() - 1];
	
	// Recalculate the bounding box
	g_BBox.calculate();
	
	// Rebuild the object tree view now.
	g_qWindow->buildObjList ();
	
	return g_CurrentFile;
}

// =============================================================================
// ParseLine (str)
//
// Parses a string line containing an LDraw object and returns the object parsed.
// =============================================================================
LDObject* ParseLine (str zLine) {
	str zNoWhitespace = zLine;
	StripWhitespace (zNoWhitespace);
	if (!~zNoWhitespace) {
		// Line was empty, or only consisted of whitespace
		return new LDEmpty;
	}
	
	char c = zLine[0];
	vector<str> tokens = zLine / " ";
	printf ("line: %s\n", zLine.chars());
	
	switch (c - '0') {
	case 0:
		{
			// Comment
			LDComment* obj = new LDComment;
			obj->zText = zLine.substr (1, -1);
			printf ("\t-> comment (%s)\n", obj->zText.chars());
			return obj;
		}
	
	case 1:
		{
			// Subfile
			LDSubfile* obj = new LDSubfile;
			obj->dColor = atoi (tokens[1]);
			obj->vPosition = ParseVertex (zLine, 2); // 2 - 4
			
			for (short i = 0; i < 9; ++i)
				obj->faMatrix[i] = atof (tokens[i + 5]); // 5 - 13
			
			obj->zFileName = tokens[14];
			return obj;
		}
	
	case 2:
		{
			// Line
			LDLine* obj = new LDLine;
			obj->dColor = GetWordInt (zLine, 1);
			for (short i = 0; i < 2; ++i)
				obj->vaCoords[i] = ParseVertex (zLine, 2 + (i * 3)); // 2 - 7
			return obj;
		}
	
	case 3:
		{
			// Triangle
			LDTriangle* obj = new LDTriangle;
			obj->dColor = GetWordInt (zLine, 1);
			
			for (short i = 0; i < 3; ++i)
				obj->vaCoords[i] = ParseVertex (zLine, 2 + (i * 3)); // 2 - 10
			
			return obj;
		}
	
	case 4:
		{
			// Quadrilateral
			LDQuad* obj = new LDQuad;
			obj->dColor = GetWordInt (zLine, 1);
			
			for (short i = 0; i < 4; ++i)
				obj->vaCoords[i] = ParseVertex (zLine, 2 + (i * 3)); // 2 - 13
			
			return obj;
		}
	
	case 5:
		{
			// Conditional line
			LDCondLine* obj = new LDCondLine;
			obj->dColor = GetWordInt (zLine, 1);
			
			for (short i = 0; i < 2; ++i)
				obj->vaCoords[i] = ParseVertex (zLine, 2 + (i * 3)); // 2 - 7
			
			for (short i = 0; i < 2; ++i)
				obj->vaControl[i] = ParseVertex (zLine, 8 + (i * 3)); // 8 - 13
			return obj;
		}
		
	default:
		{
			// Strange line we couldn't parse
			LDGibberish* obj = new LDGibberish;
			obj->zContent = zLine;
			return obj;
		}
	}
}