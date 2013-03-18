#include <vector>

#include "common.h"
#include "io.h"
#include "misc.h"
#include "bbox.h"
#include "gui.h"

vector<str> g_zaFileLoadPaths;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
OpenFile* findLoadedFile (str name) {
	for (ulong i = 0; i < g_LoadedFiles.size(); i++) {
		OpenFile* const file = g_LoadedFiles[i];
		if (file->zFileName == name)
			return file;
	}
	
	return nullptr;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
OpenFile* openDATFile (str path) {
	logf ("Opening %s...\n", path.chars());
	
	FILE* fp = fopen (path.chars (), "r");
	
	if (!fp) {
		for (ushort i = 0; i < g_zaFileLoadPaths.size(); ++i) {
			str zFilePath = str::mkfmt ("%s/%s", g_zaFileLoadPaths[i].chars(), path.chars());
			fp = fopen (zFilePath.chars (), "r");
			
			if (fp)
				break;
		}
	}
	
	if (!fp) {
		logf (LOG_Error, "Couldn't open %s: %s\n", path.chars (), strerror (errno));
		return nullptr;
	}
	
	OpenFile* load = new OpenFile;
	ulong numWarnings = 0;
	
	load->zFileName = path;
	
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
	
	for (ulong i = 0; i < lines.size(); ++i) {
		LDObject* obj = parseLine (lines[i]);
		load->objects.push_back (obj);
		
		// Check for parse errors and warn abotu tthem
		if (obj->getType() == OBJ_Gibberish) {
			logf (LOG_Warning, "Couldn't parse line #%lu: %s\n",
				i, static_cast<LDGibberish*> (obj)->zReason.chars());
			logf (LOG_Warning, "- Line was: %s\n", lines[i].chars());
			numWarnings++;
		}
	}
	
	g_LoadedFiles.push_back (load);
	
	logf (LOG_Success, "File %s parsed successfully (%lu warning%s).\n",
		path.chars(), numWarnings, PLURAL (numWarnings));
	
	return load;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// Clear everything from the model
void OpenFile::close () {
	for (ulong j = 0; j < objects.size(); ++j)
		delete objects[j];
	
	delete this;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void closeAll () {
	if (!g_LoadedFiles.size())
		return;
	
	// Remove all loaded files and the objects they contain
	for (ushort i = 0; i < g_LoadedFiles.size(); i++) {
		OpenFile* f = g_LoadedFiles[i];
		f->close ();
	}
	
	// Clear the array
	g_LoadedFiles.clear();
	g_CurrentFile = NULL;
	
	g_qWindow->R->hardRefresh();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void newFile () {
	// Create a new anonymous file and set it to our current
	closeAll ();
	
	OpenFile* f = new OpenFile;
	f->zFileName = "";
	g_LoadedFiles.push_back (f);
	g_CurrentFile = f;
	
	g_BBox.calculate();
	g_qWindow->buildObjList ();
	g_qWindow->R->hardRefresh();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void openMainFile (str zPath) {
	closeAll ();
	
	OpenFile* pFile = openDATFile (zPath);
	g_CurrentFile = pFile;
	
	// Recalculate the bounding box
	g_BBox.calculate();
	
	// Rebuild the object tree view now.
	g_qWindow->buildObjList ();
	g_qWindow->setTitle ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void OpenFile::save (str zPath) {
	if (!~zPath)
		zPath = zFileName;
	
	FILE* fp = fopen (zPath, "w");
	if (!fp)
		return;
	
	// Write all entries now
	for (ulong i = 0; i < objects.size(); ++i) {
		LDObject* obj = objects[i];
		
		// LDraw requires lines to have DOS line endings
		str zLine = str::mkfmt ("%s\r\n",obj->getContents ().chars ());
		
		fwrite (zLine.chars(), 1, ~zLine, fp);
	}
	
	fclose (fp);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
#define CHECK_TOKEN_COUNT(N) \
	if (tokens.size() != N) \
		return new LDGibberish (zLine, "Bad amount of tokens");

#define CHECK_TOKEN_NUMBERS(MIN,MAX) \
	for (ushort i = MIN; i <= MAX; ++i) \
		if (!isNumber (tokens[i])) \
			return new LDGibberish (zLine, str::mkfmt ("Token #%u was `%s`, expected a number", \
				(i + 1), tokens[i].chars()));

LDObject* parseLine (str zLine) {
	str zNoWhitespace = zLine;
	stripWhitespace (zNoWhitespace);
	if (!~zNoWhitespace) {
		// Line was empty, or only consisted of whitespace
		return new LDEmpty;
	}
	
	vector<str> tokens = zLine / " ";
	
	// Rid leading all-whitespace tokens
	while (tokens.size() && !(~tokens[0]))
		tokens.erase (tokens.begin());
	
	if (~tokens[0] != 1)
		return new LDGibberish (zLine, "Illogical line code");
	
	char const c = tokens[0][0];
	switch (c - '0') {
	case 0:
		{
			// Comment
			LDComment* obj = new LDComment;
			obj->zText = zLine.substr (2, -1);
			return obj;
		}
	
	case 1:
		{
			// Subfile
			CHECK_TOKEN_COUNT (15)
			CHECK_TOKEN_NUMBERS (1, 13)
			
#ifndef WIN32
			tokens[14].replace ("\\", "/");
#endif // WIN32
			
			// Try open the file
			OpenFile* pFile = findLoadedFile (tokens[14]);
			if (!pFile)
				pFile = openDATFile (tokens[14]);
			
			// If we cannot open the file, mark it an error
			if (!pFile)
				return new LDGibberish (zLine, "Could not open referred file");
			
			LDSubfile* obj = new LDSubfile;
			obj->dColor = atoi (tokens[1]);
			obj->vPosition = parseVertex (zLine, 2); // 2 - 4
			
			for (short i = 0; i < 9; ++i)
				obj->faMatrix[i] = atof (tokens[i + 5]); // 5 - 13
			
			obj->zFileName = tokens[14];
			obj->pFile = pFile;
			return obj;
		}
	
	case 2:
		{
			CHECK_TOKEN_COUNT (8)
			CHECK_TOKEN_NUMBERS (1, 7)
			
			// Line
			LDLine* obj = new LDLine;
			obj->dColor = getWordInt (zLine, 1);
			for (short i = 0; i < 2; ++i)
				obj->vaCoords[i] = parseVertex (zLine, 2 + (i * 3)); // 2 - 7
			return obj;
		}
	
	case 3:
		{
			CHECK_TOKEN_COUNT (11)
			CHECK_TOKEN_NUMBERS (1, 10)
			
			// Triangle
			LDTriangle* obj = new LDTriangle;
			obj->dColor = getWordInt (zLine, 1);
			
			for (short i = 0; i < 3; ++i)
				obj->vaCoords[i] = parseVertex (zLine, 2 + (i * 3)); // 2 - 10
			
			return obj;
		}
	
	case 4:
		{
			CHECK_TOKEN_COUNT (14)
			CHECK_TOKEN_NUMBERS (1, 13)
			
			// Quadrilateral
			LDQuad* obj = new LDQuad;
			obj->dColor = getWordInt (zLine, 1);
			
			for (short i = 0; i < 4; ++i)
				obj->vaCoords[i] = parseVertex (zLine, 2 + (i * 3)); // 2 - 13
			
			return obj;
		}
	
	case 5:
		{
			CHECK_TOKEN_COUNT (14)
			CHECK_TOKEN_NUMBERS (1, 13)
			
			// Conditional line
			LDCondLine* obj = new LDCondLine;
			obj->dColor = getWordInt (zLine, 1);
			
			for (short i = 0; i < 2; ++i)
				obj->vaCoords[i] = parseVertex (zLine, 2 + (i * 3)); // 2 - 7
			
			for (short i = 0; i < 2; ++i)
				obj->vaControl[i] = parseVertex (zLine, 8 + (i * 3)); // 8 - 13
			return obj;
		}
		
	default: // Strange line we couldn't parse
		return new LDGibberish (zLine, "Unknown line code number");
	}
}