/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri `arezey` Piippo
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

#ifndef HISTORY_H
#define HISTORY_H

#include "common.h"

#define IMPLEMENT_HISTORY_TYPE(N) \
	virtual HistoryType_e getType () const { \
		return HISTORY_##N; \
	} \

enum HistoryType_e {
	HISTORY_Delete,
};

class HistoryEntry {
public:
	virtual void undo () {}
	virtual void redo () {}
	virtual ~HistoryEntry () {}
	
	virtual HistoryType_e getType () const {
		return (HistoryType_e)(0);
	};
};

class DeleteHistory : public HistoryEntry {
public:
	IMPLEMENT_HISTORY_TYPE (Delete)
	
	vector<ulong> indices;
	vector<LDObject*> cache;
	
	DeleteHistory (vector<ulong> indices, vector<LDObject*> cache) :
		indices (indices), cache (cache) {}
	virtual ~DeleteHistory ();
	virtual void undo ();
	virtual void redo ();
};

namespace History {
	extern std::vector<HistoryEntry*> entries;
	
	void addEntry (HistoryEntry* entry);
	void undo ();
	void redo ();
};

#endif // HISTORY_H