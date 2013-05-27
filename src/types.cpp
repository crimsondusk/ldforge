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

#include <assert.h>
#include "common.h"
#include "types.h"
#include "misc.h"

vertex::vertex (double x, double y, double z) {
	m_coords[X] = x;
	m_coords[Y] = y;
	m_coords[Z] = z;
}

// =============================================================================
void vertex::move (vertex other) {
	for (const Axis ax : g_Axes)
		m_coords[ax] += other[ax];
}

// =============================================================================
vertex vertex::midpoint (vertex& other) {
	vertex mid;
	
	for (const Axis ax : g_Axes)
		mid[ax] = (m_coords[ax] + other[ax]) / 2;
	
	return mid;
}

// =============================================================================
str vertex::stringRep (const bool mangled) {
	return fmt (mangled ? "(%s, %s, %s)" : "%s %s %s",
		ftoa (coord (X)).chars(),
		ftoa (coord (Y)).chars(),
		ftoa (coord (Z)).chars());
}

// =============================================================================
void vertex::transform (matrix matr, vertex pos) {
	double x2 = (matr[0] * x ()) + (matr[1] * y ()) + (matr[2] * z ()) + pos[X];
	double y2 = (matr[3] * x ()) + (matr[4] * y ()) + (matr[5] * z ()) + pos[Y];
	double z2 = (matr[6] * x ()) + (matr[7] * y ()) + (matr[8] * z ()) + pos[Z];
	
	x () = x2;
	y () = y2;
	z () = z2;
}

vertex vertex::operator-() const {
	return vertex (-m_coords[X], -m_coords[Y], -m_coords[Z]);
}

bool vertex::operator!= (const vertex& other) const {
	return !operator== (other);
}

double& vertex::operator[] (const Axis ax) {
	return coord ((ushort) ax);
}

const double& vertex::operator[] (const Axis ax) const {
	return coord ((ushort) ax);
}

double& vertex::operator[] (const int ax) {
	return coord (ax);
}

const double& vertex::operator[] (const int ax) const {
	return coord (ax);
}

bool vertex::operator== (const vertex& other) const {
	return coord (X) == other[X] &&
		coord (Y) == other[Y] &&
		coord (Z) == other[Z];
}

vertex& vertex::operator/= (const double d) {
	for (const Axis ax : g_Axes)
		m_coords[ax] /= d;
	
	return *this;
}

vertex vertex::operator/ (const double d) const {
	vertex other (*this);
	return other /= d;
}

vertex& vertex::operator+= (vertex other) {
	move (other);
	return *this;
}

int vertex::operator< (const vertex& other) const {
	if (operator== (other))
		return false;
	
	if (coord (X) < other[X])
		return true;
	
	if (coord (X) > other[X])
		return false;
	
	if (coord (Y) < other[Y])
		return true;
	
	if (coord (Y) > other[Y])
		return false;
	
	return coord (Z) < other[Z];
}

// =============================================================================
matrix::matrix (double vals[]) {
	for (short i = 0; i < 9; ++i)
		m_vals[i] = vals[i];
}

matrix::matrix (double fillval) {
	for (short i = 0; i < 9; ++i)
		m_vals[i] = fillval;
}

matrix::matrix (initlist<double> vals) {
	assert (vals.size() == 9);
	memcpy (&m_vals[0], &(*vals.begin ()), sizeof m_vals);
}
void matrix::puts () const {
	for (short i = 0; i < 3; ++i) {
		for (short j = 0; j < 3; ++j)
			printf ("%*f\t", 10, m_vals[(i * 3) + j]);
		
		printf ("\n");
	}
}

// =============================================================================
str matrix::stringRep () const {
	str val;
	for (short i = 0; i < 9; ++i) {
		if (i > 0)
			val += ' ';
		
		val += fmt ("%s", ftoa (m_vals[i]).chars());
	}
	
	return val;	
}

// =============================================================================
void matrix::zero () {
	memset (&m_vals[0], 0, sizeof (double) * 9);
}

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
matrix& matrix::operator= (matrix other) {
	 memcpy (&m_vals[0], &other.m_vals[0], sizeof (double) * 9);
	 return *this;
}

// =============================================================================
double matrix::determinant () const {
	return
		(val (0) * val (4) * val (8)) +
		(val (1) * val (5) * val (6)) +
		(val (2) * val (3) * val (7)) -
		(val (2) * val (4) * val (6)) -
		(val (1) * val (3) * val (8)) -
		(val (0) * val (5) * val (7));
}