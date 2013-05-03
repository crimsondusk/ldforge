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
	const char* fmt = mangled ? "(%s, %s, %s)" : "%s %s %s";
	
	return format (fmt,
		ftoa (coord (X)).chars(),
		ftoa (coord (Y)).chars(),
		ftoa (coord (Z)).chars());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void vertex::transform (matrix transmatrx, vertex pos) {
	double x2, y2, z2;
	x2 = (transmatrx[0] * coord (X)) + (transmatrx[1] * coord (Y)) + (transmatrx[2] * coord (Z)) + pos[X];
	y2 = (transmatrx[3] * coord (X)) + (transmatrx[4] * coord (Y)) + (transmatrx[5] * coord (Z)) + pos[Y];
	z2 = (transmatrx[6] * coord (X)) + (transmatrx[7] * coord (Y)) + (transmatrx[8] * coord (Z)) + pos[Z];
	
	coord (X) = x2;
	coord (Y) = y2;
	coord (Z) = z2;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
matrix::matrix (vector<double> vals) {
	assert (vals.size() == (sizeof faValues / sizeof *faValues));
	memcpy (&faValues[0], &(*vals.begin ()), sizeof faValues);
}

// -----------------------------------------------------------------------------
matrix::matrix (double fVal) {
	for (short i = 0; i < 9; ++i)
		faValues[i] = fVal;
}

// -----------------------------------------------------------------------------
matrix::matrix (double a, double b, double c, double d, double e, double f,
	double g, double h, double i)
{
	faValues[0] = a; faValues[1] = b; faValues[2] = c;
	faValues[3] = d; faValues[4] = e; faValues[5] = f;
	faValues[6] = g; faValues[7] = h; faValues[8] = i;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void matrix::zero () {
	memset (&faValues[0], 0, sizeof faValues);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
matrix matrix::mult (matrix mOther) {
	matrix mVal;
	matrix& mThis = *this;
	
	mVal.zero ();
	
	// arrrrrrrrrrrgh
	for (short i = 0; i < 3; ++i)
	for (short j = 0; j < 3; ++j)
	for (short k = 0; k < 3; ++k)
		mVal[(i * 3) + j] += mThis[(i * 3) + k] * mOther[(k * 3) + j];
	
	return mVal;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void matrix::testOutput () {
	for (short i = 0; i < 3; ++i) {
		for (short j = 0; j < 3; ++j)
			printf ("%*f\t", 10, faValues[(i * 3) + j]);
		
		printf ("\n");
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str matrix::stringRep () {
	str val;
	for (short i = 0; i < 9; ++i) {
		if (i > 0)
			val += ' ';
		
		val.appendformat ("%s", ftoa (faValues[i]).chars());
	}
	
	return val;
}