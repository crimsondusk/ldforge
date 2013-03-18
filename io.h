#ifndef __IO_H__
#define __IO_H__

#include "common.h"
#include "ldtypes.h"
#include "str.h"

// =============================================================================
// OpenFile
//
// The OpenFile class stores a file opened in LDForge either as a editable file
// for the user or for subfile caching. Its methods handle file input and output.
// =============================================================================
class OpenFile {
public:
	str zFileName, zTitle;
	vector<LDObject*> objects;
	
	// Closes this OpenFile. The object is deleted in the process.
	void close ();
	
	// Saves this file to disk.
	void save (str zPath = "");
};

// Close all current loaded files and start off blank.
void newFile ();

// Opens the given file as the main file. Everything is closed first.
void openMainFile (str zPath);

// Finds an OpenFile by name or nullptr if not open
OpenFile* findLoadedFile (str name);

// Opens the given file and parses the LDraw code within. Returns a pointer
// to the opened file or nullptr on error.
OpenFile* openDATFile (str path);

// Close all open files, whether user-opened or subfile caches.
void closeAll ();

// Parses a string line containing an LDraw object and returns the object parsed.
LDObject* parseLine (str zLine);

extern vector<str> g_zaFileLoadPaths;
extern vector<OpenFile*> g_LoadedFiles;

#endif