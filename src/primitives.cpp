#include <QDir>
#include <QThread>
#include "file.h"
#include "gui.h"
#include "primitives.h"

PrimitiveLister* g_activePrimLister = null;
vector<Primitive> g_Primitives;

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
		
		m_prims << info;
		emit update (++i);
	}
	
	// Save to a config file
	File conf (config::dirpath () + "prims.cfg", File::Write);
	for (Primitive& info : m_prims)
		fprint (conf, "%1 %2\n", info.name, info.title);
	
	conf.close ();
	
	g_primListerMutex = true;
	g_Primitives = m_prims;
	g_primListerMutex = false;
	
	g_activePrimLister = null;
	emit workDone ();
}

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

void loadPrimitives () {
	g_Primitives.clear ();
	
	// Try to load prims.cfg
	File conf (config::dirpath () + "prims.cfg", File::Read);
	if (!conf) {
		// No prims.cfg, build it
		PrimitiveLister::start ();
		return;
	}
	
	for (str line : conf) {
		int space = line.indexOf (" ");
		if (space == -1)
			continue;
		
		Primitive info;
		info.name = line.left (space);
		info.title = line.mid (space + 1);
		g_Primitives << info;
	}
}