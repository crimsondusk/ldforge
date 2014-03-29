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

#include "../main.h"

class LDSubfile;
class LDDocument;

//!
//! \brief A reference-counting pointer to LDDocument.
//!
//! The LDDocumentPointer class defines a reference-counting pointer which
//! points to LDDocument.
//!
class LDDocumentPointer
{
	PROPERTY (private, LDDocument*, pointer, setPointer, STOCK_WRITE)

	public:
		//! Constructs a null LDDocumentPointer
		LDDocumentPointer();

		//! Constructs a document pointer with the given pointer
		LDDocumentPointer (LDDocument* ptr);

		//! Copy-constructs a LDDocumentPointer.
		LDDocumentPointer (const LDDocumentPointer& other);

		//! Destructs the pointer.
		~LDDocumentPointer();

		//! \param ptr the new pointer to change to.
		LDDocumentPointer& operator= (LDDocument* ptr);

		//! Copy operator.
		//! \param other the pointer whose internal pointer to copy.
		inline LDDocumentPointer& operator= (LDDocumentPointer& other)
		{
			return operator= (other.pointer());
		}

		//! Operator overload for a->b support.
		inline LDDocument* operator->() const
		{
			return pointer();
		}

		//! Cast operator overload
		inline operator LDDocument*() const
		{
			return pointer();
		}

	private:
		void addReference();
		void removeReference();
};
