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
 *  =====================================================================
 *
 *  config.cxx: Configuration management. I don't like how unsafe QSettings
 *  is so this implements a type-safer and identifer-safer wrapping system of
 *  configuration variables. QSettings is used underlyingly, this is a matter
 *  of interface.
 */

#include <errno.h>
#include <QDir>
#include <QTextStream>
#include <QSettings>
#include "Main.h"
#include "Configuration.h"
#include "Misc.h"
#include "MainWindow.h"
#include "Document.h"

#ifdef _WIN32
# define EXTENSION ".ini"
#else
# define EXTENSION ".cfg"
#endif // _WIN32

Config*							g_configPointers[MAX_CONFIG];
static int						g_cfgPointerCursor = 0;
static QMap<QString, Config*>	g_configsByName;
static QList<Config*>			g_configs;

// =============================================================================
// Get the QSettings object.
// -----------------------------------------------------------------------------
static QSettings* getSettingsObject()
{
	QString path = qApp->applicationDirPath() + "/" UNIXNAME EXTENSION;
	return new QSettings (path, QSettings::IniFormat);
}

Config::Config (QString name) :
	m_Name (name) {}

// =============================================================================
// Load the configuration from file
// -----------------------------------------------------------------------------
bool Config::load()
{
	QSettings* settings = getSettingsObject();
	log ("config::load: Loading configuration file from %1\n", settings->fileName());

	for (Config* cfg : g_configPointers)
	{
		if (!cfg)
			break;

		QVariant val = settings->value (cfg->getName(), cfg->getDefaultAsVariant());
		cfg->loadFromVariant (val);
		g_configsByName[cfg->getName()] = cfg;
		g_configs << cfg;
	}

	settings->deleteLater();
	return true;
}

// =============================================================================
// Save the configuration to disk
// -----------------------------------------------------------------------------
bool Config::save()
{
	QSettings* settings = getSettingsObject();
	log ("Saving configuration to %1...\n", settings->fileName());

	for (Config* cfg : g_configs)
	{
		if (!cfg->isDefault())
			settings->setValue (cfg->getName(), cfg->toVariant());
		else
			settings->remove (cfg->getName());
	}

	settings->sync();
	settings->deleteLater();
	return true;
}

// =============================================================================
// Reset configuration to defaults.
// -----------------------------------------------------------------------------
void Config::reset()
{
	for (Config* cfg : g_configs)
		cfg->resetValue();
}

// =============================================================================
// Where is the configuration file located at?
// -----------------------------------------------------------------------------
QString Config::filepath (QString file)
{
	return Config::dirpath() + DIRSLASH + file;
}

// =============================================================================
// Directory of the configuration file.
// -----------------------------------------------------------------------------
QString Config::dirpath()
{
	QSettings* cfg = getSettingsObject();
	return dirname (cfg->fileName());
}

// =============================================================================
// We cannot just add config objects to a list or vector because that would rely
// on the vector's c-tor being called before the configs' c-tors. With global
// variables we cannot assume that, therefore we need to use a C-style array here.
// -----------------------------------------------------------------------------
void Config::addToArray (Config* ptr)
{
	if (g_cfgPointerCursor == 0)
		memset (g_configPointers, 0, sizeof g_configPointers);

	assert (g_cfgPointerCursor < MAX_CONFIG);
	g_configPointers[g_cfgPointerCursor++] = ptr;
}

// =============================================================================
// -----------------------------------------------------------------------------
template<class T> T* getConfigByName (QString name, Config::Type type)
{
	auto it = g_configsByName.find (name);

	if (it == g_configsByName.end())
		return null;

	Config* cfg = it.value();

	if (cfg->getType() != type)
	{
		fprint (stderr, "type of %1 is %2, not %3\n", name, cfg->getType(), type);
		abort();
	}

	return reinterpret_cast<T*> (cfg);
}

// =============================================================================
// -----------------------------------------------------------------------------
#undef IMPLEMENT_CONFIG

#define IMPLEMENT_CONFIG(NAME)										\
	NAME##Config* NAME##Config::getByName (QString name)			\
	{																\
		return getConfigByName<NAME##Config> (name, E##NAME##Type);	\
	}

IMPLEMENT_CONFIG (Int)
IMPLEMENT_CONFIG (String)
IMPLEMENT_CONFIG (Bool)
IMPLEMENT_CONFIG (Float)
IMPLEMENT_CONFIG (List)
IMPLEMENT_CONFIG (KeySequence)
IMPLEMENT_CONFIG (Vertex)
