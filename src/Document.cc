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
#include "Main.h"
#include "Configuration.h"
#include "Document.h"
#include "Misc.h"
#include "MainWindow.h"
#include "EditHistory.h"
#include "Dialogs.h"
#include "GLRenderer.h"
#include "misc/InvokationDeferer.h"
#include "moc_Document.cxx"

cfg (String, io_ldpath, "");
cfg (List, io_recentfiles, {});
extern_cfg (String, net_downloadpath);
extern_cfg (Bool, gl_logostuds);

static bool g_loadingMainFile = false;
static const int g_maxRecentFiles = 10;
static bool g_aborted = false;
static LDDocumentPointer g_logoedStud = null;
static LDDocumentPointer g_logoedStud2 = null;

LDDocument* LDDocument::m_curdoc = null;

const QStringList g_specialSubdirectories ({ "s", "48", "8" });

// =============================================================================
// -----------------------------------------------------------------------------
namespace LDPaths
{
	static QString pathError;

	struct
	{
		QString LDConfigPath;
		QString partsPath, primsPath;
	} pathInfo;

	void initPaths()
	{
		if (!tryConfigure (io_ldpath))
		{
			LDrawPathDialog dlg (false);

			if (!dlg.exec())
				exit (0);

			io_ldpath = dlg.filename();
		}
	}

	bool tryConfigure (QString path)
	{
		QDir dir;

		if (!dir.cd (path))
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

		pathInfo.partsPath = fmt ("%1" DIRSLASH "parts", path);
		pathInfo.LDConfigPath = fmt ("%1" DIRSLASH "LDConfig.ldr", path);
		pathInfo.primsPath = fmt ("%1" DIRSLASH "p", path);

		return true;
	}

	// Accessors
	QString getError()
	{
		return pathError;
	}

	QString ldconfig()
	{
		return pathInfo.LDConfigPath;
	}

	QString prims()
	{
		return pathInfo.primsPath;
	}

	QString parts()
	{
		return pathInfo.partsPath;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
LDDocument::LDDocument() :
	m_gldata (new LDGLData)
{
	setImplicit (true);
	setSavePosition (-1);
	setListItem (null);
	setHistory (new History);
	m_History->setDocument (this);
}

// =============================================================================
// -----------------------------------------------------------------------------
LDDocument::~LDDocument()
{
	// Remove this file from the list of files. This MUST be done FIRST, otherwise
	// a ton of other functions will think this file is still valid when it is not!
	g_loadedFiles.removeOne (this);

	m_History->setIgnoring (true);

	// Clear everything from the model
	for (LDObject* obj : getObjects())
		obj->deleteSelf();

	delete m_History;
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
			if (!file->isImplicit())
			{
				LDDocument::setCurrent (file);
				found = true;
				break;
			}
		}

		if (!found)
			newFile();
	}

	if (this == g_logoedStud)
		g_logoedStud = null;
	elif (this == g_logoedStud2)
		g_logoedStud2 = null;

	g_win->updateDocumentList();
	log ("Closed %1", getName());
}

// =============================================================================
// -----------------------------------------------------------------------------
LDDocument* findDocument (QString name)
{
	for (LDDocument * file : g_loadedFiles)
		if (!file->getName().isEmpty() && file->getName() == name)
			return file;

	return null;
}

// =============================================================================
// -----------------------------------------------------------------------------
QString dirname (QString path)
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
// -----------------------------------------------------------------------------
QString basename (QString path)
{
	long lastpos = path.lastIndexOf (DIRSLASH);

	if (lastpos != -1)
		return path.mid (lastpos + 1);

	return path;
}

