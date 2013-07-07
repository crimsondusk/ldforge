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

#include "common.h"
#include "colors.h"
#include "file.h"
#include "misc.h"
#include "gui.h"
#include <qcolor.h>

static LDColor* g_LDColors[MAX_COLORS];

void initColors()
{
	print( "%1: initializing color information.\n", __func__ );
	
	LDColor* col;
	
	// Always make sure there's 16 and 24 available. They're special like that.
	col = new LDColor;
	col->hexcode = "#AAAAAA";
	col->faceColor = col->hexcode;
	col->edgeColor = Qt::black;
	g_LDColors[maincolor] = col;
	
	col = new LDColor;
	col->hexcode = "#000000";
	col->edgeColor = col->faceColor = Qt::black;
	g_LDColors[edgecolor] = col;
	
	parseLDConfig();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
LDColor* getColor( short colnum )
{
	// Check bounds
	if( colnum < 0 || colnum >= MAX_COLORS )
		return null;
	
	return g_LDColors[colnum];
}

// =============================================================================
uchar luma( QColor& col )
{
	return ( 0.2126f * col.red()) +
	       ( 0.7152f * col.green()) +
	       ( 0.0722f * col.blue() );
}

// =============================================================================
// Helper function for parseLDConfig
static bool parseLDConfigTag( StringParser& pars, char const* tag, str& val )
{
	short pos;

	// Try find the token and get its position
	if( !pars.findToken( pos, tag, 1 ))
		return false;

	// Get the token after it and store it into val
	return pars.getToken( val, pos + 1 );
}

// =============================================================================
void parseLDConfig()
{
	File* f = openLDrawFile( "LDConfig.ldr", false );
	
	if( !f )
	{
		critical( fmt( QObject::tr( "Unable to open LDConfig.ldr for parsing! (%1)" ), strerror( errno )) );
		delete f;
		return;
	}
	
	// Read in the lines
	for( str line : *f )
	{
		if( line.length() == 0 || line[0] != '0' )
			continue; // empty or illogical
		
		line.remove( '\r' );
		line.remove( '\n' );
		
		// Parse the line
		StringParser pars( line, ' ' );
		
		short code = 0, alpha = 255;
		str name, facename, edgename, valuestr;
		
		// Check 0 !COLOUR, parse the name
		if( !pars.tokenCompare( 0, "0" ) || !pars.tokenCompare( 1, "!COLOUR" ) || !pars.getToken( name, 2 ))
			continue;
		
		// Replace underscores in the name with spaces for readability
		name.replace( "_", " " );
		
		// Get the CODE tag
		if( !parseLDConfigTag( pars, "CODE", valuestr ))
			continue;
		
		if( !isNumber( valuestr ))
			continue; // not a number
		
		// Ensure that the code is within [0 - 511]
		bool ok;
		code = valuestr.toShort( &ok );
		
		if( !ok || code < 0 || code >= 512 )
			continue;
		
		// VALUE and EDGE tags
		if( !parseLDConfigTag( pars, "VALUE", facename ) || !parseLDConfigTag( pars, "EDGE", edgename ))
			continue;
		
		// Ensure that our colors are correct
		QColor faceColor( facename ),
			edgeColor( edgename );
		
		if( !faceColor.isValid() || !edgeColor.isValid() )
			continue;
		
		// Parse alpha if given.
		if( parseLDConfigTag( pars, "ALPHA", valuestr ))
			alpha = clamp<short> ( valuestr.toShort(), 0, 255 );
		
		LDColor* col = new LDColor;
		col->name = name;
		col->faceColor = faceColor;
		col->edgeColor = edgeColor;
		col->hexcode = facename;
		col->faceColor.setAlpha( alpha );
		col->index = code;
		g_LDColors[code] = col;
	}
	
	delete f;
}
