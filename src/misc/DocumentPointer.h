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

#include "../Main.h"

class LDSubfile;
class LDDocument;

class LDDocumentPointer
{
	PROPERTY (private, LDDocument*, pointer, setPointer, STOCK_WRITE)

	public:
		LDDocumentPointer();
		LDDocumentPointer (LDDocument* ptr);
		LDDocumentPointer (const LDDocumentPointer& other);
		~LDDocumentPointer();
		LDDocumentPointer& operator= (LDDocument* ptr);

		inline LDDocumentPointer& operator= (LDDocumentPointer& other)
		{
			return operator= (other.pointer());
		}

		inline LDDocument* operator->() const
		{
			return pointer();
		}

		inline operator LDDocument*() const
		{
			return pointer();
		}

	private:
		void addReference();
		void removeReference();
};

