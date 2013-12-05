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

#ifndef LDFORGE_MISC_H
#define LDFORGE_MISC_H

#include <QVector>
#include "config.h"
#include "common.h"
#include "types.h"

#define NUM_PRIMES 500

class QColor;
class QAction;

// Prime numbers
extern const int g_primes[NUM_PRIMES];

// Returns whether a given string represents a floating point number.
bool numeric (const str& tok);

// Simplifies the given fraction.
void simplify (int& numer, int& denom);

str join (initlist<StringFormatArg> vals, str delim = " ");

// Grid stuff
struct gridinfo
{	const char* const name;
	FloatConfig* const confs[4];
};

extern_cfg (Int, grid);
static const int g_NumGrids = 3;
extern const gridinfo g_GridInfo[3];

inline const gridinfo& currentGrid()
{	return g_GridInfo[grid];
}

// =============================================================================
enum RotationPoint
{	ObjectOrigin,
	WorldOrigin,
	CustomPoint
};

vertex rotPoint (const QList<LDObject*>& objs);
void configRotationPoint();

// =============================================================================
namespace Grid
{	enum Type
	{	Coarse,
		Medium,
		Fine
	};

	enum Config
	{	X,
		Y,
		Z,
		Angle
	};

	double snap (double value, const Grid::Config axis);
}

// =============================================================================
// RingFinder
//
// Provides an algorithm for finding a solution of rings between radii r0 and r1.
// =============================================================================
class RingFinder
{	public:
		struct Component
		{	int num;
			double scale;
		};

		class Solution
		{	public:
				// Components of this solution
				inline const QVector<Component>& getComponents() const
				{	return m_components;
				}

				// Add a component to this solution
				inline void addComponent (const Component& a)
				{	m_components.push_back (a);
				}

				// Compare solutions
				bool operator> (const Solution& other) const;

			private:
				QVector<Component> m_components;
		};

		RingFinder() {}
		bool findRings (double r0, double r1);

		inline const Solution* bestSolution()
		{	return m_bestSolution;
		}

		inline const QVector<Solution>& allSolutions() const
		{	return m_solutions;
		}

		inline bool operator() (double r0, double r1)
		{	return findRings (r0, r1);
		}

	private:
		QVector<Solution> m_solutions;
		const Solution*   m_bestSolution;
		int               m_stack;

		bool findRingsRecursor (double r0, double r1, Solution& currentSolution);
};

extern RingFinder g_RingFinder;

// =============================================================================
template<class T> void dataswap (T& a, T& b)
{	T c = a;
	a = b;
	b = c;
}

// -----------------------------------------------------------------------------
// Plural expression
template<class T> static inline const char* plural (T n)
{	return (n != 1) ? "s" : "";
}

// -----------------------------------------------------------------------------
// Templated clamp
template<class T> static inline T clamp (T a, T min, T max)
{	return (a > max) ? max : (a < min) ? min : a;
}

// Templated minimum
template<class T> static inline T min (T a, T b)
{	return (a < b) ? a : b;
}

// Templated maximum
template<class T> static inline T max (T a, T b)
{	return (a > b) ? a : b;
}

// Templated absolute value
template<class T> static inline T abs (T a)
{	return (a >= 0) ? a : -a;
}

template<class T> inline bool isZero (T a)
{	return abs<T> (a) < 0.0001;
}

template<class T> inline bool isInteger (T a)
{	return isZero (a - (int) a);
}

template<class T> void removeDuplicates (QList<T>& a)
{	std::sort (a.begin(), a.end());
	typename QList<T>::iterator pos = std::unique (a.begin(), a.end());
	a.erase (pos, a.end());
}

#endif // LDFORGE_MISC_H
