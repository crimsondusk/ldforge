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

#include <vector>
#include <stdio.h>
#include <qmessagebox.h>
#include "common.h"
#include "config.h"
#include "file.h"
#include "misc.h"
#include "bbox.h"
#include "gui.h"
#include "history.h"

cfg (str, io_ldpath, "");
cfg (str, io_recentfiles, "");

// =============================================================================
OpenFile::OpenFile () {
	implicit = true;
	savePos = -1;
}

// =============================================================================
OpenFile::~OpenFile () {
	// Clear everything from the model
	for (LDObject* obj : objects)
		delete obj;
	
	// Clear the cache as well
	for (LDObject* obj : objCache)
		delete obj;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
OpenFile* findLoadedFile (str zName) {
	for (OpenFile* file : g_LoadedFiles)
		if (file->zFileName == zName)
			return file;
	
	return null;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
FILE* openLDrawFile (str path, bool bSubDirectories) {
	str zTruePath = path;
	
#ifndef WIN32
	zTruePath.replace ("\\", "/");
#endif // WIN32
	
	FILE* fp = fopen (path.chars (), "r");
	str zFilePath;
	
	if (fp != null)
		return fp;
	
	if (~io_ldpath.value) {
		// Try with just the LDraw path first
		zFilePath = format ("%s" DIRSLASH "%s",
			io_ldpath.value.chars(), zTruePath.chars());
		logf ("Trying %s\n", zFilePath.chars());
		
		fp = fopen (zFilePath, "r");
		if (fp != null)
			return fp;
		
		if (bSubDirectories) {
			char const* saSubdirectories[] = {
				"parts",
				"p",
			};
			
			for (char const* sSubdir : saSubdirectories) {
				zFilePath = format ("%s" DIRSLASH "%s" DIRSLASH "%s",
					io_ldpath.value.chars(), sSubdir, zTruePath.chars());
				printf ("try %s\n", zFilePath.chars());
				
				fp = fopen (zFilePath.chars (), "r");
				
				if (fp)
					return fp;
			}
		}
	}
	
	return null;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
std::vector<LDObject*> loadFileContents (FILE* fp, ulong* numWarnings) {
	char line[1024];
	vector<str> lines;
	vector<LDObject*> objs;
	ulong lnum = 0;
	
	if (numWarnings)
		*numWarnings = 0;
	
	while (fgets (line, sizeof line, fp)) {
		// Trim the trailing newline
		str data = line;
		while (data[~data - 1] == '\n' || data[~data - 1] == '\r')
			data -= 1;
		
		LDObject* obj = parseLine (data);
		assert (obj != null);
		
		// Check for parse errors and warn about tthem
		if (obj->getType() == OBJ_Gibberish) {
			logf (LOG_Warning, "Couldn't parse line #%lu: %s\n",
				lnum, static_cast<LDGibberish*> (obj)->zReason.chars());
			
			logf (LOG_Warning, "- Line was: %s\n", data.chars());
			
			if (numWarnings)
				(*numWarnings)++;
		}
		
		objs.push_back (obj);
		lnum++;
	}
	
	return objs;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
OpenFile* openDATFile (str path, bool search) {
	logf ("Opening %s...\n", path.chars());
	
	// Convert the file name to lowercase since some parts contain uppercase
	// file names. I'll assume here that the library will always use lowercase
	// file names for the actual parts..
	FILE* fp;
	if (search)
		fp = openLDrawFile (-path, true);
	else
		fp = fopen (path, "r");
	
	if (!fp) {
		logf (LOG_Error, "Couldn't open %s: %s\n", path.chars (), strerror (errno));
		return null;
	}
	
	OpenFile* load = new OpenFile;
	load->zFileName = path;
	ulong numWarnings;
	std::vector<LDObject*> objs = loadFileContents (fp, &numWarnings);
	
	for (LDObject* obj : objs)
		load->objects.push_back (obj);
	
	fclose (fp);
	g_LoadedFiles.push_back (load);
	
	logf (LOG_Success, "File %s parsed successfully (%lu warning%s).\n",
		path.chars(), numWarnings, PLURAL (numWarnings));
	
	return load;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool OpenFile::safeToClose () {
	setlocale (LC_ALL, "C");
	
	// If we have unsaved changes, warn and give the option of saving.
	if (!implicit && History::pos () != savePos) {
		switch (QMessageBox::question (g_ForgeWindow, "Unsaved Changes",
			format ("There are unsaved changes to %s. Should it be saved?", zFileName.chars ()),
			(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel), QMessageBox::Cancel))
		{
		case QMessageBox::Yes:
			if (!save ()) {
				str errormsg = format ("Failed to save %s: %s\nDo you still want to close?",
					zFileName.chars (), strerror (lastError));
				
				if (QMessageBox::critical (g_ForgeWindow, "Save Failure", errormsg,
					(QMessageBox::Yes | QMessageBox::No), QMessageBox::No) == QMessageBox::No)
				{
					return false;
				}
			}
			
			break;
		
		case QMessageBox::Cancel:
			return false;
		
		default:
			break;
		}
	}
	
	return true;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void closeAll () {
	if (!g_LoadedFiles.size())
		return;
	
	// Remove all loaded files and the objects they contain
	for (OpenFile* file : g_LoadedFiles)
		delete file;
	
	// Clear the array
	g_LoadedFiles.clear();
	g_CurrentFile = NULL;
	
	g_ForgeWindow->refresh ();
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
	g_ForgeWindow->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void addRecentFile (str zPath) {
	long lPos = io_recentfiles.value.first (zPath);
	
	// If this file already is in the list, pop it out.
	if (lPos != -1) {
		if (~io_recentfiles.value == ~zPath)
			return; // only recent file - do nothing
		
		// Pop it out.
		str zFront = io_recentfiles.value.substr (0, lPos);
		str zBack = io_recentfiles.value.substr (lPos + ~zPath + 1, -1);
		io_recentfiles.value = zFront + zBack;
	}
	
	// If there's too many recent files, drop one out.
	while (io_recentfiles.value.count ('@') > 3)
		io_recentfiles.value = io_recentfiles.value.substr (io_recentfiles.value.first ("@") + 1, -1);
	
	// Add the file
	if (~io_recentfiles.value > 0)
		io_recentfiles.value += "@";
	
	io_recentfiles += zPath;
	
	config::save ();
	g_ForgeWindow->updateRecentFilesMenu ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void openMainFile (str zPath) {
	closeAll ();
	
	OpenFile* pFile = openDATFile (zPath, false);
	
	if (!pFile) {
		// Tell the user loading failed.
		setlocale (LC_ALL, "C");
		QMessageBox::critical (g_ForgeWindow, "Load Failure",
			format ("Failed to open %s\nReason: %s", zPath.chars(), strerror (errno)),
			(QMessageBox::Close), QMessageBox::Close);
		
		return;
	}
	
	pFile->implicit = false;
	g_CurrentFile = pFile;
	
	// Recalculate the bounding box
	g_BBox.calculate();
	
	// Rebuild the object tree view now.
	g_ForgeWindow->refresh ();
	g_ForgeWindow->setTitle ();
	
	// Add it to the recent files list.
	addRecentFile (zPath);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool OpenFile::save (str path) {
	if (!~path)
		path = zFileName;
	
	FILE* fp = fopen (path, "w");
	
	if (!fp) {
		lastError = errno;
		return false;
	}
	
	// Write all entries now
	for (LDObject* obj : objects) {
		// LDraw requires lines to have DOS line endings
		str zLine = format ("%s\r\n", obj->getContents ().chars ());
		
		fwrite (zLine.chars(), 1, ~zLine, fp);
	}
	
	fclose (fp);
	
	// We have successfully saved, update the save position now.
	savePos = History::pos ();
	
	g_ForgeWindow->setTitle ();
	
	return true;
}

#define CHECK_TOKEN_COUNT(N) \
	if (tokens.size() != N) \
		return new LDGibberish (zLine, "Bad amount of tokens");

#define CHECK_TOKEN_NUMBERS(MIN,MAX) \
	for (ushort i = MIN; i <= MAX; ++i) \
		if (!isNumber (tokens[i])) \
			return new LDGibberish (zLine, format ("Token #%u was `%s`, expected a number", \
				(i + 1), tokens[i].chars()));

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static vertex parseVertex (vector<str>& s, const ushort n) {
	// Disable the locale while parsing the line or atof's behavior changes
	// between locales (i.e. fails to read decimals properly). That is
	// quite undesired...
	setlocale (LC_NUMERIC, "C");
	
	vertex v;
	v.x = atof (s[n]);
	v.y = atof (s[n + 1]);
	v.z = atof (s[n + 2]);
	
	return v;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
LDObject* parseLine (str zLine) {
	vector<str> tokens = zLine.split (" ", true);
	
	if (!tokens.size ()) {
		// Line was empty, or only consisted of whitespace
		return new LDEmpty;
	}
	
	if (~tokens[0] != 1)
		return new LDGibberish (zLine, "Illogical line code");
	
	const char c = tokens[0][0];
	switch (c - '0') {
	case 0:
		{
			// Comment
			str zComment;
			for (uint i = 1; i < tokens.size(); ++i) {
				zComment += tokens[i];
				
				if (i != tokens.size() - 1)
					zComment += ' ';
			}
			
			// Handle BFC statements
			if (tokens.size() > 2 && tokens[1] == "BFC") {
				for (short i = 0; i < LDBFC::NumStatements; ++i)
					if (zComment == format ("BFC %s", LDBFC::saStatements [i]))
						return new LDBFC ((LDBFC::Type) i);
				
				// MLCAD is notorious for stuffing these statements in parts it
				// creates. The above block only handles valid statements, so we
				// need to handle MLCAD-style invertnext separately.
				if (zComment == "BFC CERTIFY INVERTNEXT")
					return new LDBFC (LDBFC::InvertNext);
			}
			
			if (tokens.size() > 2 && tokens[1] == "!LDFORGE") {
				// Handle LDForge-specific types, they're embedded into comments
				
				if (tokens[2] == "VERTEX") {
					// Vertex (0 !LDFORGE VERTEX)
					CHECK_TOKEN_COUNT (7)
					CHECK_TOKEN_NUMBERS (3, 6)
					
					LDVertex* obj = new LDVertex;
					obj->dColor = atol (tokens[3]);
					obj->vPosition.x = atof (tokens[4]);
					obj->vPosition.y = atof (tokens[5]);
					obj->vPosition.z = atof (tokens[6]);
					
					return obj;
				}
				
				if (tokens[2] == "RADIAL") {
					CHECK_TOKEN_COUNT (20)
					CHECK_TOKEN_NUMBERS (4, 19)
					
					LDRadial::Type eType = LDRadial::NumTypes;
					
					for (int i = 0; i < LDRadial::NumTypes; ++i) {
						if (str (LDRadial::radialTypeName ((LDRadial::Type) i)).toupper ().strip (' ') == tokens[3]) {
							eType = (LDRadial::Type) i;
							break;
						}
					}
					
					if (eType == LDRadial::NumTypes)
						return new LDGibberish (zLine, format ("Unknown radial type %s", tokens[3].chars ()));
					
					LDRadial* obj = new LDRadial;
					
					obj->eRadialType = eType;			// 3
					obj->dColor = atol (tokens[4]);		// 4
					obj->dSegments = atol (tokens[5]);	// 5
					obj->dDivisions = atol (tokens[6]);	// 6
					obj->dRingNum = atol (tokens[7]);	// 7
					
					obj->vPosition = parseVertex (tokens, 8); // 8 - 10
					
					for (short i = 0; i < 9; ++i)
						obj->mMatrix[i] = atof (tokens[i + 11]); // 11 - 19
					
					return obj;
				}
			}
			
			LDComment* obj = new LDComment;
			obj->zText = zComment;
			return obj;
		}
	
	case 1:
		{
			// Subfile
			CHECK_TOKEN_COUNT (15)
			CHECK_TOKEN_NUMBERS (1, 13)
			
			// Try open the file
			OpenFile* pFile = loadSubfile (tokens[14]);
			
			// If we cannot open the file, mark it an error
			if (!pFile)
				return new LDGibberish (zLine, "Could not open referred file");
			
			LDSubfile* obj = new LDSubfile;
			obj->dColor = atol (tokens[1]);
			obj->vPosition = parseVertex (tokens, 2); // 2 - 4
			
			for (short i = 0; i < 9; ++i)
				obj->mMatrix[i] = atof (tokens[i + 5]); // 5 - 13
			
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
			obj->dColor = atol (tokens[1]);
			for (short i = 0; i < 2; ++i)
				obj->vaCoords[i] = parseVertex (tokens, 2 + (i * 3)); // 2 - 7
			return obj;
		}
	
	case 3:
		{
			CHECK_TOKEN_COUNT (11)
			CHECK_TOKEN_NUMBERS (1, 10)
			
			// Triangle
			LDTriangle* obj = new LDTriangle;
			obj->dColor = atol (tokens[1]);
			
			for (short i = 0; i < 3; ++i)
				obj->vaCoords[i] = parseVertex (tokens, 2 + (i * 3)); // 2 - 10
			
			return obj;
		}
	
	case 4:
		{
			CHECK_TOKEN_COUNT (14)
			CHECK_TOKEN_NUMBERS (1, 13)
			
			// Quadrilateral
			LDQuad* obj = new LDQuad;
			obj->dColor = atol (tokens[1]);
			
			for (short i = 0; i < 4; ++i)
				obj->vaCoords[i] = parseVertex (tokens, 2 + (i * 3)); // 2 - 13
			
			return obj;
		}
	
	case 5:
		{
			CHECK_TOKEN_COUNT (14)
			CHECK_TOKEN_NUMBERS (1, 13)
			
			// Conditional line
			LDCondLine* obj = new LDCondLine;
			obj->dColor = atol (tokens[1]);
			
			for (short i = 0; i < 4; ++i)
				obj->vaCoords[i] = parseVertex (tokens, 2 + (i * 3)); // 2 - 13
			
			return obj;
		}
	
	default: // Strange line we couldn't parse
		return new LDGibberish (zLine, "Unknown line code number");
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
OpenFile* loadSubfile (str zFile) {
	// Try open the file
	OpenFile* pFile = findLoadedFile (zFile);
	if (!pFile)
		pFile = openDATFile (zFile, true);
	
	return pFile;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void reloadAllSubfiles () {
	if (!g_CurrentFile)
		return;
	
	// First, close all but the current open file.
	for (OpenFile* file : g_LoadedFiles)
		if (file != g_CurrentFile)
			delete file;
	
	g_LoadedFiles.clear ();
	g_LoadedFiles.push_back (g_CurrentFile);
	
	// Go through all objects in the current file and reload the subfiles
	for (LDObject* obj : g_CurrentFile->objects) {
		if (obj->getType() == OBJ_Subfile) {
			// Note: ref->pFile is invalid right now since all subfiles were closed.
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			OpenFile* pFile = loadSubfile (ref->zFileName);
			
			if (pFile)
				ref->pFile = pFile;
			else {
				// Couldn't load the file, mark it an error
				ref->replace (new LDGibberish (ref->getContents (), "Could not open referred file"));
			}
		}
		
		// Reparse gibberish files. It could be that they are invalid because
		// the file could not be opened. Circumstances may be different now.
		if (obj->getType() == OBJ_Gibberish)
			obj->replace (parseLine (static_cast<LDGibberish*> (obj)->zContents));
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ulong OpenFile::addObject (LDObject* obj) {
	if (this != g_CurrentFile) {
		objects.push_back (obj);
		return objects.size() - 1;
	}
	
	const ulong ulSpot = g_ForgeWindow->getInsertionPoint ();
	objects.insert (objects.begin() + ulSpot, obj);
	return ulSpot;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void OpenFile::forgetObject (LDObject* obj) {
	// Find the index for the given object
	ulong ulIndex;
	for (ulIndex = 0; ulIndex < (ulong)objects.size(); ++ulIndex)
		if (objects[ulIndex] == obj)
			break; // found it
	
	if (ulIndex >= objects.size ())
		return; // was not found
	
	// Erase it from memory
	objects.erase (objects.begin() + ulIndex);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
std::vector<partListEntry> g_PartList;

void initPartList () {
	logf ("%s: initializing parts.lst\n", __func__);
	
	FILE* fp = openLDrawFile ("parts.lst", false);
	
	if (!fp)
		return;
	
	char sLine[1024];
	while (fgets (sLine, sizeof sLine, fp)) {
		// Locate the first whitespace
		char* cpWhite = strstr (sLine, " ");
		
		char sName[65];
		size_t uLength = (cpWhite - sLine);
		
		if (uLength >= 64)
			continue; // too long
		
		strncpy (sName, sLine, uLength);
		sName[uLength] = '\0';
		
		// Surf through the whitespace sea!
		while (*cpWhite == ' ')
			cpWhite++;
		
		// Get the end point
		char* cpEnd = strstr (sLine, "\r");
		
		if (cpEnd == null) {
			// must not be DOS-formatted
			cpEnd = strstr (sLine, "\n");
		}
		
		assert (cpEnd != null);
		
		// Make the file title now
		char sTitle[81];
		uLength = (cpEnd - cpWhite);
		strncpy (sTitle, cpWhite, uLength);
		sTitle[uLength] = '\0';
		
		// Add it to the array.
		partListEntry entry;
		strcpy (entry.sName, sName);
		strcpy (entry.sTitle, sTitle);
		g_PartList.push_back (entry);
	}
}