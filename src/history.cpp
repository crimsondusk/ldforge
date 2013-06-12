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

EXTERN_ACTION (undo)
EXTERN_ACTION (redo)

History::History () {
	setPos (-1);
}

void History::undo () {
	if (m_changesets.size () == 0 || pos () == -1)
		return;
	
	const list& set = changeset (pos ());
	
	// Iterate the list in reverse and undo all actions
	for (const AbstractHistoryEntry* change : c_rev<AbstractHistoryEntry*> (set))
		change->undo ();
	
	setPos (pos () - 1);
	g_win->refresh ();
	updateActions ();
}

void History::redo () {
	if (pos () == (long) m_changesets.size ())
		return;
	
	const list& set = changeset (pos () + 1);
	
	// Redo things - in the order as they were done in the first place
	for (const AbstractHistoryEntry* change : set)
		change->redo ();
	
	setPos (pos () + 1);
	g_win->refresh ();
	updateActions ();
}

void History::clear () {
	
}

void History::updateActions () const {
	ACTION (undo)->setEnabled (pos () != -1);
	ACTION (redo)->setEnabled (pos () < (long) m_changesets.size () - 1);
}

void History::open () {
	if (opened ())
		return;
	
	setOpened (true);
}

void History::close () {
	if (!opened ())
		return;
	
	setOpened (false);
	
	if (m_currentArchive.size () == 0)
		return;
	
	while (pos () < size () - 1)
		m_changesets.erase (size () - 1);
	
	m_changesets << m_currentArchive;
	m_currentArchive.clear ();
	setPos (pos () + 1);
	updateActions ();
}

void History::add (AbstractHistoryEntry* entry) {
	if (!opened ()) {
		delete entry;
		return;
	}
	
	entry->setParent (this);
	m_currentArchive << entry;
}

// =============================================================================
void AddHistory::undo () const {
	LDOpenFile* f = parent ()->file ();
	LDObject* obj = f->object (index ());
	f->forgetObject (obj);
	delete obj;
}

void AddHistory::redo () const {
	LDOpenFile* f = parent ()->file ();
	LDObject* obj = parseLine (code ());
	f->insertObj (index (), obj);
	g_win->R ()->compileObject (obj);
}

AddHistory::~AddHistory () {}

// =============================================================================
// heh
void DelHistory::undo () const {
	LDOpenFile* f = parent ()->file ();
	LDObject* obj = parseLine (code ());
	f->insertObj (index (), obj);
	g_win->R ()->compileObject (obj);
}

void DelHistory::redo () const {
	LDOpenFile* f = parent ()->file ();
	LDObject* obj = f->object (index ());
	f->forgetObject (obj);
	delete obj;
}

DelHistory::~DelHistory () {}