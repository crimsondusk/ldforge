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

#ifndef __FILE_H__
#define __FILE_H__

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
	vector<LDObject*> objCache; // Cache of this file's contents, if desired
	
	int lastError;
	
	// Is this file implicit? Implicit files are opened automatically for
	// caching purposes and are hidden from the user.
	bool implicit;
	
	OpenFile ();
	~OpenFile ();
	
	// Saves this file to disk.
	bool save (str zPath = "");
	
	// Perform safety checks. Do this before closing any files!
	bool safeToClose ();
	
	// Adds an object to this file at the appropriate location.
	ulong addObject (LDObject* obj);
	
	// Deletes the given object from the object chain.
	void forgetObject (LDObject* obj);
	
	// At what point was this file last saved?
	long savePos;
	
	LDObject* object (size_t uPos) const {
		return objects[uPos];
	}
};

// Close all current loaded files and start off blank.
void newFile ();

// Opens the given file as the main file. Everything is closed first.
void openMainFile (str zPath);

// Finds an OpenFile by name or null if not open
OpenFile* findLoadedFile (str name);

// Opens the given file and parses the LDraw code within. Returns a pointer
// to the opened file or null on error.
OpenFile* openDATFile (str path, bool search);

// Opens the given file and returns a pointer to it, potentially looking in /parts and /p
FILE* openLDrawFile (str path, bool bSubDirectories);

// Close all open files, whether user-opened or subfile caches.
void closeAll ();

// Parses a string line containing an LDraw object and returns the object parsed.
LDObject* parseLine (str zLine);

// Retrieves the pointer to - or loads - the given subfile.
OpenFile* loadSubfile (str zFile);

// Re-caches all subfiles.
void reloadAllSubfiles ();

typedef struct {
	char sName[65], sTitle[81];
} partListEntry;

// Init and parse parts.lst
void initPartList ();

extern vector<OpenFile*> g_LoadedFiles;

#endif // __FILE_H__