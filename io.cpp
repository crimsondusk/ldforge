#include <vector>

#include "common.h"
#include "io.h"
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
		if (!file->zFileName.icompare (name))
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
	logf ("Opening %s...\n", path.chars());
	
	FILE* fp = fopen (path.chars (), "r");
	
	if (!fp) {
		logf (LOG_Error, "Couldn't open %s: %s\n", path.chars (), strerror (errno));
		return NULL;
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
		LDObject* obj = ParseLine (lines[i]);
		load->objects.push_back (obj);
		
		// Check for warnings
		if (obj->getType() == OBJ_Gibberish) {
			logf (LOG_Warning, "Couldn't parse line #%lu: %s\n",
				i, static_cast<LDGibberish*> (obj)->zReason.chars());
			logf (LOG_Warning, "- Line was: %s\n", lines[i].chars());
			numWarnings++;
		}
	}
	
	// Serialize all of the objects
	for (ulong i = 0; i < load->objects.size(); ++i)
		load->objects[i].serialize ();
	
	g_LoadedFiles.push_back (load);
	g_CurrentFile = g_LoadedFiles[g_LoadedFiles.size() - 1];
	
	// Recalculate the bounding box
	g_BBox.calculate();
	
	// Rebuild the object tree view now.
	g_qWindow->buildObjList ();
	g_qWindow->setTitle ();
	
	logf (LOG_Success, "File %s parsed successfully (%lu warning%s).\n",
		path.chars(), numWarnings, PLURAL (numWarnings));
	
	return g_CurrentFile;
}

// =============================================================================
// isNumber (char*)
//
// Returns whether a given string represents a floating point number
// TODO: Does LDraw support scientific notation?
// =============================================================================
static bool isNumber (char* sToken) {
	char* sPointer = &sToken[0];
	bool bGotDot = false;
	
	// Allow leading hyphen for negatives
	if (*sPointer == '-')
		sPointer++;
	
	while (*sPointer != '\0') {
		if (*sPointer == '.' && !bGotDot) {
			// Decimal point
			bGotDot = true;
			sPointer++;
			continue;
		}
		
		if (*sPointer >= '0' && *sPointer <= '9') {
			sPointer++;
			continue; // Digit
		}
		
		// If the above cases didn't catch this character, it was
		// illegal and this is therefore not a number.
		return false;
	}
	
	return true;
}

// =============================================================================
// ParseLine (str)
//
// Parses a string line containing an LDraw object and returns the object parsed.
// =============================================================================
#define CHECK_TOKEN_COUNT(N) \
	if (tokens.size() != N) \
		return new LDGibberish (zLine, "Bad amount of tokens");

#define CHECK_TOKEN_NUMBERS(MIN,MAX) \
	for (ushort i = MIN; i <= MAX; ++i) \
		if (!isNumber (tokens[i])) \
			return new LDGibberish (zLine, str::mkfmt ("Token #%u was `%s`, expected a number", \
				(i + 1), tokens[i].chars()));

LDObject* ParseLine (str zLine) {
	str zNoWhitespace = zLine;
	StripWhitespace (zNoWhitespace);
	if (!~zNoWhitespace) {
		// Line was empty, or only consisted of whitespace
		return new LDEmpty;
	}
	
	char c = zLine[0];
	vector<str> tokens = zLine / " ";
	
	if (~tokens[0] != 1)
		return new LDGibberish (zLine, "Illogical line code");
	
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
			CHECK_TOKEN_COUNT (15)
			CHECK_TOKEN_NUMBERS (1, 13)
			
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
			CHECK_TOKEN_COUNT (8)
			CHECK_TOKEN_NUMBERS (1, 7)
			
			// Line
			LDLine* obj = new LDLine;
			obj->dColor = GetWordInt (zLine, 1);
			for (short i = 0; i < 2; ++i)
				obj->vaCoords[i] = ParseVertex (zLine, 2 + (i * 3)); // 2 - 7
			return obj;
		}
	
	case 3:
		{
			CHECK_TOKEN_COUNT (11)
			CHECK_TOKEN_NUMBERS (1, 10)
			
			// Triangle
			LDTriangle* obj = new LDTriangle;
			obj->dColor = GetWordInt (zLine, 1);
			
			for (short i = 0; i < 3; ++i)
				obj->vaCoords[i] = ParseVertex (zLine, 2 + (i * 3)); // 2 - 10
			
			return obj;
		}
	
	case 4:
		{
			CHECK_TOKEN_COUNT (14)
			CHECK_TOKEN_NUMBERS (1, 13)
			
			// Quadrilateral
			LDQuad* obj = new LDQuad;
			obj->dColor = GetWordInt (zLine, 1);
			
			for (short i = 0; i < 4; ++i)
				obj->vaCoords[i] = ParseVertex (zLine, 2 + (i * 3)); // 2 - 13
			
			return obj;
		}
	
	case 5:
		{
			CHECK_TOKEN_COUNT (14)
			CHECK_TOKEN_NUMBERS (1, 13)
			
			// Conditional line
			LDCondLine* obj = new LDCondLine;
			obj->dColor = GetWordInt (zLine, 1);
			
			for (short i = 0; i < 2; ++i)
				obj->vaCoords[i] = ParseVertex (zLine, 2 + (i * 3)); // 2 - 7
			
			for (short i = 0; i < 2; ++i)
				obj->vaControl[i] = ParseVertex (zLine, 8 + (i * 3)); // 8 - 13
			return obj;
		}
		
	default: // Strange line we couldn't parse
		return new LDGibberish (zLine, "Unknown line code number");
	}
}