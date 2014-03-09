/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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

#include <QObject>
#include <QStringList>
#include <QTextStream>
#include <QFile>
#include <assert.h>
#include "Main.h"
#include "Types.h"
#include "Misc.h"
#include "LDObject.h"
#include "Document.h"

// =============================================================================
//
Vertex::Vertex (double x, double y, double z)
{
	m_coords[X] = x;
	m_coords[Y] = y;
	m_coords[Z] = z;
}

// =============================================================================
//
void Vertex::move (const Vertex& other)
{
	for_axes (ax)
		m_coords[ax] += other[ax];
}

// =============================================================================
//
double Vertex::distanceTo (const Vertex& other) const
{
	double dx = abs (x() - other.x());
	double dy = abs (y() - other.y());
	double dz = abs (z() - other.z());
	return sqrt ((dx * dx) + (dy * dy) + (dz * dz));
}

// =============================================================================
//
Vertex Vertex::midpoint (const Vertex& other)
{
	Vertex mid;

	for_axes (ax)
		mid[ax] = (getCoordinate (ax) + other[ax]) / 2;

	return mid;
}

// =============================================================================
//
QString Vertex::toString (bool mangled) const
{
	QString formatstr = "%1 %2 %3";

	if (mangled)
		formatstr = "(%1, %2, %3)";

	return format (formatstr, x(), y(), z());
}

// =============================================================================
//
void Vertex::transform (const Matrix& matr, const Vertex& pos)
{
	double x2 = (matr[0] * x()) + (matr[1] * y()) + (matr[2] * z()) + pos[X];
	double y2 = (matr[3] * x()) + (matr[4] * y()) + (matr[5] * z()) + pos[Y];
	double z2 = (matr[6] * x()) + (matr[7] * y()) + (matr[8] * z()) + pos[Z];

	x() = x2;
	y() = y2;
	z() = z2;
}

// =============================================================================
//
Vertex Vertex::operator-() const
{
	return Vertex (-m_coords[X], -m_coords[Y], -m_coords[Z]);
}

// =============================================================================
//
bool Vertex::operator!= (const Vertex& other) const
{
	return !operator== (other);
}

// =============================================================================
//
bool Vertex::operator== (const Vertex& other) const
{
	return getCoordinate (X) == other[X] &&
		   getCoordinate (Y) == other[Y] &&
		   getCoordinate (Z) == other[Z];
}

// =============================================================================
//
Vertex& Vertex::operator/= (const double d)
{
	for_axes (ax)
		m_coords[ax] /= d;

	return *this;
}

// =============================================================================
//
Vertex Vertex::operator/ (const double d) const
{
	Vertex other (*this);
	return other /= d;
}

// =============================================================================
//
Vertex& Vertex::operator+= (const Vertex& other)
{
	move (other);
	return *this;
}

// =============================================================================
//
Vertex Vertex::operator+ (const Vertex& other) const
{
	Vertex newvert (*this);
	newvert.move (other);
	return newvert;
}

// =============================================================================
//
int Vertex::operator< (const Vertex& other) const
{
	if (operator== (other))
		return false;

	if (getCoordinate (X) < other[X])
		return true;

	if (getCoordinate (X) > other[X])
		return false;

	if (getCoordinate (Y) < other[Y])
		return true;

	if (getCoordinate (Y) > other[Y])
		return false;

	return getCoordinate (Z) < other[Z];
}

// =============================================================================
//
Matrix::Matrix (double vals[])
{
	for (int i = 0; i < 9; ++i)
		m_vals[i] = vals[i];
}

// =============================================================================
//
Matrix::Matrix (double fillval)
{
	for (int i = 0; i < 9; ++i)
		m_vals[i] = fillval;
}

// =============================================================================
//
Matrix::Matrix (const std::initializer_list< double >& vals)
{
	assert (vals.size() == 9);
	memcpy (&m_vals[0], & (*vals.begin()), sizeof m_vals);
}

// =============================================================================
//
void Matrix::dump() const
{
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
			print ("%1\t", m_vals[ (i * 3) + j]);

		print ("\n");
	}
}

