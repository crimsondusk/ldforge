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

// =============================================================================
// This file is included one way or another in every source file of LDForge.
// Stuff defined and included here is universally included.

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <QString>
#include <QTextFormat>

#include "Version.h"
#include "PropertyMacro.h"
#include "Configuration.h"

// =============================================
#define alias auto&
#define elif(A) else if (A)

// Null pointer
static const std::nullptr_t null = nullptr;

#ifdef WIN32
# define DIRSLASH "\\"
# define DIRSLASH_CHAR '\\'
#else // WIN32
# define DIRSLASH "/"
# define DIRSLASH_CHAR '/'
#endif // WIN32

#ifdef __GNUC__
#define FUNCNAME __PRETTY_FUNCTION__
#else
#define FUNCNAME __func__
#endif // __GNUC__

#ifdef IN_IDE_PARSER
void dlog(void, ...) {}
#else
# ifdef DEBUG
#  define dlog(...) log (QString (__PRETTY_FUNCTION__) + ": " __VA_ARGS__)
# else
#  define dlog(...)
# endif // DEBUG
#endif // IN_IDE_PARSER

#define dvalof(A) dlog ("value of '%1' = %2\n", #A, A)

// Replace assert with a version that shows a GUI dialog if possible.
// On Windows I just can't get the actual error messages otherwise.
void assertionFailure (const char* file, int line, const char* funcname, const char* expr);

#undef assert

#ifdef DEBUG
# define assert(N) { ((N) ? (void) 0 : assertionFailure (__FILE__, __LINE__, FUNCNAME, #N)); }
#else
# define assert(N) {}
#endif // DEBUG

// Version string identifier. These are defined in Version.cc.
const char* versionString();
const char* fullVersionString();

#define properties private
#define typedefs public
#define for_axes(AX) for (const Axis AX : std::initializer_list<const Axis> ({X, Y, Z}))

// =============================================================================
#ifdef IN_IDE_PARSER // KDevelop workarounds:
# error IN_IDE_PARSER is defined (this code is only for KDevelop workarounds)
# define COMPILE_DATE "14-01-10 10:31:09"

# ifndef va_start
#  define va_start(va, arg)
# endif // va_start

# ifndef va_end
#  define va_end(va)
# endif // va_end

static const char* __func__ = ""; // Current function name
typedef void FILE; // :|
#endif // IN_IDE_PARSER
