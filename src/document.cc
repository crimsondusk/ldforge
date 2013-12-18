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
 *  =====================================================================
 *
 *  file.cpp: File I/O and management.
 *  - File loading, parsing, manipulation, saving, closing.
 *  - LDraw path verification.
 */

#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QApplication>
#include "main.h"
#include "config.h"
#include "document.h"
#include "misc.h"
#include "gui.h"
#include "history.h"
#include "dialogs.h"
#include "gldraw.h"
#include "moc_document.cpp"

cfg (String, io_ldpath, "");
cfg (List, io_recentfiles, {});
extern_cfg (String, net_downloadpath);
extern_cfg (Bool, gl_logostuds);

static bool g_loadingMainFile = false;
static const int g_MaxRecentFiles = 5;
static bool g_aborted = false;
static LDDocument* g_logoedStud = null;
static LDDocument* g_logoedStud2 = null;

LDDocument* LDDocument::m_curdoc = null;

// =============================================================================
// -----------------------------------------------------------------------------
namespace LDPaths
{	static str pathError;

	struct
	{	str LDConfigPath;
		str partsPath, primsPath;
	} pathInfo;

	void initPaths()
	{	if (!tryConfigure (io_ldpath))
		{	LDrawPathDialog dlg (false);

			if (!dlg.exec())
				exit (0);

			io_ldpath = dlg.filename();
		}
	}

	bool tryConfigure (str path)
	{	QDir dir;

		if (!dir.cd (path))
		{	pathError = "Directory does not exist.";
			return false;
		}

		QStringList mustHave = { "LDConfig.ldr", "parts", "p" };
		QStringList contents = dir.entryList (mustHave);

		if (contents.size() != mustHave.size())
		{	pathError = "Not an LDraw directory! Must<br />have LDConfig.ldr, parts/ and p/.";
			return false;
		}

		pathInfo.partsPath = fmt ("%1" DIRSLASH "parts", path);
		pathInfo.LDConfigPath = fmt ("%1" DIRSLASH "LDConfig.ldr", path);
		pathInfo.primsPath = fmt ("%1" DIRSLASH "p", path);

		return true;
	}

	// Accessors
	str getError()
	{	return pathError;
	}

	str ldconfig()
	{	return pathInfo.LDConfigPath;
	}

	str prims()
	{	return pathInfo.primsPath;
	}

