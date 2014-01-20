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

#ifndef LDFORGE_DOCUMENT_H
#define LDFORGE_DOCUMENT_H

#include <QObject>
#include "main.h"
#include "ldtypes.h"
#include "history.h"

class History;
class OpenProgressDialog;
class LDDocumentPointer;
struct LDGLData;

namespace LDPaths
{
	void initPaths();
	bool tryConfigure (QString path);

	QString ldconfig();
	QString prims();
	QString parts();
	QString getError();
}

// =============================================================================
// LDDocument
//
// The LDDocument class stores a document opened in LDForge either as a editable
// file for the user or for subfile caching. Its methods handle file input and
// output.
//
// A file is implicit when they are opened automatically for caching purposes
// and are hidden from the user. User-opened files are explicit (not implicit).
//
// The default name is a placeholder, initially suggested name for a file. The
// primitive generator uses this to give initial names to primitives.
// =============================================================================
class LDDocument : public QObject
{
	properties:
		Q_OBJECT
		PROPERTY (private,	LDObjectList,			Objects, 		LIST_OPS,	STOCK_WRITE)
		PROPERTY (private,	History*,					History,		NO_OPS,		STOCK_WRITE)
		PROPERTY (private,	LDObjectList,			Vertices,		LIST_OPS,	STOCK_WRITE)
		PROPERTY (private,	QList<LDDocumentPointer*>,	References,		LIST_OPS,	STOCK_WRITE)
		PROPERTY (public,	QString,					Name,			STR_OPS,	STOCK_WRITE)
		PROPERTY (public,	QString,					FullPath,		STR_OPS,	STOCK_WRITE)
		PROPERTY (public,	QString,					DefaultName,	STR_OPS,	STOCK_WRITE)
		PROPERTY (public,	bool,						Implicit,		BOOL_OPS,	STOCK_WRITE)
		PROPERTY (public,	LDObjectList,			Cache,			LIST_OPS,	STOCK_WRITE)
		PROPERTY (public,	long,						SavePosition,	NUM_OPS,	STOCK_WRITE)
		PROPERTY (public,	QListWidgetItem*,			ListItem,		NO_OPS,		STOCK_WRITE)

	public:
		LDDocument();
		~LDDocument();

		int addObject (LDObject* obj); // Adds an object to this file at the end of the file.
		void addObjects (const LDObjectList objs);
		void clearSelection();
		void forgetObject (LDObject* obj); // Deletes the given object from the object chain.
		QString getDisplayName();
		const LDObjectList& getSelection() const;
		bool hasUnsavedChanges() const; // Does this document.have unsaved changes?
		LDObjectList inlineContents (LDSubfile::InlineFlags flags);
		void insertObj (int pos, LDObject* obj);
		int getObjectCount() const;
		LDObject* getObject (int pos) const;
		bool save (QString path = ""); // Saves this file to disk.
		void swapObjects (LDObject* one, LDObject* other);
		bool isSafeToClose(); // Perform safety checks. Do this before closing any files!
		void setObject (int idx, LDObject* obj);
		void addReference (LDDocumentPointer* ptr);
		void removeReference (LDDocumentPointer* ptr);

		inline LDDocument& operator<< (LDObject* obj)
		{
			addObject (obj);
			return *this;
		}

		inline void addHistoryStep()
		{
			m_History->addStep();
		}

		inline void undo()
		{
			m_History->undo();
		}

		inline void redo()
		{
			m_History->redo();
		}

		inline void clearHistory()
		{
			m_History->clear();
		}

		inline void addToHistory (AbstractHistoryEntry* entry)
		{
			*m_History << entry;
		}

		static void closeUnused();
		static LDDocument* current();
		static void setCurrent (LDDocument* f);
		static void closeInitialFile();
		static int countExplicitFiles();

		// Turns a full path into a relative path
		static QString shortenName (QString a);

	protected:
		void addToSelection (LDObject* obj);
		void removeFromSelection (LDObject* obj);

		LDGLData* getGLData()
		{
			return m_gldata;
		}

		friend class LDObject;
		friend class GLRenderer;

	private:
		LDObjectList			m_sel;
		LDGLData*				m_gldata;

		// If set to true, next inline of this document discards the cache and
		// re-builds it.
		bool					m_needsCache;

		static LDDocument*		m_curdoc;
};

inline LDDocument* getCurrentDocument()
{
	return LDDocument::current();
}

// Close all current loaded files and start off blank.
void newFile();

// Opens the given file as the main file. Everything is closed first.
void openMainFile (QString path);

// Finds an OpenFile by name or null if not open
LDDocument* findDocument (QString name);

// Opens the given file and parses the LDraw code within. Returns a pointer
// to the opened file or null on error.
LDDocument* openDocument (QString path, bool search);

// Opens the given file and returns a pointer to it, potentially looking in /parts and /p
QFile* openLDrawFile (QString relpath, bool subdirs, QString* pathpointer = null);

// Close all open files, whether user-opened or subfile caches.
void closeAll();

// Parses a string line containing an LDraw object and returns the object parsed.
LDObject* parseLine (QString line);

// Retrieves the pointer to the given document by file name. Document is loaded
// from file if necessary. Can return null if neither succeeds.
LDDocument* getDocument (QString filename);

// Re-caches all subfiles.
void reloadAllSubfiles();

// Is it safe to close all files?
bool safeToCloseAll();

LDObjectList loadFileContents (QFile* f, int* numWarnings, bool* ok = null);

extern QList<LDDocument*> g_loadedFiles;

inline const LDObjectList& selection()
{
	return getCurrentDocument()->getSelection();
}

void addRecentFile (QString path);
void loadLogoedStuds();
QString basename (QString path);
QString dirname (QString path);

extern QList<LDDocument*> g_loadedFiles; // Vector of all currently opened files.

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// FileLoader
//
// Loads the given file and parses it to LDObjects using parseLine. It's a
// separate class so as to be able to do the work progressively through the
// event loop, allowing the program to maintain responsivity during loading.
// =============================================================================
class LDFileLoader : public QObject
{
	Q_OBJECT
	PROPERTY (private,	LDObjectList,	Objects,			NO_OPS,		STOCK_WRITE)
	PROPERTY (private,	bool,					Done,				BOOL_OPS,	STOCK_WRITE)
	PROPERTY (private,	int,					Progress,		NUM_OPS,		STOCK_WRITE)
	PROPERTY (private,	bool,					Aborted,			BOOL_OPS,	STOCK_WRITE)
	PROPERTY (public,		QStringList,		Lines,			NO_OPS,		STOCK_WRITE)
	PROPERTY (public,		int*,					Warnings,		NO_OPS,		STOCK_WRITE)
	PROPERTY (public,		bool,					OnForeground,	BOOL_OPS,	STOCK_WRITE)

	public slots:
		void start();
		void abort();

	private:
		OpenProgressDialog* dlg;

	private slots:
		void work (int i);

	signals:
		void progressUpdate (int progress);
		void workDone();
};

#endif // LDFORGE_DOCUMENT_H
