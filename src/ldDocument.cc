/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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
#include <QTime>
#include <QApplication>

#include "main.h"
#include "configuration.h"
#include "ldDocument.h"
#include "miscallenous.h"
#include "mainWindow.h"
#include "editHistory.h"
#include "dialogs.h"
#include "glRenderer.h"
#include "misc/invokeLater.h"
#include "glCompiler.h"

CFGENTRY (String,			ldrawPath, "")
CFGENTRY (List,				recentFiles, {})
EXTERN_CFGENTRY (String,	downloadFilePath)
EXTERN_CFGENTRY (Bool,		useLogoStuds)

static bool g_loadingMainFile = false;
static const int g_maxRecentFiles = 10;
static bool g_aborted = false;
static LDDocumentPointer g_logoedStud = null;
static LDDocumentPointer g_logoedStud2 = null;

LDDocument* LDDocument::m_curdoc = null;

const QStringList g_specialSubdirectories ({ "s", "48", "8" });

// =============================================================================
//
namespace LDPaths
{
	static String pathError;

	struct
	{
		String LDConfigPath;
		String partsPath, primsPath;
	} pathInfo;

	void initPaths()
	{
		if (not tryConfigure (cfg::ldrawPath))
		{
			LDrawPathDialog dlg (false);

			if (not dlg.exec())
				exit (0);

			cfg::ldrawPath = dlg.filename();
		}
	}

	bool tryConfigure (String path)
	{
		QDir dir;

		if (not dir.cd (path))
		{
			pathError = "Directory does not exist.";
			return false;
		}

		QStringList mustHave = { "LDConfig.ldr", "parts", "p" };
		QStringList contents = dir.entryList (mustHave);

		if (contents.size() != mustHave.size())
		{
			pathError = "Not an LDraw directory! Must<br />have LDConfig.ldr, parts/ and p/.";
			return false;
		}

		pathInfo.partsPath = format ("%1" DIRSLASH "parts", path);
		pathInfo.LDConfigPath = format ("%1" DIRSLASH "LDConfig.ldr", path);
		pathInfo.primsPath = format ("%1" DIRSLASH "p", path);

		return true;
	}

	// Accessors
	String getError()
	{
		return pathError;
	}

	String ldconfig()
	{
		return pathInfo.LDConfigPath;
	}

	String prims()
	{
		return pathInfo.primsPath;
	}

	String parts()
	{
		return pathInfo.partsPath;
	}
}

// =============================================================================
//
LDDocument::LDDocument() :
	m_flags (0),
	m_gldata (new LDGLData)
{
	setImplicit (true);
	setSavePosition (-1);
	setTabIndex (-1);
	setHistory (new History);
	history()->setDocument (this);
	m_needsReCache = true;
}

// =============================================================================
//
LDDocument::~LDDocument()
{
	// Remove this file from the list of files. This MUST be done FIRST, otherwise
	// a ton of other functions will think this file is still valid when it is not!
	g_loadedFiles.removeOne (this);
	m_flags |= DOCF_IsBeingDestroyed;
	m_history->setIgnoring (true);

	// Clear everything from the model
	for (LDObject* obj : objects())
		obj->destroy();

	delete history();
	delete m_gldata;

	// If we just closed the current file, we need to set the current
	// file as something else.
	if (this == getCurrentDocument())
	{
		bool found = false;

		// Try find an explicitly loaded file - if we can't find one,
		// we need to create a new file to switch to.
		for (LDDocument* file : g_loadedFiles)
		{
			if (not file->isImplicit())
			{
				LDDocument::setCurrent (file);
				found = true;
				break;
			}
		}

		if (not found)
			newFile();
	}

	if (this == g_logoedStud)
		g_logoedStud = null;
	elif (this == g_logoedStud2)
		g_logoedStud2 = null;

	g_win->updateDocumentList();
	print ("Closed %1", name());
}

// =============================================================================
//
LDDocument* findDocument (String name)
{
	for (LDDocument * file : g_loadedFiles)
		if (not file->name().isEmpty() && file->name() == name)
			return file;

	return null;
}