// =============================================================================
// -----------------------------------------------------------------------------
static QString findLDrawFilePath (QString relpath, bool subdirs)
{
	QString fullPath;

	// LDraw models use Windows-style path separators. If we're not on Windows,
	// replace the path separator now before opening any files. Qt expects
	// forward-slashes as directory separators.
#ifndef WIN32
	relpath.replace ("\\", "/");
#endif // WIN32

	// Try find it relative to other currently open documents. We want a file
	// in the immediate vicinity of a current model to override stock LDraw stuff.
	QString reltop = basename (dirname (relpath));

	for (LDDocument* doc : g_loadedFiles)
	{
		if (doc->getFullPath().isEmpty())
			continue;

		QString partpath = fmt ("%1/%2", dirname (doc->getFullPath()), relpath);
		QFile f (partpath);

		if (f.exists())
		{
			// ensure we don't mix subfiles and 48-primitives with non-subfiles and non-48
			QString proptop = basename (dirname (partpath));

			bool bogus = false;

			for (QString s : g_specialSubdirectories)
			{
				if ((proptop == s && reltop != s) || (reltop == s && proptop != s))
				{
					bogus = true;
					break;
				}
			}

			if (!bogus)
				return partpath;
		}
	}

	if (QFile::exists (relpath))
		return relpath;

	// Try with just the LDraw path first
	fullPath = fmt ("%1" DIRSLASH "%2", io_ldpath, relpath);

	if (QFile::exists (fullPath))
		return fullPath;

	if (subdirs)
	{
		// Look in sub-directories: parts and p. Also look in net_downloadpath, since that's
		// where we download parts from the PT to.
		for (const QString& topdir : initlist<QString> ({ io_ldpath, net_downloadpath }))
		{
			for (const QString& subdir : initlist<QString> ({ "parts", "p" }))
			{
				fullPath = fmt ("%1" DIRSLASH "%2" DIRSLASH "%3", topdir, subdir, relpath);

				if (QFile::exists (fullPath))
					return fullPath;
			}
		}
	}

	// Did not find the file.
	return "";
}

