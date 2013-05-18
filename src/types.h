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
typedef unsigned short ushort;
typedef unsigned long ulong;

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
class matrix {
public:
	matrix () {}
	matrix (initlist<double> vals);
	matrix (double fillval);
	matrix (double vals[]);
	
	double			determinant	() const;
	matrix			mult			(matrix other);
	void			puts			() const;
	str				stringRep		() const;
	void			zero			();
	double&			val				(const uint idx) { return m_vals[idx]; }
	const double&	val				(const uint idx) const { return m_vals[idx]; }
	
	matrix&			operator=		(matrix other);
	matrix			operator*		(matrix other) { return mult (other); }
	double&			operator[]		(const uint idx) { return m_vals[idx]; }
	const double&	operator[]		(const uint idx) const { return m_vals[idx]; }

private:
	double m_vals[9];
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
	vertex () {}
	vertex (double x, double y, double z);
	
	double&			coord			(const ushort n) { return m_coords[n]; }
	const double&	coord			(const ushort n) const { return m_coords[n]; }
	vertex			midpoint		(vertex& other);
	void			move			(vertex other);
	str				stringRep		(const bool mangled);
	void			transform		(matrix matr, vertex pos);
	double&			x				() { return m_coords[X]; }
	const double&	x				() const { return m_coords[X]; }
	double&			y				() { return m_coords[Y]; }
	const double&	y				() const { return m_coords[Y]; }
	double&			z				() { return m_coords[Z]; }
	const double&	z				() const { return m_coords[Z]; }
	
	vertex&			operator+=		(vertex other);
	vertex			operator/		(const double d) const;
	vertex&			operator/=		(const double d);
	bool			operator==		(const vertex& other) const;
	bool			operator!=		(const vertex& other) const;
	vertex			operator-		() const;
	double&			operator[]		(const Axis ax);
	const double&	operator[]		(const Axis ax) const;
	double&			operator[]		(const int ax);
	const double&	operator[]		(const int ax) const;

private:
	double m_coords[3];
};

#endif // TYPES_H