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

#pragma once
#include <QString>
#include "basics.h"

//! \file Format.h
//! Contains string formatting-related functions and classes.

//!
//! Converts a given value into a string that can be retrieved with text().
//! Used as the argument type to the formatting functions, hence its name.
//!
class StringFormatArg
{
	public:
		StringFormatArg (const QString& a) : m_text (a) {}
		StringFormatArg (const char& a) : m_text (a) {}
		StringFormatArg (const uchar& a) : m_text (a) {}
		StringFormatArg (const QChar& a) : m_text (a) {}
		StringFormatArg (int a) : m_text (QString::number (a)) {}
		StringFormatArg (long a) : m_text (QString::number (a)) {}
		StringFormatArg (const float& a) : m_text (QString::number (a)) {}
		StringFormatArg (const double& a) : m_text (QString::number (a)) {}
		StringFormatArg (const Vertex& a) : m_text (a.toString()) {}
		StringFormatArg (const Matrix& a) : m_text (a.toString()) {}
		StringFormatArg (const char* a) : m_text (a) {}

		StringFormatArg (const void* a)
		{
			m_text.sprintf ("%p", a);
		}

		template<typename T>
		StringFormatArg (const QList<T>& a)
		{
			m_text = "{";

			for (const T& it : a)
			{
				if (&it != &a.first())
					m_text += ", ";

				StringFormatArg arg (it);
				m_text += arg.text();
			}

			m_text += "}";
		}

		inline QString text() const
		{
			return m_text;
		}

	private:
		QString m_text;
};

//!
//! Helper function for \c format
//!
template<typename Arg1, typename... Rest>
void formatHelper (QString& str, Arg1 arg1, Rest... rest)
{
	str = str.arg (StringFormatArg (arg1).text());
	formatHelper (str, rest...);
}

//!
//! Overload of \c formatHelper() with no template args
//!
static void formatHelper (QString& str) __attribute__ ((unused));
static void formatHelper (QString& str)
{
	(void) str;
}

//!
//! @brief Format the message with the given args.
//!
//! The formatting ultimately uses QString's arg() method to actually format
//! the args so the format string should be prepared accordingly, with %1
//! referring to the first arg, %2 to the second, etc.
//!
//! \param fmtstr The string to format
//! \param args The args to format with
//! \return The formatted string
//!
template<typename... Args>
QString format (QString fmtstr, Args... args)
{
	formatHelper (fmtstr, args...);
	return fmtstr;
}

//!
//! From MessageLog.cc - declared here so that I don't need to include
//! messageLog.h here. Prints the given message to log.
//!
void printToLog (const QString& msg);

//!
//! Format and print the given args to the message log.
//! \param fmtstr The string to format
//! \param args The args to format with
//!
template<typename... Args>
void print (QString fmtstr, Args... args)
{
	formatHelper (fmtstr, args...);
	printToLog (fmtstr);
}

//!
//! Format and print the given args to the given file descriptor
//! \param fp The file descriptor to print to
//! \param fmtstr The string to format
//! \param args The args to format with
//!
template<typename... Args>
void fprint (FILE* fp, QString fmtstr, Args... args)
{
	formatHelper (fmtstr, args...);
	fprintf (fp, "%s", qPrintable (fmtstr));
}

//!
//! Overload of \c fprint with a QIODevice
//! \param dev The IO device to print to
//! \param fmtstr The string to format
//! \param args The args to format with
//!
template<typename... Args>
void fprint (QIODevice& dev, QString fmtstr, Args... args)
{
	formatHelper (fmtstr, args...);
	dev.write (fmtstr.toUtf8());
}

//!
//! Exactly like print() except no-op in release builds.
//! \param fmtstr The string to format
//! \param args The args to format with
//!
template<typename... Args>
void dprint (QString fmtstr, Args... args)
{
#ifndef RELEASE
	formatHelper (fmtstr, args...);
	printToLog (fmtstr);
#else
	(void) fmtstr;
	(void) args;
#endif
}
