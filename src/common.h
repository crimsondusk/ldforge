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

// =============================================================================
// This file is included one way or another in every source file of LDForge.
// Stuff defined and included here is universally included.

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <stdint.h>
#include <stdarg.h>
#include <QString>
#include <QMutex>

#include "string.h"
#include "config.h"
#include "types.h"

#define APPNAME "LDForge"
#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 999
#define BUILD_ID BUILD_INTERNAL

// =============================================
#if (BUILD_ID != BUILD_INTERNAL)
#define RELEASE
#endif // BUILD_ID

#define BUILD_INTERNAL	0
#define BUILD_ALPHA		1
#define BUILD_BETA		2
#define BUILD_RC			3
#define BUILD_RELEASE		4

#ifndef RELEASE
# define devf(...) doDevf (__func__, __VA_ARGS__);
#else
# define devf(...)
#endif // RELEASE

class QFile;
extern QFile g_file_stdout;
extern QFile g_file_stderr;

void doDevf (const char* func, const char* fmtstr, ...);

// Version string identifier
str versionString ();
const char* versionMoniker ();
str fullVersionString ();

#ifdef __GNUC__
# define FORMAT_PRINTF(M,N) __attribute__ ((format (printf, M, N)))
# define ATTR(N) __attribute__ ((N))
#else
# define FORMAT_PRINTF(M,N)
# define ATTR(N)
#endif // __GNUC__

#ifdef WIN32
#define DIRSLASH "\\"
#define DIRSLASH_CHAR '\\'
#else // WIN32
#define DIRSLASH "/"
#define DIRSLASH_CHAR '/'
#endif // WIN32

#ifdef RELEASE
#define NDEBUG // remove asserts
#endif // RELEASE

#define PROP_NAME(GET) m_##GET

#define READ_ACCESSOR(T, GET) \
	T const& GET () const

#define SET_ACCESSOR(T, SET) \
	void SET (T val)

// Read-only, private property with a get accessor
#define DECLARE_READ_PROPERTY(T, GET, SET) \
private: \
	T PROP_NAME (GET); \
	SET_ACCESSOR (T, SET); \
public: \
	READ_ACCESSOR (T, GET);

// Read/write private property with get and set accessors
#define DECLARE_PROPERTY(T, GET, SET) \
private: \
	T PROP_NAME (GET); \
public: \
	READ_ACCESSOR (T, GET); \
	SET_ACCESSOR (T, SET);

#define DEFINE_PROPERTY(T, CLASS, GET, SET) \
	READ_ACCESSOR (T, CLASS::GET) { return PROP_NAME (GET); } \
	SET_ACCESSOR (T, CLASS::SET) { PROP_NAME (GET) = val; }

// Shortcuts
#define PROPERTY(T, GET, SET) \
private: \
	T PROP_NAME (GET); \
public: \
	READ_ACCESSOR (T, GET) { return PROP_NAME (GET); } \
	SET_ACCESSOR (T, SET) { PROP_NAME (GET) = val; }

#define READ_PROPERTY(T, GET, SET) \
private: \
	T PROP_NAME (GET); \
	SET_ACCESSOR (T, SET) { PROP_NAME (GET) = val; } \
public: \
	READ_ACCESSOR (T, GET) { return PROP_NAME (GET); }

#ifdef null
#undef null
#endif // null

#ifdef __GNUC__
#define FUNCNAME __PRETTY_FUNCTION__
#else
#define FUNCNAME __func__
#endif // __GNUC__

// printf replacement
#ifndef IN_IDE_PARSER
#define print(...) doPrint (g_file_stdout, {__VA_ARGS__})
#define fprint(F, ...) doPrint (F, {__VA_ARGS__})
#else
void print (const char* fmtstr, ...);
void fprint (QFile& f, const char* fmtstr, ...);
#endif
void doPrint (QFile& f, initlist<StringFormatArg> args);
void doPrint (FILE* f, initlist<StringFormatArg> args);

// Replace assert with a version that shows a GUI dialog if possible
#ifdef assert
#undef assert
#endif // assert

void assertionFailure (const char* file, const ulong line, const char* funcname, const char* expr);
void fatalError (const char* file, const ulong line, const char* funcname, str errmsg);

#define assert(N) \
	(N) ? ((void)(0)) : assertionFailure(__FILE__, __LINE__, FUNCNAME, #N)

#define fatal(MSG) \
	fatalError (__FILE__, __LINE__, FUNCNAME, MSG)

// Null pointer
static const std::nullptr_t null = nullptr;

// Main and edge color identifiers
static const short maincolor = 16;
static const short edgecolor = 24;

static const short lores = 16;
static const short hires = 48;

static const bool yup = true;
static const bool nope = false;

class ForgeWindow;
class LDObject;
class bbox;
class LDOpenFile;
class QApplication;

// -----------------------------------------------------------------------------
// Plural expression
template<class T> static inline const char* plural (T n) {
	return (n != 1) ? "s" : "";
}

// -----------------------------------------------------------------------------
// Templated clamp
template<class T> static inline T clamp (T a, T min, T max) {
	return (a > max) ? max : (a < min) ? min : a;
}

// Templated minimum
template<class T> static inline T min (T a, T b) {
	return (a < b) ? a : b;
}

// Templated maximum
template<class T> static inline T max (T a, T b) {
	return (a > b) ? a : b;
}

// Templated absolute value
template<class T> static inline T abs (T a) {
	return (a >= 0) ? a : -a;
}

// Quick QString to const char* conversion
static inline const char* qchars (const QString& qstr) {
	return qstr.toStdString ().c_str ();
}

static const double pi = 3.14159265358979323846f;

// -----------------------------------------------------------------------------
#ifdef IN_IDE_PARSER // KDevelop workarounds:
# error IN_IDE_PARSER is defined (this code is only for KDevelop workarounds)

// Current function name
static const char* __func__ = "";

# ifndef va_start
#  define va_start(va, arg)
# endif // va_start

# ifndef va_end
#  define va_end(va)
# endif // va_end

typedef void FILE; // :|
#endif // IN_IDE_PARSER

// -----------------------------------------------------------------------------
enum LogType {
	LOG_Normal,
	LOG_Warning,
	LOG_Error,
	LOG_Dev,
};

// logf - universal access to the message log. Defined here so that I don't have
// to include gui.h here and recompile everything every time that file changes.
// logf is defined in main.cpp
void logf (const char* fmtstr, ...) FORMAT_PRINTF (1, 2);
void logf (LogType type, const char* fmtstr, ...) FORMAT_PRINTF (2, 3);
// -----------------------------------------------------------------------------
// Vertex at (0, 0, 0)
extern const vertex g_origin;

// -----------------------------------------------------------------------------
// Pointer to the OpenFile which is currently being edited by the user.
extern LDOpenFile* g_curfile;

// -----------------------------------------------------------------------------
// Pointer to the bounding box.
extern bbox g_BBox;

// -----------------------------------------------------------------------------
// Vector of all currently opened files.
extern vector<LDOpenFile*> g_loadedFiles;

// -----------------------------------------------------------------------------
// Pointer to the main application.
extern const QApplication* g_app;

// -----------------------------------------------------------------------------
// Identity matrix
extern const matrix g_identity;

#endif // COMMON_H