// =============================================================================
//
String dirname (String path)
{
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
//
String basename (String path)
{
	long lastpos = path.lastIndexOf (DIRSLASH);

	if (lastpos != -1)
		return path.mid (lastpos + 1);

	return path;
}

// =============================================================================
//
static String findLDrawFilePath (String relpath, bool subdirs)
{
	String fullPath;

	// LDraw models use Windows-style path separators. If we're not on Windows,
	// replace the path separator now before opening any files. Qt expects
	// forward-slashes as directory separators.
#ifndef WIN32
	relpath.replace ("\\", "/");
#endif // WIN32

	// Try find it relative to other currently open documents. We want a file
	// in the immediate vicinity of a current model to override stock LDraw stuff.
	String reltop = basename (dirname (relpath));

	for (LDDocument* doc : g_loadedFiles)
	{
		if (doc->fullPath().isEmpty())
			continue;

		String partpath = format ("%1/%2", dirname (doc->fullPath()), relpath);
		QFile f (partpath);

		if (f.exists())
		{
			// ensure we don't mix subfiles and 48-primitives with non-subfiles and non-48
			String proptop = basename (dirname (partpath));

			bool bogus = false;

			for (String s : g_specialSubdirectories)
			{
				if ((proptop == s && reltop != s) || (reltop == s && proptop != s))
				{
					bogus = true;
					break;
				}
			}

			if (not bogus)
				return partpath;
		}
	}

	if (QFile::exists (relpath))
		return relpath;

	// Try with just the LDraw path first
	fullPath = format ("%1" DIRSLASH "%2", cfg::ldrawPath, relpath);

	if (QFile::exists (fullPath))
		return fullPath;

	if (subdirs)
	{
		// Look in sub-directories: parts and p. Also look in net_downloadpath, since that's
		// where we download parts from the PT to.
		for (const String& topdir : QList<String> ({ cfg::ldrawPath, cfg::downloadFilePath }))
		{
			for (const String& subdir : QList<String> ({ "parts", "p" }))
			{
				fullPath = format ("%1" DIRSLASH "%2" DIRSLASH "%3", topdir, subdir, relpath);

				if (QFile::exists (fullPath))
					return fullPath;
			}
		}
	}

	// Did not find the file.
	return "";
}

QFile* openLDrawFile (String relpath, bool subdirs, String* pathpointer)
{
	print ("Opening %1...\n", relpath);
	String path = findLDrawFilePath (relpath, subdirs);

	if (pathpointer != null)
		*pathpointer = path;

	if (path.isEmpty())
		return null;

	QFile* fp = new QFile (path);

	if (fp->open (QIODevice::ReadOnly))
		return fp;

	fp->deleteLater();
	return null;
}

// =============================================================================
//
void LDFileLoader::start()
{
	setDone (false);
	setProgress (0);
	setAborted (false);

	if (isOnForeground())
	{
		g_aborted = false;

		// Show a progress dialog if we're loading the main ldDocument.here so we can
		// show progress updates and keep the WM posted that we're still here.
		// Of course we cannot exec() the dialog because then the dialog would
		// block.
		dlg = new OpenProgressDialog (g_win);
		dlg->setNumLines (lines().size());
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
//
void LDFileLoader::work (int i)
{
	// User wishes to abort, so stop here now.
	if (isAborted())
	{
		for (LDObject* obj : m_objects)
			obj->destroy();

		m_objects.clear();
		setDone (true);
		return;
	}

	// Parse up to 300 lines per iteration
	int max = i + 300;

	for (; i < max && i < (int) lines().size(); ++i)
	{
		String line = lines()[i];

		// Trim the trailing newline
		QChar c;

		while (line.endsWith ("\n") || line.endsWith ("\r"))
			line.chop (1);

		LDObject* obj = parseLine (line);

		// Check for parse errors and warn about tthem
		if (obj->type() == LDObject::EError)
		{
			print ("Couldn't parse line #%1: %2", progress() + 1, static_cast<LDError*> (obj)->reason());

			if (warnings() != null)
				(*warnings())++;
		}

		m_objects << obj;
		setProgress (i);

		// If we have a dialog pointer, update the progress now
		if (isOnForeground())
			dlg->updateProgress (i);
	}

	// If we're done now, tell the environment we're done and stop.
	if (i >= ((int) lines().size()) - 1)
	{
		emit workDone();
		setDone (true);
		return;
	}

	// Otherwise, continue, by recursing back.
	if (not isDone())
	{
		// If we have a dialog to show progress output to, we cannot just call
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
			work (i);
	}
}

// =============================================================================
//
void LDFileLoader::abort()
{
	setAborted (true);

	if (isOnForeground())
		g_aborted = true;
}

// =============================================================================
//
LDObjectList loadFileContents (QFile* fp, int* numWarnings, bool* ok)
{
	QStringList lines;
	LDObjectList objs;

	if (numWarnings)
		*numWarnings = 0;

	// Read in the lines
	while (not fp->atEnd())
		lines << String::fromUtf8 (fp->readLine());

	LDFileLoader* loader = new LDFileLoader;
	loader->setWarnings (numWarnings);
	loader->setLines (lines);
	loader->setOnForeground (g_loadingMainFile);
	loader->start();

	// After start() returns, if the loader isn't done yet, it's delaying
	// its next iteration through the event loop. We need to catch this here
	// by telling the event loop to tick, which will tick the file loader again.
	// We keep doing this until the file loader is ready.
	while (not loader->isDone())
		qApp->processEvents();

	// If we wanted the success value, supply that now
	if (ok)
		*ok = not loader->isAborted();

	objs = loader->objects();
	return objs;
}

// =============================================================================
//
LDDocument* openDocument (String path, bool search, bool implicit)
{
	// Convert the file name to lowercase since some parts contain uppercase
	// file names. I'll assume here that the library will always use lowercase
	// file names for the actual parts..
	QFile* fp;
	String fullpath;

	if (search)
		fp = openLDrawFile (path.toLower(), true, &fullpath);
	else
	{
		fp = new QFile (path);
		fullpath = path;

		if (not fp->open (QIODevice::ReadOnly))
		{
			delete fp;
			return null;
		}
	}

	if (not fp)
		return null;

	LDDocument* load = new LDDocument;
	load->setImplicit (implicit);
	load->setFullPath (fullpath);
	load->setName (LDDocument::shortenName (load->fullPath()));
	dprint ("name: %1 (%2)", load->name(), load->fullPath());
	g_loadedFiles << load;

	// Don't take the file loading as actual edits to the file
	load->history()->setIgnoring (true);

	int numWarnings;
	bool ok;
	LDObjectList objs = loadFileContents (fp, &numWarnings, &ok);
	fp->close();
	fp->deleteLater();

	if (not ok)
	{
		g_loadedFiles.removeOne (load);
		delete load;
		return null;
	}

	load->addObjects (objs);

	if (g_loadingMainFile)
	{
		LDDocument::setCurrent (load);
		g_win->R()->setDocument (load);
		print (QObject::tr ("File %1 parsed successfully (%2 errors)."), path, numWarnings);
	}

	load->history()->setIgnoring (false);
	return load;
}

// =============================================================================
//
bool LDDocument::isSafeToClose()
{
	typedef QMessageBox msgbox;
	setlocale (LC_ALL, "C");

	// If we have unsaved changes, warn and give the option of saving.
	if (hasUnsavedChanges())
	{
		String message = format (tr ("There are unsaved changes to %1. Should it be saved?"), getDisplayName());

		int button = msgbox::question (g_win, tr ("Unsaved Changes"), message,
			(msgbox::Yes | msgbox::No | msgbox::Cancel), msgbox::Cancel);

		switch (button)
		{
			case msgbox::Yes:
			{
				// If we don't have a file path yet, we have to ask the user for one.
				if (name().length() == 0)
				{
					String newpath = QFileDialog::getSaveFileName (g_win, tr ("Save As"),
						getCurrentDocument()->name(), tr ("LDraw files (*.dat *.ldr)"));

					if (newpath.length() == 0)
						return false;

					setName (newpath);
				}

				if (not save())
				{
					message = format (tr ("Failed to save %1 (%2)\nDo you still want to close?"),
						name(), strerror (errno));

					if (msgbox::critical (g_win, tr ("Save Failure"), message,
						(msgbox::Yes | msgbox::No), msgbox::No) == msgbox::No)
					{
						return false;
					}
				}
				break;
			}

			case msgbox::Cancel:
				return false;

			default:
				break;
		}
	}

	return true;
}

// =============================================================================
//
void closeAll()
{
	// Remove all loaded files and the objects they contain
	QList<LDDocument*> files = g_loadedFiles;

	for (LDDocument* file : files)
		delete file;
}

// =============================================================================
//
void newFile()
{
	// Create a new anonymous file and set it to our current
	LDDocument* f = new LDDocument;
	f->setName ("");
	f->setImplicit (false);
	g_loadedFiles << f;
	LDDocument::setCurrent (f);
	LDDocument::closeInitialFile();
	g_win->R()->setDocument (f);
	g_win->doFullRefresh();
	g_win->updateTitle();
	g_win->updateActions();
}

// =============================================================================
//
void addRecentFile (String path)
{
	auto& rfiles = cfg::recentFiles;
	int idx = rfiles.indexOf (path);

	// If this file already is in the list, pop it out.
	if (idx != -1)
	{
		if (rfiles.size() == 1)
			return; // only recent file - abort and do nothing

		// Pop it out.
		rfiles.removeAt (idx);
	}

	// If there's too many recent files, drop one out.
	while (rfiles.size() > (g_maxRecentFiles - 1))
		rfiles.removeAt (0);

	// Add the file
	rfiles << path;

	Config::save();
	g_win->updateRecentFilesMenu();
}

// =============================================================================
// Open an LDraw file and set it as the main model
// =============================================================================
void openMainFile (String path)
{
	g_loadingMainFile = true;

	// If there's already a file with the same name, this file must replace it.
	LDDocument* documentToReplace = null;
	String shortName = LDDocument::shortenName (path);

	for (LDDocument* doc : g_loadedFiles)
	{
		if (doc->name() == shortName)
		{
			documentToReplace = doc;
			break;
		}
	}

	// We cannot open this file if the document this would replace is not
	// safe to close.
	if (documentToReplace != null && not documentToReplace->isSafeToClose())
	{
		g_loadingMainFile = false;
		return;
	}

	LDDocument* file = openDocument (path, false, false);

	if (not file)
	{
		// Loading failed, thus drop down to a new file since we
		// closed everything prior.
		newFile();

		if (not g_aborted)
		{
			// Tell the user loading failed.
			setlocale (LC_ALL, "C");
			critical (format (QObject::tr ("Failed to open %1: %2"), path, strerror (errno)));
		}

		g_loadingMainFile = false;
		return;
	}

	file->setImplicit (false);

	// Replace references to the old file with the new file.
	if (documentToReplace != null)
	{
		for (LDDocumentPointer* ptr : documentToReplace->references())
		{	dprint ("ptr: %1 (%2)\n",
				ptr, ptr->pointer() ? ptr->pointer()->name() : "<null>");

			*ptr = file;
		}

		assert (documentToReplace->references().isEmpty());
		delete documentToReplace;
	}

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
//
bool LDDocument::save (String savepath)
{
	if (not savepath.length())
		savepath = fullPath();

	QFile f (savepath);

	if (not f.open (QIODevice::WriteOnly))
		return false;

	// If the second object in the list holds the file name, update that now.
	// Only do this if the file is explicitly open.
	LDObject* nameObject = getObject (1);

	if (not isImplicit() && nameObject != null && nameObject->type() == LDObject::EComment)
	{
		LDComment* nameComment = static_cast<LDComment*> (nameObject);

		if (nameComment->text().left (6) == "Name: ")
		{
			String newname = shortenName (savepath);
			nameComment->setText (format ("Name: %1", newname));
			g_win->buildObjList();
		}
	}

	// File is open, now save the model to it. Note that LDraw requires files to
	// have DOS line endings, so we terminate the lines with \r\n.
	for (LDObject* obj : objects())
		f.write ((obj->asText() + "\r\n").toUtf8());

	// File is saved, now clean up.
	f.close();

	// We have successfully saved, update the save position now.
	setSavePosition (history()->position());
	setFullPath (savepath);
	setName (shortenName (savepath));

	g_win->updateDocumentListItem (this);
	g_win->updateTitle();
	return true;
}

// =============================================================================
//
class LDParseError : public std::exception
{
	PROPERTY (private, String,	error,	setError,	STOCK_WRITE)
	PROPERTY (private, String,	line,	setLine,	STOCK_WRITE)

	public:
		LDParseError (String line, String a) :
			m_error (a),
			m_line (line) {}

		const char* what() const throw()
		{
			return qPrintable (error());
		}
};

// =============================================================================
//
void checkTokenCount (String line, const QStringList& tokens, int num)
{
	if (tokens.size() != num)
		throw LDParseError (line, format ("Bad amount of tokens, expected %1, got %2", num, tokens.size()));
}

// =============================================================================
//
void checkTokenNumbers (String line, const QStringList& tokens, int min, int max)
{
	bool ok;

	// Check scientific notation, e.g. 7.99361e-15
	QRegExp scient ("\\-?[0-9]+\\.[0-9]+e\\-[0-9]+");

	for (int i = min; i <= max; ++i)
	{
		tokens[i].toDouble (&ok);

		if (not ok && not scient.exactMatch (tokens[i]))
		{
			throw LDParseError (line, format ("Token #%1 was `%2`, expected a number (matched length: %3)",
				(i + 1), tokens[i], scient.matchedLength()));
		}
	}
}

// =============================================================================
//
static Vertex parseVertex (QStringList& s, const int n)
{
	Vertex v;
	v.apply ([&] (Axis ax, double& a) { a = s[n + ax].toDouble(); });
	return v;
}

// =============================================================================
// This is the LDraw code parser function. It takes in a string containing LDraw
// code and returns the object parsed from it. parseLine never returns null,
// the object will be LDError if it could not be parsed properly.
// =============================================================================
LDObject* parseLine (String line)
{
	try
	{
		QStringList tokens = line.split (" ", String::SkipEmptyParts);

		if (tokens.size() <= 0)
		{
			// Line was empty, or only consisted of whitespace
			return new LDEmpty;
		}

		if (tokens[0].length() != 1 || not tokens[0][0].isDigit())
			throw LDParseError (line, "Illogical line code");

		int num = tokens[0][0].digitValue();

		switch (num)
		{
			case 0:
			{
				// Comment
				String commentText (line.mid (line.indexOf ("0") + 2));
				String commentTextSimplified (commentText.simplified());

				// Handle BFC statements
				if (tokens.size() > 2 && tokens[1] == "BFC")
				{
					for (int i = 0; i < LDBFC::NumStatements; ++i)
						if (commentTextSimplified == format ("BFC %1", LDBFC::k_statementStrings [i]))
							return new LDBFC ((LDBFC::Statement) i);

					// MLCAD is notorious for stuffing these statements in parts it
					// creates. The above block only handles valid statements, so we
					// need to handle MLCAD-style invertnext, clip and noclip separately.
					if (commentTextSimplified == "BFC CERTIFY INVERTNEXT")
						return new LDBFC (LDBFC::InvertNext);
					elif (commentTextSimplified == "BFC CERTIFY CLIP")
						return new LDBFC (LDBFC::Clip);
					elif (commentTextSimplified == "BFC CERTIFY NOCLIP")
						return new LDBFC (LDBFC::NoClip);
				}

				if (tokens.size() > 2 && tokens[1] == "!LDFORGE")
				{
					// Handle LDForge-specific types, they're embedded into comments too
					if (tokens[2] == "VERTEX")
					{
						// Vertex (0 !LDFORGE VERTEX)
						checkTokenCount (line, tokens, 7);
						checkTokenNumbers (line, tokens, 3, 6);

						LDVertex* obj = new LDVertex;
						obj->setColor (tokens[3].toLong());
						obj->pos.apply ([&](Axis ax, double& value) { value = tokens[4 + ax].toDouble(); });
						return obj;
					}
					elif (tokens[2] == "OVERLAY")
					{
						checkTokenCount (line, tokens, 9);
						checkTokenNumbers (line, tokens, 5, 8);

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
				obj->setText (commentText);
				return obj;
			}

			case 1:
			{
				// Subfile
				checkTokenCount (line, tokens, 15);
				checkTokenNumbers (line, tokens, 1, 13);

				// Try open the file. Disable g_loadingMainFile temporarily since we're
				// not loading the main file now, but the subfile in question.
				bool tmp = g_loadingMainFile;
				g_loadingMainFile = false;
				LDDocument* load = getDocument (tokens[14]);
				g_loadingMainFile = tmp;

				// If we cannot open the file, mark it an error. Note we cannot use LDParseError
				// here because the error object needs the document reference.
				if (not load)
				{
					LDError* obj = new LDError (line, format ("Could not open %1", tokens[14]));
					obj->setFileReferenced (tokens[14]);
					return obj;
				}

				LDSubfile* obj = new LDSubfile;
				obj->setColor (tokens[1].toLong());
				obj->setPosition (parseVertex (tokens, 2));  // 2 - 4

				Matrix transform;

				for (int i = 0; i < 9; ++i)
					transform[i] = tokens[i + 5].toDouble(); // 5 - 13

				obj->setTransform (transform);
				obj->setFileInfo (load);
				return obj;
			}

			case 2:
			{
				checkTokenCount (line, tokens, 8);
				checkTokenNumbers (line, tokens, 1, 7);

				// Line
				LDLine* obj = new LDLine;
				obj->setColor (tokens[1].toLong());

				for (int i = 0; i < 2; ++i)
					obj->setVertex (i, parseVertex (tokens, 2 + (i * 3)));   // 2 - 7

				return obj;
			}

			case 3:
			{
				checkTokenCount (line, tokens, 11);
				checkTokenNumbers (line, tokens, 1, 10);

				// Triangle
				LDTriangle* obj = new LDTriangle;
				obj->setColor (tokens[1].toLong());

				for (int i = 0; i < 3; ++i)
					obj->setVertex (i, parseVertex (tokens, 2 + (i * 3)));   // 2 - 10

				return obj;
			}

			case 4:
			case 5:
			{
				checkTokenCount (line, tokens, 14);
				checkTokenNumbers (line, tokens, 1, 13);

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

			default:
				throw LDError (line, "Unknown line code number");
		}
	}
	catch (LDParseError& e)
	{
		// Strange line we couldn't parse
		return new LDError (e.line(), e.error());
	}
}

// =============================================================================
//
LDDocument* getDocument (String filename)
{
	// Try find the file in the list of loaded files
	LDDocument* doc = findDocument (filename);

	// If it's not loaded, try open it
	if (not doc)
		doc = openDocument (filename, true, true);

	return doc;
}

// =============================================================================
//
void reloadAllSubfiles()
{
	if (not getCurrentDocument())
		return;

	g_loadedFiles.clear();
	g_loadedFiles << getCurrentDocument();

	// Go through all objects in the current file and reload the subfiles
	for (LDObject* obj : getCurrentDocument()->objects())
	{
		if (obj->type() == LDObject::ESubfile)
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			LDDocument* fileInfo = getDocument (ref->fileInfo()->name());

			if (fileInfo)
				ref->setFileInfo (fileInfo);
			else
				ref->replace (new LDError (ref->asText(), format ("Could not open %1", ref->fileInfo()->name())));
		}

		// Reparse gibberish files. It could be that they are invalid because
		// of loading errors. Circumstances may be different now.
		if (obj->type() == LDObject::EError)
			obj->replace (parseLine (static_cast<LDError*> (obj)->contents()));
	}
}

// =============================================================================
//
int LDDocument::addObject (LDObject* obj)
{
	history()->add (new AddHistory (objects().size(), obj));
	m_objects << obj;
	addKnownVerticesOf (obj);

#ifdef DEBUG
	if (not isImplicit())
		dprint ("Added object #%1 (%2)\n", obj->id(), obj->typeName());
#endif

	obj->setDocument (this);
	g_win->R()->compileObject (obj);
	return getObjectCount() - 1;
}

// =============================================================================
//
void LDDocument::addObjects (const LDObjectList objs)
{
	for (LDObject* obj : objs)
		if (obj)
			addObject (obj);
}

// =============================================================================
//
void LDDocument::insertObj (int pos, LDObject* obj)
{
	history()->add (new AddHistory (pos, obj));
	m_objects.insert (pos, obj);
	obj->setDocument (this);
	g_win->R()->compileObject (obj);
	addKnownVerticesOf (obj);

#ifdef DEBUG
	if (not isImplicit())
		dprint ("Inserted object #%1 (%2) at %3\n", obj->id(), obj->typeName(), pos);
#endif
}

// =============================================================================
//
void LDDocument::addKnownVerticesOf (LDObject* obj)
{
	if (isImplicit())
		return;

	if (obj->type() == LDObject::ESubfile)
	{
		LDSubfile* ref = static_cast<LDSubfile*> (obj);

		for (Vertex vrt : ref->fileInfo()->inlineVertices())
		{
			vrt.transform (ref->transform(), ref->position());
			addKnownVertexReference (vrt);
		}
	}
	else
	{
		for (int i = 0; i < obj->numVertices(); ++i)
			addKnownVertexReference (obj->vertex (i));
	}
}

// =============================================================================
//
void LDDocument::removeKnownVerticesOf (LDObject* obj)
{
	if (isImplicit())
		return;

	if (obj->type() == LDObject::ESubfile)
	{
		LDSubfile* ref = static_cast<LDSubfile*> (obj);

		for (Vertex vrt : ref->fileInfo()->inlineVertices())
		{
			vrt.transform (ref->transform(), ref->position());
			removeKnownVertexReference (vrt);
		}
	}
	else
	{
		for (int i = 0; i < obj->numVertices(); ++i)
			removeKnownVertexReference (obj->vertex (i));
	}
}

// =============================================================================
//
void LDDocument::forgetObject (LDObject* obj)
{
	int idx = obj->lineNumber();
	obj->deselect();
	assert (m_objects[idx] == obj);

	if (not isImplicit() && not (flags() & DOCF_IsBeingDestroyed))
	{
		history()->add (new DelHistory (idx, obj));
		removeKnownVerticesOf (obj);
	}

	m_objects.removeAt (idx);
	obj->setDocument (null);
}

// =============================================================================
//
void LDDocument::vertexChanged (const Vertex& a, const Vertex& b)
{
	removeKnownVertexReference (a);
	addKnownVertexReference (b);
}

// =============================================================================
//
void LDDocument::addKnownVertexReference (const Vertex& a)
{
	if (isImplicit())
		return;

	auto it = m_vertices.find (a);

	if (it == m_vertices.end())
		m_vertices[a] = 1;
	else
		++it.value();
}

// =============================================================================
//
void LDDocument::removeKnownVertexReference (const Vertex& a)
{
	if (isImplicit())
		return;

	auto it = m_vertices.find (a);
	assert (it != m_vertices.end());

	// If there's no more references to a given vertex, it is to be removed.
	if (--it.value() == 0)
		m_vertices.erase (it);
}

// =============================================================================
//
bool safeToCloseAll()
{
	for (LDDocument* f : g_loadedFiles)
		if (not f->isSafeToClose())
			return false;

	return true;
}

// =============================================================================
//
void LDDocument::setObject (int idx, LDObject* obj)
{
	assert (idx >= 0 && idx < m_objects.size());

	// Mark this change to history
	if (not m_history->isIgnoring())
	{
		String oldcode = getObject (idx)->asText();
		String newcode = obj->asText();
		*m_history << new EditHistory (idx, oldcode, newcode);
	}

	removeKnownVerticesOf (m_objects[idx]);
	m_objects[idx]->deselect();
	m_objects[idx]->setDocument (null);
	obj->setDocument (this);
	addKnownVerticesOf (obj);
	g_win->R()->compileObject (obj);
	m_objects[idx] = obj;
}

// =============================================================================
//
// Close all documents we don't need anymore
//
void LDDocument::closeUnused()
{
	for (int i = 0; i < g_loadedFiles.size(); ++i)
	{
		LDDocument* file = g_loadedFiles[i];

		if (file->isImplicit() && file->references().isEmpty())
			delete g_loadedFiles[i--];
	}
}

// =============================================================================
//
LDObject* LDDocument::getObject (int pos) const
{
	if (m_objects.size() <= pos)
		return null;

	return m_objects[pos];
}

// =============================================================================
//
int LDDocument::getObjectCount() const
{
	return objects().size();
}

// =============================================================================
//
bool LDDocument::hasUnsavedChanges() const
{
	return !isImplicit() && history()->position() != savePosition();
}

// =============================================================================
//
String LDDocument::getDisplayName()
{
	if (not name().isEmpty())
		return name();

	if (not defaultName().isEmpty())
		return "[" + defaultName() + "]";

	return tr ("untitled");
}

// =============================================================================
//
void LDDocument::initializeCachedData()
{
	if (not m_needsReCache)
		return;

	LDObjectList objs = inlineContents (true, true);
	m_storedVertices.clear();

	for (LDObject* obj : objs)
	{
		assert (obj->type() != LDObject::ESubfile);
		LDPolygon* data = obj->getPolygon();

		if (data != null)
		{
			m_polygonData << *data;
			delete data;
		}

		for (int i = 0; i < obj->numVertices(); ++i)
			m_storedVertices << obj->vertex (i);

		obj->destroy();
	}

	removeDuplicates (m_storedVertices);
	m_needsReCache = false;
}

// =============================================================================
//
QList<LDPolygon> LDDocument::inlinePolygons()
{
	initializeCachedData();
	return polygonData();
}

// =============================================================================
// -----------------------------------------------------------------------------
LDObjectList LDDocument::inlineContents (bool deep, bool renderinline)
{
	// Possibly substitute with logoed studs:
	// stud.dat -> stud-logo.dat
	// stud2.dat -> stud-logo2.dat
	if (cfg::useLogoStuds && renderinline)
	{
		// Ensure logoed studs are loaded first
		loadLogoedStuds();

		if (name() == "stud.dat" && g_logoedStud != null)
			return g_logoedStud->inlineContents (deep, renderinline);
		elif (name() == "stud2.dat" && g_logoedStud2 != null)
			return g_logoedStud2->inlineContents (deep, renderinline);
	}

	LDObjectList objs, objcache;

	for (LDObject* obj : objects())
	{
		// Skip those without scemantic meaning
		if (not obj->isScemantic())
			continue;

		// Got another sub-file reference, inline it if we're deep-inlining. If not,
		// just add it into the objects normally. Yay, recursion!
		if (deep == true && obj->type() == LDObject::ESubfile)
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			LDObjectList otherobjs = ref->inlineContents (deep, renderinline);

			for (LDObject* otherobj : otherobjs)
				objs << otherobj;
		}
		else
			objs << obj->createCopy();
	}

	return objs;
}

// =============================================================================
//
LDDocument* LDDocument::current()
{
	return m_curdoc;
}

// =============================================================================
// Sets the given file as the current one on display. At some point in time this
// was an operation completely unheard of. ;)
//
// TODO: f can be temporarily null. This probably should not be the case.
// =============================================================================
void LDDocument::setCurrent (LDDocument* f)
{
	// Implicit files were loaded for caching purposes and must never be set
	// current.
	if (f && f->isImplicit())
		return;

	m_curdoc = f;

	if (g_win && f)
	{
		// A ton of stuff needs to be updated
		g_win->updateDocumentListItem (f);
		g_win->buildObjList();
		g_win->updateTitle();
		g_win->R()->setDocument (f);
		g_win->R()->compiler()->needMerge();
		print ("Changed file to %1", f->getDisplayName());
	}
}

// =============================================================================
//
int LDDocument::countExplicitFiles()
{
	int count = 0;

	for (LDDocument* f : g_loadedFiles)
	{
		if (not f->isImplicit())
			++count;
	}

	return count;
}

// =============================================================================
// This little beauty closes the initial file that was open at first when opening
// a new file over it.
// =============================================================================
void LDDocument::closeInitialFile()
{
	if (countExplicitFiles() == 2 &&
		g_loadedFiles[0]->name().isEmpty() &&
		not g_loadedFiles[1]->name().isEmpty() &&
		not g_loadedFiles[0]->hasUnsavedChanges())
	{
		delete g_loadedFiles[0];
	}
}

// =============================================================================
//
void loadLogoedStuds()
{
	if (g_logoedStud && g_logoedStud2)
		return;

	delete g_logoedStud;
	delete g_logoedStud2;

	g_logoedStud = openDocument ("stud-logo.dat", true, true);
	g_logoedStud2 = openDocument ("stud2-logo.dat", true, true);

	print (LDDocument::tr ("Logoed studs loaded.\n"));
}

// =============================================================================
//
void LDDocument::addToSelection (LDObject* obj) // [protected]
{
	if (obj->isSelected())
		return;

	assert (obj->document() == this);
	m_sel << obj;
	g_win->R()->compileObject (obj);
	obj->setSelected (true);
}

// =============================================================================
//
void LDDocument::removeFromSelection (LDObject* obj) // [protected]
{
	if (not obj->isSelected())
		return;

	assert (obj->document() == this);
	m_sel.removeOne (obj);
	g_win->R()->compileObject (obj);
	obj->setSelected (false);
}

// =============================================================================
//
void LDDocument::clearSelection()
{
	for (LDObject* obj : m_sel)
		removeFromSelection (obj);

	assert (m_sel.isEmpty());
}

// =============================================================================
//
const LDObjectList& LDDocument::getSelection() const
{
	return m_sel;
}

// =============================================================================
//
void LDDocument::swapObjects (LDObject* one, LDObject* other)
{
	int a = m_objects.indexOf (one);
	int b = m_objects.indexOf (other);
	assert (a != b && a != -1 && b != -1);
	m_objects[b] = one;
	m_objects[a] = other;
	addToHistory (new SwapHistory (one->id(), other->id()));
}

// =============================================================================
//
String LDDocument::shortenName (String a) // [static]
{
	String shortname = basename (a);
	String topdirname = basename (dirname (a));

	if (g_specialSubdirectories.contains (topdirname))
		shortname.prepend (topdirname + "\\");

	return shortname;
}

// =============================================================================
//
void LDDocument::addReference (LDDocumentPointer* ptr)
{
	m_references << ptr;
}

// =============================================================================
//
void LDDocument::removeReference (LDDocumentPointer* ptr)
{
	m_references.removeOne (ptr);

	if (references().isEmpty())
		invokeLater (closeUnused);
}

// =============================================================================
//
QList<Vertex> LDDocument::inlineVertices()
{
	initializeCachedData();
	return m_storedVertices;
}
