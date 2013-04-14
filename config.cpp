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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <QDir>
#include "common.h"
#include "str.h"
#include "config.h"

std::vector<config*> g_pConfigPointers;

// =============================================================================
const char* g_WeekdayNames[7] = {
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
};

// =============================================================================
static const char* g_MonthNames[12] = {
	"Januray",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November"
	"December",
};

static const char* g_ConfigTypeNames[] = {
	"None",
	"Integer",
	"String",
	"Float",
	"Boolean",
	"Key sequence",
};

// =============================================================================
// Load the configuration from file
bool config::load () {
	printf ("config::load: loading configuration file.\n");
	
	// Locale must be disabled for atof
	setlocale (LC_NUMERIC, "C");
	
	FILE* fp = fopen (filepath().chars(), "r");
	char linedata[MAX_INI_LINE];
	char* line;
	size_t ln = 0;
	
	if (!fp)
		return false; // can't open for reading
	
	// Read the values.
	while (fgets (linedata, MAX_INI_LINE, fp)) {
		ln++;
		line = linedata;
		
		while (*line != 0 && (*line <= 32 || *line >= 127))
			line++; // Skip junk
		
		if (*line == '\0' || line[0] == '#')
			continue; // Empty line or comment.
		
		// Find the equals sign.
		char* equals = strchr (line, '=');
		if (!equals) {
			fprintf (stderr, "couldn't find `=` sign in entry `%s`\n", line);
			continue;
		}
		
		str entry = str (line).substr (0, equals - line);
		
		// Find the config entry for this.
		config* cfg = null;
		for (config* i : g_pConfigPointers)
			if (entry == i->name)
				cfg = i;
		
		if (!cfg) {
			fprintf (stderr, "unknown config `%s`\n", entry.chars());
			continue;
		}
		
		str valstring = str (line).substr (equals - line + 1, -1);
		
		// Trim the crap off the end
		while (~valstring) {
			char c = valstring[~valstring - 1];
			if (c <= 32 || c >= 127)
				valstring -= 1;
			else
				break;
		}
		
		switch (const_cast<config*> (cfg)->getType()) {
		case CONFIG_int:
			static_cast<intconfig*> (cfg)->value = atoi (valstring.chars());
			break;
		
		case CONFIG_str:
			static_cast<strconfig*> (cfg)->value = valstring;
			break;
		
		case CONFIG_float:
			static_cast<floatconfig*> (cfg)->value = atof (valstring.chars());
			break;
		
		case CONFIG_bool:
		{
			bool& val = static_cast<boolconfig*> (cfg)->value;
			
			if (+valstring == "TRUE" || valstring == "1")
				val = true;
			else if (+valstring == "FALSE" || valstring == "0")
				val = false;
			break;
		}
		
		case CONFIG_keyseq:
			static_cast<keyseqconfig*> (cfg)->value = keyseq::fromString (valstring.chars ());
			break;
		
		default:
			break;
		}
	}
	
	fclose (fp);
	return true;
}

// =============================================================================
// Write a given formatted string to the given file stream
static size_t writef (FILE* fp, const char* fmt, ...) {
	va_list va;
	
	va_start (va, fmt);
	char* buf = vdynformat (fmt, va, 256);
	va_end (va);
	
	size_t len = fwrite (buf, 1, strlen (buf), fp);
	delete[] buf;
	
	return len;
}

// =============================================================================
// Save the configuration to disk
bool config::save () {
	// The function will write floats, disable the locale now so that they
	// are written properly.
	setlocale (LC_NUMERIC, "C");
	
	// If the directory doesn't exist, create it now.
	if (!QDir (dirpath().chars()).exists ()) {
		fprintf (stderr, "Creating config path %s...\n", dirpath().chars());
		if (!QDir ().mkpath (dirpath().chars())) {
			fprintf (stderr, "Failed to create the directory. Configuration cannot be saved!\n");
			return false; // Couldn't create directory
		}
	}
	
	FILE* fp = fopen (filepath().chars(), "w");
	printf ("writing cfg to %s\n", filepath().chars());
	
	if (!fp) {
		printf ("Couldn't open %s for writing\n", filepath().chars());
		return false;
	}
	
	const time_t curtime = time (NULL);
	const struct tm* timeinfo = localtime (&curtime);
	const char* daysuffix =
		(timeinfo->tm_mday % 10 == 1) ? "st" :
		(timeinfo->tm_mday % 10 == 2) ? "nd" :
		(timeinfo->tm_mday % 10 == 3) ? "rd" : "th";
	
	writef (fp, "# Configuration file for " APPNAME "\n");
	writef (fp, "# Written on %s, %s %d%s %d %.2d:%.2d:%.2d\n",
		g_WeekdayNames[timeinfo->tm_wday], g_MonthNames[timeinfo->tm_mon],
		timeinfo->tm_mday, daysuffix, timeinfo->tm_year + 1900,
		timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	
	for (config* cfg : g_pConfigPointers) {
		str valstring;
		switch (cfg->getType()) {
		case CONFIG_int:
			valstring.format ("%d", static_cast<intconfig*> (cfg)->value);
			break;
		case CONFIG_str:
			valstring = static_cast<strconfig*> (cfg)->value;
			break;
		case CONFIG_float:
			valstring.format ("%f", static_cast<floatconfig*> (cfg)->value);
			
			// Trim any trailing zeros
			if (valstring.first (".") != -1) {
				while (valstring[~valstring - 1] == '0')
					valstring -= 1;
				
				// But don't trim the only one out...
				if (valstring[~valstring - 1] == '.')
					valstring += '0';
			}
			
			break;
		case CONFIG_bool:
			valstring = (static_cast<boolconfig*> (cfg)->value) ? "true" : "false";
			break;
		case CONFIG_keyseq:
			valstring = static_cast<keyseqconfig*> (cfg)->value.toString ();
			break;
		default:
			break;
		}
		
		const char* sDefault = (cfg->getType() != CONFIG_keyseq) ? cfg->defaultstring :
			static_cast<keyseqconfig*> (cfg)->defval.toString ().toUtf8 ().constData ();
		
		// Write the entry now.
		writef (fp, "\n# [%s] default: %s\n", g_ConfigTypeNames[cfg->getType()], sDefault);
		writef (fp, "%s=%s\n", cfg->name, valstring.chars());
	}
	
	fclose (fp);
	return true;
}

// =============================================================================
void config::reset () {
	for (size_t i = 0; i < NUM_CONFIG; i++)
		g_pConfigPointers[i]->resetValue ();
}

// =============================================================================
str config::filepath () {
	str path;
	path.format ("%s" CONFIGFILE, dirpath().chars());
	return path;
}

// =============================================================================
str config::dirpath () {
	str path = (QDir::homePath ().toStdString().c_str());
	path += "/." APPNAME "/";
	
	return path;
}