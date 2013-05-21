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

#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

#include <stdlib.h>
#include "common.h"
#include "config.h"
#include "file.h"
#include "misc.h"
#include "bbox.h"
#include "gui.h"
#include "history.h"
#include "dialogs.h"
#include "gldraw.h"

cfg (str, io_ldpath, "");
cfg (str, io_recentfiles, "");

// =============================================================================
namespace LDPaths {
	static str pathError;
	
	struct {
		str LDConfigPath;
		str partsPath, primsPath;
	} pathInfo;
	
	void initPaths () {
		if (!tryConfigure (io_ldpath)) {
			LDrawPathDialog dlg (false);
			
			if (!dlg.exec ())
				exit (0);
			
			io_ldpath = dlg.path ();
		}
	}
	
	bool tryConfigure (str path) {
		QDir dir;
		
		if (!dir.cd (path)) {
			pathError = "Directory does not exist.";
			return false;
		}
		
		QStringList mustHave = {"LDConfig.ldr", "parts", "p"};
		QStringList contents = dir.entryList (mustHave);
		
		if (contents.size () != mustHave.size ()) {
			pathError = "Not an LDraw directory! Must<br />have LDConfig.ldr, parts/ and p/.";
			return false;
		}
		
		pathInfo.partsPath = fmt ("%s" DIRSLASH "parts", path.chars ());
		pathInfo.LDConfigPath = fmt ("%s" DIRSLASH "LDConfig.ldr", path.chars ());
		pathInfo.primsPath = fmt ("%s" DIRSLASH "p", path.chars ());
		
		return true;
	}
	
	// Accessors
	str getError () { return pathError; }
	str ldconfig () { return pathInfo.LDConfigPath; }
	str prims () { return pathInfo.primsPath; }
	str parts () { return pathInfo.partsPath; }
}

// =============================================================================
LDOpenFile::LDOpenFile () {
	m_implicit = true;
	savePos = -1;
}

// =============================================================================
LDOpenFile::~LDOpenFile () {
	// Clear everything from the model
	for (LDObject* obj : m_objs)
		delete obj;
	
	// Clear the cache as well
	for (LDObject* obj : m_objCache)
		delete obj;
}

// =============================================================================
LDOpenFile* findLoadedFile (str zName) {
	for (LDOpenFile* file : g_loadedFiles)
		if (file->m_filename == zName)
			return file;
	
	return null;
}

// =============================================================================
str dirname (str path) {
	long lastpos; // FIXME: why doesn't str::last () work here?
	for (lastpos = path.len () - 1; lastpos >= 0; --lastpos)
		if (path[(size_t) lastpos] == DIRSLASH_CHAR)
			break;
	
	if (lastpos > 0)
		return path.substr (0, lastpos);
	
	return "";
}

