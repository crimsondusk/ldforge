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
#include <QDir>
#include <QTextStream>
#include <QSettings>
#include "common.h"
#include "config.h"
#include "misc.h"
#include "gui.h"
#include "file.h"

Config* g_configPointers[MAX_CONFIG];
static ushort g_cfgPointerCursor = 0;

// =============================================================================
// -----------------------------------------------------------------------------
static QSettings* getSettingsObject() {
#ifdef PORTABLE
# ifdef _WIN32
#  define EXTENSION ".ini"
# else
#  define EXTENSION ".cfg"
# endif // _WIN32
	return new QSettings (str (APPNAME).toLower() + EXTENSION, QSettings::IniFormat);
#else
	return new QSettings;
#endif // PORTABLE
}

Config::Config (const char* name, const char* defstring) :
	name (name), m_defstring (defstring) {}

// =============================================================================
// -----------------------------------------------------------------------------
// Load the configuration from file
bool Config::load() {
	QSettings* settings = getSettingsObject();
	print ("config::load: Loading configuration file from %1...\n", settings->fileName());
	
	for (Config* cfg : g_configPointers) {
		if (!cfg)
			break;
		
		QVariant val = settings->value (cfg->name, cfg->defaultVariant());
		cfg->loadFromVariant (val);
	}
	
	settings->deleteLater();
	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
// Save the configuration to disk
bool Config::save() {
	QSettings* settings = getSettingsObject();
	print ("Saving configuration to %1...\n", settings->fileName());
	
	for (Config* cfg : g_configPointers) {
		if (!cfg)
			break;
		
		settings->setValue (cfg->name, cfg->toVariant());
	}
	
	settings->sync();
	settings->deleteLater();
	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
void Config::reset() {
	for (Config* cfg : g_configPointers) {
		if (!cfg)
			break;
		
		cfg->resetValue();
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
str Config::filepath (str file) {
	return Config::dirpath() + DIRSLASH + file;
}

// =============================================================================
// -----------------------------------------------------------------------------
str Config::dirpath() {
	QSettings* cfg = getSettingsObject();
	return dirname (cfg->fileName());
}

// =============================================================================
// -----------------------------------------------------------------------------
str Config::defaultString() const {
	str defstring = m_defstring;
	
	// String types inevitably get extra quotes in their default string due to
	// preprocessing stuff. We can only remove them now...
	if (getType() == String) {
		defstring.remove (0, 1);
		defstring.chop (1);
	}
	
	return defstring;
}

// =============================================================================
// We cannot just add config objects to a list or vector because that would rely
// on the vector's c-tor being called before the configs' c-tors. With global
// variables we cannot assume that!! Therefore we need to use a C-style array here.
// -----------------------------------------------------------------------------
void Config::addToArray (Config* ptr) {
	if (g_cfgPointerCursor == 0)
		memset (g_configPointers, 0, sizeof g_configPointers);
	
	assert (g_cfgPointerCursor < MAX_CONFIG);
	g_configPointers[g_cfgPointerCursor++] = ptr;
}