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
#include "misc.h"

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

enum Axis { X, Y, Z };
static const Axis g_Axes[3] = {X, Y, Z};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// matrix
// 
// A templated, mathematical N x N matrix
// =============================================================================
template<int N> class matrix {
public:
	// Constructors
	matrix () {}
	matrix (initlist<double> vals) {
		assert (vals.size() == N * N);
		memcpy (&m_vals[0], &(*vals.begin ()), sizeof m_vals);
	}
	
	matrix (double fillval) {
		for (short i = 0; i < (N * N); ++i)
			m_vals[i] = fillval;
	}
	
	matrix (double vals[]) {
		for (short i = 0; i < (N * N); ++i)
			m_vals[i] = vals[i];
	}
	
	template<int M> matrix (matrix<M> other) {
		assert (M >= N);
		
		for (short i = 0; i < M; ++i)
		for (short j = 0; j < M; ++j) {
			const short idx = (i * M) + j;
			m_vals[idx] = other[idx];
		}
	}
	
	matrix<N> mult (matrix<N> other) {
		matrix val;
		val.zero ();

		for (short i = 0; i < N; ++i)
		for (short j = 0; j < N; ++j)
		for (short k = 0; k < N; ++k)
			val[(i * N) + j] += m_vals[(i * N) + k] * other[(k * N) + j];

		return val;
	}
	
	matrix<N>& operator= (matrix<N> other) {
		memcpy (&m_vals[0], &other.m_vals[0], sizeof (double) * N * N);
		return *this;
	}
	
	matrix<N> operator* (matrix<N> other) {
		return mult (other);
	}
	
	double& operator[] (const uint idx) {
		return m_vals[idx];
	}
	
	const double& operator[] (const uint idx) const {
		return m_vals[idx];
	}
	
	void zero () {
		memset (&m_vals[0], 0, sizeof (double) * N * N);
	}
	
	void puts () const {
		for (short i = 0; i < N; ++i) {
			for (short j = 0; j < N; ++j)
				printf ("%*f\t", 10, m_vals[(i * 3) + j]);
			
			printf ("\n");
		}
	}
	
	str stringRep () const {
		str val;
		for (short i = 0; i < N * N; ++i) {
			if (i > 0)
				val += ' ';
			
			val.appendformat ("%s", ftoa (m_vals[i]).chars());
		}
		
		return val;	
	}

private:
	double m_vals[N * N];
};

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
	
	void move (vertex other) {
		for (const Axis ax : g_Axes)
			m_coords[ax] += other[ax];
	}
	
	vertex& operator+= (vertex other) {
		move (other);
		return *this;
	}
	
	vertex operator/ (const double d) const {
		vertex other (*this);
		return other /= d;
	}
	
	vertex& operator/= (const double d) {
		for (const Axis ax : g_Axes)
			m_coords[ax] /= d;
		return *this;
	}
	
	vertex operator- () const {
		return vertex (-m_coords[X], -m_coords[Y], -m_coords[Z]);
	}
	
	double& coord (const ushort n) {
		return m_coords[n];
	}
	
	const double& coord (const ushort n) const {
		return m_coords[n];
	}
	
	double& operator[] (const Axis ax) {
		return coord ((ushort) ax);
	}
	
	const double& operator[] (const Axis ax) const {
		return coord ((ushort) ax);
	}
	
	vertex midpoint (vertex& other);
	str stringRep (const bool mangled);
	void transform (matrix<3> matr, vertex pos);
};

#endif // TYPES_H