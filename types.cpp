#include <assert.h>
#include "common.h"
#include "types.h"
#include "misc.h"

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
vertex vertex::midpoint (vertex& other) {
	vertex mid;
	
	for (const Axis ax : g_Axes)
		mid[ax] = (m_coords[ax] + other[ax]) / 2;
	
	return mid;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str vertex::stringRep (const bool mangled) {
	return fmt (mangled ? "(%s, %s, %s)" : "%s %s %s",
		ftoa (coord (X)).chars(),
		ftoa (coord (Y)).chars(),
		ftoa (coord (Z)).chars());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void vertex::transform (matrix<3> matr, vertex pos) {
	double x2, y2, z2;
	x2 = (matr[0] * coord (X)) + (matr[1] * coord (Y)) + (matr[2] * coord (Z)) + pos[X];
	y2 = (matr[3] * coord (X)) + (matr[4] * coord (Y)) + (matr[5] * coord (Z)) + pos[Y];
	z2 = (matr[6] * coord (X)) + (matr[7] * coord (Y)) + (matr[8] * coord (Z)) + pos[Z];
	
	coord (X) = x2;
	coord (Y) = y2;
	coord (Z) = z2;
}