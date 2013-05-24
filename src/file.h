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
#include <QObject>

class OpenProgressDialog;
namespace LDPaths {
	void initPaths ();
	bool tryConfigure (str path);
	
	str ldconfig ();
	str prims ();
	str parts ();
	str getError ();
}

// =============================================================================
// LDOpenFile
//
// The LDOpenFile class stores a file opened in LDForge either as a editable file
// for the user or for subfile caching. Its methods handle file input and output.
// 
// A file is implicit when they are opened automatically for caching purposes
// and are hidden from the user. User-opened files are explicit (not implicit).
// =============================================================================
class LDOpenFile {
	PROPERTY (str, name, setName)
	PROPERTY (bool, implicit, setImplicit)
	MUTABLE_READ_PROPERTY (vector<LDObject*>, objs)
	PROPERTY (vector<LDObject*>, cache, setCache)
	PROPERTY (long, savePos, setSavePos)
	
public:
	LDOpenFile ();
	~LDOpenFile ();
	
	// Saves this file to disk.
	bool save (str zPath = "");
	
	// Perform safety checks. Do this before closing any files!
	bool safeToClose ();
	
	// Adds an object to this file at the end of the file.
	ulong addObject (LDObject* obj);
	
	// Deletes the given object from the object chain.
	void forgetObject (LDObject* obj);
	
	LDObject* object (ulong pos) const { return m_objs[pos]; }
	LDObject* obj (ulong pos) const { return object (pos); }
	
	void insertObj (const ulong pos, LDObject* obj);
	ulong numObjs () const { return m_objs.size (); }
	void setObject (ulong idx, LDObject* obj);
};

// Close all current loaded files and start off blank.
void newFile ();

// Opens the given file as the main file. Everything is closed first.
void openMainFile (str zPath);

// Finds an OpenFile by name or null if not open
LDOpenFile* findLoadedFile (str name);

// Opens the given file and parses the LDraw code within. Returns a pointer
// to the opened file or null on error.
LDOpenFile* openDATFile (str path, bool search);

// Opens the given file and returns a pointer to it, potentially looking in /parts and /p
FILE* openLDrawFile (str path, bool bSubDirectories);

// Close all open files, whether user-opened or subfile caches.
void closeAll ();

// Parses a string line containing an LDraw object and returns the object parsed.
LDObject* parseLine (str line);

// Retrieves the pointer to - or loads - the given subfile.
LDOpenFile* loadSubfile (str zFile);

// Re-caches all subfiles.
void reloadAllSubfiles ();

// Is it safe to close all files?
bool safeToCloseAll ();

typedef struct {
	char sName[65], sTitle[81];
} partListEntry;

// Init and parse parts.lst
void initPartList ();

vector<LDObject*> loadFileContents (FILE* fp, ulong* numWarnings, bool* ok = null);

extern vector<LDOpenFile*> g_loadedFiles;
extern vector<partListEntry> g_PartList;

str basename (str path);
str dirname (str path);

class FileLoader : public QObject {
	Q_OBJECT
	
	READ_PROPERTY (vector<LDObject*>, objs)
	READ_PROPERTY (bool, done)
	READ_PROPERTY (ulong, progress)
	PROPERTY (FILE*, filePointer, setFilePointer)
	PROPERTY (ulong*, warningsPointer, setWarningsPointer)
	
public:
	bool abortflag;
	
public slots:
	void work ();
	
signals:
	void progressUpdate (int progress);
	void workDone ();
};

#endif // FILE_H