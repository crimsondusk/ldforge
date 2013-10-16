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

#include <math.h>
#include <locale.h>
#include <QColor>
#include "common.h"
#include "misc.h"
#include "gui.h"
#include "dialogs.h"
#include "ui_rotpoint.h"

RingFinder g_RingFinder;

// Prime number table.
const int g_primes[NUM_PRIMES] =
{	2,    3,    5,    7,   11,   13,   17,   19,   23,   29,
	31,   37,   41,   43,   47,   53,   59,   61,   67,   71,
	73,   79,   83,   89,   97,  101,  103,  107,  109,  113,
	127,  131,  137,  139,  149,  151,  157,  163,  167,  173,
	179,  181,  191,  193,  197,  199,  211,  223,  227,  229,
	233,  239,  241,  251,  257,  263,  269,  271,  277,  281,
	283,  293,  307,  311,  313,  317,  331,  337,  347,  349,
	353,  359,  367,  373,  379,  383,  389,  397,  401,  409,
	419,  421,  431,  433,  439,  443,  449,  457,  461,  463,
	467,  479,  487,  491,  499,  503,  509,  521,  523,  541,
	547,  557,  563,  569,  571,  577,  587,  593,  599,  601,
	607,  613,  617,  619,  631,  641,  643,  647,  653,  659,
	661,  673,  677,  683,  691,  701,  709,  719,  727,  733,
	739,  743,  751,  757,  761,  769,  773,  787,  797,  809,
	811,  821,  823,  827,  829,  839,  853,  857,  859,  863,
	877,  881,  883,  887,  907,  911,  919,  929,  937,  941,
	947,  953,  967,  971,  977,  983,  991,  997, 1009, 1013,
	1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069,
	1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151,
	1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223,
	1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 1291,
	1297, 1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 1373,
	1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451,
	1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511,
	1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583,
	1597, 1601, 1607, 1609, 1613, 1619, 1621, 1627, 1637, 1657,
	1663, 1667, 1669, 1693, 1697, 1699, 1709, 1721, 1723, 1733,
	1741, 1747, 1753, 1759, 1777, 1783, 1787, 1789, 1801, 1811,
	1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877, 1879, 1889,
	1901, 1907, 1913, 1931, 1933, 1949, 1951, 1973, 1979, 1987,
	1993, 1997, 1999, 2003, 2011, 2017, 2027, 2029, 2039, 2053,
	2063, 2069, 2081, 2083, 2087, 2089, 2099, 2111, 2113, 2129,
	2131, 2137, 2141, 2143, 2153, 2161, 2179, 2203, 2207, 2213,
	2221, 2237, 2239, 2243, 2251, 2267, 2269, 2273, 2281, 2287,
	2293, 2297, 2309, 2311, 2333, 2339, 2341, 2347, 2351, 2357,
	2371, 2377, 2381, 2383, 2389, 2393, 2399, 2411, 2417, 2423,
	2437, 2441, 2447, 2459, 2467, 2473, 2477, 2503, 2521, 2531,
	2539, 2543, 2549, 2551, 2557, 2579, 2591, 2593, 2609, 2617,
	2621, 2633, 2647, 2657, 2659, 2663, 2671, 2677, 2683, 2687,
	2689, 2693, 2699, 2707, 2711, 2713, 2719, 2729, 2731, 2741,
	2749, 2753, 2767, 2777, 2789, 2791, 2797, 2801, 2803, 2819,
	2833, 2837, 2843, 2851, 2857, 2861, 2879, 2887, 2897, 2903,
	2909, 2917, 2927, 2939, 2953, 2957, 2963, 2969, 2971, 2999,
	3001, 3011, 3019, 3023, 3037, 3041, 3049, 3061, 3067, 3079,
	3083, 3089, 3109, 3119, 3121, 3137, 3163, 3167, 3169, 3181,
	3187, 3191, 3203, 3209, 3217, 3221, 3229, 3251, 3253, 3257,
	3259, 3271, 3299, 3301, 3307, 3313, 3319, 3323, 3329, 3331,
	3343, 3347, 3359, 3361, 3371, 3373, 3389, 3391, 3407, 3413,
	3433, 3449, 3457, 3461, 3463, 3467, 3469, 3491, 3499, 3511,
	3517, 3527, 3529, 3533, 3539, 3541, 3547, 3557, 3559, 3571,
};

