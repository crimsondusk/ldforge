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
#include <qprogressbar.h>
#include <QDir>
#include <QThread>

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

cfg( str, io_ldpath, "" );
cfg( str, io_recentfiles, "" );

static bool g_loadingMainFile = false;
static const int g_MaxRecentFiles = 5;

// =============================================================================
namespace LDPaths
{
	static str pathError;
	
	struct
	{
		str LDConfigPath;
		str partsPath, primsPath;
	} pathInfo;
	
	void initPaths ()
	{
		if( !tryConfigure ( io_ldpath ))
		{
			LDrawPathDialog dlg (false);
			
			if( !dlg.exec () )
				exit( 0 );
			
			io_ldpath = dlg.filename();
		}
	}
	
	bool tryConfigure( str path )
	{
		QDir dir;
		
		if( !dir.cd( path ))
		{
			pathError = "Directory does not exist.";
			return false;
		}
		
		QStringList mustHave = { "LDConfig.ldr", "parts", "p" };
		QStringList contents = dir.entryList( mustHave );
		
		if (contents.size () != mustHave.size ()) {
			pathError = "Not an LDraw directory! Must<br />have LDConfig.ldr, parts/ and p/.";
			return false;
		}
		
		pathInfo.partsPath = fmt( "%1" DIRSLASH "parts", path );
		pathInfo.LDConfigPath = fmt( "%1" DIRSLASH "LDConfig.ldr", path );
		pathInfo.primsPath = fmt( "%1" DIRSLASH "p", path );
		
		return true;
	}
	
	// Accessors
	str getError() { return pathError; }
	str ldconfig() { return pathInfo.LDConfigPath; }
	str prims() { return pathInfo.primsPath; }
	str parts() { return pathInfo.partsPath; }
}

// =============================================================================
LDOpenFile::LDOpenFile()
{
	setImplicit( true );
	setSavePos( -1 );
	m_history.setFile( this );
}

// =============================================================================
LDOpenFile::~LDOpenFile()
{
	// Clear everything from the model
	for( LDObject* obj : m_objs )
		delete obj;
	
	// Clear the cache as well
	for( LDObject* obj : m_cache )
		delete obj;
}

// =============================================================================
LDOpenFile* findLoadedFile( str name )
{
	for( LDOpenFile* file : g_loadedFiles )
		if( file->name () == name )
			return file;
	
	return null;
}

// =============================================================================
str dirname( str path ) {
	long lastpos = path.lastIndexOf( DIRSLASH );
	
	if( lastpos > 0 )
		return path.left( lastpos );
	
#ifndef _WIN32
	if( path[0] == DIRSLASH_CHAR )
		return DIRSLASH;
#endif // _WIN32
	
	return "";
}

// =============================================================================
str basename (str path) {
	long lastpos = path.lastIndexOf( DIRSLASH );
	
	if( lastpos != -1 )
		return path.mid( lastpos + 1 );
	
	return path;
}

