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

#ifndef LDFORGE_MISC_H
#define LDFORGE_MISC_H

#include <QVector>
#include "config.h"
#include "main.h"
#include "types.h"

#define NUM_PRIMES 500

class LDDocument;
class QColor;
class QAction;

// Prime numbers
extern const int g_primes[NUM_PRIMES];

// Returns whether a given string represents a floating point number.
bool numeric (const str& tok);

// Simplifies the given fraction.
void simplify (int& numer, int& denom);

void roundToDecimals (double& a, int decimals);

str join (initlist<StringFormatArg> vals, str delim = " ");

// Grid stuff
struct gridinfo
{
	const char* const	name;
	float* const			confs[4];
};

extern_cfg (Int, grid);
static const int g_NumGrids = 3;
extern const gridinfo g_GridInfo[3];

inline const gridinfo& currentGrid()
{
	return g_GridInfo[grid];
}

// =============================================================================
enum RotationPoint
{
	ObjectOrigin,
	WorldOrigin,
	CustomPoint
};

Vertex rotPoint (const QList<LDObject*>& objs);
void configRotationPoint();

// =============================================================================
namespace Grid
{
	enum Type
	{
		Coarse,
		Medium,
		Fine
	};

	enum Config
	{
		X,
		Y,
		Z,
		Angle
	};

	double snap (double value, const Grid::Config axis);
}

// -----------------------------------------------------------------------------
class InvokationDeferer : public QObject
{
	Q_OBJECT

	public:
		using FunctionType = void(*)();

		explicit InvokationDeferer (QObject* parent = 0);
		void addFunctionCall (FunctionType func);

	signals:
		void functionAdded();

	private:
		QList<FunctionType>	m_funcs;

	private slots:
		void invokeFunctions();
};

void invokeLater (InvokationDeferer::FunctionType func);

// -----------------------------------------------------------------------------
// Plural expression
template<class T> static inline const char* plural (T n)
{
	return (n != 1) ? "s" : "";
}

// -----------------------------------------------------------------------------
// Templated clamp
template<class T> static inline T clamp (T a, T min, T max)
{
	return (a > max) ? max : (a < min) ? min : a;
}

// Templated minimum
template<class T> static inline T min (T a, T b)
{
	return (a < b) ? a : b;
}

// Templated maximum
template<class T> static inline T max (T a, T b)
{
	return (a > b) ? a : b;
}

// Templated absolute value
template<class T> static inline T abs (T a)
{
	return (a >= 0) ? a : -a;
}

template<class T> inline bool isZero (T a)
{
	return abs<T> (a) < 0.0001;
}

template<class T> inline bool isInteger (T a)
{
	return isZero (a - (int) a);
}

template<class T> void removeDuplicates (QList<T>& a)
{
	std::sort (a.begin(), a.end());
	a.erase (std::unique (a.begin(), a.end()), a.end());
}

#endif // LDFORGE_MISC_H
