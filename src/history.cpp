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

EXTERN_ACTION (undo)
EXTERN_ACTION (redo)

History::History () {
	setPos (-1);
}

void History::undo () {
	
}

void History::redo () {
	
}

void History::clear () {
	
}

void History::updateActions () {
	ACTION (undo)->setEnabled (false);
	ACTION (redo)->setEnabled (false);
}

