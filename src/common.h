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
#include "string.h"
#include "config.h"
#include "types.h"

#define APPNAME "LDForge"

#define VERSION_MAJOR 0
#define VERSION_MAJOR_STR "0"
#define VERSION_MINOR 1
#define VERSION_MINOR_STR "1"

// ============---
// #define RELEASE

#ifndef RELEASE
# define devf(...) doDevf (__func__, __VA_ARGS__);
#else
# define devf(...)
#endif // RELEASE

void doDevf (const char* func, const char* fmtstr, ...);

// Version string identifier
static const str versionString = fmt ("%d.%d", VERSION_MAJOR, VERSION_MINOR);

#ifdef __GNUC__
# define FORMAT_PRINTF(M,N) __attribute__ ((format (printf, M, N)))
# define ATTR(N) __attribute__ ((N))
#else
# define FORMAT_PRINTF(M,N)
# define ATTR(N)
#endif // __GNUC__

#ifdef WIN32
#define DIRSLASH "\\"
#else // WIN32
#define DIRSLASH "/"
#endif // WIN32

#ifdef RELEASE
#define NDEBUG // remove asserts
#endif // RELEASE

#ifdef null
#undef null
#endif // null

// Null pointer
static const std::nullptr_t null = nullptr;

// Main and edge color identifiers
static const short maincolor = 16;
static const short edgecolor = 24;

static const bool yup = true;
static const bool nope = false;

class ForgeWindow;
class LDObject;
class bbox;
class OpenFile;
class QApplication;

// =============================================================================
// Plural expression
#define PLURAL(n) ((n != 1) ? "s" : "")

// -----------------------------------------------------------------------------
// Shortcut for formatting
#define PERFORM_FORMAT(in, out) \
	va_list v; \
	va_start (v, in); \
	char* out = vdynformat (in, v, 256); \
	va_end (v);

// -----------------------------------------------------------------------------
// Shortcuts for stuffing vertices into printf-formatting.
#define FMT_VERTEX "(%.3f, %.3f, %.3f)"
#define FVERTEX(V) V.x, V.y, V.z

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
static inline const char* qchars (QString qstr) {
	return qstr.toStdString ().c_str ();
}

static const double pi = 3.14159265358979323846f;

#ifdef IN_IDE_PARSER // KDevelop workaround
// Current function name
static const char* __func__ = "";
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
extern OpenFile* g_curfile;

// -----------------------------------------------------------------------------
// Pointer to the bounding box.
extern bbox g_BBox;

// -----------------------------------------------------------------------------
// Vector of all currently opened files.
extern vector<OpenFile*> g_loadedFiles;

// -----------------------------------------------------------------------------
// Pointer to the main application.
extern const QApplication* g_app;

// -----------------------------------------------------------------------------
// Identity matrix
extern const matrix g_identity;

#endif // COMMON_H