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

config* g_configPointers[MAX_CONFIG];
static ushort g_cfgPointerCursor = 0;

// =============================================================================
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

// =============================================================================
// Load the configuration from file
bool config::load() {
	QSettings* settings = getSettingsObject();
	print ("config::load: Loading configuration file from %1...\n", settings->fileName());
	
	for (config* cfg : g_configPointers) {
		if (!cfg)
			break;
		
		cfg->loadFromConfig (settings);
	}
	
	settings->deleteLater();
	return true;
}

// =============================================================================
void intconfig::loadFromConfig (const QSettings* cfg) {
	QVariant val = cfg->value (name, str::number (defval));
	value = val.toInt();
}

void floatconfig::loadFromConfig (const QSettings* cfg) {
	QVariant val = cfg->value (name, str::number (defval));
	value = val.toFloat();
}

void strconfig::loadFromConfig (const QSettings* cfg) {
	QVariant val = cfg->value (name, defval);
	value = val.toString();
}

void boolconfig::loadFromConfig (const QSettings* cfg) {
	QVariant val = cfg->value (name, str::number (defval));
	value = val.toBool();
}

void keyseqconfig::loadFromConfig (const QSettings* cfg) {
	QVariant val = cfg->value (name, defval.toString());
	value = keyseq (val.toString());
}

// =============================================================================
// TODO: make virtual
str config::toString() const {
	switch (getType()) {
	case Type_int:
		return fmt ("%1", static_cast<const intconfig*> (this)->value);
		break;
	
	case Type_str:
		return static_cast<const strconfig*> (this)->value;
		break;
	
	case Type_float:
		return fmt ("%1", static_cast<const floatconfig*> (this)->value);
		break;
	
	case Type_bool:
		return (static_cast<const boolconfig*> (this)->value) ? "true" : "false";
		break;
	
	case Type_keyseq:
		return static_cast<const keyseqconfig*> (this)->value.toString();
		break;
	
	default:
		break;
	}
	
	return "";
}

// =============================================================================
// Save the configuration to disk
bool config::save() {
	QSettings* settings = getSettingsObject();
	print ("Saving configuration to %1...\n", settings->fileName());
	
	for (config* cfg : g_configPointers) {
		if (!cfg)
			break;
		
		settings->setValue (cfg->name, cfg->toString());
	}
	
	settings->sync();
	settings->deleteLater();
	return true;
}

// =============================================================================
void config::reset() {
	for (config* cfg : g_configPointers) {
		if (!cfg)
			break;
		
		cfg->resetValue();
	}
}

// =============================================================================
str config::filepath (str file) {
	return config::dirpath() + DIRSLASH + file;
}

// =============================================================================
str config::dirpath() {
	QSettings* cfg = getSettingsObject();
	return dirname (cfg->fileName());
}

// =============================================================================
str config::defaultString() const {
	str defstring = m_defstring;
	
	// String types inevitably get extra quotes in their default string due to
	// preprocessing stuff. We can only remove them now...
	if (getType() == Type_str) {
		defstring.remove (0, 1);
		defstring.chop (1);
	}
	
	return defstring;
}

// =============================================================================
void addConfig (config* ptr) {
	if (g_cfgPointerCursor == 0)
		memset (g_configPointers, 0, sizeof g_configPointers);
	
	assert (g_cfgPointerCursor < MAX_CONFIG);
	g_configPointers[g_cfgPointerCursor++] = ptr;
}