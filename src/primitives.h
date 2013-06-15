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

#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include "common.h"
#include "types.h"
#include <QRegExp>

class PrimitiveCategory;
struct Primitive {
	str name, title;
	PrimitiveCategory* cat;
};

class PrimitiveCategory {
	PROPERTY (str, name, setName)
	
public:
	enum Type {
		Filename,
		Title
	};
	
	struct RegexEntry {
		QRegExp regex;
		Type type;
	};
	
	typedef vector<RegexEntry>::it it;
	typedef vector<RegexEntry>::c_it c_it;
	
	vector<RegexEntry> regexes;
	vector<Primitive> prims;
	static vector<Primitive> uncat;
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// PrimitiveLister
// 
// Worker object that scans the primitives folder for primitives and
// builds an index of them.
// =============================================================================
class PrimitiveLister : public QObject {
	Q_OBJECT
	
public:
	static void start ();
	
public slots:
	void work ();
	
signals:
	void starting (ulong num);
	void workDone ();
	void update (ulong i);
	
private:
	vector<Primitive> m_prims;
};

extern vector<PrimitiveCategory> g_PrimitiveCategories;

void loadPrimitives ();

#endif // PRIMITIVES_H