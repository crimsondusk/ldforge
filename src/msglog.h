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

#ifndef MSGLOG_H
#define MSGLOG_H

#include <QObject>
#include <QDate>
#include "common.h"
#include "types.h"

class GLRenderer;
class QTimer;
class MessageManager : public QObject
{
	Q_OBJECT
	PROPERTY( GLRenderer*, renderer, setRenderer )
	
public:
	class Line
	{
	public:
		Line( str text );
		bool update( bool& changed );
		
		str text;
		float alpha;
		QDateTime expiry;
	};
	
	typedef vector<Line>::it it;
	typedef vector<Line>::c_it c_it;
	
	explicit MessageManager( QObject* parent = 0 );
	void addLine( str line );
	c_it begin() const;
	c_it end() const;
	
	MessageManager& operator<<( str line );
	
private:
	vector<Line> m_lines;
	QTimer* m_ticker;
	
private slots:
	void tick();
};

#endif // MSGLOG_H