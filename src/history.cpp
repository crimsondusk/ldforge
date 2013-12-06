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

#include "history.h"
#include "ldtypes.h"
#include "file.h"
#include "misc.h"
#include "gui.h"
#include "gldraw.h"

bool g_fullRefresh = false;

// =============================================================================
// -----------------------------------------------------------------------------
History::History() :
	m_Position (-1) {}

// =============================================================================
// -----------------------------------------------------------------------------
void History::undo()
{	if (m_changesets.isEmpty() || getPosition() == -1)
		return;

	const Changeset& set = getChangeset (getPosition());
	g_fullRefresh = false;

	// Iterate the list in reverse and undo all actions
	for (auto it = set.end() - 1; it != set.begin(); --it)
		(*it)->undo();

	decreasePosition();

	if (!g_fullRefresh)
		g_win->refresh();
	else
		g_win->doFullRefresh();

	g_win->updateActions();
}

// =============================================================================
// -----------------------------------------------------------------------------
void History::redo()
{	if (getPosition() == (long) m_changesets.size())
		return;

	const Changeset& set = getChangeset (getPosition() + 1);
	g_fullRefresh = false;

	// Redo things - in the order as they were done in the first place
	for (const AbstractHistoryEntry* change : set)
		change->redo();

	setPosition (getPosition() + 1);

	if (!g_fullRefresh)
		g_win->refresh();
	else
		g_win->doFullRefresh();

	g_win->updateActions();
}

// =============================================================================
// -----------------------------------------------------------------------------
void History::clear()
{	for (Changeset set : m_changesets)
		for (AbstractHistoryEntry* change : set)
			delete change;

	m_changesets.clear();
}

// =============================================================================
// -----------------------------------------------------------------------------
void History::addStep()
{	if (m_currentChangeset.isEmpty())
		return;

	while (getPosition() < getSize() - 1)
		m_changesets.removeLast();

	m_changesets << m_currentChangeset;
	m_currentChangeset.clear();
	setPosition (getPosition() + 1);
	g_win->updateActions();
}

// =============================================================================
// -----------------------------------------------------------------------------
void History::add (AbstractHistoryEntry* entry)
{	if (isIgnoring())
	{	delete entry;
		return;
	}

	entry->setParent (this);
	m_currentChangeset << entry;
}

// =============================================================================
// -----------------------------------------------------------------------------
void AddHistory::undo() const
{	LDFile* f = getParent()->getFile();
	LDObject* obj = f->getObject (getIndex());
	f->forgetObject (obj);
	delete obj;

	g_fullRefresh = true;
}

// =============================================================================
// -----------------------------------------------------------------------------
void AddHistory::redo() const
{	LDFile* f = getParent()->getFile();
	LDObject* obj = parseLine (getCode());
	f->insertObj (getIndex(), obj);
	g_win->R()->compileObject (obj);
}

// =============================================================================
// heh
// -----------------------------------------------------------------------------
void DelHistory::undo() const
{	LDFile* f = getParent()->getFile();
	LDObject* obj = parseLine (getCode());
	f->insertObj (getIndex(), obj);
	g_win->R()->compileObject (obj);
}

// =============================================================================
// -----------------------------------------------------------------------------
void DelHistory::redo() const
{	LDFile* f = getParent()->getFile();
	LDObject* obj = f->getObject (getIndex());
	f->forgetObject (obj);
	delete obj;

	g_fullRefresh = true;
}

// =============================================================================
// -----------------------------------------------------------------------------
void EditHistory::undo() const
{	LDObject* obj = LDFile::current()->getObject (getIndex());
	LDObject* newobj = parseLine (getOldCode());
	obj->replace (newobj);
	g_win->R()->compileObject (newobj);
}

// =============================================================================
// -----------------------------------------------------------------------------
void EditHistory::redo() const
{	LDObject* obj = LDFile::current()->getObject (getIndex());
	LDObject* newobj = parseLine (getNewCode());
	obj->replace (newobj);
	g_win->R()->compileObject (newobj);
}

// =============================================================================
// -----------------------------------------------------------------------------
void SwapHistory::undo() const
{	LDObject::fromID (a)->swap (LDObject::fromID (b));
}

void SwapHistory::redo() const
{	undo(); // :v
}