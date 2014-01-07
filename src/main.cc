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
#include "gui.h"
#include "document.h"
#include "misc.h"
#include "config.h"
#include "colors.h"
#include "types.h"
#include "primitives.h"
#include "gldraw.h"
#include "configDialog.h"
#include "dialogs.h"
#include "crashcatcher.h"

QList<LDDocument*> g_loadedFiles;
ForgeWindow* g_win = null;
const QApplication* g_app = null;
File g_file_stdout (stdout, File::Write);
File g_file_stderr (stderr, File::Write);
static str g_versionString, g_fullVersionString;

const Vertex g_origin (0.0f, 0.0f, 0.0f);
const Matrix g_identity ({1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f});

cfg (Bool, firststart, true);

// =============================================================================
// -----------------------------------------------------------------------------
int main (int argc, char* argv[])
{
	QApplication app (argc, argv);
	app.setOrganizationName (APPNAME);
	app.setApplicationName (APPNAME);
	g_app = &app;

	initCrashCatcher();
	LDDocument::setCurrent (null);

	// Load or create the configuration
	if (!Config::load())
	{
		log ("Creating configuration file...\n");

		if (Config::save())
			log ("Configuration file successfully created.\n");
		else
			log ("failed to create configuration file!\n");
	}

	LDPaths::initPaths();
	initColors();
	ForgeWindow* win = new ForgeWindow;
	newFile();
	win->show();

	// If this is the first start, get the user to configuration. Especially point
	// them to the profile tab, it's the most important form to fill in.
	if (firststart)
	{
		(new ConfigDialog (ConfigDialog::ProfileTab))->exec();
		firststart = false;
		Config::save();
	}

	loadPrimitives();
	return app.exec();
}

// =============================================================================
// -----------------------------------------------------------------------------
void doPrint (File& f, initlist<StringFormatArg> args)
{
	str msg = DoFormat (args);
	f.write (msg.toUtf8());
	f.flush();
}

// =============================================================================
// -----------------------------------------------------------------------------
void doPrint (FILE* fp, initlist<StringFormatArg> args)
{
	str msg = DoFormat (args);
	fwrite (msg.toStdString().c_str(), 1, msg.length(), fp);
	fflush (fp);
}

// =============================================================================
// -----------------------------------------------------------------------------
QString versionString()
{
	if (g_versionString.length() == 0)
	{
#if VERSION_PATCH == 0
		g_versionString = fmt ("%1.%2", VERSION_MAJOR, VERSION_MINOR);
#else
		g_versionString = fmt ("%1.%2.%3", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
#endif // VERSION_PATCH
	}

	return g_versionString;
}

// =============================================================================
// -----------------------------------------------------------------------------
QString fullVersionString()
{
#if BUILD_ID != BUILD_RELEASE && defined (GIT_DESCRIBE)
	return GIT_DESCRIBE;
#else
	return "v" + versionString();
#endif
}