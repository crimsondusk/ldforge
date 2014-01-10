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
#include "types.h"
#include "misc.h"
#include "ldtypes.h"
#include "document.h"

// =============================================================================
// -----------------------------------------------------------------------------
QString DoFormat (QList<StringFormatArg> args)
{
	assert (args.size() >= 1);
	QString text = args[0].value();

	for (uchar i = 1; i < args.size(); ++i)
		text = text.arg (args[i].value());

	return text;
}

// =============================================================================
// -----------------------------------------------------------------------------
Vertex::Vertex (double x, double y, double z)
{
	m_coords[X] = x;
	m_coords[Y] = y;
	m_coords[Z] = z;
}

// =============================================================================
// -----------------------------------------------------------------------------
void Vertex::move (const Vertex& other)
{
	for_axes (ax)
		m_coords[ax] += other[ax];
}

// =============================================================================
// -----------------------------------------------------------------------------
double Vertex::distanceTo (const Vertex& other) const
{
	double dx = abs (x() - other.x());
	double dy = abs (y() - other.y());
	double dz = abs (z() - other.z());
	return sqrt ((dx * dx) + (dy * dy) + (dz * dz));
}

// =============================================================================
// -----------------------------------------------------------------------------
Vertex Vertex::midpoint (const Vertex& other)
{
	Vertex mid;

	for_axes (ax)
		mid[ax] = (m_coords[ax] + other[ax]) / 2;

	return mid;
}

// =============================================================================
// -----------------------------------------------------------------------------
QString Vertex::stringRep (bool mangled) const
{
	QString fmtstr = "%1 %2 %3";

	if (mangled)
		fmtstr = "(%1, %2, %3)";

	return fmt (fmtstr, coord (X), coord (Y), coord (Z));
}

// =============================================================================
// -----------------------------------------------------------------------------
void Vertex::transform (Matrix matr, Vertex pos)
{
	double x2 = (matr[0] * x()) + (matr[1] * y()) + (matr[2] * z()) + pos[X];
	double y2 = (matr[3] * x()) + (matr[4] * y()) + (matr[5] * z()) + pos[Y];
	double z2 = (matr[6] * x()) + (matr[7] * y()) + (matr[8] * z()) + pos[Z];

	x() = x2;
	y() = y2;
	z() = z2;
}

// =============================================================================
// -----------------------------------------------------------------------------
Vertex Vertex::operator-() const
{
	return Vertex (-m_coords[X], -m_coords[Y], -m_coords[Z]);
}

// =============================================================================
// -----------------------------------------------------------------------------
bool Vertex::operator!= (const Vertex& other) const
{
	return !operator== (other);
}

// =============================================================================
// -----------------------------------------------------------------------------
double& Vertex::operator[] (const Axis ax)
{
	return coord ( (int) ax);
}

const double& Vertex::operator[] (const Axis ax) const
{
	return coord ( (int) ax);
}

double& Vertex::operator[] (const int ax)
{
	return coord (ax);
}

const double& Vertex::operator[] (const int ax) const
{
	return coord (ax);
}

// =============================================================================
// -----------------------------------------------------------------------------
bool Vertex::operator== (const Vertex& other) const
{
	return coord (X) == other[X] &&
		   coord (Y) == other[Y] &&
		   coord (Z) == other[Z];
}

// =============================================================================
// -----------------------------------------------------------------------------
Vertex& Vertex::operator/= (const double d)
{
	for_axes (ax)
		m_coords[ax] /= d;

	return *this;
}

// =============================================================================
// -----------------------------------------------------------------------------
Vertex Vertex::operator/ (const double d) const
{
	Vertex other (*this);
	return other /= d;
}

// =============================================================================
// -----------------------------------------------------------------------------
Vertex& Vertex::operator+= (const Vertex& other)
{
	move (other);
	return *this;
}

// =============================================================================
// -----------------------------------------------------------------------------
Vertex Vertex::operator+ (const Vertex& other) const
{
	Vertex newvert (*this);
	newvert.move (other);
	return newvert;
}