// =============================================================================
//
QString Matrix::toString() const
{
	QString val;

	for (int i = 0; i < 9; ++i)
	{
		if (i > 0)
			val += ' ';

		val += QString::number (m_vals[i]);
	}

	return val;
}

// =============================================================================
//
void Matrix::zero()
{
	memset (&m_vals[0], 0, sizeof m_vals);
}

// =============================================================================
//
Matrix Matrix::mult (const Matrix& other) const
{
	Matrix val;
	val.zero();

	for (int i = 0; i < 3; ++i)
	for (int j = 0; j < 3; ++j)
	for (int k = 0; k < 3; ++k)
		val[(i * 3) + j] += m_vals[(i * 3) + k] * other[(k * 3) + j];

	return val;
}

// =============================================================================
//
Matrix& Matrix::operator= (const Matrix& other)
{
	memcpy (&m_vals[0], &other.m_vals[0], sizeof m_vals);
	return *this;
}

// =============================================================================
//
double Matrix::getDeterminant() const
{
	return (value (0) * value (4) * value (8)) +
		   (value (1) * value (5) * value (6)) +
		   (value (2) * value (3) * value (7)) -
		   (value (2) * value (4) * value (6)) -
		   (value (1) * value (3) * value (8)) -
		   (value (0) * value (5) * value (7));
}

// =============================================================================
//
bool Matrix::operator== (const Matrix& other) const
{
	for (int i = 0; i < 9; ++i)
		if (value (i) != other[i])
			return false;

	return true;
}

// =============================================================================
//
LDBoundingBox::LDBoundingBox()
{
	reset();
}

// =============================================================================
//
void LDBoundingBox::calculateFromCurrentDocument()
{
	reset();

	if (!getCurrentDocument())
		return;

	for (LDObject* obj : getCurrentDocument()->objects())
		calcObject (obj);
}

// =============================================================================
//
void LDBoundingBox::calcObject (LDObject* obj)
{
	switch (obj->type())
	{
		case LDObject::ELine:
		case LDObject::ETriangle:
		case LDObject::EQuad:
		case LDObject::ECondLine:
		{
			for (int i = 0; i < obj->vertices(); ++i)
				calcVertex (obj->vertex (i));
		} break;

		case LDObject::ESubfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			LDObjectList objs = ref->inlineContents (true, false);

			for (LDObject * obj : objs)
			{
				calcObject (obj);
				obj->destroy();
			}
		}
		break;

		default:
			break;
	}
}

// =============================================================================
//
LDBoundingBox& LDBoundingBox::operator<< (const Vertex& v)
{
	calcVertex (v);
	return *this;
}

// =============================================================================
//
LDBoundingBox& LDBoundingBox::operator<< (LDObject* obj)
{
	calcObject (obj);
	return *this;
}

// =============================================================================
//
void LDBoundingBox::calcVertex (const Vertex& vertex)
{
	for_axes (ax)
	{
		m_vertex0[ax] = min (vertex[ax], m_vertex0[ax]);
		m_vertex1[ax] = max (vertex[ax], m_vertex1[ax]);
	}

	setEmpty (false);
}

// =============================================================================
//
void LDBoundingBox::reset()
{
	m_vertex0[X] = m_vertex0[Y] = m_vertex0[Z] = 10000.0;
	m_vertex1[X] = m_vertex1[Y] = m_vertex1[Z] = -10000.0;
	setEmpty (true);
}

// =============================================================================
//
double LDBoundingBox::longestMeasurement() const
{
	double xscale = (m_vertex0[X] - m_vertex1[X]);
	double yscale = (m_vertex0[Y] - m_vertex1[Y]);
	double zscale = (m_vertex0[Z] - m_vertex1[Z]);
	double size = zscale;

	if (xscale > yscale)
	{
		if (xscale > zscale)
			size = xscale;
	}
	elif (yscale > zscale)
		size = yscale;

	if (abs (size) >= 2.0f)
		return abs (size / 2);

	return 1.0f;
}

// =============================================================================
//
Vertex LDBoundingBox::center() const
{
	return Vertex (
		(m_vertex0[X] + m_vertex1[X]) / 2,
		(m_vertex0[Y] + m_vertex1[Y]) / 2,
		(m_vertex0[Z] + m_vertex1[Z]) / 2);
}
