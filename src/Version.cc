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

#include <stdio.h>
#include <string.h>
#include "Version.h"
#include "Git.h"

char gVersionString[64] = {'\0'};
char gFullVersionString[256] = {'\0'};

// -----------------------------------------------------------------------------
//
const char* versionString()
{
	if (gVersionString[0] == '\0')
	{
#if VERSION_PATCH == 0
		sprintf (gVersionString, "%d.%d", VERSION_MAJOR, VERSION_MINOR);
#else
		sprintf (gVersionString, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
#endif // VERSION_PATCH
	}

	return gVersionString;
}

// -----------------------------------------------------------------------------
//
const char* fullVersionString()
{
	if (gFullVersionString[0] == '\0')
	{
#if BUILD_ID != BUILD_RELEASE
		strcpy (gFullVersionString, GIT_DESCRIPTION);
#else
		sprintf (gFullVersionString, "v%s", versionString());
#endif
	}

	return gFullVersionString;
}
