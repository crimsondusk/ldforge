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

#include "msglog.h"
#include "gldraw.h"
#include "gui.h"
#include <QTimer>
#include <QDate>

static const unsigned int g_maxMessages = 5;
static const int g_expiry = 5;
static const int g_fadeTime = 500; // msecs

MessageManager::MessageManager( QObject* parent ) : QObject( parent )
{
	m_ticker = new QTimer;
	m_ticker->start( 100 );
	connect( m_ticker, SIGNAL( timeout() ), this, SLOT( tick() ));
}

MessageManager::Line::Line( str text ) : text( text ), alpha( 1.0f )
{
	// Init expiry
	expiry = QDateTime::currentDateTime().addSecs( g_expiry );
}

// =============================================================================
// Check this line's expiry and update alpha accordingly. Returns true if the
// line is still around, false if it expired.
bool MessageManager::Line::update( bool& changed )
{
	changed = false;
	
	QDateTime now = QDateTime::currentDateTime();
	if( now >= expiry )
	{
		changed = true;
		return false;
	}
	
	int msec = now.msecsTo( expiry );
	if( msec <= g_fadeTime )
	{
		alpha = ( (float) msec ) / g_fadeTime;
		changed = true;
	}
	
	return true;
}

void MessageManager::addLine( str line )
{
	// If there's too many entries, pop the excess out
	while( m_lines.size() >= g_maxMessages )
		m_lines.erase( 0 );
	
	m_lines << Line( line );
	
	// Update the renderer view
	if( renderer() )
		renderer()->update();
}

MessageManager& MessageManager::operator<<( str line )
{
	addLine( line );
	return *this;
}

void MessageManager::tick()
{
	bool changed = false;
	
	for( uint i = 0; i < m_lines.size(); ++i )
	{
		bool lineChanged;
		
		if( !m_lines[i].update( lineChanged ))
			m_lines.erase( i-- );
		
		changed |= lineChanged;
	}
	
	if( changed && renderer() )
		renderer()->update();
}

MessageManager::c_it MessageManager::begin() const
{
	return m_lines.begin();
}

MessageManager::c_it MessageManager::end() const
{
	return m_lines.end();
}

void DoLog( std::initializer_list<StringFormatArg> args )
{
	g_win->addMessage( DoFormat( args ));
}