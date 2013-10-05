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

#include <QObject>
#include <QStringList>
#include <QTextStream>
#include <qfile.h>
#include <assert.h>
#include "common.h"
#include "types.h"
#include "misc.h"
#include "ldtypes.h"
#include "file.h"

// =============================================================================
// -----------------------------------------------------------------------------
str DoFormat (List<StringFormatArg> args)
{	assert (args.size() >= 1);
	str text = args[0].value();

	for (uchar i = 1; i < args.size(); ++i)
		text = text.arg (args[i].value());

	return text;
}

// =============================================================================
// -----------------------------------------------------------------------------
vertex::vertex (double x, double y, double z)
{	m_coords[X] = x;
	m_coords[Y] = y;
	m_coords[Z] = z;
}

// =============================================================================
// -----------------------------------------------------------------------------
void vertex::move (const vertex& other)
{	for (const Axis ax : g_Axes)
		m_coords[ax] += other[ax];
}

// =============================================================================
// -----------------------------------------------------------------------------
vertex vertex::midpoint (const vertex& other)
{	vertex mid;

for (const Axis ax : g_Axes)
		mid[ax] = (m_coords[ax] + other[ax]) / 2;

	return mid;
}

// =============================================================================
// -----------------------------------------------------------------------------
str vertex::stringRep (bool mangled) const
{	str fmtstr = "%1 %2 %3";

	if (mangled)
		fmtstr = "(%1, %2, %3)";

	return fmt (fmtstr, coord (X), coord (Y), coord (Z));
}

// =============================================================================
// -----------------------------------------------------------------------------
void vertex::transform (matrix matr, vertex pos)
{	double x2 = (matr[0] * x()) + (matr[1] * y()) + (matr[2] * z()) + pos[X];
	double y2 = (matr[3] * x()) + (matr[4] * y()) + (matr[5] * z()) + pos[Y];
	double z2 = (matr[6] * x()) + (matr[7] * y()) + (matr[8] * z()) + pos[Z];

	x() = x2;
	y() = y2;
	z() = z2;
}

// =============================================================================
// -----------------------------------------------------------------------------
vertex vertex::operator-() const
{	return vertex (-m_coords[X], -m_coords[Y], -m_coords[Z]);
}

// =============================================================================
// -----------------------------------------------------------------------------
bool vertex::operator!= (const vertex& other) const
{	return !operator== (other);
}

// =============================================================================
// -----------------------------------------------------------------------------
double& vertex::operator[] (const Axis ax)
{	return coord ( (ushort) ax);
}

const double& vertex::operator[] (const Axis ax) const
{	return coord ( (ushort) ax);
}

double& vertex::operator[] (const int ax)
{	return coord (ax);
}

const double& vertex::operator[] (const int ax) const
{	return coord (ax);
}

// =============================================================================
// -----------------------------------------------------------------------------
bool vertex::operator== (const vertex& other) const
{	return coord (X) == other[X] &&
		   coord (Y) == other[Y] &&
		   coord (Z) == other[Z];
}

// =============================================================================
// -----------------------------------------------------------------------------
vertex& vertex::operator/= (const double d)
{	for (const Axis ax : g_Axes)
		m_coords[ax] /= d;

	return *this;
}

// =============================================================================
// -----------------------------------------------------------------------------
vertex vertex::operator/ (const double d) const
{	vertex other (*this);
	return other /= d;
}

// =============================================================================
// -----------------------------------------------------------------------------
vertex& vertex::operator+= (const vertex& other)
{	move (other);
	return *this;
}

// =============================================================================
// -----------------------------------------------------------------------------
vertex vertex::operator+ (const vertex& other) const
{	vertex newvert (*this);
	newvert.move (other);
	return newvert;
}

