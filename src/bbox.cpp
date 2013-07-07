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
#include "bbox.h"
#include "ldtypes.h"
#include "file.h"

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bbox::bbox()
{
	reset();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void bbox::calculate()
{
	reset();
	
	if( !g_curfile )
		return;
	
	for( LDObject* obj : g_curfile->objs() )
		calcObject( obj );
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void bbox::calcObject( LDObject* obj )
{
	switch( obj->getType() )
	{
	case LDObject::Line:
	case LDObject::Triangle:
	case LDObject::Quad:
	case LDObject::CondLine:
		for( short i = 0; i < obj->vertices(); ++i )
			calcVertex( obj->getVertex( i ) );
		
		break;

	case LDObject::Subfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*>( obj );
			vector<LDObject*> objs = ref->inlineContents( true, true );
			
			for( LDObject * obj : objs )
			{
				calcObject( obj );
				delete obj;
			}
		}
		break;

	default:
		break;
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void bbox::calcVertex( const vertex& v )
{
	for( const Axis ax : g_Axes )
	{
		if( v[ax] < m_v0[ax] )
			m_v0[ax] = v[ax];
		
		if( v[ax] > m_v1[ax] )
			m_v1[ax] = v[ax];
	}
	
	m_empty = false;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void bbox::reset()
{
	m_v0[X] = m_v0[Y] = m_v0[Z] = 0x7FFFFFFF;
	m_v1[X] = m_v1[Y] = m_v1[Z] = 0xFFFFFFFF;
	
	m_empty = true;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
double bbox::size() const
{
	double xscale = ( m_v0[X] - m_v1[X] );
	double yscale = ( m_v0[Y] - m_v1[Y] );
	double zscale = ( m_v0[Z] - m_v1[Z] );
	double size = zscale;
	
	if( xscale > yscale )
	{
		if( xscale > zscale )
			size = xscale;
	}
	elif( yscale > zscale )
		size = yscale;
	
	if( abs( size ) >= 2.0f )
		return abs( size / 2 );
	
	return 1.0f;
}

// =============================================================================
vertex bbox::center() const
{
	return vertex(
		( m_v0[X] + m_v1[X] ) / 2,
		( m_v0[Y] + m_v1[Y] ) / 2,
		( m_v0[Z] + m_v1[Z] ) / 2 );
}
