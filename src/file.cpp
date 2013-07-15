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
#include <QApplication>

#include <stdlib.h>
#include "common.h"
#include "config.h"
#include "file.h"
#include "misc.h"
#include "gui.h"
#include "history.h"
#include "dialogs.h"
#include "gldraw.h"

cfg (str, io_ldpath, "");
cfg (str, io_recentfiles, "");

static bool g_loadingMainFile = false;
static const int g_MaxRecentFiles = 5;
static bool g_aborted = false;

LDOpenFile* LDOpenFile::m_curfile = null;

DEFINE_PROPERTY (QListWidgetItem*, LDOpenFile, listItem, setListItem)

// =============================================================================
namespace LDPaths {
	static str pathError;
	
	struct {
		str LDConfigPath;
		str partsPath, primsPath;
	} pathInfo;
	
	void initPaths() {
		if (!tryConfigure (io_ldpath)) {
			LDrawPathDialog dlg (false);
			
			if (!dlg.exec ())
				exit (0);
			
			io_ldpath = dlg.filename();
		}
	}
	
	bool tryConfigure (str path) {
		QDir dir;
		
		if (!dir.cd (path)) {
			pathError = "Directory does not exist.";
			return false;
		}
		
		QStringList mustHave = { "LDConfig.ldr", "parts", "p" };
		QStringList contents = dir.entryList (mustHave);
		
		if (contents.size() != mustHave.size()) {
			pathError = "Not an LDraw directory! Must<br />have LDConfig.ldr, parts/ and p/.";
			return false;
		}
		
		pathInfo.partsPath = fmt ("%1" DIRSLASH "parts", path);
		pathInfo.LDConfigPath = fmt ("%1" DIRSLASH "LDConfig.ldr", path);
		pathInfo.primsPath = fmt ("%1" DIRSLASH "p", path);
		
		return true;
	}
	
	// Accessors
	str getError() { return pathError; }
	str ldconfig() { return pathInfo.LDConfigPath; }
	str prims() { return pathInfo.primsPath; }
	str parts() { return pathInfo.partsPath; }
}

// =============================================================================
LDOpenFile::LDOpenFile() {
	setImplicit (true);
	setSavePos (-1);
	setListItem (null);
	m_history.setFile (this);
}

// =============================================================================
LDOpenFile::~LDOpenFile() {
	ulong i;
	
	// Clear everything from the model
	for (LDObject* obj : m_objs)
		delete obj;
	
	// Clear the cache as well
	for (LDObject* obj : m_cache)
		delete obj;
	
	// Remove this file from the list of files
	for (i = 0; i < g_loadedFiles.size(); ++i) {
		if (g_loadedFiles[i] == this) {
			g_loadedFiles.erase (i);
			break;
		}
	}
	
	// If we just closed the current file, we need to set the current
	// file as something else.
	if (this == LDOpenFile::current()) {
		if (i > 0)
			i--;
		
		// If we closed the last file, create a blank one.
		if (g_loadedFiles.size() < i + 1)
			newFile();
		else
			LDOpenFile::setCurrent (g_loadedFiles[i]);
	}
	
	g_win->updateFileList();
}

// =============================================================================
LDOpenFile* findLoadedFile (str name) {
	for (LDOpenFile* file : g_loadedFiles)
		if (file->name () == name)
			return file;
	
	return null;
}

// =============================================================================
str dirname (str path) {
	long lastpos = path.lastIndexOf (DIRSLASH);
	
	if (lastpos > 0)
		return path.left (lastpos);
	
#ifndef _WIN32
	if (path[0] == DIRSLASH_CHAR)
		return DIRSLASH;
#endif // _WIN32
	
	return "";
}

// =============================================================================
str basename (str path) {
	long lastpos = path.lastIndexOf (DIRSLASH);
	
	if (lastpos != -1)
		return path.mid (lastpos + 1);
	
	return path;
}

