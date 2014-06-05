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

#pragma once
#include <QObject>
#include <QDate>
#include "main.h"
#include "basics.h"

class GLRenderer;
class QTimer;

//
// The message manager is an object which keeps track of messages that appear
// on the renderer's screen. Each line is contained in a separate object which
// contains the text, expiry time and alpha. The message manager is doubly
// linked to its corresponding renderer.
//
// Message manager calls its tick() function regularly to update the messages,
// where each line's expiry is checked for. Lines begin to fade out when nearing
// their expiry. If the message manager's lines change, the renderer undergoes
// repainting.
//
class MessageManager : public QObject
{
Q_OBJECT
PROPERTY (public, GLRenderer*, renderer, setRenderer, STOCK_WRITE)

public:
	// A single line of the message log.
	class Line
	{
		public:
			// Constructs a line with the given \c text
			Line (QString text);

			// Check this line's expiry and update alpha accordingly. @changed
			// is updated to whether the line has somehow changed since the
			// last update.
			//
			// Returns true if the line is to still stick around, false if it
			// expired.
			bool update (bool& changed);

			QString text;
			float alpha;
			QDateTime expiry;
	};

	// Constructs the message manager.
	explicit MessageManager (QObject* parent = null);

	// Adds a line with the given \c text to the message manager.
	void addLine (QString line);

	// Returns all active lines in the message manager.
	const QList<Line>& getLines() const;

private:
	QList<Line>	m_lines;
	QTimer*		m_ticker;

private slots:
	void tick();
};
