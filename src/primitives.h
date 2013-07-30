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
#include <QDialog>

class Ui_MakePrimUI;
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
	
	typedef List<RegexEntry>::it it;
	typedef List<RegexEntry>::c_it c_it;
	
	List<RegexEntry> regexes;
	List<Primitive> prims;
	static List<Primitive> uncat;
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
	List<Primitive> m_prims;
};

extern List<PrimitiveCategory> g_PrimitiveCategories;

void loadPrimitives ();
bool primitiveLoaderBusy ();

enum PrimitiveType
{
	Circle,
	Cylinder,
	Disc,
	DiscNeg,
	Ring,
	Cone,
};

// =============================================================================
class PrimitivePrompt : public QDialog {
	Q_OBJECT
	
public:
	explicit PrimitivePrompt( QWidget* parent = null, Qt::WindowFlags f = 0 );
	virtual ~PrimitivePrompt();
	Ui_MakePrimUI* ui;
	
public slots:
	void hiResToggled( bool on );
};

void generatePrimitive();

#endif // PRIMITIVES_H