// =============================================================================
File* openLDrawFile (str relpath, bool subdirs) {
	print ("%1: Try to open %2\n", __func__, relpath);
	File* f = new File;
	str fullPath;
	
	// LDraw models use Windows-style path separators. If we're not on Windows,
	// replace the path separator now before opening any files.
#ifndef WIN32
	relpath.replace ("\\", "/");
#endif // WIN32
	
	if (LDOpenFile::current()) {
		// First, try find the file in the current model's file path. We want a file
		// in the immediate vicinity of the current model to override stock LDraw stuff.
		str partpath = fmt ("%1" DIRSLASH "%2", dirname (LDOpenFile::current()->name ()), relpath);
		
		if (f->open (partpath, File::Read)) {
			return f;
		}
	}
	
	if (f->open (relpath, File::Read))
		return f;
	
	// Try with just the LDraw path first
	fullPath = fmt ("%1" DIRSLASH "%2", io_ldpath, relpath);
	
	if (f->open (fullPath, File::Read))
		return f;
	
	if (subdirs) {
		// Look in sub-directories: parts and p
		for (auto subdir : initlist<const str> ({ "parts", "p" })) {
			fullPath = fmt ("%1" DIRSLASH "%2" DIRSLASH "%3", io_ldpath, subdir, relpath);
			
			if (f->open (fullPath, File::Read))
				return f;
		}
	}
	
	// Did not find the file.
	print ("could not find %1\n", relpath);
	delete f;
	return null;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void FileLoader::start() {
	setDone (false);
	setProgress (0);
	setAborted (false);
	
	if (concurrent()) {
		g_aborted = false;
		
		// Show a progress dialog if we're loading the main file here and move
		// the actual work to a separate thread as this can be a rather intensive
		// operation and if we don't respond quickly enough, the program can be
		// deemed inresponsive.. which is a bad thing.
		dlg = new OpenProgressDialog (g_win);
		dlg->setNumLines (lines().size());
		dlg->setModal (true);
		dlg->show();
		
		// Connect the loader in so we can show updates
		connect (this, SIGNAL (workDone()), dlg, SLOT (accept()));
		connect (dlg, SIGNAL (rejected()), this, SLOT (abort()));
	} else
		dlg = null;
	
	work (0);
}

void FileLoader::work (ulong i) {
	if (aborted()) {
		// We were flagged for abortion, so abort.
		for (LDObject* obj : m_objs)
			delete obj;
		
		m_objs.clear();
		setDone (true);
		return;
	}
	
	ulong max = i + 300;
	
	for (; i < max && i < lines().size(); ++i) {
		str line = lines() [i];
		
		// Trim the trailing newline
		qchar c;
		
		while ((c = line[line.length () - 1]) == '\n' || c == '\r')
			line.chop (1);
		
		LDObject* obj = parseLine (line);
		
		// Check for parse errors and warn about tthem
		if (obj->getType () == LDObject::Error) {
			log ("Couldn't parse line #%1: %2", m_progress + 1, static_cast<LDErrorObject*> (obj)->reason);
			
			if (m_warningsPointer) {
				(*m_warningsPointer) ++;
			}
		}
		
		m_objs << obj;
		setProgress (i);
		
		if (concurrent())
			dlg->updateProgress (i);
	}
	
	if (i >= lines().size() - 1) {
		emit workDone();
		setDone (true);
	}
	
	if (!done()) {
		if (concurrent())
			QMetaObject::invokeMethod (this, "work", Qt::QueuedConnection, Q_ARG (ulong, i + 1));
		else
			work (i + 1);
	}
}

void FileLoader::abort() {
	setAborted (true);
	
	if (concurrent())
		g_aborted = true;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
vector<LDObject*> loadFileContents (File* f, ulong* numWarnings, bool* ok) {
	vector<str> lines;
	vector<LDObject*> objs;
	
	if (numWarnings)
		*numWarnings = 0;
	
	// Calculate the amount of lines
	for (str line : *f)
		lines << line;
	
	f->rewind();
	
	FileLoader* loader = new FileLoader;
	loader->setWarningsPointer (numWarnings);
	loader->setLines (lines);
	loader->setConcurrent (g_loadingMainFile);
	loader->start();
	
	while (loader->done() == false)
		qApp->processEvents();
	
	// If we wanted the success value, supply that now
	if (ok)
		*ok = !loader->aborted();
	
	objs = loader->objs();
	return objs;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
LDOpenFile* openDATFile (str path, bool search) {
	// Convert the file name to lowercase since some parts contain uppercase
	// file names. I'll assume here that the library will always use lowercase
	// file names for the actual parts..
	File* f;
	
	if (search)
		f = openLDrawFile (path.toLower (), true);
	else {
		f = new File (path, File::Read);
		
		if (!*f) {
			delete f;
			return null;
		}
	}
	
	if (!f)
		return null;
	
	LDOpenFile* load = new LDOpenFile;
	load->setName (path);
	
	ulong numWarnings;
	bool ok;
	vector<LDObject*> objs = loadFileContents (f, &numWarnings, &ok);
	
	if (!ok)
		return null;
	
	for (LDObject* obj : objs)
		load->addObject (obj);
	
	delete f;
	g_loadedFiles << load;
	
	if (g_loadingMainFile) {
		LDOpenFile::setCurrent (load);
		g_win->R()->setFile (load);
		log (QObject::tr ("File %1 parsed successfully (%2 errors)."), path, numWarnings);
	}
	
	return load;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool LDOpenFile::safeToClose() {
	typedef QMessageBox msgbox;
	setlocale (LC_ALL, "C");
	
	// If we have unsaved changes, warn and give the option of saving.
	if (hasUnsavedChanges()) {
		str message = fmt ("There are unsaved changes to %1. Should it be saved?",
						   (name().length() > 0) ? name() : "<anonymous>");
		
		int button = msgbox::question (g_win, "Unsaved Changes", message,
			(msgbox::Yes | msgbox::No | msgbox::Cancel), msgbox::Cancel);
		
		switch (button) {
		case msgbox::Yes:
			// If we don't have a file path yet, we have to ask the user for one.
			if (name().length() == 0) {
				str newpath = QFileDialog::getSaveFileName (g_win, "Save As",
					LDOpenFile::current()->name(), "LDraw files (*.dat *.ldr)");
				
				if (newpath.length() == 0)
					return false;
				
				setName (newpath);
			}
			
			if (!save ()) {
				message = fmt (QObject::tr ("Failed to save %1: %2\nDo you still want to close?"),
					name(), strerror (errno));
				
				if (msgbox::critical (g_win, "Save Failure", message,
					(msgbox::Yes | msgbox::No), msgbox::No) == msgbox::No)
				{
					return false;
				}
			}
			break;
		
		case msgbox::Cancel:
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
void closeAll() {
	if (!g_loadedFiles.size())
		return;
	
	// Remove all loaded files and the objects they contain
	for (LDOpenFile* file : g_loadedFiles)
		delete file;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void newFile () {
	// Create a new anonymous file and set it to our current
	LDOpenFile* f = new LDOpenFile;
	f->setName ("");
	f->setImplicit (false);
	g_loadedFiles << f;
	LDOpenFile::setCurrent (f);
	
	if (g_loadedFiles.size() == 2)
		LDOpenFile::closeInitialFile (0);
	
	g_win->R()->setFile (f);
	g_win->fullRefresh();
	g_win->updateTitle();
	f->history().updateActions();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void addRecentFile (str path) {
	QStringList rfiles = io_recentfiles.value.split ('@');
	int idx = rfiles.indexOf (path);
	
	// If this file already is in the list, pop it out.
	if (idx != -1) {
		if (rfiles.size () == 1)
			return; // only recent file - abort and do nothing
		
		// Pop it out.
		rfiles.removeAt (idx);
	}
	
	// If there's too many recent files, drop one out.
	while (rfiles.size() > (g_MaxRecentFiles - 1))
		rfiles.removeAt (0);
	
	// Add the file
	rfiles << path;
	
	// Rebuild the config string
	io_recentfiles = rfiles.join ("@");
	
	config::save();
	g_win->updateRecentFilesMenu();
}

// =============================================================================
// -----------------------------------------------------------------------------
// Open an LDraw file and set it as the main model
// =============================================================================
void openMainFile (str path) {
	g_loadingMainFile = true;
	LDOpenFile* file = openDATFile (path, false);
	
	if (!file) {
		// Loading failed, thus drop down to a new file since we
		// closed everything prior.
		newFile ();
		
		if (!g_aborted) {
			// Tell the user loading failed.
			setlocale (LC_ALL, "C");
			critical (fmt (QObject::tr ("Failed to open %1: %2"), path, strerror (errno)));
		}
		
		g_loadingMainFile = false;
		return;
	}
	
	file->setImplicit (false);
	
	// If we have an anonymous, unchanged file open as the only open file
	// (aside of the one we just opened), close it now.
	if (g_loadedFiles.size() == 2)
		LDOpenFile::closeInitialFile (0);
	
	// Rebuild the object tree view now.
	g_win->fullRefresh();
	g_win->updateTitle();
	g_win->R()->setFile (file);
	g_win->R()->resetAngles();
	
	// Add it to the recent files list.
	addRecentFile (path);
	g_loadingMainFile = false;
	
	LDOpenFile::setCurrent (file);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool LDOpenFile::save (str savepath) {
	if (!savepath.length())
		savepath = name();
	
	File f (savepath, File::Write);
	
	if (!f)
		return false;
	
	// If the second object in the list holds the file name, update that now.
	// Only do this if the file is explicitly open. If it's saved into a directory
	// called "s" or "48", prepend that into the name.
	LDCommentObject* fpathComment = null;
	LDObject* first = object (1);
	
	if (!implicit() && first != null && first->getType() == LDObject::Comment) {
		fpathComment = static_cast<LDCommentObject*> (first);
		
		if (fpathComment->text.left (6) == "Name: ") {
			str newname;
			str dir = basename (dirname (savepath));
			
			if (dir == "s" || dir == "48")
				newname = dir + "\\";
			
			newname += basename (savepath);
			fpathComment->text = fmt ("Name: %1", newname);
			g_win->buildObjList();
		}
	}
	
	// File is open, now save the model to it. Note that LDraw requires files to
	// have DOS line endings, so we terminate the lines with \r\n.
	for (LDObject* obj : objs())
		f.write (obj->raw () + "\r\n");
	
	// File is saved, now clean up.
	f.close();
	
	// We have successfully saved, update the save position now.
	setSavePos (history().pos());
	setName (savepath);
	
	g_win->updateFileListItem (this);
	g_win->updateTitle();
	return true;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
#define CHECK_TOKEN_COUNT(N) \
	if (tokens.size() != N) \
		return new LDErrorObject (line, "Bad amount of tokens");

#define CHECK_TOKEN_NUMBERS( MIN, MAX ) \
	for (ushort i = MIN; i <= MAX; ++i) \
		if (!isNumber (tokens[i])) \
			return new LDErrorObject (line, fmt ("Token #%1 was `%2`, expected a number", (i + 1), tokens[i]));

static vertex parseVertex (QStringList& s, const ushort n) {
	vertex v;
	
	for (const Axis ax : g_Axes)
		v[ax] = atof (s[n + ax]);
	
	return v;
}

// =============================================================================
// -----------------------------------------------------------------------------
// This is the LDraw code parser function. It takes in a string containing LDraw
// code and returns the object parsed from it. parseLine never returns null,
// the object will be LDError if it could not be parsed properly.
// =============================================================================
LDObject* parseLine (str line) {
	QStringList tokens = line.split (" ", str::SkipEmptyParts);
	
	if (tokens.size() <= 0) {
		// Line was empty, or only consisted of whitespace
		return new LDEmptyObject;
	}
	
	if (tokens[0].length() != 1 || tokens[0][0].isDigit() == false)
		return new LDErrorObject (line, "Illogical line code");
	
	int num = tokens[0][0].digitValue();
	
	switch (num) {
	case 0: {
		// Comment
		str comm = line.mid (line.indexOf ("0") + 1);
		
		// Remove any leading whitespace
		while (comm[0] == ' ')
			comm.remove (0, 1);
		
		// Handle BFC statements
		if (tokens.size() > 2 && tokens[1] == "BFC") {
			for (short i = 0; i < LDBFCObject::NumStatements; ++i)
				if (comm == fmt ("BFC %1", LDBFCObject::statements [i]))
					return new LDBFCObject ((LDBFCObject::Type) i);
			
			// MLCAD is notorious for stuffing these statements in parts it
			// creates. The above block only handles valid statements, so we
			// need to handle MLCAD-style invertnext separately.
			if (comm == "BFC CERTIFY INVERTNEXT")
				return new LDBFCObject (LDBFCObject::InvertNext);
		}
		
		if (tokens.size() > 2 && tokens[1] == "!LDFORGE") {
			// Handle LDForge-specific types, they're embedded into comments too
			if (tokens[2] == "VERTEX") {
				// Vertex (0 !LDFORGE VERTEX)
				CHECK_TOKEN_COUNT (7)
				CHECK_TOKEN_NUMBERS (3, 6)
				
				LDVertexObject* obj = new LDVertexObject;
				obj->setColor (tokens[3].toLong ());
				
				for (const Axis ax : g_Axes)
					obj->pos[ax] = tokens[4 + ax].toDouble (); // 4 - 6
				
				return obj;
			} elif (tokens[2] == "OVERLAY") {
				CHECK_TOKEN_COUNT (9);
				CHECK_TOKEN_NUMBERS (5, 8)
				
				LDOverlayObject* obj = new LDOverlayObject;
				obj->setFilename (tokens[3]);
				obj->setCamera (tokens[4].toLong());
				obj->setX (tokens[5].toLong());
				obj->setY (tokens[6].toLong());
				obj->setWidth (tokens[7].toLong());
				obj->setHeight (tokens[8].toLong());
				return obj;
			}
		}
		
		// Just a regular comment:
		LDCommentObject* obj = new LDCommentObject;
		obj->text = comm;
		return obj;
	}
	
	case 1: {
		// Subfile
		CHECK_TOKEN_COUNT (15)
		CHECK_TOKEN_NUMBERS (1, 13)
		
		// Try open the file. Disable g_loadingMainFile temporarily since we're
		// not loading the main file now, but the subfile in question.
		bool tmp = g_loadingMainFile;
		g_loadingMainFile = false;
		LDOpenFile* load = getFile (tokens[14]);
		g_loadingMainFile = tmp;
		
		// If we cannot open the file, mark it an error
		if (!load)
			return new LDErrorObject (line, "Could not open referred file");
		
		LDSubfileObject* obj = new LDSubfileObject;
		obj->setColor (tokens[1].toLong());
		obj->setPosition (parseVertex (tokens, 2));  // 2 - 4
		
		matrix transform;
		
		for (short i = 0; i < 9; ++i)
			transform[i] = tokens[i + 5].toDouble(); // 5 - 13
		
		obj->setTransform (transform);
		obj->setFileInfo (load);
		return obj;
	}
	
	case 2: {
		CHECK_TOKEN_COUNT (8)
		CHECK_TOKEN_NUMBERS (1, 7)
		
		// Line
		LDLineObject* obj = new LDLineObject;
		obj->setColor (tokens[1].toLong());
		
		for (short i = 0; i < 2; ++i)
			obj->setVertex (i, parseVertex (tokens, 2 + (i * 3)));   // 2 - 7
		
		return obj;
	}
	
	case 3: {
		CHECK_TOKEN_COUNT (11)
		CHECK_TOKEN_NUMBERS (1, 10)
		
		// Triangle
		LDTriangleObject* obj = new LDTriangleObject;
		obj->setColor (tokens[1].toLong());
		
		for (short i = 0; i < 3; ++i)
			obj->setVertex (i, parseVertex (tokens, 2 + (i * 3)));   // 2 - 10
		
		return obj;
	}
	
	case 4:
	case 5: {
		CHECK_TOKEN_COUNT (14)
		CHECK_TOKEN_NUMBERS (1, 13)
		
		// Quadrilateral / Conditional line
		LDObject* obj;
		
		if (num == 4)
			obj = new LDQuadObject;
		else
			obj = new LDCondLineObject;
		
		obj->setColor (tokens[1].toLong());
		
		for (short i = 0; i < 4; ++i)
			obj->setVertex (i, parseVertex (tokens, 2 + (i * 3)));   // 2 - 13
		
		return obj;
	}
	
	default: // Strange line we couldn't parse
		return new LDErrorObject (line, "Unknown line code number");
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
LDOpenFile* getFile (str filename) {
	// Try find the file in the list of loaded files
	LDOpenFile* load = findLoadedFile (filename);
	
	// If it's not loaded, try open it
	if (!load)
		load = openDATFile (filename, true);
	
	return load;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void reloadAllSubfiles () {
	if (!LDOpenFile::current())
		return;
	
	g_loadedFiles.clear();
	g_loadedFiles << LDOpenFile::current();
	
	// Go through all objects in the current file and reload the subfiles
	for (LDObject* obj : LDOpenFile::current()->objs()) {
		if (obj->getType() == LDObject::Subfile) {
			LDSubfileObject* ref = static_cast<LDSubfileObject*> (obj);
			LDOpenFile* fileInfo = getFile (ref->fileInfo()->name());
			
			if (fileInfo)
				ref->setFileInfo (fileInfo);
			else
				ref->replace (new LDErrorObject (ref->raw(), "Could not open referred file"));
		}
		
		// Reparse gibberish files. It could be that they are invalid because
		// of loading errors. Circumstances may be different now.
		if (obj->getType() == LDObject::Error)
			obj->replace (parseLine (static_cast<LDErrorObject*> (obj)->contents));
	}
	
	// Close all files left unused
	LDOpenFile::closeUnused();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ulong LDOpenFile::addObject (LDObject* obj) {
	m_history.add (new AddHistory (m_objs.size(), obj));
	m_objs << obj;
	
	if (obj->getType() == LDObject::Vertex)
		m_vertices << obj;
	
	obj->setFile (this);
	return numObjs() - 1;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDOpenFile::insertObj (const ulong pos, LDObject* obj) {
	m_history.add (new AddHistory (pos, obj));
	m_objs.insert (pos, obj);
	obj->setFile (this);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDOpenFile::forgetObject (LDObject* obj) {
	ulong idx = obj->getIndex();
	m_history.add (new DelHistory (idx, obj));
	m_objs.erase (idx);
	obj->setFile (null);
}

// =============================================================================
bool safeToCloseAll () {
	for (LDOpenFile* f : g_loadedFiles)
		if (!f->safeToClose())
			return false;
	
	return true;
}

// =============================================================================
void LDOpenFile::setObject (ulong idx, LDObject* obj) {
	assert (idx < numObjs());
	
	// Mark this change to history
	str oldcode = object (idx)->raw ();
	str newcode = obj->raw();
	m_history << new EditHistory (idx, oldcode, newcode);
	
	obj->setFile (this);
	m_objs[idx] = obj;
}

static vector<LDOpenFile*> getFilesUsed (LDOpenFile* node) {
	vector<LDOpenFile*> filesUsed;
	
	for (LDObject* obj : *node) {
		if (obj->getType() != LDObject::Subfile)
			continue;
		
		LDSubfileObject* ref = static_cast<LDSubfileObject*> (obj);
		filesUsed << ref->fileInfo();
		filesUsed << getFilesUsed (ref->fileInfo());
	}
	
	return filesUsed;
}

// =============================================================================
// Find out which files are unused and close them.
void LDOpenFile::closeUnused () {
	vector<LDOpenFile*> filesUsed = getFilesUsed (LDOpenFile::current());
	
	// Anything that's explicitly opened must not be closed
	for (LDOpenFile* file : g_loadedFiles)
		if (!file->implicit())
			filesUsed << file;
	
	// Remove duplicated entries
	filesUsed.makeUnique();
	
	// Close all open files that aren't in filesUsed
	for (LDOpenFile* file : g_loadedFiles) {
		bool isused = false;
	
	for (LDOpenFile* usedFile : filesUsed) {
			if (file == usedFile) {
				isused = true;
				break;
			}
		}
		
		if (!isused)
			delete file;
	}
	
	g_loadedFiles.clear();
	g_loadedFiles << filesUsed;
}

LDObject* LDOpenFile::object (ulong pos) const {
	if (m_objs.size() <= pos)
		return null;
	
	return m_objs[pos];
}

LDObject* LDOpenFile::obj (ulong pos) const {
	return object (pos);
}

ulong LDOpenFile::numObjs() const {
	return m_objs.size();
}

LDOpenFile& LDOpenFile::operator<< (vector<LDObject*> objs) {
	for (LDObject* obj : objs)
		addObject (obj);
	
	return *this;
}

bool LDOpenFile::hasUnsavedChanges() const {
	return !implicit() && history().pos() != savePos();
}

// =============================================================================
LDOpenFile* LDOpenFile::current() {
	return m_curfile;
}

void LDOpenFile::setCurrent (LDOpenFile* f) {
	m_curfile = f;
	
	if (g_win && f)
		g_win->updateFileListItem (f);
}

void LDOpenFile::closeInitialFile (ulong idx) {
	if (g_loadedFiles.size() > idx && g_loadedFiles[idx]->name() == "" && !g_loadedFiles[idx]->hasUnsavedChanges())
		delete g_loadedFiles[idx];
}