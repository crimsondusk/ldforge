#include <QDir>
#include <QThread>
#include <QRegExp>
#include "file.h"
#include "gui.h"
#include "primitives.h"

vector<PrimitiveCategory> g_PrimitiveCategories;
static PrimitiveLister* g_activePrimLister = null;
static bool g_primListerMutex = false;
vector<Primitive> g_primitives;

static void populateCategories ();
static void loadPrimitiveCatgories ();

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void loadPrimitives () {
	print ("Loading primitives...\n");
	
	loadPrimitiveCatgories ();
	
	// Try to load prims.cfg
	File conf (config::dirpath () + "prims.cfg", File::Read);
	if (!conf) {
		// No prims.cfg, build it
		PrimitiveLister::start ();
	} else {
		for (str line : conf) {
			int space = line.indexOf (" ");
			if (space == -1)
				continue;
			
			Primitive info;
			info.name = line.left (space);
			info.title = line.mid (space + 1);
			g_primitives << info;
		}
	}
	
	populateCategories ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void recursiveGetFilenames (QDir dir, vector<str>& fnames) {
	QFileInfoList flist = dir.entryInfoList ();
	for (const QFileInfo& info : flist) {
		if (info.fileName () == "." || info.fileName () == "..")
			continue; // skip . and ..
		
		if (info.isDir ())
			recursiveGetFilenames (QDir (info.absoluteFilePath ()), fnames);
		else
			fnames << info.absoluteFilePath ();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void PrimitiveLister::work () {
	g_activePrimLister = this;
	m_prims.clear ();
	
	QDir dir (LDPaths::prims ());
	assert (dir.exists ());
	
	ulong baselen = dir.absolutePath ().length ();
	
	vector<str> fnames;
	recursiveGetFilenames (dir, fnames);
	emit starting (fnames.size ());
	
	ulong i = 0;
	for (str fname : fnames) {
		File f (fname, File::Read);
		
		Primitive info;
		info.name = fname.mid (baselen + 1); // make full path relative
		info.name.replace ('/', '\\'); // use DOS backslashes, they're expected
		
		if (!f.readLine (info.title))
			info.title = "";
		
		info.title = info.title.simplified ();
		
		if (info.title[0] == '0') {
			info.title.remove (0, 1); // remove 0
			info.title = info.title.simplified ();
		}
		
		// Figure which category to use
		info.cat = null;
		
		m_prims << info;
		emit update (++i);
	}
	
	// Save to a config file
	File conf (config::dirpath () + "prims.cfg", File::Write);
	for (Primitive& info : m_prims)
		fprint (conf, "%1 %2\n", info.name, info.title);
	
	conf.close ();
	
	g_primListerMutex = true;
	g_primitives = m_prims;
	g_primListerMutex = false;
	g_activePrimLister = null;
	emit workDone ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void PrimitiveLister::start () {
	if (g_activePrimLister)
		return;
	
	PrimitiveLister* lister = new PrimitiveLister;
	QThread* listerThread = new QThread;
	lister->moveToThread (listerThread);
	connect (lister, SIGNAL (starting (ulong)), g_win, SLOT (primitiveLoaderStart (ulong)));
	connect (lister, SIGNAL (update (ulong)), g_win, SLOT (primitiveLoaderUpdate (ulong)));
	connect (lister, SIGNAL (workDone ()), g_win, SLOT (primitiveLoaderEnd ()));
	connect (listerThread, SIGNAL (started ()), lister, SLOT (work ()));
	connect (listerThread, SIGNAL (finished ()), lister, SLOT (deleteLater ()));
	listerThread->start ();
}

static PrimitiveCategory* findCategory (str name) {
	for (PrimitiveCategory& cat : g_PrimitiveCategories)
		if (cat.name () == name)
			return &cat;
	
	return null;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void populateCategories () {
	for (PrimitiveCategory& cat : g_PrimitiveCategories)
		cat.prims.clear ();
	
	PrimitiveCategory* unmatched = findCategory ("Other");
	
	if (!unmatched) {
		// Shouldn't happen.. but catch it anyway.
		print ("No `Other` category found! Creating one...\n");
		PrimitiveCategory cat;
		cat.setName ("Other");
		unmatched = &(g_PrimitiveCategories << cat);
	}
	
	for (Primitive& prim : g_primitives) {
		bool matched = false;
		
		// Go over the categories and their regexes, if and when there's a match,
		// the primitive's category is set to the category the regex beloings to.
		for (PrimitiveCategory& cat : g_PrimitiveCategories) {
			for (PrimitiveCategory::RegexEntry& entry : cat.regexes) {
				switch (entry.type) {
				case PrimitiveCategory::Filename:
					// f-regex, check against filename
					matched = entry.regex.exactMatch (prim.name);
					break;
					
				case PrimitiveCategory::Title:
					// t-regex, check against title
					matched = entry.regex.exactMatch (prim.title);
					break;
				}
				
				if (matched) {
					prim.cat = &cat;
					break;
				}
			}
			
			// Drop out if a category was decided on.
			if (prim.cat)
				break;
		}
		
		// If there was a match, add the primitive to the category.
		// Otherwise, add it to the list of unmatched primitives.
		if (prim.cat)
			prim.cat->prims << prim;
		else
			unmatched->prims << prim;
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void loadPrimitiveCatgories () {
	g_PrimitiveCategories.clear ();
	
	File f (config::dirpath () + "primregexps.cfg", File::Read);
	
	if (!f)
		f.open (":/data/primitive-categories.cfg", File::Read);
	
	if (!f)
		critical ("Failed to open primitive categories!");
	
	if (f) {
		PrimitiveCategory cat;
		
		for (str line : f) {
			int colon;
			
			if (line.length () == 0 || line[0] == '#')
				continue;
			
			if ((colon = line.indexOf (":")) == -1) {
				if (cat.regexes.size () > 0)
					g_PrimitiveCategories << cat;
				
				cat.regexes.clear ();
				cat.prims.clear ();
				cat.setName (line);
			} else {
				str cmd = line.left (colon);
				
				PrimitiveCategory::Type type = PrimitiveCategory::Filename;
				if (cmd == "f") {
					type = PrimitiveCategory::Filename;
				} else if (cmd == "t") {
					type = PrimitiveCategory::Title;
				}
				
				QRegExp regex (line.mid (colon + 1));
				PrimitiveCategory::RegexEntry entry = { regex, type };
				cat.regexes << entry;
			}
		}
		
		if (cat.regexes.size () > 0)
			g_PrimitiveCategories << cat;
	}
	
	// Add a category for unmatched primitives
	PrimitiveCategory cat;
	cat.setName ("Other");
	g_PrimitiveCategories << cat;
}