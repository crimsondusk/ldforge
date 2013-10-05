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

#ifndef LDFORGE_COMMON_H
#define LDFORGE_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <QString>
#include <QMutex>

#include "config.h"

#define APPNAME "LDForge"
#define VERSION_MAJOR 0
#define VERSION_MINOR 2
#define VERSION_PATCH 999
#define BUILD_ID BUILD_INTERNAL

#define BUILD_INTERNAL 0
#define BUILD_ALPHA    1
#define BUILD_BETA     2
#define BUILD_RC       3
#define BUILD_RELEASE  4

// RC Number for BUILD_RC
#define RC_NUMBER      0

// =============================================
#if (BUILD_ID != BUILD_INTERNAL)
#define RELEASE
#endif // BUILD_ID

#define alias auto&
#define elif else if

// Null pointer
static const std::nullptr_t null = nullptr;

#ifdef __GNUC__
# define FORMAT_PRINTF(M,N) __attribute__ ((format (printf, M, N)))
# define ATTR(N) __attribute__ ((N))
#else
# define FORMAT_PRINTF(M,N)
# define ATTR(N)
#endif // __GNUC__

#ifdef WIN32
# define DIRSLASH "\\"
# define DIRSLASH_CHAR '\\'
#else // WIN32
# define DIRSLASH "/"
# define DIRSLASH_CHAR '/'
#endif // WIN32

#ifdef RELEASE
# define NDEBUG // remove asserts
#endif // RELEASE

#define PROP_NAME(GET) m_##GET

#define READ_ACCESSOR(T, GET) \
	T const& GET() const

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

// Property whose set accessor is a public slot
// TODO: make this replace PROPERTY
#define SLOT_PROPERTY (T, GET, SET) \
private: \
	T PROP_NAME (GET); \
public: \
	READ_ACCESSOR (T, GET) { return PROP_NAME (GET); } \
public slots: \
	SET_ACCESSOR (T, SET) { PROP_NAME (GET) = val; }

#ifdef null
#undef null
#endif // null

#ifdef __GNUC__
#define FUNCNAME __PRETTY_FUNCTION__
#else
#define FUNCNAME __func__
#endif // __GNUC__

// Replace assert with a version that shows a GUI dialog if possible.
// On Windows I just can't get the actual error messages otherwise.
#ifdef assert
#undef assert
#endif // assert

void assertionFailure (const char* file, int line, const char* funcname, const char* expr);
#define assert(N) ((N) ? (void) 0 : assertionFailure (__FILE__, __LINE__, FUNCNAME, #N))

// Version string identifier
QString versionString();
QString versionMoniker();
QString fullVersionString();

// -----------------------------------------------------------------------------
#ifdef IN_IDE_PARSER // KDevelop workarounds:
# error IN_IDE_PARSER is defined (this code is only for KDevelop workarounds)

# ifndef va_start
#  define va_start(va, arg)
# endif // va_start

# ifndef va_end
#  define va_end(va)
# endif // va_end

static const char* __func__ = ""; // Current function name
typedef void FILE; // :|
#endif // IN_IDE_PARSER

#endif // LDFORGE_COMMON_H
