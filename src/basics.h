/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <QString>
#include <QObject>
#include <QStringList>
#include <QMetaType>
#include "macros.h"

class LDObject;
class QFile;
class QTextStream;

using int8 = qint8;
using int16 = qint16;
using int32 = qint32;
using int64 = qint64;
using uint8 = quint8;
using uint16 = quint16;
using uint32 = quint32;
using uint64 = quint64;

template<typename T, typename R>
using Pair = std::pair<T, R>;

enum Axis
{
	X,
	Y,
	Z
};

// =============================================================================
//
class LDObject;
using LDObjectList = QList<LDObject*>;

//!
//! \brief A mathematical 3 x 3 matrix
//!
class Matrix
{
	public:
		//! Constructs a matrix with undetermined values.
		Matrix() {}

		//! Constructs a matrix with the given values.
		//! \note \c vals is expected to have exactly 9 elements.
		Matrix (const std::initializer_list<double>& vals);

		//! Constructs a matrix all 9 elements initialized to the same value.
		//! \param fillval the value to initialize the matrix coordinates as
		Matrix (double fillval);

		//! Constructs a matrix with a C-array.
		//! \note \c vals is expected to have exactly 9 elements.
		Matrix (double vals[]);

		//! Calculates the matrix's determinant.
		//! \returns the calculated determinant.
		double			getDeterminant() const;

		//! Multiplies this matrix with \c other
		//! \param other the matrix to multiply with.
		//! \returns the resulting matrix
		//! \note a.mult(b) is not equivalent to b.mult(a)!
		Matrix			mult (const Matrix& other) const;

		//! Prints the matrix to stdout.
		void			dump() const;

		//! \returns a string representation of the matrix.
		QString			toString() const;

		//! Zeroes the matrix out.
		void			zero();

		//! Assigns the matrix values to the values of \c other.
		//! \param other the matrix to assign this to.
		//! \returns a reference to self
		Matrix&			operator= (const Matrix& other);

		//! \returns a mutable reference to a value by \c idx
		inline double& value (int idx)
		{
			return m_vals[idx];
		}

		//! An overload of \c value() for const matrices.
		//! \returns a const reference to a value by \c idx
		inline const double& value (int idx) const
		{
			return m_vals[idx];
		}

		//! An operator overload for \c mult().
		//! \returns the multiplied matrix.
		inline Matrix operator* (const Matrix& other) const
		{
			return mult (other);
		}

		//! An operator overload for \c value().
		//! \returns a mutable reference to a value by \c idx
		inline double& operator[] (int idx)
		{
			return value (idx);
		}

		//! An operator overload for \c value() const.
		//! \returns a const reference to a value by \c idx
		inline const double& operator[] (int idx) const
		{
			return value (idx);
		}

		//! \param other the matrix to check against
		//! \returns whether the two matrices have the same values.
		bool operator== (const Matrix& other) const;

	private:
		double m_vals[9];
};

//!
//! \brief A vertex in 3D space
//!
//! Contains a single point in 3D space. Not to be confused with
//! LDVertex, which is a vertex used in an LDraw part file.
//!
//! This also sees use as a position vector.
//!
class Vertex
{
	public:
		//! Constructs a zero vertex
		Vertex() :
			m_coords{0, 0, 0} {}

		//! Constructs a vertex with the given \c x, \c y and \c z.
		Vertex (double x, double y, double z);

		//! \returns the distance from this vertex to \c other
		double			distanceTo (const Vertex& other) const;

		//! \returns the vertex at the midpoint between this and \c other
		Vertex			midpoint (const Vertex& other);

		//! Moves this vertex using \param other as a position vector.
		void			move (const Vertex& other);

		//! Yields a string representation of the vertex. The string returned
		//! can possibly be mangled.
		//! - As mangled: {1.5, 2.8, 3.14}
		//! - Without mangling: 1.5 2.8 3.14
		//!
		//! The mangled version is suitable for printing to the user, the
		//! non-mangled one is used when writing the vertex to LDraw files.
		//!
		//! \returns a string representation of this vertex
		//! \param mangled whether to return a mangled representation or not
		QString			toString (bool mangled) const;

