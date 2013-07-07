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
#include "misc.h"

vector<PrimitiveCategory> g_PrimitiveCategories;
static PrimitiveLister* g_activePrimLister = null;
static bool g_primListerMutex = false;
vector<Primitive> g_primitives;

static const str g_Other = QObject::tr( "Other" );

static void populateCategories();
static void loadPrimitiveCatgories();

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void loadPrimitives()
{
	print( "Loading primitives...\n" );
	
	loadPrimitiveCatgories();
	
	// Try to load prims.cfg
	File conf( config::dirpath() + "prims.cfg", File::Read );
	
	if( !conf )
	{
		// No prims.cfg, build it
		PrimitiveLister::start();
	}
	else
	{
		for( str line : conf )
		{
			int space = line.indexOf( " " );
			
			if( space == -1 )
				continue;
			
			Primitive info;
			info.name = line.left( space );
			info.title = line.mid( space + 1 );
			g_primitives << info;
		}
		
		populateCategories();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void recursiveGetFilenames( QDir dir, vector<str>& fnames )
{
	QFileInfoList flist = dir.entryInfoList();
	
	for( const QFileInfo & info : flist )
	{
		if( info.fileName() == "." || info.fileName() == ".." )
			continue; // skip . and ..
		
		if( info.isDir() )
			recursiveGetFilenames( QDir( info.absoluteFilePath() ), fnames );
		else
			fnames << info.absoluteFilePath();
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void PrimitiveLister::work()
{
	g_activePrimLister = this;
	m_prims.clear();
	
	QDir dir( LDPaths::prims() );
	ulong baselen = dir.absolutePath().length();
	ulong i = 0;
	vector<str> fnames;
	
	assert( dir.exists() );
	recursiveGetFilenames( dir, fnames );
	emit starting( fnames.size() );
	
	for( str fname : fnames )
	{
		File f( fname, File::Read );
		
		Primitive info;
		info.name = fname.mid( baselen + 1 ); // make full path relative
		info.name.replace( '/', '\\' ); // use DOS backslashes, they're expected
		info.cat = null;
		
		if( !f.readLine( info.title ))
			info.title = "";
		
		info.title = info.title.simplified();
		
		if( info.title[0] == '0' )
		{
			info.title.remove( 0, 1 ); // remove 0
			info.title = info.title.simplified();
		}
		
		m_prims << info;
		emit update( ++i );
	}
	
	// Save to a config file
	File conf( config::dirpath() + "prims.cfg", File::Write );
	
	for( Primitive & info : m_prims )
		fprint( conf, "%1 %2\n", info.name, info.title );
	
	conf.close();
	
	g_primListerMutex = true;
	g_primitives = m_prims;
	populateCategories();
	g_primListerMutex = false;
	g_activePrimLister = null;
	emit workDone();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void PrimitiveLister::start()
{
	if( g_activePrimLister )
		return;
	
	PrimitiveLister* lister = new PrimitiveLister;
	QThread* listerThread = new QThread;
	lister->moveToThread( listerThread );
	connect( lister, SIGNAL( starting( ulong )), g_win, SLOT( primitiveLoaderStart( ulong )) );
	connect( lister, SIGNAL( update( ulong )), g_win, SLOT( primitiveLoaderUpdate( ulong )) );
	connect( lister, SIGNAL( workDone() ), g_win, SLOT( primitiveLoaderEnd() ));
	connect( listerThread, SIGNAL( started() ), lister, SLOT( work() ));
	connect( listerThread, SIGNAL( finished() ), lister, SLOT( deleteLater() ));
	listerThread->start();
}

static PrimitiveCategory* findCategory( str name )
{
	for( PrimitiveCategory & cat : g_PrimitiveCategories )
		if( cat.name() == name )
			return &cat;
	
	return null;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void populateCategories()
{
	for( PrimitiveCategory & cat : g_PrimitiveCategories )
		cat.prims.clear();
	
	PrimitiveCategory* unmatched = findCategory( g_Other );
	
	if( !unmatched )
	{
		// Shouldn't happen.. but catch it anyway.
		PrimitiveCategory cat;
		cat.setName( g_Other );
		unmatched = &( g_PrimitiveCategories << cat );
	}
	
	for( Primitive & prim : g_primitives )
	{
		bool matched = false;
		
		// Go over the categories and their regexes, if and when there's a match,
		// the primitive's category is set to the category the regex beloings to.
		for( PrimitiveCategory & cat : g_PrimitiveCategories )
		{
			for( PrimitiveCategory::RegexEntry & entry : cat.regexes )
			{
				switch( entry.type )
				{
				case PrimitiveCategory::Filename:
					// f-regex, check against filename
					matched = entry.regex.exactMatch( prim.name );
					break;
				
				case PrimitiveCategory::Title:
					// t-regex, check against title
					matched = entry.regex.exactMatch( prim.title );
					break;
				}
				
				if( matched )
				{
					prim.cat = &cat;
					break;
				}
			}
			
			// Drop out if a category was decided on.
			if( prim.cat )
				break;
		}
		
		// If there was a match, add the primitive to the category.
		// Otherwise, add it to the list of unmatched primitives.
		if( prim.cat )
			prim.cat->prims << prim;
		else
			unmatched->prims << prim;
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static void loadPrimitiveCatgories()
{
	g_PrimitiveCategories.clear();
	
	File f( config::dirpath() + "primregexps.cfg", File::Read );
	
	if( !f )
		f.open( ":/data/primitive-categories.cfg", File::Read );
	
	if( !f )
		critical( QObject::tr( "Failed to open primitive categories!" ));
	
	if( f )
	{
		PrimitiveCategory cat;
	
		for( str line : f )
		{
			int colon;
			
			if( line.length() == 0 || line[0] == '#' )
				continue;
			
			if( ( colon = line.indexOf( ":" )) == -1 )
			{
				if( cat.regexes.size() > 0 )
					g_PrimitiveCategories << cat;
				
				cat.regexes.clear();
				cat.prims.clear();
				cat.setName( line );
			}
			else
			{
				str cmd = line.left( colon );
				
				PrimitiveCategory::Type type = PrimitiveCategory::Filename;
				
				if( cmd == "f" )
					type = PrimitiveCategory::Filename;
				elif( cmd == "t" )
					type = PrimitiveCategory::Title;
				else
					continue;
				
				QRegExp regex( line.mid( colon + 1 ));
				PrimitiveCategory::RegexEntry entry = { regex, type };
				cat.regexes << entry;
			}
		}
		
		if( cat.regexes.size() > 0 )
			g_PrimitiveCategories << cat;
	}
	
	// Add a category for unmatched primitives
	PrimitiveCategory cat;
	cat.setName( g_Other );
	g_PrimitiveCategories << cat;
}

// =============================================================================
bool primitiveLoaderBusy()
{
	return g_primListerMutex;
}

double radialPoint( int i, int divs, double ( *func )( double ))
{
	return ( *func )(( i * 2 * pi ) / divs );
}

vector<LDObject*> makePrimitive( PrimitiveType type, int segs, int divs, int num )
{
	vector<LDObject*> objs;
	
	for( int i = 0; i < segs; ++i )
	{
		double x = radialPoint( i, divs, cos ),
			nextX = radialPoint( i + 1, divs, cos ),
			prevX = radialPoint(( i + 15 ) % 16, divs, cos ),
			z = radialPoint( i, divs, sin ),
			nextZ = radialPoint( i + 1, divs, sin ),
			prevZ = radialPoint(( i + 15 ) % 16, divs, sin );
		
		switch( type )
		{
		case Circle:
		{
			vertex v0( x, 0.0f, z ),
				   v1( nextX, 0.0f, nextZ );
			
			LDLine* line = new LDLine;
			line->setVertex( 0, v0 );
			line->setVertex( 1, v1 );
			line->setColor( edgecolor );
			objs << line;
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
				x2 = nextX;
				x3 = x;
				z2 = nextZ;
				z3 = z;
				
				y0 = y1 = 0.0f;
				y2 = y3 = 1.0f;
			}
			else
			{
				x2 = nextX * ( num + 1 );
				x3 = x * ( num + 1 );
				z2 = nextZ * ( num + 1 );
				z3 = z * ( num + 1 );
				
				x *= num;
				nextX *= num;
				z *= num;
				nextZ *= num;
				
				if( type == Ring )
					y0 = y1 = y2 = y3 = 0.0f;
				else
				{
					y0 = y1 = 1.0f;
					y2 = y3 = 0.0f;
				}
			}
			
			vertex v0( x, y0, z ),
				   v1( nextX, y1, nextZ ),
				   v2( x2, y2, z2 ),
				   v3( x3, y3, z3 );
			
			LDQuad* quad = new LDQuad;
			quad->setColor( maincolor );
			quad->setVertex( 0, v0 );
			quad->setVertex( 1, v1 );
			quad->setVertex( 2, v2 );
			quad->setVertex( 3, v3 );
			objs << quad;
			
			LDCondLine* cond = null;
			if( type == Cylinder )
			{
				cond = new LDCondLine;
				cond->setColor( edgecolor );
				cond->setVertex( 0, v0 );
				cond->setVertex( 1, v3 );
				cond->setVertex( 2, vertex( nextX, 0.0f, nextZ ));
				cond->setVertex( 3, vertex( prevX, 0.0f, prevZ ));
			}
			
			if( cond )
				objs << cond;
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
				x2 = ( x >= 0.0f ) ? 1.0f : -1.0f;
				z2 = ( z >= 0.0f ) ? 1.0f : -1.0f;
			}
			
			vertex v0( x, 0.0f, z ),
				   v1( nextX, 0.0f, nextZ ),
				   v2( x2, 0.0f, z2 );
			
			// Disc negatives need to go the other way around, otherwise
			// they'll end up upside-down.
			LDTriangle* seg = new LDTriangle;
			seg->setColor( maincolor );
			seg->setVertex( type == Disc ? 0 : 2, v0 );
			seg->setVertex( 1, v1 );
			seg->setVertex( type == Disc ? 2 : 0, v2 );
			objs << seg;
		}
		break;
		
		default:
			break;
		}
	}
	
	return objs;
}

str primitiveTypeName( PrimitiveType type )
{
	// Not translated as primitives are in English.
	return type == Circle   ? "Circle" :
	       type == Cylinder ? "Cylinder" :
	       type == Disc     ? "Disc" :
	       type == DiscNeg  ? "Disc Negative" :
	       type == Ring     ? "Ring" : "Cone";
}

static const str g_radialNameRoots[] =
{
	"edge",
	"cyli",
	"disc",
	"ndis",
	"ring",
	"con"
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str radialFileName( PrimitiveType type, int segs, int divs, int num )
{
	short numer = segs,
		denom = divs;
	
	// Simplify the fractional part, but the denominator must be at least 4.
	simplify( numer, denom );
	
	if( denom < 4 )
	{
		const short factor = 4 / denom;
		
		numer *= factor;
		denom *= factor;
	}
	
	// Compose some general information: prefix, fraction, root, ring number
	str prefix = ( divs == lores ) ? "" : fmt( "%1/", divs );
	str frac = fmt( "%1-%2", numer, denom );
	str root = g_radialNameRoots[type];
	str numstr = ( type == Ring || type == Cone ) ? fmt( "%1", num ) : "";
	
	// Truncate the root if necessary (7-16rin4.dat for instance).
	// However, always keep the root at least 2 characters.
	int extra = ( frac.length() + numstr.length() + root.length()) - 8;
	root.chop( min<short>( max<short>( extra, 0 ), 2 ));
	
	// Stick them all together and return the result.
	return prefix + frac + root + numstr + ".dat";
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void generatePrimitive()
{
	QDialog* dlg = new QDialog( g_win );
	Ui::MakePrimUI ui;
	ui.setupUi( dlg );
	
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
		ui.rb_ring->isChecked()     ? Ring : Cone;
	
	// Make the description
	str frac = ftoa( ( ( float ) segs ) / divs );
	str name = radialFileName( type, segs, divs, num );
	str descr;
	
	// Ensure that there's decimals, even if they're 0.
	if( frac.indexOf( "." ) == -1 )
		frac += ".0";
	
	if( type == Ring || type == Cone )
		descr = fmt( "%1 %2 x %3", primitiveTypeName( type ), num, frac );
	else
		descr = fmt( "%1 %2", primitiveTypeName( type ), frac );
	
	LDOpenFile* f = new LDOpenFile;
	f->setName( QFileDialog::getSaveFileName( null, QObject::tr( "Save Primitive" ), name ));
	
	*f << new LDComment( descr );
	*f << new LDComment( fmt( "Name: %1", name ));
	*f << new LDComment( fmt( "Author: LDForge" ));
	*f << new LDComment( fmt( "!LDRAW_ORG Unofficial_%1Primitive", divs == hires ? "48_" : "" ));
	*f << new LDComment( CALicense );
	*f << new LDEmpty;
	*f << new LDBFC( LDBFC::CertifyCCW );
	*f << new LDEmpty;
	*f << makePrimitive( type, segs, divs, num );
	
	g_win->save( f, false );
	delete f;
}
