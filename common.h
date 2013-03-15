#ifndef __COMMON_H__
#define __COMMON_H__

#define APPNAME "ldforge"
#define APPNAME_DISPLAY "LDForge"
#define APPNAME_CAPS "LDFORGE"

#define VERSION_MAJOR 0
#define VERSION_MAJOR_STR "0"
#define VERSION_MINOR 1
#define VERSION_MINOR_STR "1"

#define VERSION_STRING VERSION_MAJOR_STR "." VERSION_MINOR_STR

#define CONFIG_WITH_QT

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <stdint.h>
#include "stdarg.h"
#include "str.h"
#include "config.h"

#ifdef __GNUC__
 #define FORMAT_PRINTF(M,N) __attribute__ ((format (printf, M, N)))
#else
 #define FORMAT_PRINTF(M,N)
#endif // __GNUC__

using std::vector;

class LDForgeWindow;
class LDObject;
class bbox;
class OpenFile;

// =============================================================================
// vertex (v)
// 
// Vertex class. Not to be confused with LDVertex, which is a vertex used in an
// LDraw code file.
//
// Methods:
// - midpoint (vertex&): returns a midpoint
// =============================================================================
class vertex {
public:
	double x, y, z;
	
	// =========================================================================
	// Midpoint between this vertex and another vertex.
	vertex midpoint (vertex& other);
	str getStringRep ();
};

// =============================================================================
// bearing
// 
// A bearing is a combination of an angle and a pitch. Essentially a 3D angle.
// The project method projects a vertex from a given vertex by a given length.
// 
// Prefix: g, since b is bool
// =============================================================================
class bearing {
	double fAngle, fPitch;
	
	vertex project (vertex& vSource, ulong ulLength);
};

// =============================================================================
// Plural expression
#define PLURAL(n) ((n != 1) ? "s" : "")

// Shortcut for formatting
#define PERFORM_FORMAT(in, out) \
	va_list v; \
	va_start (v, in); \
	char* out = vdynformat (in, v, 256); \
	va_end (v);

// Shortcuts for stuffing vertices into printf-formatting.
#define FMT_VERTEX "(%.3f, %.3f, %.3f)"
#define FVERTEX(V) V.x, V.y, V.z

typedef unsigned char byte;

template<class T> inline T clamp (T a, T min, T max) {
	return (a > max) ? max : (a < min) ? min : a;
}

template<class T> inline T min (T a, T b) {
	return (a < b) ? a : b;
}

template<class T> inline T max (T a, T b) {
	return (a > b) ? a : b;
}

static const double pi = 3.14159265358979323846f;

// main.cpp
enum logtype_e {
	LOG_Normal,
	LOG_Success,
	LOG_Info,
	LOG_Warning,
	LOG_Error,
};

void logf (const char* fmt, ...) FORMAT_PRINTF (1, 2);
void logf (logtype_e eType, const char* fmt, ...) FORMAT_PRINTF (2, 3);

extern OpenFile* g_CurrentFile;
extern bbox g_BBox;
extern LDForgeWindow* g_qWindow;
extern vector<OpenFile*> g_LoadedFiles;

#ifndef unix
typedef unsigned int uint;
typedef unsigned long ulong;
#endif // unix

typedef int8_t xchar;
typedef int16_t xshort;
typedef int32_t xlong;
typedef int64_t xlonglong;
typedef uint8_t xuchar;
typedef uint16_t xushort;
typedef uint32_t xulong;
typedef uint64_t xulonglong;

#endif