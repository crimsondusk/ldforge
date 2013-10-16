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

#include <QTimer>
#include <QDate>
#include "messagelog.h"
#include "gldraw.h"
#include "gui.h"
#include "moc_messagelog.cpp"

static const int g_maxMessages = 5;
static const int g_expiry = 5;
static const int g_fadeTime = 500; // msecs

// =============================================================================
// -----------------------------------------------------------------------------
MessageManager::MessageManager (QObject* parent) :
			QObject (parent)
{	m_ticker = new QTimer;
	m_ticker->start (100);
	connect (m_ticker, SIGNAL (timeout()), this, SLOT (tick()));
}

// =============================================================================
// -----------------------------------------------------------------------------
MessageManager::Line::Line (str text) :
			text (text),
			alpha (1.0f),
			expiry (QDateTime::currentDateTime().addSecs (g_expiry)) {}

// =============================================================================
// Check this line's expiry and update alpha accordingly. Returns true if the
// line is to still stick around, false if it expired. 'changed' is updated to
// whether the line has somehow changed since the last update.
// -----------------------------------------------------------------------------
bool MessageManager::Line::update (bool& changed)
{	changed = false;
	QDateTime now = QDateTime::currentDateTime();
	int msec = now.msecsTo (expiry);

	if (now >= expiry)
	{	// Message line has expired
		changed = true;
		return false;
	}

	if (msec <= g_fadeTime)
	{	// Message line has not expired but is fading out
		alpha = ( (float) msec) / g_fadeTime;
		changed = true;
	}

	return true;
}

// =============================================================================
// Add a line to the message manager.
// -----------------------------------------------------------------------------
void MessageManager::addLine (str line)
{	// If there's too many entries, pop the excess out
	while (m_lines.size() >= g_maxMessages)
		m_lines.erase (0);

	m_lines << Line (line);

	// Update the renderer view
	if (renderer())
		renderer()->update();
}

// =============================================================================
// Ticks the message manager. All lines are ticked and the renderer scene is
// redrawn if something changed.
// -----------------------------------------------------------------------------
void MessageManager::tick()
{	if (m_lines.size() == 0)
		return;

	bool changed = false;

	for (int i = 0; i < m_lines.size(); ++i)
	{	bool lineChanged;

		if (!m_lines[i].update (lineChanged))
			m_lines.removeAt (i--);

		changed |= lineChanged;
	}

	if (changed && renderer())
		renderer()->update();
}

// =============================================================================
// -----------------------------------------------------------------------------
const List<MessageManager::Line>& MessageManager::getLines() const
{	return m_lines;
}

// =============================================================================
// log() interface - format the argument list and add the resulting string to
// the main message manager.
// -----------------------------------------------------------------------------
void DoLog (std::initializer_list<StringFormatArg> args)
{	const str msg = DoFormat (args);
	g_win->addMessage (msg);

	// Also print it to stdout
	print ("%1\n", msg);
}
