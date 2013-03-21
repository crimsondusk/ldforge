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

#ifndef __BBOX_H__
#define __BBOX_H__

#include "common.h"

// =============================================================================
// The bounding box, bbox for short, is the
// box that encompasses the model we have open.
// v0 is the minimum vertex, v1 is the maximum vertex.
// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class bbox {
public:
	vertex v0, v1;
	
	bbox ();
	
	void reset ();
	void calculate ();
	double calcSize ();
	
private:
	void checkVertex (vertex v);
};

#endif