// =============================================================================
File* openLDrawFile (str relpath, bool subdirs) {
	print( "%1: Try to open %2\n", __func__, relpath );
	File* f = new File;
	
	// LDraw models use Windows-style path separators. If we're not on Windows,
	// replace the path separator now before opening any files.
#ifndef WIN32
	relpath.replace( "\\", "/" );
#endif // WIN32
	
	if( g_curfile )
	{
		// First, try find the file in the current model's file path. We want a file
		// in the immediate vicinity of the current model to override stock LDraw stuff.
		str partpath = fmt ("%1" DIRSLASH "%2", dirname (g_curfile->name ()), relpath);
		
		if (f->open (partpath, File::Read))
			return f;
	}
	
	if (f->open (relpath, File::Read))
		return f;
	
	str fullPath;
	if( io_ldpath.value.length() > 0 )
	{
		// Try with just the LDraw path first
		fullPath = fmt ("%1" DIRSLASH "%2", io_ldpath, relpath);
		
		if( f->open( fullPath, File::Read ))
			return f;
		
		if( subdirs )
		{
			// Look in sub-directories: parts and p
			for( auto subdir : initlist<const str>({ "parts", "p" }))
			{
				fullPath = fmt ("%1" DIRSLASH "%2" DIRSLASH "%3",
					io_ldpath, subdir, relpath);
				
				if (f->open (fullPath, File::Read))
					return f;
			}
		}
	}
	
	// Did not find the file.
	delete f;
	return null;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void FileLoader::work()
{
	m_progress = 0;
	abortflag = false;
	
	for( str line : *PROP_NAME( file )) {
		// Trim the trailing newline
		qchar c;
		while(( c = line[line.length () - 1] ) == '\n' || c == '\r' )
			line.chop( 1 );
		
		LDObject* obj = parseLine( line );
		
		// Check for parse errors and warn about tthem
		if( obj->getType () == LDObject::Gibberish )
		{
/*
			logf( LOG_Warning, "Couldn't parse line #%lu: %s\n",
				m_progress + 1, qchars (static_cast<LDGibberish*> (obj)->reason ));
			
			logf (LOG_Warning, "- Line was: %s\n", qchars (line));
*/
			
			if( m_warningsPointer )
				( *m_warningsPointer )++;
		}
		
		m_objs << obj;
		m_progress++;
		emit progressUpdate( m_progress );
		
		if( abortflag )
		{
			// We were flagged for abortion, so abort.
			for( LDObject* obj : m_objs )
				delete obj;
			
			m_objs.clear();
			return;
		}
	}
	
	emit workDone();
	m_done = true;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
vector<LDObject*> loadFileContents (File* f, ulong* numWarnings, bool* ok) {
	vector<str> lines;
	vector<LDObject*> objs;
	
	if( numWarnings )
		*numWarnings = 0;
	
	FileLoader* loader = new FileLoader;
	loader->setFile( f );
	loader->setWarningsPointer( numWarnings );
	
	// Calculate the amount of lines
	ulong numLines = 0;
	for( str line : *f )
		numLines++;
	
	f->rewind();
	
	if (g_loadingMainFile)
	{
		// Show a progress dialog if we're loading the main file here and move
		// the actual work to a separate thread as this can be a rather intensive
		// operation and if we don't respond quickly enough, the program can be
		// deemed inresponsive.. which is a bad thing.
		
		// Init the thread and move the loader into it
		QThread* loaderThread = new QThread;
		QObject::connect( loaderThread, SIGNAL( started() ), loader, SLOT( work() ));
		QObject::connect( loaderThread, SIGNAL( finished() ), loader, SLOT( deleteLater() ));
		loader->moveToThread (loaderThread);
		loaderThread->start ();
		
		// Now create a progress prompt for the operation
		OpenProgressDialog* dlg = new OpenProgressDialog (g_win);
		dlg->setNumLines( numLines );
		
		// Connect the loader in so we can show updates
		QObject::connect( loader, SIGNAL( progressUpdate( int )), dlg, SLOT( updateProgress( int )));
		QObject::connect( loader, SIGNAL( workDone() ), dlg, SLOT( accept() ));
		
		// Show the prompt. If the user hits cancel, tell the loader to abort.
		if( !dlg->exec() )
			loader->abortflag = true;
	} else
		loader->work();
	
	// If we wanted the success value, supply that now
	if( ok )
		*ok = loader->done();
	
	// If the loader was done, return the objects it generated
	if( loader->done() )
		objs = loader->objs();
	
	return objs;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
LDOpenFile* openDATFile( str path, bool search )
{
	// Convert the file name to lowercase since some parts contain uppercase
	// file names. I'll assume here that the library will always use lowercase
	// file names for the actual parts..
	File* f;
	if( search )
		f = openLDrawFile (path.toLower (), true);
	else {
		f = new File( path, File::Read );
		
		if( !*f )
		{
			delete f;
			f = null;
		}
	}
	
	if( !f )
		return null;
	
	LDOpenFile* oldLoad = g_curfile;
	LDOpenFile* load = new LDOpenFile;
	load->setName( path );
	
	if( g_loadingMainFile )
	{
		g_curfile = load;
		g_win->R()->setFile( load );
	}
	
	ulong numWarnings;
	bool ok;
	vector<LDObject*> objs = loadFileContents (f, &numWarnings, &ok);
	
	if( !ok )
	{
		load = oldLoad;
		return null;
	}
	
	for (LDObject* obj : objs)
		load->addObject (obj);
	
	delete f;
	g_loadedFiles << load;
	
	/*
	logf ("File %s parsed successfully (%lu warning%s).\n",
		qchars (path), numWarnings, plural (numWarnings));
	*/
	
	return load;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool LDOpenFile::safeToClose()
{
	typedef QMessageBox msgbox;
	setlocale( LC_ALL, "C" );
	
	// If we have unsaved changes, warn and give the option of saving.
	if( !implicit() && history().pos() != savePos() )
	{
		str message = fmt( "There are unsaved changes to %1. Should it be saved?",
			(name().length() > 0) ? name() : "<anonymous>");
		switch( msgbox::question( g_win, "Unsaved Changes", message,
			( msgbox::Yes | msgbox::No | msgbox::Cancel ), msgbox::Cancel ))
		{
		case msgbox::Yes:
			// If we don't have a file path yet, we have to ask the user for one.
			if( name().length() == 0 )
			{
				str newpath = QFileDialog::getSaveFileName( g_win, "Save As",
					g_curfile->name(), "LDraw files (*.dat *.ldr)" );
				
				if( newpath.length() == 0 )
					return false;
				
				setName( newpath );
			}
			
			if( !save () ) {
				message = fmt( "Failed to save %1: %2\nDo you still want to close?",
					name(), strerror( errno ));
				
				if( QMessageBox::critical( g_win, "Save Failure", message,
					( QMessageBox::Yes | QMessageBox::No ), QMessageBox::No ) == QMessageBox::No)
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
void closeAll()
{
	if( !g_loadedFiles.size() )
		return;
	
	// Remove all loaded files and the objects they contain
	for( LDOpenFile* file : g_loadedFiles )
		delete file;
	
	// Clear the array
	g_loadedFiles.clear();
	g_curfile = null;
	
	g_win->R()->setFile (null);
	g_win->fullRefresh();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void newFile ()
{
	closeAll();
	
	// Create a new anonymous file and set it to our current
	LDOpenFile* f = new LDOpenFile;
	f->setName( "" );
	f->setImplicit( false );
	g_loadedFiles << f;
	g_curfile = f;
	
	g_BBox.reset();
	g_win->R()->setFile( f );
	g_win->fullRefresh();
	g_win->updateTitle();
	f->history().updateActions();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void addRecentFile( str path )
{
	QStringList rfiles = io_recentfiles.value.split( '@' );
	
	int idx = rfiles.indexOf( path );
	
	// If this file already is in the list, pop it out.
	if( idx != -1 )
	{
		if( rfiles.size () == 1 )
			return; // only recent file - abort and do nothing
		
		// Pop it out.
		rfiles.removeAt( idx );
	}
	
	// If there's too many recent files, drop one out.
	while( rfiles.size() > ( g_MaxRecentFiles - 1 ))
		rfiles.removeAt( 0 );
	
	// Add the file
	rfiles.push_back( path );
	
	// Rebuild the config string
	io_recentfiles = rfiles.join( "@" );
	
	config::save();
	g_win->updateRecentFilesMenu();
}

// =============================================================================
// -----------------------------------------------------------------------------
// Open an LDraw file and set it as the main model
// =============================================================================
void openMainFile( str path )
{
	g_loadingMainFile = true;
	closeAll();
	
	LDOpenFile* file = openDATFile( path, false );
	
	if (!file)
	{
		// Loading failed, thus drop down to a new file since we
		// closed everything prior.
		newFile ();
		
		// Tell the user loading failed.
		setlocale( LC_ALL, "C" );
		critical( fmt( "Failed to open %1: %2", path, strerror( errno )));
		
		g_loadingMainFile = false;
		return;
	}
	
	file->setImplicit( false );
	g_curfile = file;
	
	// Recalculate the bounding box
	g_BBox.calculate();
	
	// Rebuild the object tree view now.
	g_win->fullRefresh();
	g_win->updateTitle();
	g_win->R()->setFile( file );
	g_win->R()->resetAngles();
	
	// Add it to the recent files list.
	addRecentFile( path );
	g_loadingMainFile = false;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool LDOpenFile::save( str savepath )
{
	if( !savepath.length() )
		savepath = name();
	
	File f( savepath, File::Write );
	
	if( !f )
		return false;
	
	// If the second object in the list holds the file name, update that now.
	// Only do this if the file is explicitly open. If it's saved into a directory
	// called "s" or "48", prepend that into the name.
	LDComment* fpathComment = null;
	LDObject* first = object( 1 );
	if( !implicit() && first != null && first->getType() == LDObject::Comment )
	{
		fpathComment = static_cast<LDComment*>( first );
		
		if( fpathComment->text.left( 6 ) == "Name: " ) {
			str newname;
			str dir = basename( dirname( savepath ));
			
			if( dir == "s" || dir == "48" )
				newname = dir + "\\";
			
			newname += basename( savepath );
			fpathComment->text = fmt( "Name: %1", newname );
			g_win->buildObjList();
		}
	}
	
	// File is open, now save the model to it. Note that LDraw requires files to
	// have DOS line endings, so we terminate the lines with \r\n.
	for( LDObject* obj : objs() )
		f.write( obj->raw () + "\r\n" );
	
	// File is saved, now clean up.
	f.close();
	
	// We have successfully saved, update the save position now.
	setSavePos( history().pos() );
	setName( savepath );
	
	g_win->updateTitle();
	return true;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
#define CHECK_TOKEN_COUNT( N ) \
	if( tokens.size() != N ) \
		return new LDGibberish( line, "Bad amount of tokens" );

#define CHECK_TOKEN_NUMBERS( MIN, MAX ) \
	for (ushort i = MIN; i <= MAX; ++i) \
		if (!isNumber (tokens[i])) \
			return new LDGibberish (line, fmt ("Token #%1 was `%2`, expected a number", \
				(i + 1), tokens[i]));

static vertex parseVertex( QStringList& s, const ushort n )
{
	vertex v;
	for( const Axis ax : g_Axes )
		v[ax] = atof( s[n + ax] );
	
	return v;
}

// =============================================================================
// -----------------------------------------------------------------------------
// This is the LDraw code parser function. It takes in a string containing LDraw
// code and returns the object parsed from it. parseLine never returns null,
// the object will be LDGibberish if it could not be parsed properly.
// =============================================================================
LDObject* parseLine( str line )
{
	QStringList tokens = line.split( " ", str::SkipEmptyParts );
	
	if( tokens.size() <= 0 )
	{
		// Line was empty, or only consisted of whitespace
		return new LDEmpty;
	}
	
	if( tokens[0].length() != 1 )
		return new LDGibberish (line, "Illogical line code");
	
	int num = tokens[0][0].toAscii() - '0';
	switch( num )
	{
	case 0:
		{
			// Comment
			str comm = line.mid( line.indexOf( "0" ) + 1 );
			
			// Remove any leading whitespace
			while( comm[0] == ' ' )
				comm.remove( 0, 1 );
			
			// Handle BFC statements
			if (tokens.size() > 2 && tokens[1] == "BFC") {
				for (short i = 0; i < LDBFC::NumStatements; ++i)
					if (comm == fmt ("BFC %1", LDBFC::statements [i]))
						return new LDBFC ((LDBFC::Type) i);
				
				// MLCAD is notorious for stuffing these statements in parts it
				// creates. The above block only handles valid statements, so we
				// need to handle MLCAD-style invertnext separately.
				if (comm == "BFC CERTIFY INVERTNEXT")
					return new LDBFC (LDBFC::InvertNext);
			}
			
			if( tokens.size() > 2 && tokens[1] == "!LDFORGE" )
			{
				// Handle LDForge-specific types, they're embedded into comments too
				if( tokens[2] == "VERTEX" )
				{
					// Vertex (0 !LDFORGE VERTEX)
					CHECK_TOKEN_COUNT (7)
					CHECK_TOKEN_NUMBERS (3, 6)
					
					LDVertex* obj = new LDVertex;
					obj->setColor (tokens[3].toLong ());
					
					for (const Axis ax : g_Axes)
						obj->pos[ax] = tokens[4 + ax].toDouble (); // 4 - 6
					
					return obj;
				}
				elif( tokens[2] == "OVERLAY" )
				{
					CHECK_TOKEN_COUNT( 9 );
					CHECK_TOKEN_NUMBERS( 5, 8 )
					
					LDOverlay* obj = new LDOverlay;
					obj->setFilename( tokens[3] );
					obj->setCamera( tokens[4].toLong() );
					obj->setX( tokens[5].toLong() );
					obj->setY( tokens[6].toLong() );
					obj->setWidth( tokens[7].toLong() );
					obj->setHeight( tokens[8].toLong() );
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
			CHECK_TOKEN_COUNT( 15 )
			CHECK_TOKEN_NUMBERS( 1, 13 )
			
			// Try open the file. Disable g_loadingMainFile temporarily since we're
			// not loading the main file now, but the subfile in question.
			bool tmp = g_loadingMainFile;
			g_loadingMainFile = false;
			LDOpenFile* load = getFile( tokens[14] );
			g_loadingMainFile = tmp;
			
			// If we cannot open the file, mark it an error
			if( !load )
				return new LDGibberish( line, "Could not open referred file" );
			
			LDSubfile* obj = new LDSubfile;
			obj->setColor( tokens[1].toLong() );
			obj->setPosition( parseVertex( tokens, 2 )); // 2 - 4
			
			matrix transform;
			for( short i = 0; i < 9; ++i )
				transform[i] = tokens[i + 5].toDouble(); // 5 - 13
			
			obj->setTransform( transform );
			obj->setFileInfo( load );
			return obj;
		}
	
	case 2:
		{
			CHECK_TOKEN_COUNT( 8 )
			CHECK_TOKEN_NUMBERS( 1, 7 )
			
			// Line
			LDLine* obj = new LDLine;
			obj->setColor( tokens[1].toLong() );
			for( short i = 0; i < 2; ++i )
				obj->setVertex( i, parseVertex( tokens, 2 + ( i * 3 ))); // 2 - 7
			return obj;
		}
	
	case 3:
		{
			CHECK_TOKEN_COUNT( 11 )
			CHECK_TOKEN_NUMBERS( 1, 10 )
			
			// Triangle
			LDTriangle* obj = new LDTriangle;
			obj->setColor( tokens[1].toLong() );
			
			for( short i = 0; i < 3; ++i )
				obj->setVertex( i, parseVertex( tokens, 2 + ( i * 3 ))); // 2 - 10
			
			return obj;
		}
	
	case 4:
	case 5:
		{
			CHECK_TOKEN_COUNT( 14 )
			CHECK_TOKEN_NUMBERS( 1, 13 )
			
			// Quadrilateral / Conditional line
			LDObject* obj = ( num == 4 ) ? ( (LDObject*) new LDQuad ) : ( (LDObject*) new LDCondLine );
			obj->setColor( tokens[1].toLong() );
			
			for( short i = 0; i < 4; ++i )
				obj->setVertex( i, parseVertex( tokens, 2 + ( i * 3 ))); // 2 - 13
			
			return obj;
		}
	
	default: // Strange line we couldn't parse
		return new LDGibberish( line, "Unknown line code number" );
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
LDOpenFile* getFile( str fname )
{
	// Try find the file in the list of loaded files
	LDOpenFile* load = findLoadedFile( fname );
	
	// If it's not loaded, try open it
	if( !load )
		load = openDATFile( fname, true );
	
	return load;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void reloadAllSubfiles () {
	if( !g_curfile )
		return;
	
	g_loadedFiles.clear();
	g_loadedFiles << g_curfile;
	
	// Go through all objects in the current file and reload the subfiles
	for( LDObject* obj : g_curfile->objs() )
	{
		if( obj->getType() == LDObject::Subfile )
		{
			LDSubfile* ref = static_cast<LDSubfile*>( obj );
			LDOpenFile* fileInfo = getFile( ref->fileInfo()->name() );
			
			if (fileInfo)
				ref->setFileInfo( fileInfo );
			else
				ref->replace( new LDGibberish( ref->raw(), "Could not open referred file" ));
		}
		
		// Reparse gibberish files. It could be that they are invalid because
		// of loading errors. Circumstances may be different now.
		if( obj->getType() == LDObject::Gibberish )
			obj->replace( parseLine( static_cast<LDGibberish*>( obj )->contents ));
	}
	
	// Close all files left unused
	LDOpenFile::closeUnused();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ulong LDOpenFile::addObject (LDObject* obj)
{
	PROP_NAME( history ).add( new AddHistory( PROP_NAME( objs ).size(), obj ));
	PROP_NAME( objs ) << obj;
	
	if( obj->getType() == LDObject::Vertex )
		PROP_NAME( vertices ) << obj;
	
	if( this == g_curfile )
		g_BBox.calcObject( obj );
	
	return numObjs() - 1;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDOpenFile::insertObj (const ulong pos, LDObject* obj) {
	m_history.add( new AddHistory( pos, obj ));
	m_objs.insert( pos, obj );
	
	if( this == g_curfile )
		g_BBox.calcObject( obj );
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void LDOpenFile::forgetObject( LDObject* obj ) {
	ulong idx = obj->getIndex( this );
	m_history.add( new DelHistory( idx, obj ));
	m_objs.erase( idx );
	
	// Update the bounding box
	if( this == g_curfile )
		g_BBox.calculate ();
}

// =============================================================================
bool safeToCloseAll () {
	for( LDOpenFile* f : g_loadedFiles )
		if( !f->safeToClose() )
			return false;
	
	return true;
}

// =============================================================================
void LDOpenFile::setObject (ulong idx, LDObject* obj) {
	assert( idx < numObjs() );
	
	// Mark this change to history
	str oldcode = object( idx )->raw ();
	str newcode = obj->raw();
	m_history << new EditHistory( idx, oldcode, newcode );
	
	m_objs[idx] = obj;
}

static vector<LDOpenFile*> getFilesUsed (LDOpenFile* node) {
	vector<LDOpenFile*> filesUsed;
	
	for (LDObject* obj : *node)
	{
		if( obj->getType() != LDObject::Subfile )
			continue;
		
		LDSubfile* ref = static_cast<LDSubfile*>( obj );
		filesUsed << ref->fileInfo();
		filesUsed << getFilesUsed( ref->fileInfo() );
	}
	
	return filesUsed;
}

// =============================================================================
// Find out which files are unused and close them.
void LDOpenFile::closeUnused ()
{
	vector<LDOpenFile*> filesUsed = getFilesUsed (g_curfile);
	
	// Anything that's explicitly opened must not be closed
	for( LDOpenFile* file : g_loadedFiles )
		if( !file->implicit() )
			filesUsed << file;
	
	// Remove duplicated entries
	filesUsed.makeUnique();
	
	// Close all open files that aren't in filesUsed
	for( LDOpenFile* file : g_loadedFiles )
	{
		bool isused = false;
		
		for( LDOpenFile* usedFile : filesUsed )
		{
			if( file == usedFile )
			{
				isused = true;
				break;
			}
		}
		
		if( !isused )
			delete file;
	}
	
	g_loadedFiles.clear();
	g_loadedFiles << filesUsed;
}

LDObject* LDOpenFile::object( ulong pos ) const
{
	if( m_objs.size() <= pos )
		return null;
	
	return m_objs[pos];
}

LDOpenFile& LDOpenFile::operator<<( vector<LDObject*> objs )
{
	for( LDObject* obj : objs )
		m_objs << obj;
	
	return *this;
}