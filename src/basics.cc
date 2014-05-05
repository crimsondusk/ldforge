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
#include "main.h"
#include "basics.h"
#include "miscallenous.h"
#include "ldObject.h"
#include "ldDocument.h"

Vertex::Vertex() :
	QVector3D() {}

Vertex::Vertex (const QVector3D& a) :
	QVector3D (a) {}

Vertex::Vertex (qreal xpos, qreal ypos, qreal zpos) :
	QVector3D(xpos, ypos, zpos) {}


void Vertex::transform (const Matrix& matr, const Vertex& pos)
{
	double x2 = (matr[0] * x()) + (matr[1] * y()) + (matr[2] * z()) + pos.x();
	double y2 = (matr[3] * x()) + (matr[4] * y()) + (matr[5] * z()) + pos.y();
	double z2 = (matr[6] * x()) + (matr[7] * y()) + (matr[8] * z()) + pos.z();
	setX (x2);
	setY (y2);
	setZ (z2);
}

void Vertex::apply (ApplyFunction func)
{
	double newX = x(), newY = y(), newZ = z();
	func (X, newX);
	func (Y, newY);
	func (Z, newZ);
	*this = Vertex (newX, newY, newZ);
}

void Vertex::apply (ApplyConstFunction func) const
{
	func (X, x());
	func (Y, y());
	func (Z, z());
}

double Vertex::operator[] (Axis ax) const
{
	switch (ax)
	{
		case X: return x();
		case Y: return y();
		case Z: return z();
	}

	assert (false);
	return 0.0;
}

void Vertex::setCoordinate (Axis ax, qreal value)
{
	switch (ax)
	{
		case X: setX (value); break;
		case Y: setY (value); break;
		case Z: setZ (value); break;
	}
}

String Vertex::toString (bool mangled) const
{
	if (mangled)
		return format ("(%1, %2, %3)", x(), y(), z());

	return format ("%1 %2 %3", x(), y(), z());
}

bool Vertex::operator< (const Vertex& other) const
{
	if (x() != other.x()) return x() < other.x();
	if (y() != other.y()) return y() < other.y();
	if (z() != other.z()) return z() < other.z();
	return false;
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
String Matrix::toString() const
{
	String val;

	for (int i = 0; i < 9; ++i)
	{
		if (i > 0)
			val += ' ';

		val += String::number (m_vals[i]);
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

	if (not getCurrentDocument())
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
			for (int i = 0; i < obj->numVertices(); ++i)
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
	m_vertex0.setX (min (vertex.x(), m_vertex0.x()));
	m_vertex0.setY (min (vertex.y(), m_vertex0.y()));
	m_vertex0.setZ (min (vertex.z(), m_vertex0.z()));
	m_vertex1.setX (max (vertex.x(), m_vertex1.x()));
	m_vertex1.setY (max (vertex.y(), m_vertex1.y()));
	m_vertex1.setZ (max (vertex.z(), m_vertex1.z()));
	setEmpty (false);
}

// =============================================================================
//
void LDBoundingBox::reset()
{
	m_vertex0 = Vertex (10000.0, 10000.0, 10000.0);
	m_vertex1 = Vertex (-10000.0, -10000.0, -10000.0);
	setEmpty (true);
}

// =============================================================================
//
double LDBoundingBox::longestMeasurement() const
{
	double xscale = (m_vertex0.x() - m_vertex1.x());
	double yscale = (m_vertex0.y() - m_vertex1.y());
	double zscale = (m_vertex0.z() - m_vertex1.z());
	double size = zscale;

	if (xscale > yscale)
	{
		if (xscale > zscale)
			size = xscale;
	}
	elif (yscale > zscale)
		size = yscale;

	if (abs (size) >= 2.0)
		return abs (size / 2);

	return 1.0;
}

// =============================================================================
//
Vertex LDBoundingBox::center() const
{
	return Vertex (
		(m_vertex0.x() + m_vertex1.x()) / 2,
		(m_vertex0.y() + m_vertex1.y()) / 2,
		(m_vertex0.z() + m_vertex1.z()) / 2);
}
