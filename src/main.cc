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

#include <QApplication>
#include <QMessageBox>
#include <QAbstractButton>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include "mainWindow.h"
#include "ldDocument.h"
#include "miscallenous.h"
#include "configuration.h"
#include "colors.h"
#include "basics.h"
#include "primitives.h"
#include "glRenderer.h"
#include "configDialog.h"
#include "dialogs.h"
#include "crashCatcher.h"

QList<LDDocument*> g_loadedFiles;
MainWindow* g_win = null;
static String g_versionString, g_fullVersionString;

const Vertex g_origin (0.0f, 0.0f, 0.0f);
const Matrix g_identity ({1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f});

CFGENTRY (Bool, firstStart, true);

// =============================================================================
//
int main (int argc, char* argv[])
{
	QApplication app (argc, argv);
	app.setOrganizationName (APPNAME);
	app.setApplicationName (APPNAME);
	initCrashCatcher();
	LDDocument::setCurrent (null);

	// Load or create the configuration
	if (not Config::load())
	{
		print ("Creating configuration file...\n");

		if (Config::save())
			print ("Configuration file successfully created.\n");
		else
			critical ("Failed to create configuration file!\n");
	}

	LDPaths::initPaths();
	initColors();
	MainWindow* win = new MainWindow;
	newFile();
	win->show();

	// If this is the first start, get the user to configuration. Especially point
	// them to the profile tab, it's the most important form to fill in.
	if (cfg::firstStart)
	{
		(new ConfigDialog (ConfigDialog::ProfileTab))->exec();
		cfg::firstStart = false;
		Config::save();
	}

	loadPrimitives();
	return app.exec();
}
