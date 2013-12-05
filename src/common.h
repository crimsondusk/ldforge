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

// Hack to make KDevelop parse QString properly. Q_REQUIRED_RESULT expands into
// an __attribute__((warn_unused_result)) KDevelop's lexer doesn't seem to
// understand, yielding an error and leaving some methods unlexed.
//
// The following first includes <QChar> to get Q_REQUIRED_RESULT defined first,
// then re-defining it as nothing. This causes Q_REQUIRED_RESULT to essentially
// "vanish" from QString's methods when KDevelop lexes them.
//
// Similar reasoning for Q_DECL_HIDDEN, except with Q_OBJECT this time.
#ifdef IN_IDE_PARSER
# include <QChar>
# undef Q_REQUIRED_RESULT
# undef Q_DECL_HIDDEN
# define Q_REQUIRED_RESULT
# define Q_DECL_HIDDEN
#endif

#include <stdio.h>
#include <stdlib.h>
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
#ifdef DEBUG
# undef RELEASE
#endif // DEBUG

#ifdef RELEASE
# undef DEBUG
#endif // RELEASE

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

#define PROPERTY( ACCESS, TYPE, NAME, OPS, CALLBACK )			\
	private:																	\
		TYPE m_##NAME;														\
		DEFINE_##CALLBACK( NAME )										\
																				\
	public:																	\
		inline TYPE const& GET_READ_METHOD( NAME, OPS ) const	\
		{	return m_##NAME; 												\
		}																		\
																				\
	ACCESS:																	\
		inline void set##NAME( TYPE const& NAME##_ )				\
		{	m_##NAME = NAME##_;											\
			TRIGGER_CALLBACK( NAME, CALLBACK )						\
		}																		\
																				\
		DEFINE_PROPERTY_##OPS( TYPE, NAME, CALLBACK )

#define GET_READ_METHOD( NAME, OPS ) \
	GET_READ_METHOD_##OPS( NAME )

#define GET_READ_METHOD_BOOL_OPS( NAME ) is##NAME()
#define GET_READ_METHOD_NO_OPS( NAME ) get##NAME()
#define GET_READ_METHOD_STR_OPS( NAME ) get##NAME()
#define GET_READ_METHOD_NUM_OPS( NAME ) get##NAME()

#define DEFINE_WITH_CB( NAME ) void NAME##Changed();
#define DEFINE_NO_CB( NAME )

#define DEFINE_PROPERTY_NO_OPS( TYPE, NAME, CALLBACK  )

#define DEFINE_PROPERTY_STR_OPS( TYPE, NAME, CALLBACK )	\
		void append##NAME( TYPE a )								\
		{	m_##NAME.append( a );									\
			TRIGGER_CALLBACK( NAME, CALLBACK )					\
		}																	\
																			\
		void prepend##NAME( TYPE a )								\
		{	m_##NAME.prepend( a );									\
			TRIGGER_CALLBACK( NAME, CALLBACK )					\
		}																	\
																			\
		void replaceIn##NAME( TYPE a, TYPE b )					\
		{	m_##NAME.replace( a, b );								\
			TRIGGER_CALLBACK( NAME, CALLBACK )					\
		}

#define DEFINE_PROPERTY_NUM_OPS( TYPE, NAME, CALLBACK )	\
		inline void increase##NAME( TYPE a = 1 )				\
		{	m_##NAME += a;												\
			TRIGGER_CALLBACK( NAME, CALLBACK )					\
		}																	\
																			\
		inline void decrease##NAME( TYPE a = 1 )				\
		{	m_##NAME -= a;												\
			TRIGGER_CALLBACK( NAME, CALLBACK )					\
		}

#define DEFINE_PROPERTY_BOOL_OPS( TYPE, NAME, CALLBACK )	\
		inline void toggle##NAME()									\
		{	m_##NAME = !m_##NAME;									\
			TRIGGER_CALLBACK( NAME, CALLBACK )					\
		}

#define TRIGGER_CALLBACK( NAME, CALLBACK ) \
	TRIGGER_CALLBACK_##CALLBACK( NAME );

#define TRIGGER_CALLBACK_NO_CB( NAME )
#define TRIGGER_CALLBACK_WITH_CB( NAME ) \
	NAME##Changed();

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
void assertionFailure (const char* file, int line, const char* funcname, const char* expr);

#ifdef assert
# undef assert
#endif // assert

#ifdef DEBUG
# define assert(N) { ((N) ? (void) 0 : assertionFailure (__FILE__, __LINE__, FUNCNAME, #N)); }
#else
# define assert(N) {}
#endif // DEBUG

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
