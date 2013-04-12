/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri `arezey` Piippo
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

#ifndef __COMMON_H__
#define __COMMON_H__

#define APPNAME "ldforge"
#define APPNAME_DISPLAY "LDForge"
#define APPNAME_CAPS "LDFORGE"

#define VERSION_MAJOR 0
#define VERSION_MAJOR_STR "0"
#define VERSION_MINOR 1
#define VERSION_MINOR_STR "1"

// ============---
// #define RELEASE

#define VERSION_STRING VERSION_MAJOR_STR "." VERSION_MINOR_STR

#define CONFIG_WITH_QT

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <stdint.h>
#include <stdarg.h>
#include "str.h"
#include "config.h"
#include "types.h"

#ifdef __GNUC__
 #define FORMAT_PRINTF(M,N) __attribute__ ((format (printf, M, N)))
#else
 #define FORMAT_PRINTF(M,N)
#endif // __GNUC__

#ifdef WIN32
#define DIRSLASH "\\"
#else // WIN32
#define DIRSLASH "/"
#endif // WIN32

#ifdef RELEASE
#define NDEBUG // remove asserts
#endif // RELEASE

static const double fMaxCoord = 10000.0;
static const short dMainColor = 16;
static const short dEdgeColor = 24;

using std::vector;

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
template<class T> inline T clamp (T a, T min, T max) {
	return (a > max) ? max : (a < min) ? min : a;
}

template<class T> inline T min (T a, T b) {
	return (a < b) ? a : b;
}

template<class T> inline T max (T a, T b) {
	return (a > b) ? a : b;
}

static inline const char* qchars (QString qstr) {
	return qstr.toStdString ().c_str ();
}

static const double pi = 3.14159265358979323846f;

// -----------------------------------------------------------------------------
// main.cpp
enum logtype_e {
	LOG_Normal,
	LOG_Success,
	LOG_Info,
	LOG_Warning,
	LOG_Error,
	LOG_Dev,
};

void logf (const char* fmt, ...) FORMAT_PRINTF (1, 2);
void logf (logtype_e eType, const char* fmt, ...) FORMAT_PRINTF (2, 3);

// -----------------------------------------------------------------------------
// Vertex at (0, 0, 0)
extern const vertex g_Origin;

// -----------------------------------------------------------------------------
// Pointer to the OpenFile which is currently being edited by the user.
extern OpenFile* g_CurrentFile;

// -----------------------------------------------------------------------------
// Pointer to the bounding box.
extern bbox g_BBox;

// -----------------------------------------------------------------------------
// Vector of all currently opened files.
extern vector<OpenFile*> g_LoadedFiles;

// -----------------------------------------------------------------------------
// Pointer to the main application.
extern QApplication* g_qMainApp;

// -----------------------------------------------------------------------------
// Identity matrix
extern const matrix g_mIdentity;

typedef unsigned int uint;
typedef unsigned long ulong;

// Typedef out the _t suffices :)
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#endif