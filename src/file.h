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

#ifndef FILE_H
#define FILE_H

#include "common.h"
#include "ldtypes.h"

namespace LDPaths {
	void initPaths ();
	bool tryConfigure (str path);
	
	str ldconfig ();
	str prims ();
	str parts ();
	str getError ();
}

// =============================================================================
// OpenFile
//
// The OpenFile class stores a file opened in LDForge either as a editable file
// for the user or for subfile caching. Its methods handle file input and output.
// =============================================================================
class OpenFile {
public:
	typedef std::vector<LDObject*>::iterator it;
	typedef std::vector<LDObject*>::const_iterator c_it;
	
	str m_filename, m_title;
	vector<LDObject*> m_objs;
	vector<LDObject*> m_objCache; // Cache of this file's contents, if desired
	
	int lastError;
	
	// Is this file implicit? Implicit files are opened automatically for
	// caching purposes and are hidden from the user.
	bool m_implicit;
	
	OpenFile ();
	~OpenFile ();
	
	// Saves this file to disk.
	bool save (str zPath = "");
	
	// Perform safety checks. Do this before closing any files!
	bool safeToClose ();
	
	// Adds an object to this file at the end of the file.
	ulong addObject (LDObject* obj);
	
	// Deletes the given object from the object chain.
	void forgetObject (LDObject* obj);
	
	// At what point was this file last saved?
	long savePos;
	
	LDObject* object (ulong pos) const {
		return m_objs[pos];
	}
	
	void insertObj (const ulong pos, LDObject* obj);
	
	it begin () { return m_objs.begin (); }
	it end () { return m_objs.end (); }
	c_it cbegin () const { return m_objs.cbegin (); }
	c_it cend () const { return m_objs.cend (); }
};

// Close all current loaded files and start off blank.
void newFile ();

// Opens the given file as the main file. Everything is closed first.
void openMainFile (str zPath);

// Finds an OpenFile by name or null if not open
OpenFile* findLoadedFile (str name);

// Opens the given file and parses the LDraw code within. Returns a pointer
// to the opened file or null on error.
OpenFile* openDATFile (str path, bool search, bool mainfile);

// Opens the given file and returns a pointer to it, potentially looking in /parts and /p
FILE* openLDrawFile (str path, bool bSubDirectories);

// Close all open files, whether user-opened or subfile caches.
void closeAll ();

// Parses a string line containing an LDraw object and returns the object parsed.
LDObject* parseLine (str line);

// Retrieves the pointer to - or loads - the given subfile.
OpenFile* loadSubfile (str zFile);

// Re-caches all subfiles.
void reloadAllSubfiles ();

// Is it safe to close all files?
bool safeToCloseAll ();

typedef struct {
	char sName[65], sTitle[81];
} partListEntry;

// Init and parse parts.lst
void initPartList ();

std::vector< LDObject* > loadFileContents (FILE* fp, ulong* numWarnings);

extern vector<OpenFile*> g_loadedFiles;
extern vector<partListEntry> g_PartList;

#endif // FILE_H