// =============================================================================
FILE* openLDrawFile (str relpath, bool subdirs) {
	printf ("%s: Try to open %s\n", __func__, relpath.c ());
#ifndef WIN32
	relpath.replace ("\\", "/");
#endif // WIN32
	
	if (g_curfile != null) {
		str partpath = fmt ("%s" DIRSLASH "%s", dirname (g_curfile->m_filename).c (), relpath.c ());
		printf ("try %s\n", partpath.c ());
		FILE* fp = fopen (partpath, "r");
		
		if (fp != null)
			return fp;
	}
	
	printf ("try %s\n", relpath.chars ());
	FILE* fp = fopen (relpath, "r");
	str fullPath;
	
	if (fp != null)
		return fp;
	
	if (~io_ldpath.value) {
		// Try with just the LDraw path first
		fullPath = fmt ("%s" DIRSLASH "%s", io_ldpath.value.chars(), relpath.chars());
		printf ("try %s\n", fullPath.chars ());
		
		fp = fopen (fullPath, "r");
		if (fp != null)
			return fp;
		
		if (subdirs) {
			for (auto subdir : initlist<const char*> ({"parts", "p"})) {
				fullPath = fmt ("%s" DIRSLASH "%s" DIRSLASH "%s",
					io_ldpath.value.chars(), subdir, relpath.chars());
				
				printf ("try %s\n", fullPath.chars ());
				fp = fopen (fullPath.chars (), "r");
				
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
		if (obj->getType() == LDObject::Gibberish) {
			logf (LOG_Warning, "Couldn't parse line #%lu: %s\n",
				lnum, static_cast<LDGibberish*> (obj)->reason.chars());
			
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
LDOpenFile* openDATFile (str path, bool search, bool mainfile) {
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
	
	LDOpenFile* load = new LDOpenFile;
	load->m_filename = path;
	
	if (mainfile)
		g_curfile = load;
	
	ulong numWarnings;
	std::vector<LDObject*> objs = loadFileContents (fp, &numWarnings);
	
	for (LDObject* obj : objs)
		load->m_objs.push_back (obj);
	
	fclose (fp);
	g_loadedFiles.push_back (load);
	
	logf ("File %s parsed successfully (%lu warning%s).\n",
		path.chars(), numWarnings, plural (numWarnings));
	
	return load;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool LDOpenFile::safeToClose () {
	setlocale (LC_ALL, "C");
	
	// If we have unsaved changes, warn and give the option of saving.
	if (!m_implicit && History::pos () != savePos) {
		switch (QMessageBox::question (g_win, "Unsaved Changes",
			fmt ("There are unsaved changes to %s. Should it be saved?",
			(m_filename.len () > 0) ? m_filename.chars () : "<anonymous>"),
			(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel), QMessageBox::Cancel))
		{
		case QMessageBox::Yes:
			// If we don't have a file path yet, we have to ask the user for one.
			if (m_filename.len () == 0) {
				str path = QFileDialog::getSaveFileName (g_win, "Save As",
					g_curfile->m_filename, "LDraw files (*.dat *.ldr)");
				
				if (path.len () == 0)
					return false;
				
				m_filename = path;
			}
			
			if (!save ()) {
				str errormsg = fmt ("Failed to save %s: %s\nDo you still want to close?",
					m_filename.chars (), strerror (lastError));
				
				if (QMessageBox::critical (g_win, "Save Failure", errormsg,
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
	if (!g_loadedFiles.size())
		return;
	
	// Remove all loaded files and the objects they contain
	for (LDOpenFile* file : g_loadedFiles)
		delete file;
	
	// Clear the array
	g_loadedFiles.clear();
	g_curfile = NULL;
	
	g_win->fullRefresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void newFile () {
	// Create a new anonymous file and set it to our current
	closeAll ();
	
	LDOpenFile* f = new LDOpenFile;
	f->m_filename = "";
	f->m_implicit = false;
	g_loadedFiles.push_back (f);
	g_curfile = f;
	
	History::clear ();
	
	g_BBox.reset ();
	g_win->fullRefresh ();
	g_win->updateTitle ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void addRecentFile (str path) {
	size_t pos = io_recentfiles.value.first (path);
	
	// If this file already is in the list, pop it out.
	if (pos != npos) {
		if (~io_recentfiles.value == ~path)
			return; // only recent file - do nothing
		
		// Pop it out.
		str front = io_recentfiles.value.substr (0, pos);
		str back;
		
		if (pos + ~path + 1 < io_recentfiles.value.len ())
			back = io_recentfiles.value.substr (pos + ~path + 1, -1);
		else
			back = "";
		
		io_recentfiles.value = front + back;
	}
	
	// If there's too many recent files, drop one out.
	while (io_recentfiles.value.count ('@') > 3)
		io_recentfiles.value = io_recentfiles.value.substr (io_recentfiles.value.first ("@") + 1, -1);
	
	// Add the file
	if (~io_recentfiles.value > 0)
		io_recentfiles.value += "@";
	
	io_recentfiles += path;
	
	config::save ();
	g_win->updateRecentFilesMenu ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void openMainFile (str path) {
	closeAll ();
	
	LDOpenFile* file = openDATFile (path, false, true);
	
	if (!file) {
		// Tell the user loading failed.
		setlocale (LC_ALL, "C");
		critical (fmt ("Failed to open %s: %s", path.chars(), strerror (errno)));
		return;
	}
	
	file->m_implicit = false;
	g_curfile = file;
	
	// Recalculate the bounding box
	g_BBox.calculate();
	
	// Rebuild the object tree view now.
	g_win->fullRefresh ();
	g_win->updateTitle ();
	g_win->R ()->resetAngles ();
	
	History::clear ();
	
	// Add it to the recent files list.
	addRecentFile (path);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool LDOpenFile::save (str path) {
	if (!~path)
		path = m_filename;
	
	FILE* fp = fopen (path, "w");
	
	if (!fp) {
		lastError = errno;
		return false;
	}
	
	// If the second object in the list holds the file name, update that now.
	// Only do this if the file is explicitly open. If it's saved into a directory
	// called "s" or "48", prepend that into the name.
	LDComment* fpathComment = null;
	if (m_implicit == false && m_objs.size () >= 2 && object (1)->getType () == LDObject::Comment) {
		fpathComment = static_cast<LDComment*> (object (1));
		
		if (fpathComment->text.substr (0, 6) == "Name: ") {
			str newfname;
			str dir = basename (dirname (path));
			
			if (dir == "s" || dir == "48")
				newfname = fmt ("%s\\", dir.c ());
			
			newfname += basename (path);
			fpathComment->text = fmt ("Name: %s", newfname.c ());
			g_win->buildObjList ();
		}
	}
	
	// Write all entries now
	for (LDObject* obj : m_objs) {
		// LDraw requires files to have DOS line endings
		str line = fmt ("%s\r\n", obj->getContents ().chars ());
		fwrite (line.chars(), 1, line.len (), fp);
	}
	
	fclose (fp);
	
	// We have successfully saved, update the save position now.
	savePos = History::pos ();
	m_filename = path;
	
	g_win->updateTitle ();
	return true;
}

#define CHECK_TOKEN_COUNT(N) \
	if (tokens.size() != N) \
		return new LDGibberish (line, "Bad amount of tokens");

#define CHECK_TOKEN_NUMBERS(MIN,MAX) \
	for (ushort i = MIN; i <= MAX; ++i) \
		if (!isNumber (tokens[i])) \
			return new LDGibberish (line, fmt ("Token #%u was `%s`, expected a number", \
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
	for (const Axis ax : g_Axes)
		v[ax] = atof (s[n + ax]);
	
	return v;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
LDObject* parseLine (str line) {
	vector<str> tokens = line.split (" ");
	
	if (!tokens.size ()) {
		// Line was empty, or only consisted of whitespace
		return new LDEmpty;
	}
	
	if (~tokens[0] != 1)
		return new LDGibberish (line, "Illogical line code");
	
	const char c = tokens[0][0];
	switch (c - '0') {
	case 0:
		{
			// Comment
			str comm;
			for (uint i = 1; i < tokens.size(); ++i) {
				comm += tokens[i];
				
				if (i != tokens.size() - 1)
					comm += ' ';
			}
			
			// Handle BFC statements
			if (tokens.size() > 2 && tokens[1] == "BFC") {
				for (short i = 0; i < LDBFC::NumStatements; ++i)
					if (comm == fmt ("BFC %s", LDBFC::statements [i]))
						return new LDBFC ((LDBFC::Type) i);
				
				// MLCAD is notorious for stuffing these statements in parts it
				// creates. The above block only handles valid statements, so we
				// need to handle MLCAD-style invertnext separately.
				if (comm == "BFC CERTIFY INVERTNEXT")
					return new LDBFC (LDBFC::InvertNext);
			}
			
			if (tokens.size() > 2 && tokens[1] == "!LDFORGE") {
				// Handle LDForge-specific types, they're embedded into comments
				
				if (tokens[2] == "VERTEX") {
					// Vertex (0 !LDFORGE VERTEX)
					CHECK_TOKEN_COUNT (7)
					CHECK_TOKEN_NUMBERS (3, 6)
					
					LDVertex* obj = new LDVertex;
					obj->color = atol (tokens[3]);
					
					for (const Axis ax : g_Axes)
						obj->pos[ax] = atof (tokens[4 + ax]); // 4 - 6
					
					return obj;
				}
				
				if (tokens[2] == "RADIAL") {
					CHECK_TOKEN_COUNT (20)
					CHECK_TOKEN_NUMBERS (4, 19)
					
					LDRadial::Type eType = LDRadial::NumTypes;
					
					for (int i = 0; i < LDRadial::NumTypes; ++i) {
						if (str (LDRadial::radialTypeName ((LDRadial::Type) i)).upper ().strip (' ') == tokens[3]) {
							eType = (LDRadial::Type) i;
							break;
						}
					}
					
					if (eType == LDRadial::NumTypes)
						return new LDGibberish (line, fmt ("Unknown radial type %s", tokens[3].chars ()));
					
					LDRadial* obj = new LDRadial;
					
					obj->radType = eType;			// 3
					obj->color = atol (tokens[4]);		// 4
					obj->segs = atol (tokens[5]);	// 5
					obj->divs = atol (tokens[6]);	// 6
					obj->ringNum = atol (tokens[7]);	// 7
					
					obj->pos = parseVertex (tokens, 8); // 8 - 10
					
					for (short i = 0; i < 9; ++i)
						obj->transform[i] = atof (tokens[i + 11]); // 11 - 19
					
					return obj;
				}
			}
			
			LDComment* obj = new LDComment;
			obj->text = comm;
			return obj;
		}
	
	case 1:
		{
			// Subfile
			CHECK_TOKEN_COUNT (15)
			CHECK_TOKEN_NUMBERS (1, 13)
			
			// Try open the file
			LDOpenFile* pFile = loadSubfile (tokens[14]);
			
			// If we cannot open the file, mark it an error
			if (!pFile)
				return new LDGibberish (line, "Could not open referred file");
			
			LDSubfile* obj = new LDSubfile;
			obj->color = atol (tokens[1]);
			obj->pos = parseVertex (tokens, 2); // 2 - 4
			
			for (short i = 0; i < 9; ++i)
				obj->transform[i] = atof (tokens[i + 5]); // 5 - 13
			
			obj->fileName = tokens[14];
			obj->fileInfo = pFile;
			return obj;
		}
	
	case 2:
		{
			CHECK_TOKEN_COUNT (8)
			CHECK_TOKEN_NUMBERS (1, 7)
			
			// Line
			LDLine* obj = new LDLine;
			obj->color = atol (tokens[1]);
			for (short i = 0; i < 2; ++i)
				obj->coords[i] = parseVertex (tokens, 2 + (i * 3)); // 2 - 7
			return obj;
		}
	
	case 3:
		{
			CHECK_TOKEN_COUNT (11)
			CHECK_TOKEN_NUMBERS (1, 10)
			
			// Triangle
			LDTriangle* obj = new LDTriangle;
			obj->color = atol (tokens[1]);
			
			for (short i = 0; i < 3; ++i)
				obj->coords[i] = parseVertex (tokens, 2 + (i * 3)); // 2 - 10
			
			return obj;
		}
	
	case 4:
		{
			CHECK_TOKEN_COUNT (14)
			CHECK_TOKEN_NUMBERS (1, 13)
			
			// Quadrilateral
			LDQuad* obj = new LDQuad;
			obj->color = atol (tokens[1]);
			
			for (short i = 0; i < 4; ++i)
				obj->coords[i] = parseVertex (tokens, 2 + (i * 3)); // 2 - 13
			
			return obj;
		}
	
	case 5:
		{
			CHECK_TOKEN_COUNT (14)
			CHECK_TOKEN_NUMBERS (1, 13)
			
			// Conditional line
			LDCondLine* obj = new LDCondLine;
			obj->color = atol (tokens[1]);
			
			for (short i = 0; i < 4; ++i)
				obj->coords[i] = parseVertex (tokens, 2 + (i * 3)); // 2 - 13
			
			return obj;
		}
	
	default: // Strange line we couldn't parse
		return new LDGibberish (line, "Unknown line code number");
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
LDOpenFile* loadSubfile (str zFile) {
	// Try open the file
	LDOpenFile* pFile = findLoadedFile (zFile);
	if (!pFile)
		pFile = openDATFile (zFile, true, false);
	
	return pFile;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void reloadAllSubfiles () {
	if (!g_curfile)
		return;
	
	// First, close all but the current open file.
	for (LDOpenFile* file : g_loadedFiles)
		if (file != g_curfile)
			delete file;
	
	g_loadedFiles.clear ();
	g_loadedFiles.push_back (g_curfile);
	
	// Go through all objects in the current file and reload the subfiles
	for (LDObject* obj : g_curfile->m_objs) {
		if (obj->getType() == LDObject::Subfile) {
			// Note: ref->pFile is invalid right now since all subfiles were closed.
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			LDOpenFile* pFile = loadSubfile (ref->fileName);
			
			if (pFile)
				ref->fileInfo = pFile;
			else {
				// Couldn't load the file, mark it an error
				ref->replace (new LDGibberish (ref->getContents (), "Could not open referred file"));
			}
		}
		
		// Reparse gibberish files. It could be that they are invalid because
		// the file could not be opened. Circumstances may be different now.
		if (obj->getType() == LDObject::Gibberish)
			obj->replace (parseLine (static_cast<LDGibberish*> (obj)->contents));
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ulong LDOpenFile::addObject (LDObject* obj) {
	m_objs.push_back (obj);
	
	if (this == g_curfile)
		g_BBox.calcObject (obj);
	
	return m_objs.size() - 1;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDOpenFile::insertObj (const ulong pos, LDObject* obj) {
	m_objs.insert (m_objs.begin () + pos, obj);
	
	if (this == g_curfile)
		g_BBox.calcObject (obj);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDOpenFile::forgetObject (LDObject* obj) {
	// Find the index for the given object
	ulong ulIndex;
	for (ulIndex = 0; ulIndex < (ulong)m_objs.size(); ++ulIndex)
		if (m_objs[ulIndex] == obj)
			break; // found it
	
	if (ulIndex >= m_objs.size ())
		return; // was not found
	
	// Erase it from memory
	m_objs.erase (m_objs.begin() + ulIndex);
	
	// Update the bounding box
	if (this == g_curfile)
		g_BBox.calculate ();
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

// =============================================================================
bool safeToCloseAll () {
	for (LDOpenFile* f : g_loadedFiles)
		if (!f->safeToClose ())
			return false;
	
	return true;
}