// =============================================================================
// -----------------------------------------------------------------------------
// Grid stuff
cfg (Int, grid, Grid::Medium);

cfg (Float, grid_coarse_x,			5.0f);
cfg (Float, grid_coarse_y,			5.0f);
cfg (Float, grid_coarse_z,			5.0f);
cfg (Float, grid_coarse_angle,	45.0f);
cfg (Float, grid_medium_x,			1.0f);
cfg (Float, grid_medium_y,			1.0f);
cfg (Float, grid_medium_z,			1.0f);
cfg (Float, grid_medium_angle,	22.5f);
cfg (Float, grid_fine_x,			0.1f);
cfg (Float, grid_fine_y,			0.1f);
cfg (Float, grid_fine_z,			0.1f);
cfg (Float, grid_fine_angle,		7.5f);
cfg (Int, edit_rotpoint, 0);
cfg (Float, edit_rotpoint_x, 0.0f); // TODO: make a VertexConfig and use it here
cfg (Float, edit_rotpoint_y, 0.0f);
cfg (Float, edit_rotpoint_z, 0.0f);

const gridinfo g_GridInfo[3] =
{	{ "Coarse", { &grid_coarse_x, &grid_coarse_y, &grid_coarse_z, &grid_coarse_angle }},
	{ "Medium", { &grid_medium_x, &grid_medium_y, &grid_medium_z, &grid_medium_angle }},
	{ "Fine",   { &grid_fine_x,   &grid_fine_y,   &grid_fine_z,   &grid_fine_angle   }}
};

// =============================================================================
// Snap the given coordinate value on the current grid's given axis.
// -----------------------------------------------------------------------------
double Grid::snap (double in, const Grid::Config axis)
{	const double gridval = currentGrid().confs[axis]->value;
	const long mult = abs (in / gridval);
	const bool neg = (in < 0);
	double out = mult * gridval;

	if (abs<double> (in) - (mult * gridval) > gridval / 2)
		out += gridval;

	if (neg && out != 0)
		out *= -1;

	return out;
}

// =============================================================================
// Float to string. Removes trailing zeroes and is locale-independant.
// TODO: Replace with QString::number()
// -----------------------------------------------------------------------------
str ftoa (double num)
{	// Disable the locale first so that the decimal point will not
	// turn into anything weird (like commas)
	setlocale (LC_NUMERIC, "C");

	str rep;
	rep.sprintf ("%f", num);

	// Remove trailing zeroes
	while (rep.right (1) == "0")
		rep.chop (1);

	// If there were only zeroes in the decimal place, remove
	// the decimal point now.
	if (rep.right (1) == ".")
		rep.chop (1);

	return rep;
}