// =============================================================================
// -----------------------------------------------------------------------------
int Vertex::operator< (const Vertex& other) const
{
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
// -----------------------------------------------------------------------------
Matrix::Matrix (double vals[])
{
	for (int i = 0; i < 9; ++i)
		m_vals[i] = vals[i];
}

// =============================================================================
// -----------------------------------------------------------------------------
Matrix::Matrix (double fillval)
{
	for (int i = 0; i < 9; ++i)
		m_vals[i] = fillval;
}

// =============================================================================
// -----------------------------------------------------------------------------
Matrix::Matrix (initlist<double> vals)
{
	assert (vals.size() == 9);
	memcpy (&m_vals[0], & (*vals.begin()), sizeof m_vals);
}

// =============================================================================
// -----------------------------------------------------------------------------
void Matrix::puts() const
{
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
			log ("%1\t", m_vals[ (i * 3) + j]);

		log ("\n");
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
QString Matrix::stringRep() const
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
// -----------------------------------------------------------------------------
void Matrix::zero()
{
	memset (&m_vals[0], 0, sizeof m_vals);
}

// =============================================================================
// -----------------------------------------------------------------------------
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
// -----------------------------------------------------------------------------
Matrix& Matrix::operator= (const Matrix& other)
{
	memcpy (&m_vals[0], &other.m_vals[0], sizeof m_vals);
	return *this;
}

// =============================================================================
// -----------------------------------------------------------------------------
double Matrix::getDeterminant() const
{
	return (val (0) * val (4) * val (8)) +
		   (val (1) * val (5) * val (6)) +
		   (val (2) * val (3) * val (7)) -
		   (val (2) * val (4) * val (6)) -
		   (val (1) * val (3) * val (8)) -
		   (val (0) * val (5) * val (7));
}

// =============================================================================
// -----------------------------------------------------------------------------
bool Matrix::operator== (const Matrix& other) const
{
	for (int i = 0; i < 9; ++i)
		if (val (i) != other[i])
			return false;

	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
LDBoundingBox::LDBoundingBox()
{
	reset();
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDBoundingBox::calculate()
{
	reset();

	if (!getCurrentDocument())
		return;

	for (LDObject* obj : getCurrentDocument()->getObjects())
		calcObject (obj);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDBoundingBox::calcObject (LDObject* obj)
{
	switch (obj->getType())
	{
		case LDObject::ELine:
		case LDObject::ETriangle:
		case LDObject::EQuad:
		case LDObject::ECondLine:
		{
			for (int i = 0; i < obj->vertices(); ++i)
				calcVertex (obj->getVertex (i));
		} break;

		case LDObject::ESubfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			LDObjectList objs = ref->inlineContents (LDSubfile::DeepCacheInline);

			for (LDObject * obj : objs)
			{
				calcObject (obj);
				obj->deleteSelf();
			}
		}
		break;

		default:
			break;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
LDBoundingBox& LDBoundingBox::operator<< (const Vertex& v)
{
	calcVertex (v);
	return *this;
}

// =============================================================================
// -----------------------------------------------------------------------------
LDBoundingBox& LDBoundingBox::operator<< (LDObject* obj)
{
	calcObject (obj);
	return *this;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDBoundingBox::calcVertex (const Vertex& v)
{
	for_axes (ax)
	{
		m_Vertex0[ax] = min (v[ax], m_Vertex0[ax]);
		m_Vertex1[ax] = max (v[ax], m_Vertex1[ax]);
	}

	setEmpty (false);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDBoundingBox::reset()
{
	m_Vertex0[X] = m_Vertex0[Y] = m_Vertex0[Z] = 10000.0;
	m_Vertex1[X] = m_Vertex1[Y] = m_Vertex1[Z] = -10000.0;
	setEmpty (true);
}

// =============================================================================
// -----------------------------------------------------------------------------
double LDBoundingBox::size() const
{
	double xscale = (m_Vertex0[X] - m_Vertex1[X]);
	double yscale = (m_Vertex0[Y] - m_Vertex1[Y]);
	double zscale = (m_Vertex0[Z] - m_Vertex1[Z]);
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
// -----------------------------------------------------------------------------
Vertex LDBoundingBox::center() const
{
	return Vertex (
		(m_Vertex0[X] + m_Vertex1[X]) / 2,
		(m_Vertex0[Y] + m_Vertex1[Y]) / 2,
		(m_Vertex0[Z] + m_Vertex1[Z]) / 2);
}
