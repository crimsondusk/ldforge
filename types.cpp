#include <assert.h>
#include "common.h"
#include "types.h"
#include "misc.h"

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
vertex vertex::midpoint (vertex& other) {
	vertex mid;
	mid.x = (x + other.x);
	mid.y = (y + other.y);
	mid.z = (z + other.z);
	return mid;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str vertex::stringRep (const bool mangled) {
	const char* fmt = mangled ? "(%s, %s, %s)" : "%s %s %s";
	
	return format (fmt,
		ftoa (x).chars(),
		ftoa (y).chars(),
		ftoa (z).chars());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void vertex::transform (matrix transmatrx, vertex pos) {
	double x2, y2, z2;
	x2 = (transmatrx[0] * x) + (transmatrx[1] * y) + (transmatrx[2] * z) + pos.x;
	y2 = (transmatrx[3] * x) + (transmatrx[4] * y) + (transmatrx[5] * z) + pos.y;
	z2 = (transmatrx[6] * x) + (transmatrx[7] * y) + (transmatrx[8] * z) + pos.z;
	
	x = x2;
	y = y2;
	z = z2;
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