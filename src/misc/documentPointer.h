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

#ifndef LDFORGE_DOCUMENT_POINTER_H
#define LDFORGE_DOCUMENT_POINTER_H

#include "../main.h"

class LDSubfile;
class LDDocument;

class LDDocumentPointer
{
	PROPERTY (private, LDDocument*,			Pointer,	NO_OPS, STOCK_WRITE)

	public:
		LDDocumentPointer();
		LDDocumentPointer (LDDocument* ptr);
		LDDocumentPointer (const LDDocumentPointer& other);
		~LDDocumentPointer();
		LDDocumentPointer& operator= (LDDocument* ptr);

		inline LDDocumentPointer& operator= (LDDocumentPointer& other)
		{
			return operator= (other.getPointer());
		}

		inline LDDocument* operator->() const
		{
			return getPointer();
		}

		inline operator LDDocument*() const
		{
			return getPointer();
		}

	private:
		void addReference();
		void removeReference();
};

#endif // LDFORGE_DOCUMENT_POINTER_H