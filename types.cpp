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
	double x2 = (matr[0] * x ()) + (matr[1] * y ()) + (matr[2] * z ()) + pos[X];
	double y2 = (matr[3] * x ()) + (matr[4] * y ()) + (matr[5] * z ()) + pos[Y];
	double z2 = (matr[6] * x ()) + (matr[7] * y ()) + (matr[8] * z ()) + pos[Z];
	
	x () = x2;
	y () = y2;
	z () = z2;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void vertex::transform (matrix<4> matr) {
	double x2 = (matr[0] * x ()) + (matr[4] + y ()) + (matr[8] + z ()) + matr[12];
	double y2 = (matr[1] * x ()) + (matr[5] + y ()) + (matr[9] + z ()) + matr[13];
	double z2 = (matr[2] * x ()) + (matr[6] + y ()) + (matr[10] + z ()) + matr[14];
	double w2 = (matr[3] * x ()) + (matr[7] + y ()) + (matr[11] + z ()) + matr[15];
	
	x () = x2 / w2;
	y () = y2 / w2;
	z () = z2 / w2;
}