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

#include "history.h"
#include "ldtypes.h"
#include "document.h"
#include "misc.h"
#include "gui.h"
#include "gldraw.h"

// =============================================================================
//
History::History() :
	m_Position (-1) {}

// =============================================================================
//
void History::undo()
{
	if (m_changesets.isEmpty() || getPosition() == -1)
		return;

	// Don't take the changes done here as actual edits to the document
	setIgnoring (true);

	const Changeset& set = getChangeset (getPosition());

	// Iterate the list in reverse and undo all actions
	for (int i = set.size() - 1; i >= 0; --i)
	{
		AbstractHistoryEntry* change = set[i];
		change->undo();
	}

	decreasePosition();
	g_win->refresh();
	g_win->updateActions();
	dlog ("Position is now %1", getPosition());
	setIgnoring (false);
}

// =============================================================================
//
void History::redo()
{
	if (getPosition() == m_changesets.size())
		return;

	setIgnoring (true);
	const Changeset& set = getChangeset (getPosition() + 1);

	// Redo things - in the order as they were done in the first place
	for (const AbstractHistoryEntry* change : set)
		change->redo();

	setPosition (getPosition() + 1);
	g_win->refresh();
	g_win->updateActions();
	dlog ("Position is now %1", getPosition());
	setIgnoring (false);
}

// =============================================================================
//
void History::clear()
{
	for (Changeset set : m_changesets)
		for (AbstractHistoryEntry* change : set)
			delete change;

	m_changesets.clear();
	dlog ("History: cleared");
}

// =============================================================================
//
void History::addStep()
{
	if (m_currentChangeset.isEmpty())
		return;

	while (getPosition() < getSize() - 1)
	{
		Changeset last = m_changesets.last();

		for (AbstractHistoryEntry* entry : last)
			delete entry;

		m_changesets.removeLast();
	}

	dlog ("History: step added (%1 changes)", m_currentChangeset.size());
	m_changesets << m_currentChangeset;
	m_currentChangeset.clear();
	setPosition (getPosition() + 1);
	g_win->updateActions();
}

// =============================================================================
//
void History::add (AbstractHistoryEntry* entry)
{
	if (isIgnoring())
	{
		delete entry;
		return;
	}

	entry->setParent (this);
	m_currentChangeset << entry;
	dlog ("History: added entry of type %1", entry->getTypeName());
}

// =============================================================================
//
void AddHistory::undo() const
{
	LDObject* obj = getParent()->getDocument()->getObject (getIndex());
	obj->deleteSelf();
}

// =============================================================================
//
void AddHistory::redo() const
{
	LDObject* obj = parseLine (getCode());
	getParent()->getDocument()->insertObj (getIndex(), obj);
	g_win->R()->compileObject (obj);
}

// =============================================================================
//
DelHistory::DelHistory (int idx, LDObject* obj) :
	m_Index (idx),
	m_Code (obj->raw()) {}

// =============================================================================
// heh
//
void DelHistory::undo() const
{
	LDObject* obj = parseLine (getCode());
	getParent()->getDocument()->insertObj (getIndex(), obj);
	g_win->R()->compileObject (obj);
}

// =============================================================================
//
void DelHistory::redo() const
{
	LDObject* obj = getParent()->getDocument()->getObject (getIndex());
	obj->deleteSelf();
}

// =============================================================================
//
void EditHistory::undo() const
{
	LDObject* obj = getCurrentDocument()->getObject (getIndex());
	LDObject* newobj = parseLine (getOldCode());
	obj->replace (newobj);
	g_win->R()->compileObject (newobj);
}

// =============================================================================
//
void EditHistory::redo() const
{
	LDObject* obj = getCurrentDocument()->getObject (getIndex());
	LDObject* newobj = parseLine (getNewCode());
	obj->replace (newobj);
	g_win->R()->compileObject (newobj);
}

// =============================================================================
//
void SwapHistory::undo() const
{
	LDObject::fromID (a)->swap (LDObject::fromID (b));
}

// =============================================================================
//
void SwapHistory::redo() const
{
	undo();
}