// =============================================================================
// TODO: I guess Qt must have something like this stashed somewhere?
// -----------------------------------------------------------------------------
bool isNumber (const str& tok)
{	bool gotDot = false;

	for (int i = 0; i < tok.length(); ++i)
	{	const qchar c = tok[i];

		// Allow leading hyphen for negatives
		if (i == 0 && c == '-')
			continue;

		// Check for decimal point
		if (!gotDot && c == '.')
		{	gotDot = true;
			continue;
		}

		if (c >= '0' && c <= '9')
			continue; // Digit

		// If the above cases didn't catch this character, it was
		// illegal and this is therefore not a number.
		return false;
	}

	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
void simplify (int& numer, int& denom)
{	bool repeat;

	do
	{	repeat = false;

		for (int x = 0; x < NUM_PRIMES; x++)
		{	const int prime = g_primes[NUM_PRIMES - x - 1];

			if (numer <= prime || denom <= prime)
				continue;

			if ( (numer % prime == 0) && (denom % prime == 0))
			{	numer /= prime;
				denom /= prime;
				repeat = true;
				break;
			}
		}
	}
	while (repeat);
}

// =============================================================================
// -----------------------------------------------------------------------------
vertex rotPoint (const List<LDObject*>& objs)
{	LDBoundingBox box;

	switch (edit_rotpoint)
	{	case ObjectOrigin:

			// Calculate center vertex
		for (LDObject * obj : objs)
				if (obj->hasMatrix())
					box << dynamic_cast<LDMatrixObject*> (obj)->position();
				else
					box << obj;

			return box.center();

		case WorldOrigin:
			return g_origin;

		case CustomPoint:
			return vertex (edit_rotpoint_x, edit_rotpoint_y, edit_rotpoint_z);
	}

	return vertex();
}

// =============================================================================
// -----------------------------------------------------------------------------
void configRotationPoint()
{	QDialog* dlg = new QDialog;
	Ui::RotPointUI ui;
	ui.setupUi (dlg);

	switch (edit_rotpoint)
	{	case ObjectOrigin:
			ui.objectPoint->setChecked (true);
			break;

		case WorldOrigin:
			ui.worldPoint->setChecked (true);
			break;

		case CustomPoint:
			ui.customPoint->setChecked (true);
			break;
	}

	ui.customX->setValue (edit_rotpoint_x);
	ui.customY->setValue (edit_rotpoint_y);
	ui.customZ->setValue (edit_rotpoint_z);

	if (!dlg->exec())
		return;

	edit_rotpoint =
		(ui.objectPoint->isChecked()) ? ObjectOrigin :
		(ui.worldPoint->isChecked())  ? WorldOrigin :
		CustomPoint;

	edit_rotpoint_x = ui.customX->value();
	edit_rotpoint_y = ui.customY->value();
	edit_rotpoint_z = ui.customZ->value();
}

// =============================================================================
// -----------------------------------------------------------------------------
str join (initlist<StringFormatArg> vals, str delim)
{	QStringList list;

	for (const StringFormatArg& arg : vals)
		list << arg.value();

	return list.join (delim);
}

// =============================================================================
// TODO: I'm quite sure Qt has this covered as well.
// -----------------------------------------------------------------------------
double atof (str val)
{	// Disable the locale while parsing the line or atof's behavior changes
	// between locales (i.e. fails to read decimals properly). That is
	// quite undesired...
	setlocale (LC_NUMERIC, "C");

	char* buf = new char[val.length()];
	char* bufptr = &buf[0];

	for (QChar& c : val)
		*bufptr++ = c.toLatin1();

	*bufptr = '\0';

	double fval = atof (buf);
	delete[] buf;
	return fval;
}

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
// -----------------------------------------------------------------------------
bool RingFinder::findRings (double r0, double r1)
{	m_solution.clear();
	return findRingsRecursor (r0, r1);
}

bool RingFinder::findRingsRecursor (double r0, double r1)
{	// Find the scale and number of a ring between r1 and r0.
	double scale = r1 - r0;
	double num = r0 / scale;
	print ("r0: %1, r1: %2, scale: %3, num: %4\n", r0, r1, scale, num);

	// If the ring number is integral, we have found a fitting ring to r0 -> r1!
	if (isInteger (num))
	{	SolutionComponent cmp;
		cmp.scale = scale;
		cmp.num = (int) ceil (num);
		m_solution << cmp;
	}
	else
	{	// If not, find an intermediate <r> between the radii
		double r = ceil (num) * scale;
		print ("\tr: %1\n", r);

		// If r is the same as r0 or r1, we simply cannot find any rings between
		// r0 and r1. Stop and return failure.
		if (isZero (r0 - r) || isZero (r1 - r))
		{	print ("failure!\n");
			return false;
		}

		// Split this ring into r0 -> r and r -> r1. Recurse to possibly find
		// the rings for these. If either recurse fails, the entire algorithm
		// fails as well.
		if (!findRingsRecursor (r0, r) || !findRingsRecursor (r, r1))
			return false;
	}

	// The algorithm did not fail, thus we succeeded!
	return true;
}