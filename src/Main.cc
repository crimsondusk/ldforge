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
#include <QDate>
#include "MainWindow.h"
#include "Document.h"
#include "Misc.h"
#include "Configuration.h"
#include "Colors.h"
#include "Types.h"
#include "Primitives.h"
#include "GLRenderer.h"
#include "ConfigurationDialog.h"
#include "Dialogs.h"
#include "CrashCatcher.h"

QList<LDDocument*> g_loadedFiles;
MainWindow* g_win = null;
static QString g_versionString, g_fullVersionString;

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
	MainWindow* win = new MainWindow;
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
void doPrint (QFile& f, QList<StringFormatArg> args)
{
	QString msg = DoFormat (args);
	f.write (msg.toUtf8());
	f.flush();
}

// =============================================================================
// -----------------------------------------------------------------------------
void doPrint (FILE* fp, QList<StringFormatArg> args)
{
	QString msg = DoFormat (args);
	fwrite (msg.toStdString().c_str(), 1, msg.length(), fp);
	fflush (fp);
}

