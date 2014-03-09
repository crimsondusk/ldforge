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
#include "Main.h"
#include "Types.h"
#include <QRegExp>
#include <QDialog>

class LDDocument;
class Ui_MakePrimUI;
class PrimitiveCategory;
struct Primitive
{
	QString				name,
						title;
	PrimitiveCategory*	category;
};

class PrimitiveCategory : public QObject
{
	Q_OBJECT
	PROPERTY (public, QString, name, setName, STOCK_WRITE)

	public:
		enum RegexType
		{
			EFilenameRegex,
			ETitleRegex
		};

		struct RegexEntry
		{
			QRegExp		regex;
			RegexType	type;
		};

		QList<RegexEntry> regexes;
		QList<Primitive> prims;

		explicit PrimitiveCategory (QString name, QObject* parent = 0);
		bool isValidToInclude();

		static void loadCategories();
		static void populateCategories();
};

// =============================================================================
//
// PrimitiveScanner
//
// Worker object that scans the primitives folder for primitives and
// builds an index of them.
//
class PrimitiveScanner : public QObject
{
	Q_OBJECT

	public:
		explicit			PrimitiveScanner (QObject* parent = 0);
		virtual				~PrimitiveScanner();
		static void			start();

	public slots:
		void				work();

	signals:
		void				starting (int num);
		void				workDone();
		void				update (int i);

	private:
		QList<Primitive>	m_prims;
		QStringList			m_files;
		int					m_i;
		int					m_baselen;
};

extern QList<PrimitiveCategory*> g_PrimitiveCategories;

void loadPrimitives();
PrimitiveScanner* getActivePrimitiveScanner();

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
class PrimitivePrompt : public QDialog
{
	Q_OBJECT

	public:
		explicit PrimitivePrompt (QWidget* parent = null, Qt::WindowFlags f = 0);
		virtual ~PrimitivePrompt();
		Ui_MakePrimUI* ui;

	public slots:
		void hiResToggled (bool on);
};

void makeCircle (int segs, int divs, double radius, QList<QLineF>& lines);
LDDocument* generatePrimitive (PrimitiveType type, int segs, int divs, int num);

// Gets a primitive by the given specs. If the primitive cannot be found, it will
// be automatically generated.
LDDocument* getPrimitive (PrimitiveType type, int segs, int divs, int num);

QString radialFileName (PrimitiveType type, int segs, int divs, int num);
