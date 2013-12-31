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

#ifndef LDFORGE_MISC_RINGFINDER_H
#define LDFORGE_MISC_RINGFINDER_H

#include "../main.h"

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

#endif // LDFORGE_MISC_RINGFINDER_H