		//! Transforms this vertex with \c matr as transformation matrix
		//! and \c pos as the position column of the 4x4 matrix.
		void			transform (const Matrix& matr, const Vertex& pos);

		//! An operator overload for \c move().
		Vertex&			operator+= (const Vertex& other);

		//! An operator overload for \c move(), using a temporary vertex.
		Vertex			operator+ (const Vertex& other) const;

		//! Divides all values by \c d.
		Vertex			operator/ (const double d) const;

		//! Divides all values by \c d.
		Vertex&			operator/= (const double d);

		//! Checks whether this vertex has the same values as \c other.
		bool			operator== (const Vertex& other) const;

		//! Checks whether this vertex has different values than \c other.
		bool			operator!= (const Vertex& other) const;

		//! \returns a negated version the vertex
		Vertex			operator-() const;

		//! \returns whether the vertex has lesser values than \c other.
		int				operator< (const Vertex& other) const;

		//! An operator overload for \c getCoordinate().
		inline double& operator[] (const Axis ax)
		{
			return getCoordinate ((int) ax);
		}

		//! An operator overload for \c getCoordinate() const.
		inline const double& operator[] (const Axis ax) const
		{
			return getCoordinate ((int) ax);
		}

		//! An operator overload for \c getCoordinate().
		inline double& operator[] (const int ax)
		{
			return getCoordinate (ax);
		}

		//! An operator overload for \c getCoordinate() const.
		inline const double& operator[] (const int ax) const
		{
			return getCoordinate (ax);
		}

		//! \returns a mutable reference for the coordinate designated by \param n.
		inline double& getCoordinate (int n)
		{
			return m_coords[n];
		}

		//! An overload of \c getCoordinate for const vertices.
		//! \returns a const reference for the coordinate designated by \param n.
		inline const double& getCoordinate (int n) const
		{
			return m_coords[n];
		}

		//! \returns a mutable reference to X.
		inline double& x()
		{
			return m_coords[X];
		}

		//! An overload of \c x() for const vertices.
		//! \returns a const reference to X.
		inline const double& x() const
		{
			return m_coords[X];
		}

		//! \returns a mutable reference to Y.
		inline double& y()
		{
			return m_coords[Y];
		}

		//! An overload of \c y() for const vertices.
		//! \returns a const reference to Y.
		inline const double& y() const
		{
			return m_coords[Y];
		}

		//! \returns a mutable reference to Z.
		inline double& z()
		{
			return m_coords[Z];
		}

		//! An overload of \c z() for const vertices.
		//! \returns a const reference to Z.
		inline const double& z() const
		{
			return m_coords[Z];
		}

	private:
		double m_coords[3];
};

Q_DECLARE_METATYPE (Vertex)

//!
//! Defines a bounding box that encompasses a given set of objects.
//! vertex0 is the minimum vertex, vertex1 is the maximum vertex.
//
class LDBoundingBox
{
	PROPERTY (private,	bool,		isEmpty,	setEmpty,		STOCK_WRITE)
	PROPERTY (private,	Vertex,		vertex0,	setVertex0,		STOCK_WRITE)
	PROPERTY (private,	Vertex,		vertex1,	setVertex1,		STOCK_WRITE)

	public:
		//! Constructs an empty bounding box.
		LDBoundingBox();

		//! Clears the bounding box
		void reset();

		//! Calculates the bounding box's values from the objects in the current
		//! document.
		void calculateFromCurrentDocument();

		//! \returns the length of the bounding box on the longest measure.
		double longestMeasurement() const;

		//! Calculates the given \c obj to the bounding box, adjusting
		//! extremas if necessary.
		void calcObject (LDObject* obj);

		//! Calculates the given \c vertex to the bounding box, adjusting
		//! extremas if necessary.
		void calcVertex (const Vertex& vertex);

		//! \returns the center of the bounding box.
		Vertex center() const;

		//! An operator overload for \c calcObject()
		LDBoundingBox& operator<< (LDObject* obj);

		//! An operator overload for \c calcVertex()
		LDBoundingBox& operator<< (const Vertex& v);
};

extern const Vertex g_origin; // Vertex at (0, 0, 0)
extern const Matrix g_identity; // Identity matrix

static const double pi = 3.14159265358979323846;
