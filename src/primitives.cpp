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

#include <QDir>
#include <QThread>
#include <QRegExp>
#include <QFileDialog>
#include "file.h"
#include "gui.h"
#include "primitives.h"
#include "ui_makeprim.h"

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
		
		populateCategories ();
	}
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
		info.cat = null;
		
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
	g_primitives = m_prims;
	populateCategories ();
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
				} else
					continue;
				
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

// =============================================================================
bool primitiveLoaderBusy()
{
	return g_primListerMutex;
}

vector<LDObject*> makePrimitive( PrimitiveType type, int segs, int divs, int num )
{
	vector<LDObject*> objs;
	
	for( int i = 0; i < segs; ++i )
	{
		double x0 = cos(( i * 2 * pi ) / divs ),
			x1 = cos((( i + 1 ) * 2 * pi) / divs ),
			z0 = sin(( i * 2 * pi ) / divs ),
			z1 = sin((( i + 1 ) * 2 * pi ) / divs );
		
		LDObject* obj = null;
		
		switch( type )
		{
		case Circle:
			{
				vertex v0( x0, 0.0f, z0 ),
					v1( x1, 0.0f, z1 );
				
				LDLine* line = new LDLine;
				line->setVertex( 0, v0 );
				line->setVertex( 1, v1 );
				line->setColor( edgecolor );
				obj = line;
			}
			break;
		
		case Cylinder:
		case Ring:
		case Cone:
			{
				double x2, x3, z2, z3;
				double y0, y1, y2, y3;
				
				if( type == Cylinder )
				{
					x2 = x1;
					x3 = x0;
					z2 = z1;
					z3 = z0;
					
					y0 = y1 = 0.0f;
					y2 = y3 = 1.0f;
				} else {
					x2 = x1 * (num + 1);
					x3 = x0 * (num + 1);
					z2 = z1 * (num + 1);
					z3 = z0 * (num + 1);
					
					x0 *= num;
					x1 *= num;
					z0 *= num;
					z1 *= num;
					
					if( type == Ring )
						y0 = y1 = y2 = y3 = 0.0f;
					else
					{
						y0 = y1 = 1.0f;
						y2 = y3 = 0.0f;
					} 
				}
				
				vertex v0( x0, y0, z0 ),
					v1( x1, y1, z1 ),
					v2( x2, y2, z2 ),
					v3( x3, y3, z3 );
				
				LDQuad* quad = new LDQuad;
				quad->setColor( maincolor );
				quad->setVertex( 0, v0 );
				quad->setVertex( 1, v1 );
				quad->setVertex( 2, v2 );
				quad->setVertex( 3, v3 );
				obj = quad;
			}
			break;
		
		case Disc:
		case DiscNeg:
			{
				double x2, z2;
				
				if( type == Disc )
					x2 = z2 = 0.0f;
				else
				{
					x2 = ( x0 >= 0.0f ) ? 1.0f : -1.0f;
					z2 = ( z0 >= 0.0f ) ? 1.0f : -1.0f;
				}
				
				vertex v0( x0, 0.0f, z0 ),
					v1( x1, 0.0f, z1 ),
					v2( x2, 0.0f, z2 );
				
				LDTriangle* seg = new LDTriangle;
				seg->setColor( maincolor );
				seg->setVertex( 0, v0 );
				seg->setVertex( 1, v1 );
				seg->setVertex( 2, v2 );
				obj = seg;
			}
			break;
		
		default:
			break;
		}
		
		if( obj )
			objs << obj;
	}
	
	return objs;
}

str primitiveTypeName( PrimitiveType type )
{
	return type == Circle   ? "Circle" :
	       type == Cylinder ? "Cylinder" :
	       type == Disc     ? "Disc" :
	       type == DiscNeg  ? "Disc Negative" :
	       type == Ring     ? "Ring" :
	                          "Cone";
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void generatePrimitive()
{
	QDialog* dlg = new QDialog( g_win );
	Ui::MakePrimUI ui;
	ui.setupUi( dlg );
	
exec:
	if( !dlg->exec() )
		return;
	
	int segs = ui.sb_segs->value();
	int divs = ui.cb_hires->isChecked() ? hires : lores;
	int num = ui.sb_ringnum->value();
	PrimitiveType type =
		ui.rb_circle->isChecked()   ? Circle :
		ui.rb_cylinder->isChecked() ? Cylinder :
		ui.rb_disc->isChecked()     ? Disc :
		ui.rb_ndisc->isChecked()    ? DiscNeg :
		ui.rb_ring->isChecked()     ? Ring :
		                              Cone;
	
	// Make the description
	str descr = fmt ("%1 / %2 %3", segs, divs, primitiveTypeName( type ));
	
	if (type == Ring || type == Cone)
		descr += fmt (" %1", num);
	
	LDOpenFile* f = new LDOpenFile;
	
	*f << new LDComment( descr );
	*f << new LDComment( fmt( "Name: ???.dat" ));
	*f << new LDComment( fmt( "Author: LDForge" ));
	*f << new LDComment( fmt( "!LDRAW_ORG Unofficial_%1Primitive", divs == hires ? "48_" : "" ));
	*f << new LDComment( "Redistributable under CCAL version 2.0 : see CAreadme.txt" );
	*f << new LDEmpty;
	*f << new LDBFC( LDBFC::CertifyCCW );
	*f << new LDEmpty;
	*f << makePrimitive( type, segs, divs, num );
	
	g_win->save( f, true );
	delete f;
}