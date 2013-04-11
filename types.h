#ifndef __TYPES_H__
#define __TYPES_H__

#include "types.h"
#include "common.h"

class matrix;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// vertex
// 
// Vertex class. Not to be confused with LDVertex, which is a vertex used in an
// LDraw code file.
// =============================================================================
class vertex {
public:
	double x, y, z;
	
	vertex () {}
	vertex (double fX, double fY, double fZ) {
		x = fX;
		y = fY;
		z = fZ;
	}
	
	// =========================================================================
	vertex& operator+= (vertex& other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}
	
	// =========================================================================
	vertex operator- () const {
		return vertex (-x, -y, -z);
	}
	
	// =========================================================================
	// Midpoint between this vertex and another vertex.
	vertex midpoint (vertex& other);
	str getStringRep (const bool bMangled);
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
	double faValues[9];
	
	// Constructors
	matrix () {}
	matrix (std::vector<double> vals);
	matrix (double fVal);
	matrix (double a, double b, double c,
		double d, double e, double f,
		double g, double h, double i);
	
	matrix mult (matrix mOther);
	
	matrix& operator= (matrix mOther) {
		memcpy (&faValues[0], &mOther.faValues[0], sizeof faValues);
		return *this;
	}
	
	matrix operator* (matrix mOther) {
		return mult (mOther);
	}
	
	inline double& operator[] (const uint uIndex) {
		return faValues[uIndex];
	}
	
	void zero ();
	void testOutput ();
};

#endif // __TYPES_H__