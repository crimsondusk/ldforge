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

//! \file version.h
//! Contains macros related to application name and version.

#pragma once

//! The application name.
#define APPNAME			"LDForge"

//! The unix-style name of the application. used in filenames
#define UNIXNAME		"ldforge"

//! The major version number.
#define VERSION_MAJOR	0

//! The minor version number.
#define VERSION_MINOR	3

//! The patch level version number.
#define VERSION_PATCH	0

//! The build ID, use either BUILD_INTERNAL or BUILD_RELEASE
#define BUILD_ID		BUILD_INTERNAL

//! The build code for internal builds
#define BUILD_INTERNAL	0

//! The build code for release builds.
#define BUILD_RELEASE	1

// =============================================
#ifdef DEBUG
# undef RELEASE
#endif // DEBUG

#ifdef RELEASE
# undef DEBUG
#endif // RELEASE

const char* versionString();
const char* fullVersionString();
const char* compileTimeString();