// =============================================================================
// -----------------------------------------------------------------------------
int vertex::operator< (const vertex& other) const
{	if (operator== (other))
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
matrix::matrix (double vals[])
{	for (short i = 0; i < 9; ++i)
		m_vals[i] = vals[i];
}

// =============================================================================
// -----------------------------------------------------------------------------
matrix::matrix (double fillval)
{	for (short i = 0; i < 9; ++i)
		m_vals[i] = fillval;
}

// =============================================================================
// -----------------------------------------------------------------------------
matrix::matrix (initlist<double> vals)
{	assert (vals.size() == 9);
	memcpy (&m_vals[0], & (*vals.begin()), sizeof m_vals);
}

// =============================================================================
// -----------------------------------------------------------------------------
void matrix::puts() const
{	for (short i = 0; i < 3; ++i)
	{	for (short j = 0; j < 3; ++j)
			print ("%1\t", m_vals[ (i * 3) + j]);

		print ("\n");
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
str matrix::stringRep() const
{	str val;

	for (short i = 0; i < 9; ++i)
	{	if (i > 0)
			val += ' ';

		val += ftoa (m_vals[i]);
	}

	return val;
}

// =============================================================================
// -----------------------------------------------------------------------------
void matrix::zero()
{	memset (&m_vals[0], 0, sizeof m_vals);
}

// =============================================================================
// -----------------------------------------------------------------------------
matrix matrix::mult (matrix other) const
{	matrix val;
	val.zero();

	for (short i = 0; i < 3; ++i)
		for (short j = 0; j < 3; ++j)
			for (short k = 0; k < 3; ++k)
				val[ (i * 3) + j] += m_vals[ (i * 3) + k] * other[ (k * 3) + j];

	return val;
}

// =============================================================================
// -----------------------------------------------------------------------------
matrix& matrix::operator= (matrix other)
{	memcpy (&m_vals[0], &other.m_vals[0], sizeof m_vals);
	return *this;
}

// =============================================================================
// -----------------------------------------------------------------------------
double matrix::determinant() const
{	return (val (0) * val (4) * val (8)) +
		   (val (1) * val (5) * val (6)) +
		   (val (2) * val (3) * val (7)) -
		   (val (2) * val (4) * val (6)) -
		   (val (1) * val (3) * val (8)) -
		   (val (0) * val (5) * val (7));
}

// =============================================================================
// -----------------------------------------------------------------------------
StringFormatArg::StringFormatArg (const str& v)
{	m_val = v;
}

StringFormatArg::StringFormatArg (const char& v)
{	m_val = v;
}

StringFormatArg::StringFormatArg (const uchar& v)
{	m_val = v;
}

StringFormatArg::StringFormatArg (const qchar& v)
{	m_val = v;
}

StringFormatArg::StringFormatArg (const float& v)
{	m_val = ftoa (v);
}

StringFormatArg::StringFormatArg (const double& v)
{	m_val = ftoa (v);
}

StringFormatArg::StringFormatArg (const vertex& v)
{	m_val = v.stringRep (false);
}

StringFormatArg::StringFormatArg (const matrix& v)
{	m_val = v.stringRep();
}

StringFormatArg::StringFormatArg (const char* v)
{	m_val = v;
}

StringFormatArg::StringFormatArg (const StringConfig& v)
{	m_val = v.value;
}

StringFormatArg::StringFormatArg (const IntConfig& v)
{	m_val.number (v.value);
}

StringFormatArg::StringFormatArg (const FloatConfig& v)
{	m_val.number (v.value);
}

StringFormatArg::StringFormatArg (const void* v)
{	m_val.sprintf ("%p", v);
}

// =============================================================================
// -----------------------------------------------------------------------------
File::File()
{	// Make a null file
	m_file = null;
	m_textstream = null;
}

File::File (str path, OpenType rtype)
{	m_file = null;
	open (path, rtype);
}

File::File (FILE* fp, OpenType rtype)
{	m_file = null;
	open (fp, rtype);
}

// =============================================================================
// -----------------------------------------------------------------------------
File::~File()
{	if (m_file)
	{	m_file->close();
		delete m_file;

		if (m_textstream)
			delete m_textstream;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
bool File::open (FILE* fp, OpenType rtype)
{	return open ("", rtype, fp);
}

bool File::open (str path, OpenType rtype, FILE* fp)
{	close();

	if (!m_file)
		m_file = new QFile;

	m_file->setFileName (path);

	bool result;

	QIODevice::OpenMode mode =
		(rtype == Read) ? QIODevice::ReadOnly :
		(rtype == Write) ? QIODevice::WriteOnly : QIODevice::Append;

	if (fp)
		result = m_file->open (fp, mode);
	else
		result = m_file->open (mode);

	if (result)
	{	m_textstream = new QTextStream (m_file);
		return true;
	}

	delete m_file;
	m_file = null;
	return false;
}

// =============================================================================
// -----------------------------------------------------------------------------
File::iterator File::begin()
{	return iterator (this);
}

File::iterator& File::end()
{	return m_endIterator;
}

// =============================================================================
// -----------------------------------------------------------------------------
void File::write (str msg)
{	m_file->write (msg.toUtf8(), msg.length());
}

// =============================================================================
// -----------------------------------------------------------------------------
bool File::readLine (str& line)
{	if (!m_textstream || m_textstream->atEnd())
		return false;

	line = m_textstream->readLine();
	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
bool File::atEnd() const
{	assert (m_textstream != null);
	return m_textstream->atEnd();
}

// =============================================================================
// -----------------------------------------------------------------------------
bool File::isNull() const
{	return m_file == null;
}

bool File::operator!() const
{	return isNull();
}

// =============================================================================
// -----------------------------------------------------------------------------
void File::close()
{	if (!m_file)
		return;

	delete m_file;
	m_file = null;

	if (m_textstream)
	{	delete m_textstream;
		m_textstream = null;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
bool File::flush()
{	return m_file->flush();
}

// =============================================================================
// -----------------------------------------------------------------------------
File::operator bool() const
{	return !isNull();
}

// =============================================================================
// -----------------------------------------------------------------------------
void File::rewind()
{	m_file->seek (0);
}

// =============================================================================
// -----------------------------------------------------------------------------
File::iterator::iterator (File* f) : m_file (f)
{	operator++();
}

// =============================================================================
// -----------------------------------------------------------------------------
void File::iterator::operator++()
{	m_gotdata = m_file->readLine (m_text);
}

// =============================================================================
// -----------------------------------------------------------------------------
str File::iterator::operator*()
{	return m_text;
}

// =============================================================================
// The prime contestant for the weirdest operator== 2013 award?
// -----------------------------------------------------------------------------
bool File::iterator::operator== (File::iterator& other)
{	return (other.m_file == null && !m_gotdata);
}

// =============================================================================
// -----------------------------------------------------------------------------
bool File::iterator::operator!= (File::iterator& other)
{	return !operator== (other);
}

// =============================================================================
// -----------------------------------------------------------------------------
LDBoundingBox::LDBoundingBox()
{	reset();
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDBoundingBox::calculate()
{	reset();

	if (!LDFile::current())
		return;

for (LDObject * obj : LDFile::current()->objects())
		calcObject (obj);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDBoundingBox::calcObject (LDObject* obj)
{	switch (obj->getType())
	{	case LDObject::Line:
		case LDObject::Triangle:
		case LDObject::Quad:
		case LDObject::CndLine:

			for (short i = 0; i < obj->vertices(); ++i)
				calcVertex (obj->getVertex (i));

			break;

		case LDObject::Subfile:
		{	LDSubfile* ref = static_cast<LDSubfile*> (obj);
			List<LDObject*> objs = ref->inlineContents (LDSubfile::DeepCacheInline);

		for (LDObject * obj : objs)
			{	calcObject (obj);
				delete obj;
			}
		}
		break;

		default:
			break;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
LDBoundingBox& LDBoundingBox::operator<< (const vertex& v)
{	calcVertex (v);
	return *this;
}

// =============================================================================
// -----------------------------------------------------------------------------
LDBoundingBox& LDBoundingBox::operator<< (LDObject* obj)
{	calcObject (obj);
	return *this;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDBoundingBox::calcVertex (const vertex& v)
{	for (const Axis ax : g_Axes)
	{	if (v[ax] < m_v0[ax])
			m_v0[ax] = v[ax];

		if (v[ax] > m_v1[ax])
			m_v1[ax] = v[ax];
	}

	m_empty = false;
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDBoundingBox::reset()
{	m_v0[X] = m_v0[Y] = m_v0[Z] = 0x7FFFFFFF;
	m_v1[X] = m_v1[Y] = m_v1[Z] = 0xFFFFFFFF;

	m_empty = true;
}

// =============================================================================
// -----------------------------------------------------------------------------
double LDBoundingBox::size() const
{	double xscale = (m_v0[X] - m_v1[X]);
	double yscale = (m_v0[Y] - m_v1[Y]);
	double zscale = (m_v0[Z] - m_v1[Z]);
	double size = zscale;

	if (xscale > yscale)
	{	if (xscale > zscale)
			size = xscale;
	} elif (yscale > zscale)

	size = yscale;

	if (abs (size) >= 2.0f)
		return abs (size / 2);

	return 1.0f;
}

// =============================================================================
// -----------------------------------------------------------------------------
vertex LDBoundingBox::center() const
{	return vertex (
		(m_v0[X] + m_v1[X]) / 2,
		(m_v0[Y] + m_v1[Y]) / 2,
		(m_v0[Z] + m_v1[Z]) / 2);
}
