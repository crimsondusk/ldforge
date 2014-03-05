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
#include "PropertyMacro.h"

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

template<class T>
using initlist = std::initializer_list<T>;

template<class T, class R>
using pair = std::pair<T, R>;

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

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// matrix
//
// A mathematical 3 x 3 matrix
// =============================================================================
class Matrix
{
	public:
		Matrix() {}
		Matrix (initlist<double> vals);
		Matrix (double fillval);
		Matrix (double vals[]);

		double			getDeterminant() const;
		Matrix			mult (const Matrix& other) const;
		void			puts() const;
		QString			toString() const;
		void			zero();
		Matrix&			operator= (const Matrix& other);

		inline double& val (int idx)
		{
			return m_vals[idx];
		}

		inline const double& val (int idx) const
		{
			return m_vals[idx];
		}

		inline Matrix operator* (const Matrix& other) const
		{
			return mult (other);
		}

		inline double& operator[] (int idx)
		{
			return val (idx);
		}

		inline const double& operator[] (int idx) const
		{
			return val (idx);
		}

		bool operator== (const Matrix& other) const;

	private:
		double m_vals[9];
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// Vertex
//
// Vertex class, contains a single point in 3D space. Not to be confused with
// LDVertex, which is a vertex used in an LDraw part file.
// =============================================================================
class Vertex
{
	public:
		Vertex() {}
		Vertex (double x, double y, double z);

		double			distanceTo (const Vertex& other) const;
		Vertex			midpoint (const Vertex& other);
		void			move (const Vertex& other);
		QString			toString (bool mangled) const;
		void			transform (const Matrix& matr, const Vertex& pos);

		Vertex&			operator+= (const Vertex& other);
		Vertex			operator+ (const Vertex& other) const;
		Vertex			operator/ (const double d) const;
		Vertex&			operator/= (const double d);
		bool			operator== (const Vertex& other) const;
		bool			operator!= (const Vertex& other) const;
		Vertex			operator-() const;
		int				operator< (const Vertex& other) const;

		inline double& operator[] (const Axis ax)
		{
			return getCoordinate ((int) ax);
		}

		inline const double& operator[] (const Axis ax) const
		{
			return getCoordinate ((int) ax);
		}

		inline double& operator[] (const int ax)
		{
			return getCoordinate (ax);
		}

		inline const double& operator[] (const int ax) const
		{
			return getCoordinate (ax);
		}

		inline double& getCoordinate (int n)
		{
			return m_coords[n];
		}

		inline const double& getCoordinate (int n) const
		{
			return m_coords[n];
		}

		inline double& x()
		{
			return m_coords[X];
		}

		inline const double& x() const
		{
			return m_coords[X];
		}

		inline double& y()
		{
			return m_coords[Y];
		}

		inline const double& y() const
		{
			return m_coords[Y];
		}

		inline double& z()
		{
			return m_coords[Z];
		}

		inline const double& z() const
		{
			return m_coords[Z];
		}

	private:
		double m_coords[3];
};

Q_DECLARE_METATYPE (Vertex)

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// StringFormatArg
//
// Converts a given value into a string that can be retrieved with ::value().
// Used as the argument type to the formatting functions, hence its name.
// =============================================================================
class StringFormatArg
{
	public:
		StringFormatArg (const QString& a) : m_val (a) {}
		StringFormatArg (const char& a) : m_val (a) {}
		StringFormatArg (const uchar& a) : m_val (a) {}
		StringFormatArg (const QChar& a) : m_val (a) {}
		StringFormatArg (int a) : m_val (QString::number (a)) {}
		StringFormatArg (const float& a) : m_val (QString::number (a)) {}
		StringFormatArg (const double& a) : m_val (QString::number (a)) {}
		StringFormatArg (const Vertex& a) : m_val (a.toString (false)) {}
		StringFormatArg (const Matrix& a) : m_val (a.toString()) {}
		StringFormatArg (const char* a) : m_val (a) {}

		StringFormatArg (const void* a)
		{
			m_val.sprintf ("%p", a);
		}

		template<class T> StringFormatArg (const QList<T>& a)
		{
			m_val = "{ ";

			for (const T& it : a)
			{
				if (&it != &a.first())
					m_val += ", ";

				StringFormatArg arg (it);
				m_val += arg.value();
			}

			if (!a.isEmpty())
				m_val += " ";

			m_val += "}";
		}

		inline QString value() const
		{
			return m_val;
		}

	private:
		QString m_val;
};

// =============================================================================
// LDBoundingBox
//
// The bounding box is the box that encompasses a given set of objects.
// v0 is the minimum vertex, v1 is the maximum vertex.
// =============================================================================
class LDBoundingBox
{
	PROPERTY (private,	bool,		Empty,		BOOL_OPS,	STOCK_WRITE)
	PROPERTY (private,	Vertex,		Vertex0,	NO_OPS,		STOCK_WRITE)
	PROPERTY (private,	Vertex,		Vertex1,	NO_OPS,		STOCK_WRITE)

	public:
		LDBoundingBox();
		void reset();
		void calculate();
		double size() const;
		void calcObject (LDObject* obj);
		void calcVertex (const Vertex& v);
		Vertex center() const;

		LDBoundingBox& operator<< (LDObject* obj);
		LDBoundingBox& operator<< (const Vertex& v);
};

// Formatter function
QString DoFormat (QList<StringFormatArg> args);

// printf replacement
void doPrint (QFile& f, QList<StringFormatArg> args);
void doPrint (FILE* fp, QList<StringFormatArg> args);

// log() - universal access to the message log. Defined here so that I don't have
// to include MessageLog.h here and recompile everything every time that file changes.
void DoLog (std::initializer_list<StringFormatArg> args);

// Macros to access these functions
# ifndef IN_IDE_PARSER 
#define fmt(...) DoFormat ({__VA_ARGS__})
# define fprint(F, ...) doPrint (F, {__VA_ARGS__})
# define log(...) DoLog({ __VA_ARGS__ })
#else
QString fmt (const char* fmtstr, ...);
void fprint (QFile& f, const char* fmtstr, ...);
void fprint (FILE* fp, const char* fmtstr, ...);
void log (const char* fmtstr, ...);
#endif

extern const Vertex g_origin; // Vertex at (0, 0, 0)
extern const Matrix g_identity; // Identity matrix

static const double pi = 3.14159265358979323846;
