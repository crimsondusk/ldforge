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

#include "RingFinder.h"
#include "../Misc.h"

RingFinder g_RingFinder;

// =============================================================================
// This is the main algorithm of the ring finder. It tries to use math to find
// the one ring between r0 and r1. If it fails (the ring number is non-integral),
// it finds an intermediate radius (ceil of the ring number times scale) and
// splits the radius at this point, calling this function again to try find the
// rings between r0 - r and r - r1.
//
// This does not always yield into usable results. If at some point r == r0 or
// r == r1, there is no hope of finding the rings, at least with this algorithm,
// as it would fall into an infinite recursion.
//
bool RingFinder::findRingsRecursor (double r0, double r1, Solution& currentSolution)
{
	// Don't recurse too deep.
	if (m_stack >= 5)
		return false;

	// Find the scale and number of a ring between r1 and r0.
	assert (r1 >= r0);
	double scale = r1 - r0;
	double num = r0 / scale;

	// If the ring number is integral, we have found a fitting ring to r0 -> r1!
	if (isInteger (num))
	{
		Component cmp;
		cmp.scale = scale;
		cmp.num = (int) round (num);
		currentSolution.addComponent (cmp);

		// If we're still at the first recursion, this is the only
		// ring and there's nothing left to do. Guess we found the winner.
		if (m_stack == 0)
		{
			m_solutions.push_back (currentSolution);
			return true;
		}
	}
	else
	{
		// Try find solutions by splitting the ring in various positions.
		if (isZero (r1 - r0))
			return false;

		double interval;

		// Determine interval. The smaller delta between radii, the more precise
		// interval should be used. We can't really use a 0.5 increment when
		// calculating rings to 10 -> 105... that would take ages to process!
		if (r1 - r0 < 0.5)
			interval = 0.1;
		else if (r1 - r0 < 10)
			interval = 0.5;
		else if (r1 - r0 < 50)
			interval = 1;
		else
			interval = 5;

		// Now go through possible splits and try find rings for both segments.
		for (double r = r0 + interval; r < r1; r += interval)
		{
			Solution sol = currentSolution;

			m_stack++;
			bool res = findRingsRecursor (r0, r, sol) && findRingsRecursor (r, r1, sol);
			m_stack--;

			if (res)
			{
				// We succeeded in finding radii for this segment. If the stack is 0, this
				// is the first recursion to this function. Thus there are no more ring segments
				// to process and we can add the solution.
				//
				// If not, when this function ends, it will be called again with more arguments.
				// Accept the solution to this segment by setting currentSolution to sol, and
				// return true to continue processing.
				if (m_stack == 0)
					m_solutions.push_back (sol);
				else
				{
					currentSolution = sol;
					return true;
				}
			}
		}

		return false;
	}

	return true;
}

// =============================================================================
// Main function. Call this with r0 and r1. If this returns true, use bestSolution
// for the solution that was presented.
//
bool RingFinder::findRings (double r0, double r1)
{
	m_solutions.clear();
	Solution sol;

	// Recurse in and try find solutions.
	findRingsRecursor (r0, r1, sol);

	// Compare the solutions and find the best one. The solution class has an operator>
	// overload to compare two solutions.
	m_bestSolution = null;

	for (QVector<Solution>::iterator solp = m_solutions.begin(); solp != m_solutions.end(); ++solp)
	{
		const Solution& sol = *solp;

		if (m_bestSolution == null || sol.isBetterThan (m_bestSolution))
			m_bestSolution = &sol;
	}

	return (m_bestSolution != null);
}

// =============================================================================
//
bool RingFinder::Solution::isBetterThan (const Solution* other) const
{
	// If this solution has less components than the other one, this one
	// is definitely better.
	if (getComponents().size() < other->getComponents().size())
		return true;

	// vice versa
	if (other->getComponents().size() < getComponents().size())
		return false;

	// Calculate the maximum ring number. Since the solutions have equal
	// ring counts, the solutions with lesser maximum rings should result
	// in cleaner code and less new primitives, right?
	int maxA = 0,
		maxB = 0;

	for (int i = 0; i < getComponents().size(); ++i)
	{
		maxA = max (getComponents()[i].num, maxA);
		maxB = max (other->getComponents()[i].num, maxB);
	}

	if (maxA < maxB)
		return true;

	if (maxB < maxA)
		return false;

	// Solutions have equal rings and equal maximum ring numbers. Let's
	// just say this one is better, at this point it does not matter which
	// one is chosen.
	return true;
}