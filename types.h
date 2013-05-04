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

#ifndef TYPES_H
#define TYPES_H

#include "common.h"

typedef unsigned int uint;
typedef short unsigned int ushort;
typedef long unsigned int ulong;

// Typedef out the _t suffices :)
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

template<class T> using initlist = std::initializer_list<T>;
using std::vector;

class matrix;

enum Axis { X, Y, Z };
static const Axis g_Axes[3] = {X, Y, Z};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// vertex
// 
// Vertex class, contains a single point in 3D space. Not to be confused with
// LDVertex, which is a vertex used in an LDraw part file.
// =============================================================================
class vertex {
public:
	double m_coords[3];
	
	vertex () {}
	vertex (double x, double y, double z) {
		m_coords[X] = x;
		m_coords[Y] = y;
		m_coords[Z] = z;
	}
	
	// =========================================================================
	void move (vertex other) {
		for (const Axis ax : g_Axes)
			m_coords[ax] += other[ax];
	}
	
	// =========================================================================
	vertex& operator+= (vertex other) {
		move (other);
		return *this;
	}
	
	// =========================================================================
	vertex operator/ (const double d) {
		vertex other (*this);
		return other /= d;
	}
	
	// =========================================================================
	vertex& operator/= (const double d) {
		for (const Axis ax : g_Axes)
			m_coords[ax] /= d;
		return *this;
	}
	
	// =========================================================================
	vertex operator- () const {
		return vertex (-m_coords[X], -m_coords[Y], -m_coords[Z]);
	}
	
	// =========================================================================
	double& coord (const ushort n) const {
		return const_cast<double&> (m_coords[n]);
	}
	
	double& operator[] (const Axis ax) const {
		return coord ((ushort) ax);
	}
	
	double& operator[] (const ushort n) const {
		return coord (n);
	}
	
	vertex midpoint (vertex& other);
	str stringRep (const bool mangled);
	void transform (matrix mMatrix, vertex pos);
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// matrix
// 
// A mathematical 3x3 matrix
// =============================================================================
class matrix {
public:
	// Constructors
	matrix () {}
	matrix (std::vector<double> vals);
	matrix (double fillval);
	matrix (double a, double b, double c,
		double d, double e, double f,
		double g, double h, double i);
	
	matrix mult (matrix mOther);
	
	matrix& operator= (matrix mOther) {
		memcpy (&m_vals[0], &mOther.m_vals[0], sizeof m_vals);
		return *this;
	}
	
	matrix operator* (matrix mOther) {
		return mult (mOther);
	}
	
	inline double& operator[] (const uint uIndex) {
		return m_vals[uIndex];
	}
	
	void zero ();
	void testOutput ();
	str stringRep ();

private:
	double m_vals[9];
};

#endif // TYPES_H