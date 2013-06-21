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

#include <errno.h>
#include <time.h>
#include <QDir>
#include <qtextstream.h>
#include "common.h"
#include "config.h"
#include "misc.h"
#include "gui.h"

config* g_configPointers[MAX_CONFIG];
static ushort g_cfgPointerCursor = 0;

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
	print ("config::load: Loading configuration file...\n");
	print ("config::load: Path to configuration is %1\n", filepath ());
	
	// Locale must be disabled for atof
	setlocale (LC_NUMERIC, "C");
	
	File f (filepath (), File::Read);
	if (!f)
		return false;
	
	size_t ln = 0;
	
	// Read the values.
	for (str line : f) {
		ln++;
		
		if (line.isEmpty () || line[0] == '#')
			continue; // Empty line or comment.
		
		// Find the equals sign.
		int equals = line.indexOf ('=');
		if (equals == -1) {
			fprint (stderr, "couldn't find `=` sign in entry `%1`\n", line);
			continue;
		}
		
		str entry = line.left (equals);
		str valstring = line.right (line.length () - equals - 1);
		
		// Find the config entry for this.
		config* cfg = null;
		for (config* i : g_configPointers) {
			if (!i)
				break;
			
			if (entry == i->name)
				cfg = i;
		}
		
		if (!cfg) {
			fprint (stderr, "unknown config `%1`\n", entry);
			continue;
		}
		
		switch (cfg->getType()) {
		case CONFIG_int:
			static_cast<intconfig*> (cfg)->value = valstring.toInt ();
			break;
		
		case CONFIG_str:
			static_cast<strconfig*> (cfg)->value = valstring;
			break;
		
		case CONFIG_float:
			static_cast<floatconfig*> (cfg)->value = valstring.toFloat ();
			break;
		
		case CONFIG_bool:
		{
			bool& val = static_cast<boolconfig*> (cfg)->value;
			
			if (valstring.toUpper () == "TRUE" || valstring == "1")
				val = true;
			else if (valstring.toUpper () == "FALSE" || valstring == "0")
				val = false;
			break;
		}
		
		case CONFIG_keyseq:
			static_cast<keyseqconfig*> (cfg)->value = keyseq::fromString (valstring);
			break;
		
		default:
			break;
		}
	}
	
	f.close ();
	return true;
}

// =============================================================================
// Save the configuration to disk
bool config::save () {
	// The function will write floats, disable the locale now so that they
	// are written properly.
	setlocale (LC_NUMERIC, "C");
	
	// If the directory doesn't exist, create it now.
	if (QDir (dirpath ()).exists () == false) {
		fprint (stderr, "Creating config path %1...\n", dirpath ());
		if (!QDir ().mkpath (dirpath ())) {
			critical ("Failed to create the configuration directory. Configuration cannot be saved!\n");
			return false;
		}
	}
	
	File f (filepath (), File::Write);
	print ("writing cfg to %1\n", filepath ());
	
	if (!f) {
		critical (fmt ("Cannot save configuration, cannot open %1 for writing\n", filepath ()));
		return false;
	}
	
	fprint (f, "# Configuration file for " APPNAME "\n");
	
	for (config* cfg : g_configPointers) {
		if (!cfg)
			break;
		
		str valstring;
		switch (cfg->getType()) {
		case CONFIG_int:
			valstring = fmt ("%1", static_cast<intconfig*> (cfg)->value);
			break;
		
		case CONFIG_str:
			valstring = static_cast<strconfig*> (cfg)->value;
			break;
		
		case CONFIG_float:
			valstring = fmt ("%1", static_cast<floatconfig*> (cfg)->value);
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
		
		// Write the entry now.
		fprint (f, "%1=%2\n", cfg->name, valstring);
	}
	
	f.close ();
	return true;
}

// =============================================================================
void config::reset () {
	for (config* cfg : g_configPointers) {
		if (!cfg)
			break;
		
		cfg->resetValue ();
	}
}

// =============================================================================
str config::filepath () {
	str path = fmt ("%1%2.cfg", dirpath (),
		str (APPNAME).toLower ());
	return path;
}

// =============================================================================
str config::dirpath () {
#ifndef _WIN32
	return fmt ("%1" DIRSLASH ".%2" DIRSLASH,
		QDir::homePath (), str (APPNAME).toLower ());
#else
	return fmt ("%1" DIRSLASH APPNAME DIRSLASH, QDir::homePath ());
#endif // _WIN32
}

void addConfig (config* ptr) {
	if (g_cfgPointerCursor == 0)
		memset (g_configPointers, 0, sizeof g_configPointers);
	
	assert (g_cfgPointerCursor < MAX_CONFIG);
	g_configPointers[g_cfgPointerCursor++] = ptr;
}