	str parts()
	{	return pathInfo.partsPath;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
LDDocument::LDDocument()
{	setImplicit (true);
	setSavePosition (-1);
	setListItem (null);
	setHistory (new History);
	m_History->setFile (this);
}

// =============================================================================
// -----------------------------------------------------------------------------
LDDocument::~LDDocument()
{	// Clear everything from the model
	for (LDObject* obj : getObjects())
		obj->deleteSelf();

	// Clear the cache as well
	for (LDObject* obj : getCache())
		obj->deleteSelf();

	delete m_History;

	// Remove this file from the list of files
	g_loadedFiles.removeOne (this);

	// If we just closed the current file, we need to set the current
	// file as something else.
	if (this == getCurrentDocument())
	{	bool found = false;

		// Try find an explicitly loaded file - if we can't find one,
		// we need to create a new file to switch to.
		for (LDDocument* file : g_loadedFiles)
		{	if (!file->isImplicit())
			{	LDDocument::setCurrent (file);
				found = true;
				break;
			}
		}

		if (!found)
			newFile();
	}

	g_win->updateDocumentList();
}

// =============================================================================
// -----------------------------------------------------------------------------
LDDocument* findDocument (str name)
{	for (LDDocument * file : g_loadedFiles)
		if (!file->getName().isEmpty() && file->getShortName() == name)
			return file;

	return null;
}

// =============================================================================
// -----------------------------------------------------------------------------
str dirname (str path)
{	long lastpos = path.lastIndexOf (DIRSLASH);

	if (lastpos > 0)
		return path.left (lastpos);

#ifndef _WIN32

	if (path[0] == DIRSLASH_CHAR)
		return DIRSLASH;

#endif // _WIN32

	return "";
}

// =============================================================================
// -----------------------------------------------------------------------------
str basename (str path)
{	long lastpos = path.lastIndexOf (DIRSLASH);

	if (lastpos != -1)
		return path.mid (lastpos + 1);

	return path;
}

// =============================================================================
// -----------------------------------------------------------------------------
File* openLDrawFile (str relpath, bool subdirs)
{	log ("Opening %1...\n", relpath);
	File* f = new File;
	str fullPath;

	// LDraw models use Windows-style path separators. If we're not on Windows,
	// replace the path separator now before opening any files. Qt expects
	// forward-slashes as directory separators.
#ifndef WIN32
	relpath.replace ("\\", "/");
#endif // WIN32

	if (getCurrentDocument())
	{	// First, try find the file in the current model's file path. We want a file
		// in the immediate vicinity of the current model to override stock LDraw stuff.
		str partpath = fmt ("%1" DIRSLASH "%2", dirname (getCurrentDocument()->getName()), relpath);

		if (f->open (partpath, File::Read))
			return f;
	}

	if (f->open (relpath, File::Read))
		return f;

	// Try with just the LDraw path first
	fullPath = fmt ("%1" DIRSLASH "%2", io_ldpath, relpath);

	if (f->open (fullPath, File::Read))
		return f;

	if (subdirs)
	{	// Look in sub-directories: parts and p. Also look in net_downloadpath, since that's
		// where we download parts from the PT to.
		for (const str& topdir : initlist<str> ({ io_ldpath, net_downloadpath }))
		{	for (const str& subdir : initlist<str> ({ "parts", "p" }))
			{	fullPath = fmt ("%1" DIRSLASH "%2" DIRSLASH "%3", topdir, subdir, relpath);

				if (f->open (fullPath, File::Read))
					return f;
			}
		}
	}

	// Did not find the file.
	log ("Could not find %1.\n", relpath);
	delete f;
	return null;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDFileLoader::start()
{	setDone (false);
	setProgress (0);
	setAborted (false);

	if (isOnForeground())
	{	g_aborted = false;

		// Show a progress dialog if we're loading the main document.here so we can
		// show progress updates and keep the WM posted that we're still here.
		// Of course we cannot exec() the dialog because then the dialog would
		// block.
		dlg = new OpenProgressDialog (g_win);
		dlg->setNumLines (getLines().size());
		dlg->setModal (true);
		dlg->show();

		// Connect the loader in so we can show updates
		connect (this, SIGNAL (workDone()), dlg, SLOT (accept()));
		connect (dlg, SIGNAL (rejected()), this, SLOT (abort()));
	}
	else
		dlg = null;

	// Begin working
	work (0);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDFileLoader::work (int i)
{	// User wishes to abort, so stop here now.
	if (isAborted())
	{	for (LDObject* obj : m_Objects)
			obj->deleteSelf();

		m_Objects.clear();
		setDone (true);
		return;
	}

	// Parse up to 300 lines per iteration
	int max = i + 300;

	for (; i < max && i < (int) getLines().size(); ++i)
	{	str line = getLines()[i];

		// Trim the trailing newline
		QChar c;

		while (!line.isEmpty() && ((c = line[line.length() - 1]) == '\n' || c == '\r'))
			line.chop (1);

		LDObject* obj = parseLine (line);

		// Check for parse errors and warn about tthem
		if (obj->getType() == LDObject::Error)
		{	log ("Couldn't parse line #%1: %2", getProgress() + 1, static_cast<LDError*> (obj)->reason);

			if (getWarnings() != null)
				(*getWarnings())++;
		}

		m_Objects << obj;
		setProgress (i);

		// If we have a dialog pointer, update the progress now
		if (isOnForeground())
			dlg->updateProgress (i);
	}

	// If we're done now, tell the environment we're done and stop.
	if (i >= ((int) getLines().size()) - 1)
	{	emit workDone();
		setDone (true);
		return;
	}

	// Otherwise, continue, by recursing back.
	if (!isDone())
	{	// If we have a dialog to show progress output to, we cannot just call
		// work() again immediately as the dialog needs some processor cycles as
		// well. Thus, take a detour through the event loop by using the
		// meta-object system.
		//
		// This terminates the loop here and control goes back to the function
		// which called the file loader. It will keep processing the event loop
		// until we're ready (see loadFileContents), thus the event loop will
		// eventually catch the invokation we throw here and send us back. Though
		// it's not technically recursion anymore, more like a for loop. :P
		if (isOnForeground())
			QMetaObject::invokeMethod (this, "work", Qt::QueuedConnection, Q_ARG (int, i));
		else
			work (i + 1);
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDFileLoader::abort()
{	setAborted (true);

	if (isOnForeground())
		g_aborted = true;
}

// =============================================================================
// -----------------------------------------------------------------------------
QList<LDObject*> loadFileContents (File* f, int* numWarnings, bool* ok)
{	QList<str> lines;
	QList<LDObject*> objs;

	if (numWarnings)
		*numWarnings = 0;

	// Read in the lines
	for (str line : *f)
		lines << line;

	LDFileLoader* loader = new LDFileLoader;
	loader->setWarnings (numWarnings);
	loader->setLines (lines);
	loader->setOnForeground (g_loadingMainFile);
	loader->start();

	// After start() returns, if the loader isn't done yet, it's delaying
	// its next iteration through the event loop. We need to catch this here
	// by telling the event loop to tick, which will tick the file loader again.
	// We keep doing this until the file loader is ready.
	while (loader->isDone() == false)
		qApp->processEvents();

	// If we wanted the success value, supply that now
	if (ok)
		*ok = !loader->isAborted();

	objs = loader->getObjects();
	return objs;
}

// =============================================================================
// -----------------------------------------------------------------------------
LDDocument* openDocument (str path, bool search)
{	// Convert the file name to lowercase since some parts contain uppercase
	// file names. I'll assume here that the library will always use lowercase
	// file names for the actual parts..
	File* f;

	if (search)
		f = openLDrawFile (path.toLower(), true);
	else
	{	f = new File (path, File::Read);

		if (!*f)
		{	delete f;
			return null;
		}
	}

	if (!f)
		return null;

	LDDocument* load = new LDDocument;
	load->setName (path);

	// Don't take the file loading as actual edits to the file
	load->getHistory()->setIgnoring (true);

	int numWarnings;
	bool ok;
	QList<LDObject*> objs = loadFileContents (f, &numWarnings, &ok);

	if (!ok)
	{	delete f;
		delete load;
		return null;
	}

	for (LDObject* obj : objs)
		load->addObject (obj);

	delete f;
	g_loadedFiles << load;

	if (g_loadingMainFile)
	{	LDDocument::setCurrent (load);
		g_win->R()->setFile (load);
		log (QObject::tr ("File %1 parsed successfully (%2 errors)."), path, numWarnings);
	}

	load->getHistory()->setIgnoring (false);
	return load;
}

// =============================================================================
// -----------------------------------------------------------------------------
bool LDDocument::isSafeToClose()
{	typedef QMessageBox msgbox;
	setlocale (LC_ALL, "C");

	// If we have unsaved changes, warn and give the option of saving.
	if (hasUnsavedChanges())
	{	str message = fmt (tr ("There are unsaved changes to %1. Should it be saved?"),
			(getName().length() > 0) ? getName() : tr ("<anonymous>"));

		int button = msgbox::question (g_win, tr ("Unsaved Changes"), message,
			(msgbox::Yes | msgbox::No | msgbox::Cancel), msgbox::Cancel);

		switch (button)
		{	case msgbox::Yes:
			{	// If we don't have a file path yet, we have to ask the user for one.
				if (getName().length() == 0)
				{	str newpath = QFileDialog::getSaveFileName (g_win, tr ("Save As"),
						getCurrentDocument()->getName(), tr ("LDraw files (*.dat *.ldr)"));

					if (newpath.length() == 0)
						return false;

					setName (newpath);
				}

				if (!save())
				{	message = fmt (tr ("Failed to save %1 (%2)\nDo you still want to close?"),
						getName(), strerror (errno));

					if (msgbox::critical (g_win, tr ("Save Failure"), message,
						(msgbox::Yes | msgbox::No), msgbox::No) == msgbox::No)
					{	return false;
					}
				}
			} break;

			case msgbox::Cancel:
				return false;

			default:
				break;
		}
	}

	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
void closeAll()
{	// Remove all loaded files and the objects they contain
	QList<LDDocument*> files = g_loadedFiles;

	for (LDDocument* file : files)
		delete file;
}

// =============================================================================
// -----------------------------------------------------------------------------
void newFile()
{	// Create a new anonymous file and set it to our current
	LDDocument* f = new LDDocument;
	f->setName ("");
	f->setImplicit (false);
	g_loadedFiles << f;
	LDDocument::setCurrent (f);
	LDDocument::closeInitialFile();
	g_win->R()->setFile (f);
	g_win->doFullRefresh();
	g_win->updateTitle();
	g_win->updateActions();
}

// =============================================================================
// -----------------------------------------------------------------------------
void addRecentFile (str path)
{	auto& rfiles = io_recentfiles.value;
	int idx = rfiles.indexOf (path);

	// If this file already is in the list, pop it out.
	if (idx != -1)
	{	if (rfiles.size() == 1)
			return; // only recent file - abort and do nothing

		// Pop it out.
		rfiles.removeAt (idx);
	}

	// If there's too many recent files, drop one out.
	while (rfiles.size() > (g_MaxRecentFiles - 1))
		rfiles.removeAt (0);

	// Add the file
	rfiles << path;

	Config::save();
	g_win->updateRecentFilesMenu();
}

// =============================================================================
// Open an LDraw file and set it as the main model
// -----------------------------------------------------------------------------
void openMainFile (str path)
{	g_loadingMainFile = true;
	LDDocument* file = openDocument (path, false);

	if (!file)
	{	// Loading failed, thus drop down to a new file since we
		// closed everything prior.
		newFile();

		if (!g_aborted)
		{	// Tell the user loading failed.
			setlocale (LC_ALL, "C");
			critical (fmt (QObject::tr ("Failed to open %1: %2"), path, strerror (errno)));
		}

		g_loadingMainFile = false;
		return;
	}

	file->setImplicit (false);

	// If we have an anonymous, unchanged file open as the only open file
	// (aside of the one we just opened), close it now.
	LDDocument::closeInitialFile();

	// Rebuild the object tree view now.
	LDDocument::setCurrent (file);
	g_win->doFullRefresh();

	// Add it to the recent files list.
	addRecentFile (path);
	g_loadingMainFile = false;
}

// =============================================================================
// -----------------------------------------------------------------------------
bool LDDocument::save (str savepath)
{	if (!savepath.length())
		savepath = getName();

	File f (savepath, File::Write);

	if (!f)
		return false;

	// If the second object in the list holds the file name, update that now.
	// Only do this if the file is explicitly open. If it's saved into a directory
	// called "s" or "48", prepend that into the name.
	LDComment* fpathComment = null;
	LDObject* first = getObject (1);

	if (!isImplicit() && first != null && first->getType() == LDObject::Comment)
	{	fpathComment = static_cast<LDComment*> (first);

		if (fpathComment->text.left (6) == "Name: ")
		{	str newname;
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
	for (LDObject* obj : getObjects())
		f.write (obj->raw() + "\r\n");

	// File is saved, now clean up.
	f.close();

	// We have successfully saved, update the save position now.
	setSavePosition (getHistory()->getPosition());
	setName (savepath);

	g_win->updateDocumentListItem (this);
	g_win->updateTitle();
	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
#define CHECK_TOKEN_COUNT(N) \
	if (tokens.size() != N) \
		return new LDError (line, "Bad amount of tokens");

#define CHECK_TOKEN_NUMBERS(MIN, MAX) \
	for (int i = MIN; i <= MAX; ++i) \
		if (!numeric (tokens[i])) \
			return new LDError (line, fmt ("Token #%1 was `%2`, expected a number", (i + 1), tokens[i]));

// =============================================================================
// -----------------------------------------------------------------------------
static vertex parseVertex (QStringList& s, const int n)
{	vertex v;

	for_axes (ax)
		v[ax] = s[n + ax].toDouble();

	return v;
}

// =============================================================================
// This is the LDraw code parser function. It takes in a string containing LDraw
// code and returns the object parsed from it. parseLine never returns null,
// the object will be LDError if it could not be parsed properly.
// -----------------------------------------------------------------------------
LDObject* parseLine (str line)
{	QStringList tokens = line.split (" ", str::SkipEmptyParts);

	if (tokens.size() <= 0)
	{	// Line was empty, or only consisted of whitespace
		return new LDEmpty;
	}

	if (tokens[0].length() != 1 || tokens[0][0].isDigit() == false)
		return new LDError (line, "Illogical line code");

	int num = tokens[0][0].digitValue();

	switch (num)
	{	case 0:
		{	// Comment
			str comm = line.mid (line.indexOf ("0") + 1).simplified();

			// Handle BFC statements
			if (tokens.size() > 2 && tokens[1] == "BFC")
			{	for (int i = 0; i < LDBFC::NumStatements; ++i)
					if (comm == fmt ("BFC %1", LDBFC::statements [i]))
						return new LDBFC ( (LDBFC::Type) i);

				// MLCAD is notorious for stuffing these statements in parts it
				// creates. The above block only handles valid statements, so we
				// need to handle MLCAD-style invertnext, clip and noclip separately.
				struct
				{	str			a;
					LDBFC::Type	b;
				} BFCData[] =
				{	{ "INVERTNEXT", LDBFC::InvertNext },
					{ "NOCLIP", LDBFC::NoClip },
					{ "CLIP", LDBFC::Clip }
				};

				for (const auto& i : BFCData)
					if (comm == "BFC CERTIFY " + i.a)
						return new LDBFC (i.b);
			}

			if (tokens.size() > 2 && tokens[1] == "!LDFORGE")
			{	// Handle LDForge-specific types, they're embedded into comments too
				if (tokens[2] == "VERTEX")
				{	// Vertex (0 !LDFORGE VERTEX)
					CHECK_TOKEN_COUNT (7)
					CHECK_TOKEN_NUMBERS (3, 6)

					LDVertex* obj = new LDVertex;
					obj->setColor (tokens[3].toLong());

					for_axes (ax)
						obj->pos[ax] = tokens[4 + ax].toDouble(); // 4 - 6

					return obj;
				} elif (tokens[2] == "OVERLAY")
				{	CHECK_TOKEN_COUNT (9);
					CHECK_TOKEN_NUMBERS (5, 8)

					LDOverlay* obj = new LDOverlay;
					obj->setFileName (tokens[3]);
					obj->setCamera (tokens[4].toLong());
					obj->setX (tokens[5].toLong());
					obj->setY (tokens[6].toLong());
					obj->setWidth (tokens[7].toLong());
					obj->setHeight (tokens[8].toLong());
					return obj;
				}
			}

			// Just a regular comment:
			LDComment* obj = new LDComment;
			obj->text = comm;
			return obj;
		}

		case 1:
		{	// Subfile
			CHECK_TOKEN_COUNT (15)
			CHECK_TOKEN_NUMBERS (1, 13)

			// Try open the file. Disable g_loadingMainFile temporarily since we're
			// not loading the main file now, but the subfile in question.
			bool tmp = g_loadingMainFile;
			g_loadingMainFile = false;
			LDDocument* load = getDocument (tokens[14]);
			g_loadingMainFile = tmp;

			// If we cannot open the file, mark it an error
			if (!load)
			{	LDError* obj = new LDError (line, fmt ("Could not open %1", tokens[14]));
				obj->setFileReferenced (tokens[14]);
				return obj;
			}

			LDSubfile* obj = new LDSubfile;
			obj->setColor (tokens[1].toLong());
			obj->setPosition (parseVertex (tokens, 2));  // 2 - 4

			matrix transform;

			for (int i = 0; i < 9; ++i)
				transform[i] = tokens[i + 5].toDouble(); // 5 - 13

			obj->setTransform (transform);
			obj->setFileInfo (load);
			return obj;
		}

		case 2:
		{	CHECK_TOKEN_COUNT (8)
			CHECK_TOKEN_NUMBERS (1, 7)

			// Line
			LDLine* obj = new LDLine;
			obj->setColor (tokens[1].toLong());

			for (int i = 0; i < 2; ++i)
				obj->setVertex (i, parseVertex (tokens, 2 + (i * 3)));   // 2 - 7

			return obj;
		}

		case 3:
		{	CHECK_TOKEN_COUNT (11)
			CHECK_TOKEN_NUMBERS (1, 10)

			// Triangle
			LDTriangle* obj = new LDTriangle;
			obj->setColor (tokens[1].toLong());

			for (int i = 0; i < 3; ++i)
				obj->setVertex (i, parseVertex (tokens, 2 + (i * 3)));   // 2 - 10

			return obj;
		}

		case 4:
		case 5:
		{	CHECK_TOKEN_COUNT (14)
			CHECK_TOKEN_NUMBERS (1, 13)

			// Quadrilateral / Conditional line
			LDObject* obj;

			if (num == 4)
				obj = new LDQuad;
			else
				obj = new LDCondLine;

			obj->setColor (tokens[1].toLong());

			for (int i = 0; i < 4; ++i)
				obj->setVertex (i, parseVertex (tokens, 2 + (i * 3)));   // 2 - 13

			return obj;
		}

		default: // Strange line we couldn't parse
			return new LDError (line, "Unknown line code number");
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
LDDocument* getDocument (str filename)
{	// Try find the file in the list of loaded files
	LDDocument* doc = findDocument (filename);

	// If it's not loaded, try open it
	if (!doc)
		doc = openDocument (filename, true);

	return doc;
}

// =============================================================================
// -----------------------------------------------------------------------------
void reloadAllSubfiles()
{	if (!getCurrentDocument())
		return;

	g_loadedFiles.clear();
	g_loadedFiles << getCurrentDocument();

	// Go through all objects in the current file and reload the subfiles
	for (LDObject* obj : getCurrentDocument()->getObjects())
	{	if (obj->getType() == LDObject::Subfile)
		{	LDSubfile* ref = static_cast<LDSubfile*> (obj);
			LDDocument* fileInfo = getDocument (ref->getFileInfo()->getName());

			if (fileInfo)
				ref->setFileInfo (fileInfo);
			else
				ref->replace (new LDError (ref->raw(), "Could not open referred file"));
		}

		// Reparse gibberish files. It could be that they are invalid because
		// of loading errors. Circumstances may be different now.
		if (obj->getType() == LDObject::Error)
			obj->replace (parseLine (static_cast<LDError*> (obj)->contents));
	}

	// Close all files left unused
	LDDocument::closeUnused();
}

// =============================================================================
// -----------------------------------------------------------------------------
int LDDocument::addObject (LDObject* obj)
{	getHistory()->add (new AddHistory (getObjects().size(), obj));
	m_Objects << obj;

	if (obj->getType() == LDObject::Vertex)
		m_Vertices << obj;

#ifdef DEBUG
	if (!isImplicit())
		dlog ("Added object #%1 (%2)\n", obj->getID(), obj->getTypeName());
#endif

	obj->setFile (this);
	return getObjectCount() - 1;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::addObjects (const QList<LDObject*> objs)
{	for (LDObject* obj : objs)
		if (obj)
			addObject (obj);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::insertObj (int pos, LDObject* obj)
{	getHistory()->add (new AddHistory (pos, obj));
	m_Objects.insert (pos, obj);
	obj->setFile (this);

#ifdef DEBUG
	if (!isImplicit())
		dlog ("Inserted object #%1 (%2) at %3\n", obj->getID(), obj->getTypeName(), pos);
#endif
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::forgetObject (LDObject* obj)
{	int idx = obj->getIndex();
	assert (m_Objects[idx] == obj);
	dlog ("id: %1, type: %2, code: %3", obj->getID(), obj->getType(), obj->raw());
	getHistory()->add (new DelHistory (idx, obj));
	m_Objects.removeAt (idx);
	obj->setFile (null);
}

// =============================================================================
// -----------------------------------------------------------------------------
bool safeToCloseAll()
{	for (LDDocument* f : g_loadedFiles)
		if (!f->isSafeToClose())
			return false;

	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::setObject (int idx, LDObject* obj)
{	assert (idx >= 0 && idx < m_Objects.size());

	// Mark this change to history
	if (!m_History->isIgnoring())
	{	str oldcode = getObject (idx)->raw();
		str newcode = obj->raw();
		*m_History << new EditHistory (idx, oldcode, newcode);
	}

	obj->setFile (this);
	m_Objects[idx] = obj;
}

// =============================================================================
// -----------------------------------------------------------------------------
static QList<LDDocument*> getFilesUsed (LDDocument* node)
{	QList<LDDocument*> filesUsed;

	for (LDObject* obj : node->getObjects())
	{	if (obj->getType() != LDObject::Subfile)
			continue;

		LDSubfile* ref = static_cast<LDSubfile*> (obj);
		filesUsed << ref->getFileInfo();
		filesUsed << getFilesUsed (ref->getFileInfo());
	}

	return filesUsed;
}

// =============================================================================
// Find out which files are unused and close them.
// -----------------------------------------------------------------------------
void LDDocument::closeUnused()
{	QList<LDDocument*> filesUsed = getFilesUsed (getCurrentDocument());

	// Anything that's explicitly opened must not be closed
	for (LDDocument* file : g_loadedFiles)
		if (!file->isImplicit())
			filesUsed << file;

	// Remove duplicated entries
	removeDuplicates (filesUsed);

	// Close all open files that aren't in filesUsed
	for (LDDocument* file : g_loadedFiles)
	{	bool isused = false;

		for (LDDocument* usedFile : filesUsed)
		{	if (file == usedFile)
			{	isused = true;
				break;
			}
		}

		if (!isused)
			delete file;
	}

	g_loadedFiles.clear();
	g_loadedFiles << filesUsed;
}

// =============================================================================
// -----------------------------------------------------------------------------
LDObject* LDDocument::getObject (int pos) const
{	if (m_Objects.size() <= pos)
		return null;

	return m_Objects[pos];
}

// =============================================================================
// -----------------------------------------------------------------------------
int LDDocument::getObjectCount() const
{	return getObjects().size();
}

// =============================================================================
// -----------------------------------------------------------------------------
bool LDDocument::hasUnsavedChanges() const
{	return !isImplicit() && getHistory()->getPosition() != getSavePosition();
}

// =============================================================================
// -----------------------------------------------------------------------------
str LDDocument::getShortName()
{	if (!getName().isEmpty())
		return basename (getName());

	if (!getDefaultName().isEmpty())
		return getDefaultName();

	return tr ("<anonymous>");
}

// =============================================================================
// -----------------------------------------------------------------------------
QList<LDObject*> LDDocument::inlineContents (LDSubfile::InlineFlags flags)
{	// Possibly substitute with logoed studs:
	// stud.dat -> stud-logo.dat
	// stud2.dat -> stud-logo2.dat
	if (gl_logostuds && (flags & LDSubfile::RendererInline))
	{	if (getName() == "stud.dat" && g_logoedStud)
			return g_logoedStud->inlineContents (flags);
		elif (getName() == "stud2.dat" && g_logoedStud2)
			return g_logoedStud2->inlineContents (flags);
	}

	QList<LDObject*> objs, objcache;

	bool deep = flags & LDSubfile::DeepInline,
		 doCache = flags & LDSubfile::CacheInline;

	// If we have this cached, just create a copy of that
	if (deep && getCache().size())
	{	for (LDObject* obj : getCache())
			objs << obj->createCopy();
	}
	else
	{	if (!deep)
			doCache = false;

		for (LDObject* obj : getObjects())
		{	// Skip those without scemantic meaning
			if (!obj->isScemantic())
				continue;

			// Got another sub-file reference, inline it if we're deep-inlining. If not,
			// just add it into the objects normally. Also, we only cache immediate
			// subfiles and this is not one. Yay, recursion!
			if (deep && obj->getType() == LDObject::Subfile)
			{	LDSubfile* ref = static_cast<LDSubfile*> (obj);

				// We only want to cache immediate subfiles, so shed the caching
				// flag when recursing deeper in hierarchy.
				QList<LDObject*> otherobjs = ref->inlineContents (flags & ~ (LDSubfile::CacheInline));

				for (LDObject* otherobj : otherobjs)
				{	// Cache this object, if desired
					if (doCache)
						objcache << otherobj->createCopy();

					objs << otherobj;
				}
			}
			else
			{	if (doCache)
					objcache << obj->createCopy();

				objs << obj->createCopy();
			}
		}

		if (doCache)
			setCache (objcache);
	}

	return objs;
}

// =============================================================================
// -----------------------------------------------------------------------------
LDDocument* LDDocument::current()
{	return m_curdoc;
}

// =============================================================================
// Sets the given file as the current one on display. At some point in time this
// was an operation completely unheard of. ;)
//
// FIXME: f can be temporarily null. This probably should not be the case.
// -----------------------------------------------------------------------------
void LDDocument::setCurrent (LDDocument* f)
{	// Implicit files were loaded for caching purposes and must never be set
	// current.
	if (f && f->isImplicit())
		return;

	m_curdoc = f;

	if (g_win && f)
	{	// A ton of stuff needs to be updated
		g_win->updateDocumentListItem (f);
		g_win->buildObjList();
		g_win->updateTitle();
		g_win->R()->setFile (f);
		g_win->R()->resetAllAngles();
		g_win->R()->repaint();

		log ("Changed file to %1", f->getShortName());
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
int LDDocument::countExplicitFiles()
{	int count = 0;

	for (LDDocument* f : g_loadedFiles)
		if (f->isImplicit() == false)
			count++;

	return count;
}

// =============================================================================
// This little beauty closes the initial file that was open at first when opening
// a new file over it.
// -----------------------------------------------------------------------------
void LDDocument::closeInitialFile()
{	if (
		countExplicitFiles() == 2 &&
		g_loadedFiles[0]->getName() == "" &&
		!g_loadedFiles[0]->hasUnsavedChanges()
	)
		delete g_loadedFiles[0];
}

// =============================================================================
// -----------------------------------------------------------------------------
void loadLogoedStuds()
{	log ("Loading logoed studs...\n");

	delete g_logoedStud;
	delete g_logoedStud2;

	g_logoedStud = openDocument ("stud-logo.dat", true);
	g_logoedStud2 = openDocument ("stud2-logo.dat", true);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::addToSelection (LDObject* obj) // [protected]
{	if (obj->isSelected())
		return;

	assert (obj->getFile() == this);
	m_sel << obj;
	obj->setSelected (true);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::removeFromSelection (LDObject* obj) // [protected]
{	if (!obj->isSelected())
		return;

	assert (obj->getFile() == this);
	m_sel.removeOne (obj);
	obj->setSelected (false);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::clearSelection()
{	for (LDObject* obj : m_sel)
		removeFromSelection (obj);

	assert (m_sel.isEmpty());
}

// =============================================================================
// -----------------------------------------------------------------------------
const QList<LDObject*>& LDDocument::getSelection() const
{	return m_sel;
}