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

#include "documentPointer.h"
#include "../document.h"

LDDocumentPointer::LDDocumentPointer()  : m_Pointer (null) {}


// =============================================================================
// -----------------------------------------------------------------------------
LDDocumentPointer::LDDocumentPointer (LDDocument* ptr) :
	m_Pointer (ptr)
{	addReference ();
}

// =============================================================================
// -----------------------------------------------------------------------------
LDDocumentPointer::LDDocumentPointer (const LDDocumentPointer& other) :
	m_Pointer (other.getPointer())
{	addReference ();
}

// =============================================================================
// -----------------------------------------------------------------------------
LDDocumentPointer::~LDDocumentPointer()
{	removeReference();
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocumentPointer::addReference()
{	if (getPointer() != null)
		getPointer()->addReference (this);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDDocumentPointer::removeReference()
{	if (getPointer() != null)
		getPointer()->removeReference (this);
}

// =============================================================================
// -----------------------------------------------------------------------------
LDDocumentPointer& LDDocumentPointer::operator= (LDDocument* ptr)
{	if (ptr != getPointer())
	{	removeReference();
		setPointer (ptr);
		addReference();
	}

	return *this;
}