QFile* openLDrawFile (QString relpath, bool subdirs, QString* pathpointer)
{
	log ("Opening %1...\n", relpath);
	QString path = findLDrawFilePath (relpath, subdirs);

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
// -----------------------------------------------------------------------------
void LDFileLoader::start()
{
	setDone (false);
	setProgress (0);
	setAborted (false);

	if (isOnForeground())
	{
		g_aborted = false;

		// Show a progress dialog if we're loading the main Document.here so we can
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
{
	// User wishes to abort, so stop here now.
	if (isAborted())
	{
		for (LDObject* obj : m_Objects)
			obj->deleteSelf();

		m_Objects.clear();
		setDone (true);
		return;
	}

	// Parse up to 300 lines per iteration
	int max = i + 300;

	for (; i < max && i < (int) getLines().size(); ++i)
	{
		QString line = getLines()[i];

		// Trim the trailing newline
		QChar c;

		while (line.endsWith ("\n") || line.endsWith ("\r"))
			line.chop (1);

		LDObject* obj = parseLine (line);

		// Check for parse errors and warn about tthem
		if (obj->getType() == LDObject::EError)
		{
			log ("Couldn't parse line #%1: %2", getProgress() + 1, static_cast<LDError*> (obj)->reason);

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
	{
		emit workDone();
		setDone (true);
		return;
	}

	// Otherwise, continue, by recursing back.
	if (!isDone())
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
// -----------------------------------------------------------------------------
void LDFileLoader::abort()
{
	setAborted (true);

	if (isOnForeground())
		g_aborted = true;
}

// =============================================================================
// -----------------------------------------------------------------------------
LDObjectList loadFileContents (QFile* fp, int* numWarnings, bool* ok)
{
	QStringList lines;
	LDObjectList objs;

	if (numWarnings)
		*numWarnings = 0;

	// Read in the lines
	while (fp->atEnd() == false)
		lines << QString::fromUtf8 (fp->readLine());

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
LDDocument* openDocument (QString path, bool search)
{
	// Convert the file name to lowercase since some parts contain uppercase
	// file names. I'll assume here that the library will always use lowercase
	// file names for the actual parts..
	QFile* fp;
	QString fullpath;

	if (search)
		fp = openLDrawFile (path.toLower(), true, &fullpath);
	else
	{
		fp = new QFile (path);
		fullpath = path;

		if (!fp->open (QIODevice::ReadOnly))
		{
			delete fp;
			return null;
		}
	}

	if (!fp)
		return null;

	LDDocument* load = new LDDocument;
	load->setFullPath (fullpath);
	load->setName (LDDocument::shortenName (load->getFullPath()));
	dlog ("name: %1 (%2)", load->getName(), load->getFullPath());
	g_loadedFiles << load;

	// Don't take the file loading as actual edits to the file
	load->getHistory()->setIgnoring (true);

	int numWarnings;
	bool ok;
	LDObjectList objs = loadFileContents (fp, &numWarnings, &ok);
	fp->close();
	fp->deleteLater();

	if (!ok)
	{
		g_loadedFiles.removeOne (load);
		delete load;
		return null;
	}

	load->addObjects (objs);

	if (g_loadingMainFile)
	{
		LDDocument::setCurrent (load);
		g_win->R()->setFile (load);
		log (QObject::tr ("File %1 parsed successfully (%2 errors)."), path, numWarnings);
	}

	load->getHistory()->setIgnoring (false);
	return load;
}

// =============================================================================
// -----------------------------------------------------------------------------
bool LDDocument::isSafeToClose()
{
	typedef QMessageBox msgbox;
	setlocale (LC_ALL, "C");

	// If we have unsaved changes, warn and give the option of saving.
	if (hasUnsavedChanges())
	{
		QString message = fmt (tr ("There are unsaved changes to %1. Should it be saved?"),
			(getName().length() > 0) ? getName() : tr ("<anonymous>"));

		int button = msgbox::question (g_win, tr ("Unsaved Changes"), message,
			(msgbox::Yes | msgbox::No | msgbox::Cancel), msgbox::Cancel);

		switch (button)
		{
			case msgbox::Yes:
			{
				// If we don't have a file path yet, we have to ask the user for one.
				if (getName().length() == 0)
				{
					QString newpath = QFileDialog::getSaveFileName (g_win, tr ("Save As"),
						getCurrentDocument()->getName(), tr ("LDraw files (*.dat *.ldr)"));

					if (newpath.length() == 0)
						return false;

					setName (newpath);
				}

				if (!save())
				{
					message = fmt (tr ("Failed to save %1 (%2)\nDo you still want to close?"),
						getName(), strerror (errno));

					if (msgbox::critical (g_win, tr ("Save Failure"), message,
						(msgbox::Yes | msgbox::No), msgbox::No) == msgbox::No)
					{
						return false;
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
{
	// Remove all loaded files and the objects they contain
	QList<LDDocument*> files = g_loadedFiles;

	for (LDDocument* file : files)
		delete file;
}

// =============================================================================
// -----------------------------------------------------------------------------
void newFile()
{
	// Create a new anonymous file and set it to our current
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
void addRecentFile (QString path)
{
	auto& rfiles = io_recentfiles;
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
// -----------------------------------------------------------------------------
void openMainFile (QString path)
{
	g_loadingMainFile = true;

	// If there's already a file with the same name, this file must replace it.
	LDDocument* documentToReplace = null;
	QString shortName = LDDocument::shortenName (path);

	for (LDDocument* doc : g_loadedFiles)
	{
		if (doc->getName() == shortName)
		{
			documentToReplace = doc;
			break;
		}
	}

	// We cannot open this file if the document this would replace is not
	// safe to close.
	if (documentToReplace != null && documentToReplace->isSafeToClose() == false)
	{
		g_loadingMainFile = false;
		return;
	}

	LDDocument* file = openDocument (path, false);

	if (!file)
	{
		// Loading failed, thus drop down to a new file since we
		// closed everything prior.
		newFile();

		if (!g_aborted)
		{
			// Tell the user loading failed.
			setlocale (LC_ALL, "C");
			critical (fmt (QObject::tr ("Failed to open %1: %2"), path, strerror (errno)));
		}

		g_loadingMainFile = false;
		return;
	}

	file->setImplicit (false);

	// Replace references to the old file with the new file.
	if (documentToReplace != null)
	{
		for (LDDocumentPointer* ptr : documentToReplace->getReferences())
		{	dlog ("ptr: %1 (%2)\n",
				ptr, ptr->getPointer() ? ptr->getPointer()->getName() : "<null>");

			ptr->operator= (file);
		}

		assert (documentToReplace->countReferences() == 0);
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
// -----------------------------------------------------------------------------
bool LDDocument::save (QString savepath)
{
	if (!savepath.length())
		savepath = getFullPath();

	QFile f (savepath);

	if (!f.open (QIODevice::WriteOnly))
		return false;

	// If the second object in the list holds the file name, update that now.
	// Only do this if the file is explicitly open.
	LDObject* nameObject = getObject (1);

	if (!isImplicit() && nameObject != null && nameObject->getType() == LDObject::EComment)
	{
		LDComment* nameComment = static_cast<LDComment*> (nameObject);

		if (nameComment->text.left (6) == "Name: ")
		{
			QString newname = shortenName (savepath);
			nameComment->text = fmt ("Name: %1", newname);
			g_win->buildObjList();
		}
	}

	// File is open, now save the model to it. Note that LDraw requires files to
	// have DOS line endings, so we terminate the lines with \r\n.
	for (LDObject* obj : getObjects())
		f.write ((obj->raw() + "\r\n").toUtf8());

	// File is saved, now clean up.
	f.close();

	// We have successfully saved, update the save position now.
	setSavePosition (getHistory()->getPosition());
	setFullPath (savepath);
	setName (shortenName (savepath));

	g_win->updateDocumentListItem (this);
	g_win->updateTitle();
	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
class LDParseError : public std::exception
{
	PROPERTY (private, QString,	Error,	STR_OPS, STOCK_WRITE)
	PROPERTY (private, QString,	Line,		STR_OPS,	STOCK_WRITE)

	public:
		LDParseError (QString line, QString a) : m_Error (a), m_Line (line) {}

		const char* what() const throw()
		{
			return getError().toLocal8Bit().constData();
		}
};

// =============================================================================
// -----------------------------------------------------------------------------
void checkTokenCount (QString line, const QStringList& tokens, int num)
{
	if (tokens.size() != num)
		throw LDParseError (line, fmt ("Bad amount of tokens, expected %1, got %2", num, tokens.size()));
}

// =============================================================================
// -----------------------------------------------------------------------------
void checkTokenNumbers (QString line, const QStringList& tokens, int min, int max)
{
	bool ok;

	// Check scientific notation, e.g. 7.99361e-15
	QRegExp scient ("\\-?[0-9]+\\.[0-9]+e\\-[0-9]+");

	for (int i = min; i <= max; ++i)
	{
		tokens[i].toDouble (&ok);

		if (!ok && !scient.exactMatch (tokens[i]))
			throw LDParseError (line, fmt ("Token #%1 was `%2`, expected a number (matched length: %3)", (i + 1), tokens[i], scient.matchedLength()));
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
static Vertex parseVertex (QStringList& s, const int n)
{
	Vertex v;

	for_axes (ax)
		v[ax] = s[n + ax].toDouble();

	return v;
}

// =============================================================================
// This is the LDraw code parser function. It takes in a string containing LDraw
// code and returns the object parsed from it. parseLine never returns null,
// the object will be LDError if it could not be parsed properly.
// -----------------------------------------------------------------------------
LDObject* parseLine (QString line)
{
	try
	{
		QStringList tokens = line.split (" ", QString::SkipEmptyParts);

		if (tokens.size() <= 0)
		{
			// Line was empty, or only consisted of whitespace
			return new LDEmpty;
		}

		if (tokens[0].length() != 1 || tokens[0][0].isDigit() == false)
			throw LDParseError (line, "Illogical line code");

		int num = tokens[0][0].digitValue();

		switch (num)
		{
			case 0:
			{
				// Comment
				QString comm = line.mid (line.indexOf ("0") + 1).simplified();

				// Handle BFC statements
				if (tokens.size() > 2 && tokens[1] == "BFC")
				{
					for (int i = 0; i < LDBFC::NumStatements; ++i)
						if (comm == fmt ("BFC %1", LDBFC::statements [i]))
							return new LDBFC ( (LDBFC::Type) i);

					// MLCAD is notorious for stuffing these statements in parts it
					// creates. The above block only handles valid statements, so we
					// need to handle MLCAD-style invertnext, clip and noclip separately.
					struct
					{
						QString			a;
						LDBFC::Type	b;
					} BFCData[] =
					{
						{ "INVERTNEXT", LDBFC::InvertNext },
						{ "NOCLIP", LDBFC::NoClip },
						{ "CLIP", LDBFC::Clip }
					};

					for (const auto& i : BFCData)
						if (comm == "BFC CERTIFY " + i.a)
							return new LDBFC (i.b);
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

						for_axes (ax)
							obj->pos[ax] = tokens[4 + ax].toDouble(); // 4 - 6

						return obj;
					} elif (tokens[2] == "OVERLAY")
					{
						checkTokenCount (line, tokens, 9);;
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
				obj->text = comm;
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
				if (!load)
				{
					LDError* obj = new LDError (line, fmt ("Could not open %1", tokens[14]));
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

			default: // Strange line we couldn't parse
				throw LDError (line, "Unknown line code number");
		}
	}
	catch (LDParseError& e)
	{
		return new LDError (e.getLine(), e.getError());
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
LDDocument* getDocument (QString filename)
{
	// Try find the file in the list of loaded files
	LDDocument* doc = findDocument (filename);

	// If it's not loaded, try open it
	if (!doc)
		doc = openDocument (filename, true);

	return doc;
}

// =============================================================================
// -----------------------------------------------------------------------------
void reloadAllSubfiles()
{
	if (!getCurrentDocument())
		return;

	g_loadedFiles.clear();
	g_loadedFiles << getCurrentDocument();

	// Go through all objects in the current file and reload the subfiles
	for (LDObject* obj : getCurrentDocument()->getObjects())
	{
		if (obj->getType() == LDObject::ESubfile)
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			LDDocument* fileInfo = getDocument (ref->getFileInfo()->getName());

			if (fileInfo)
				ref->setFileInfo (fileInfo);
			else
				ref->replace (new LDError (ref->raw(), fmt ("Could not open %1", ref->getFileInfo()->getName())));
		}

		// Reparse gibberish files. It could be that they are invalid because
		// of loading errors. Circumstances may be different now.
		if (obj->getType() == LDObject::EError)
			obj->replace (parseLine (static_cast<LDError*> (obj)->contents));
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
int LDDocument::addObject (LDObject* obj)
{
	getHistory()->add (new AddHistory (getObjects().size(), obj));
	m_Objects << obj;

	if (obj->getType() == LDObject::EVertex)
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
void LDDocument::addObjects (const LDObjectList objs)
{
	for (LDObject* obj : objs)
		if (obj)
			addObject (obj);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::insertObj (int pos, LDObject* obj)
{
	getHistory()->add (new AddHistory (pos, obj));
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
{
	int idx = obj->getIndex();
	obj->unselect();
	assert (m_Objects[idx] == obj);

	if (!getHistory()->isIgnoring())
		getHistory()->add (new DelHistory (idx, obj));

	m_Objects.removeAt (idx);
	obj->setFile (null);
}

// =============================================================================
// -----------------------------------------------------------------------------
bool safeToCloseAll()
{
	for (LDDocument* f : g_loadedFiles)
		if (!f->isSafeToClose())
			return false;

	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::setObject (int idx, LDObject* obj)
{
	assert (idx >= 0 && idx < m_Objects.size());

	// Mark this change to history
	if (!m_History->isIgnoring())
	{
		QString oldcode = getObject (idx)->raw();
		QString newcode = obj->raw();
		*m_History << new EditHistory (idx, oldcode, newcode);
	}

	m_Objects[idx]->unselect();
	m_Objects[idx]->setFile (null);
	obj->setFile (this);
	m_Objects[idx] = obj;
}

// =============================================================================
// Close all implicit files with no references
// -----------------------------------------------------------------------------
void LDDocument::closeUnused()
{
	for (LDDocument* file : g_loadedFiles)
		if (file->isImplicit() && file->countReferences() == 0)
			delete file;
}

// =============================================================================
// -----------------------------------------------------------------------------
LDObject* LDDocument::getObject (int pos) const
{
	if (m_Objects.size() <= pos)
		return null;

	return m_Objects[pos];
}

// =============================================================================
// -----------------------------------------------------------------------------
int LDDocument::getObjectCount() const
{
	return getObjects().size();
}

// =============================================================================
// -----------------------------------------------------------------------------
bool LDDocument::hasUnsavedChanges() const
{
	return !isImplicit() && getHistory()->getPosition() != getSavePosition();
}

// =============================================================================
// -----------------------------------------------------------------------------
QString LDDocument::getDisplayName()
{
	if (!getName().isEmpty())
		return getName();

	if (!getDefaultName().isEmpty())
		return "[" + getDefaultName() + "]";

	return tr ("<anonymous>");
}

// =============================================================================
//
void LDDocument::initializeGLData()
{
	log (getDisplayName() + ": Initializing GL data");
	LDObjectList objs = inlineContents (true, true);

	for (LDObject* obj : objs)
	{
		assert (obj->getType() != LDObject::ESubfile);
		LDPolygon* data = obj->getPolygon();

		if (data != null)
		{
			m_PolygonData << *data;
			delete data;
		}

		obj->deleteSelf();
	}

	m_needsGLReInit = false;
}

// =============================================================================
//
QList<LDPolygon> LDDocument::inlinePolygons()
{
	if (m_needsGLReInit == true)
		initializeGLData();

	return m_PolygonData;
}

// =============================================================================
// -----------------------------------------------------------------------------
LDObjectList LDDocument::inlineContents (bool deep, bool renderinline)
{
	// Possibly substitute with logoed studs:
	// stud.dat -> stud-logo.dat
	// stud2.dat -> stud-logo2.dat
	if (gl_logostuds && renderinline)
	{
		// Ensure logoed studs are loaded first
		loadLogoedStuds();

		if (getName() == "stud.dat" && g_logoedStud != null)
			return g_logoedStud->inlineContents (deep, renderinline);
		elif (getName() == "stud2.dat" && g_logoedStud2 != null)
			return g_logoedStud2->inlineContents (deep, renderinline);
	}

	LDObjectList objs, objcache;

	for (LDObject* obj : getObjects())
	{
		// Skip those without scemantic meaning
		if (!obj->isScemantic())
			continue;

		// Got another sub-file reference, inline it if we're deep-inlining. If not,
		// just add it into the objects normally. Yay, recursion!
		if (deep == true && obj->getType() == LDObject::ESubfile)
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
// -----------------------------------------------------------------------------
LDDocument* LDDocument::current()
{
	return m_curdoc;
}

// =============================================================================
// Sets the given file as the current one on display. At some point in time this
// was an operation completely unheard of. ;)
//
// TODO: f can be temporarily null. This probably should not be the case.
// -----------------------------------------------------------------------------
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
		g_win->R()->setFile (f);
		g_win->R()->repaint();
		log ("Changed file to %1", f->getDisplayName());
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
int LDDocument::countExplicitFiles()
{
	int count = 0;

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
{
	if (
		countExplicitFiles() == 2 &&
		g_loadedFiles[0]->getName().isEmpty() &&
		g_loadedFiles[1]->getName().isEmpty() == false &&
		!g_loadedFiles[0]->hasUnsavedChanges()
	)
		delete g_loadedFiles[0];
}

// =============================================================================
// -----------------------------------------------------------------------------
void loadLogoedStuds()
{
	if (g_logoedStud && g_logoedStud2)
		return;

	delete g_logoedStud;
	delete g_logoedStud2;

	g_logoedStud = openDocument ("stud-logo.dat", true);
	g_logoedStud2 = openDocument ("stud2-logo.dat", true);

	log (LDDocument::tr ("Logoed studs loaded.\n"));
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::addToSelection (LDObject* obj) // [protected]
{
	if (obj->isSelected())
		return;

	assert (obj->getFile() == this);
	m_sel << obj;
	obj->setSelected (true);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::removeFromSelection (LDObject* obj) // [protected]
{
	if (!obj->isSelected())
		return;

	assert (obj->getFile() == this);
	m_sel.removeOne (obj);
	obj->setSelected (false);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::clearSelection()
{
	for (LDObject* obj : m_sel)
		removeFromSelection (obj);

	assert (m_sel.isEmpty());
}

// =============================================================================
// -----------------------------------------------------------------------------
const LDObjectList& LDDocument::getSelection() const
{
	return m_sel;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::swapObjects (LDObject* one, LDObject* other)
{
	int a = m_Objects.indexOf (one);
	int b = m_Objects.indexOf (other);
	assert (a != b && a != -1 && b != -1);
	m_Objects[b] = one;
	m_Objects[a] = other;
	addToHistory (new SwapHistory (one->getID(), other->getID()));
}

// =============================================================================
// -----------------------------------------------------------------------------
QString LDDocument::shortenName (QString a) // [static]
{
	QString shortname = basename (a);
	QString topdirname = basename (dirname (a));

	if (g_specialSubdirectories.contains (topdirname))
		shortname.prepend (topdirname + "\\");

	return shortname;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::addReference (LDDocumentPointer* ptr)
{
	pushToReferences (ptr);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocument::removeReference (LDDocumentPointer* ptr)
{
	removeFromReferences (ptr);

	if (getReferences().size() == 0)
		invokeLater (closeUnused);
}