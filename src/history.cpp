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
	m_pos (-1),
	m_opened (false) {}

// =============================================================================
// -----------------------------------------------------------------------------
void History::undo()
{	if (m_changesets.isEmpty() || pos() == -1)
		return;

	const Changeset& set = getChangeset (pos());
	g_fullRefresh = false;

	// Iterate the list in reverse and undo all actions
	for (auto it = set.end() - 1; it != set.begin(); --it)
		(*it)->undo();

	setPos (pos() - 1);

	if (!g_fullRefresh)
		g_win->refresh();
	else
		g_win->doFullRefresh();

	updateActions();
}

// =============================================================================
// -----------------------------------------------------------------------------
void History::redo()
{	if (pos() == (long) m_changesets.size())
		return;

	const Changeset& set = getChangeset (pos() + 1);
	g_fullRefresh = false;

	// Redo things - in the order as they were done in the first place
	for (const AbstractHistoryEntry* change : set)
		change->redo();

	setPos (pos() + 1);

	if (!g_fullRefresh)
		g_win->refresh();
	else
		g_win->doFullRefresh();

	updateActions();
}

// =============================================================================
// -----------------------------------------------------------------------------
void History::clear()
{	for (Changeset set : m_changesets)
		for (auto change : set)
			delete change;

	m_changesets.clear();
}

// =============================================================================
// -----------------------------------------------------------------------------
void History::updateActions() const
{	ACTION (Undo)->setEnabled (pos() != -1);
	ACTION (Redo)->setEnabled (pos() < (long) m_changesets.size() - 1);
}

// =============================================================================
// -----------------------------------------------------------------------------
void History::open()
{	if (opened())
		return;

	setOpened (true);
}

// =============================================================================
// -----------------------------------------------------------------------------
void History::close()
{	if (!opened())
		return;

	setOpened (false);

	if (m_currentArchive.isEmpty())
		return;

	while (pos() < getSize() - 1)
		m_changesets.removeLast();

	m_changesets << m_currentArchive;
	m_currentArchive.clear();
	setPos (pos() + 1);
	updateActions();
}

// =============================================================================
// -----------------------------------------------------------------------------
void History::add (AbstractHistoryEntry* entry)
{	if (!opened())
	{	delete entry;
		return;
	}

	entry->setParent (this);
	m_currentArchive << entry;
}

// =============================================================================
// -----------------------------------------------------------------------------
void AddHistory::undo() const
{	LDFile* f = parent()->file();
	LDObject* obj = f->getObject (index());
	f->forgetObject (obj);
	delete obj;

	g_fullRefresh = true;
}

// =============================================================================
// -----------------------------------------------------------------------------
void AddHistory::redo() const
{	LDFile* f = parent()->file();
	LDObject* obj = parseLine (code());
	f->insertObj (index(), obj);
	g_win->R()->compileObject (obj);
}

AddHistory::~AddHistory() {}

// =============================================================================
// heh
// -----------------------------------------------------------------------------
void DelHistory::undo() const
{	LDFile* f = parent()->file();
	LDObject* obj = parseLine (code());
	f->insertObj (index(), obj);
	g_win->R()->compileObject (obj);
}

// =============================================================================
// -----------------------------------------------------------------------------
void DelHistory::redo() const
{	LDFile* f = parent()->file();
	LDObject* obj = f->getObject (index());
	f->forgetObject (obj);
	delete obj;

	g_fullRefresh = true;
}

DelHistory::~DelHistory() {}

// =============================================================================
// -----------------------------------------------------------------------------
void EditHistory::undo() const
{	LDObject* obj = LDFile::current()->getObject (index());
	LDObject* newobj = parseLine (oldCode());
	obj->replace (newobj);
	g_win->R()->compileObject (newobj);
}

// =============================================================================
// -----------------------------------------------------------------------------
void EditHistory::redo() const
{	LDObject* obj = LDFile::current()->getObject (index());
	LDObject* newobj = parseLine (newCode());
	obj->replace (newobj);
	g_win->R()->compileObject (newobj);
}

EditHistory::~EditHistory() {}

// =============================================================================
// -----------------------------------------------------------------------------
void SwapHistory::undo() const
{	LDObject::fromID (a)->swap (LDObject::fromID (b));
}

void SwapHistory::redo() const
{	undo(); // :v
}

SwapHistory::~SwapHistory() {}
