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
	assert (vals.size() == (sizeof m_vals / sizeof *m_vals));
	memcpy (&m_vals[0], &(*vals.begin ()), sizeof m_vals);
}

// -----------------------------------------------------------------------------
matrix::matrix (double fillval) {
	for (short i = 0; i < 9; ++i)
		m_vals[i] = fillval;
}

// -----------------------------------------------------------------------------
matrix::matrix (double a, double b, double c, double d, double e, double f,
	double g, double h, double i)
{
	m_vals[0] = a; m_vals[1] = b; m_vals[2] = c;
	m_vals[3] = d; m_vals[4] = e; m_vals[5] = f;
	m_vals[6] = g; m_vals[7] = h; m_vals[8] = i;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void matrix::zero () {
	memset (&m_vals[0], 0, sizeof m_vals);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
matrix matrix::mult (matrix other) {
	matrix val;
	val.zero ();
	
	for (short i = 0; i < 3; ++i)
	for (short j = 0; j < 3; ++j)
	for (short k = 0; k < 3; ++k)
		val[(i * 3) + j] += m_vals[(i * 3) + k] * other[(k * 3) + j];
	
	return val;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void matrix::testOutput () {
	for (short i = 0; i < 3; ++i) {
		for (short j = 0; j < 3; ++j)
			printf ("%*f\t", 10, m_vals[(i * 3) + j]);
		
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
		
		val.appendformat ("%s", ftoa (m_vals[i]).chars());
	}
	
	return val;
}