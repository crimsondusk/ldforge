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

//!
//! \brief Provides an algorithm for finding solutions of rings between given radii.
//!
//! The RingFinder is a class which implements a ring finding algorithm. It is passed
//! two radii and it tries to find solutions of rings that would fill the given space.
//!
//! \note It is not fool-proof and does not always yield a solution, never assume
//! \note that one is a available as none is guaranteed.
//!
class RingFinder
{
	public:
		//! A single component in a solution
		struct Component
		{
			int num;
			double scale;
		};

		//! A solution whose components would fill the desired ring space.
		class Solution
		{
			public:
				//! \returns components of this solution
				inline const QVector<Component>& getComponents() const
				{
					return m_components;
				}

				//! Add a component to this solution
				inline void addComponent (const Component& a)
				{
					m_components.push_back (a);
				}

				//! \brief Compare solutions.
				//!
				//! Compares this solution with \c other and determines which
				//! one is superior.
				//!
				//! A solution is considered superior if solution has less
				//! components than the other one. If both solution have an
				//! equal amount components, the solution with a lesser maximum
				//! ring number is found superior, as such solutions should
				//! yield less new primitives and cleaner definitions.
				//!
				//! The solution which is found superior to every other solution
				//! will be the one returned by \c RingFinder::bestSolution().
				//!
				//! \param other the solution to check against
				//! \returns whether this solution is considered superior
				//! \returns to \c other.
				//!
				bool isSuperiorTo (const Solution* other) const;

			private:
				QVector<Component> m_components;
		};

		//! Constructs a ring finder.
		RingFinder() {}

		//! \brief Tries to find rings between \c r0 and \c r1.
		//!
		//! This is the main algorithm of the ring finder. It tries to use math
		//! to find the one ring between r0 and r1. If it fails (the ring number
		//! is non-integral), it finds an intermediate radius (ceil of the ring
		//! number times scale) and splits the radius at this point, calling this
		//! function again to try find the rings between r0 - r and r - r1.
		//!
		//! This does not always yield into usable results. If at some point r ==
		//! r0 or r == r1, there is no hope of finding the rings, at least with
		//! this algorithm, as it would fall into an infinite recursion.
		//!
		//! \param r0 the lower radius of the ring space
		//! \param r1 the higher radius of the ring space
		//! \returns whether it was possible to find a solution for the given
		//! \returns ring space.
		//!
		bool findRings (double r0, double r1);

		//! \returns the solution that was considered best. Returns \c null
		//! \returns if no suitable solution was found.
		//! \see \c RingFinder::Solution::isSuperiorTo()
		inline const Solution* bestSolution()
		{
			return m_bestSolution;
		}

		//! \returns all found solutions. The list is empty if no solutions
		//! \returns were found.
		inline const QVector<Solution>& allSolutions() const
		{
			return m_solutions;
		}

	private:
		QVector<Solution> m_solutions;
		const Solution*   m_bestSolution;
		int               m_stack;

		//! Helper function for \c findRings
		bool findRingsRecursor (double r0, double r1, Solution& currentSolution);
};

extern RingFinder g_RingFinder;

