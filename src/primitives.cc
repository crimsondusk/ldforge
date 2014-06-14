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

#include <QDir>
#include <QRegExp>
#include <QFileDialog>
#include "ldDocument.h"
#include "mainWindow.h"
#include "primitives.h"
#include "ui_makeprim.h"
#include "miscallenous.h"
#include "colors.h"

QList<PrimitiveCategory*> g_PrimitiveCategories;
QList<Primitive> g_primitives;
static PrimitiveScanner* g_activeScanner = null;
PrimitiveCategory* g_unmatched = null;

EXTERN_CFGENTRY (String, defaultName);
EXTERN_CFGENTRY (String, defaultUser);
EXTERN_CFGENTRY (Int, defaultLicense);

static const QStringList g_radialNameRoots =
{
	"edge",
	"cyli",
	"disc",
	"ndis",
	"ring",
	"con"
};

PrimitiveScanner* getActivePrimitiveScanner()
{
	return g_activeScanner;
}

// =============================================================================
//
void loadPrimitives()
{
	// Try to load prims.cfg
	QFile conf (Config::filepath ("prims.cfg"));

	if (not conf.open (QIODevice::ReadOnly))
	{
		// No prims.cfg, build it
		PrimitiveScanner::start();
	}
	else
	{
		while (not conf.atEnd())
		{
			QString line = conf.readLine();

			if (line.endsWith ("\n"))
				line.chop (1);

			if (line.endsWith ("\r"))
				line.chop (1);

			int space = line.indexOf (" ");

			if (space == -1)
				continue;

			Primitive info;
			info.name = line.left (space);
			info.title = line.mid (space + 1);
			g_primitives << info;
		}

		PrimitiveCategory::populateCategories();
		print ("%1 primitives loaded.\n", g_primitives.size());
	}
}

// =============================================================================
//
static void recursiveGetFilenames (QDir dir, QList<QString>& fnames)
{
	QFileInfoList flist = dir.entryInfoList (QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

	for (const QFileInfo& info : flist)
	{
		if (info.isDir())
			recursiveGetFilenames (QDir (info.absoluteFilePath()), fnames);
		else
			fnames << info.absoluteFilePath();
	}
}

// =============================================================================
//
PrimitiveScanner::PrimitiveScanner (QObject* parent) :
	QObject (parent),
	m_i (0)
{
	g_activeScanner = this;
	QDir dir (LDPaths::prims());
	assert (dir.exists());
	m_baselen = dir.absolutePath().length();
	recursiveGetFilenames (dir, m_files);
	emit starting (m_files.size());
	print ("Scanning primitives...");
}

// =============================================================================
//
PrimitiveScanner::~PrimitiveScanner()
{
	g_activeScanner = null;
}

// =============================================================================
//
void PrimitiveScanner::work()
{
	int j = min (m_i + 100, m_files.size());

	for (; m_i < j; ++m_i)
	{
		QString fname = m_files[m_i];
		QFile f (fname);

		if (not f.open (QIODevice::ReadOnly))
			continue;

		Primitive info;
		info.name = fname.mid (m_baselen + 1);  // make full path relative
		info.name.replace ('/', '\\');  // use DOS backslashes, they're expected
		info.category = null;
		QByteArray titledata = f.readLine();

		if (titledata != QByteArray())
			info.title = QString::fromUtf8 (titledata);

		info.title = info.title.simplified();

		if (Q_LIKELY (info.title[0] == '0'))
		{
			info.title.remove (0, 1);  // remove 0
			info.title = info.title.simplified();
		}

		m_prims << info;
	}

	if (m_i == m_files.size())
	{
		// Done with primitives, now save to a config file
		QString path = Config::filepath ("prims.cfg");
		QFile conf (path);

		if (not conf.open (QIODevice::WriteOnly | QIODevice::Text))
			critical (format ("Couldn't write primitive list %1: %2",
				path, conf.errorString()));
		else
		{
			for (Primitive& info : m_prims)
				fprint (conf, "%1 %2\r\n", info.name, info.title);

			conf.close();
		}

		g_primitives = m_prims;
		PrimitiveCategory::populateCategories();
		print ("%1 primitives scanned", g_primitives.size());
		g_activeScanner = null;
		emit workDone();
		deleteLater();
	}
	else
	{
		// Defer to event loop, pick up the work later
		emit update (m_i);
		QMetaObject::invokeMethod (this, "work", Qt::QueuedConnection);
	}
}

// =============================================================================
//
void PrimitiveScanner::start()
{
	if (g_activeScanner)
		return;

	PrimitiveCategory::loadCategories();
	PrimitiveScanner* scanner = new PrimitiveScanner;
	scanner->work();
}

// =============================================================================
//
PrimitiveCategory::PrimitiveCategory (QString name, QObject* parent) :
	QObject (parent),
	m_name (name) {}

// =============================================================================
//
void PrimitiveCategory::populateCategories()
{
	loadCategories();

	for (PrimitiveCategory* cat : g_PrimitiveCategories)
		cat->prims.clear();

	for (Primitive& prim : g_primitives)
	{
		bool matched = false;
		prim.category = null;

		// Go over the categories and their regexes, if and when there's a match,
		// the primitive's category is set to the category the regex beloings to.
		for (PrimitiveCategory* cat : g_PrimitiveCategories)
		{
			for (RegexEntry& entry : cat->regexes)
			{
				switch (entry.type)
				{
					case EFilenameRegex:
					{
						// f-regex, check against filename
						matched = entry.regex.exactMatch (prim.name);
					} break;

					case ETitleRegex:
					{
						// t-regex, check against title
						matched = entry.regex.exactMatch (prim.title);
					} break;
				}

				if (matched)
				{
					prim.category = cat;
					break;
				}
			}

			// Drop out if a category was decided on.
			if (prim.category != null)
				break;
		}

		// If there was a match, add the primitive to the category.
		// Otherwise, add it to the list of unmatched primitives.
		if (prim.category != null)
			prim.category->prims << prim;
		else
			g_unmatched->prims << prim;
	}

	// Sort the categories. Note that we do this here because we need the existing
	// order for regex matching.
	qSort (g_PrimitiveCategories.begin(), g_PrimitiveCategories.end(),
		[](PrimitiveCategory* const& a, PrimitiveCategory* const& b) -> bool
		{
			return a->name() < b->name();
		});
}

// =============================================================================
//
void PrimitiveCategory::loadCategories()
{
	for (PrimitiveCategory* cat : g_PrimitiveCategories)
		delete cat;

	g_PrimitiveCategories.clear();
	QString path = Config::dirpath() + "primregexps.cfg";

	if (not QFile::exists (path))
		path = ":/data/primitive-categories.cfg";

	QFile f (path);

	if (not f.open (QIODevice::ReadOnly))
	{
		critical (format (QObject::tr ("Failed to open primitive categories: %1"), f.errorString()));
		return;
	}

	PrimitiveCategory* cat = null;

	while (not f.atEnd())
	{
		QString line = f.readLine();
		int colon;

		if (line.endsWith ("\n"))
			line.chop (1);

		if (line.length() == 0 || line[0] == '#')
			continue;

		if ((colon = line.indexOf (":")) == -1)
		{
			if (cat && cat->isValidToInclude())
				g_PrimitiveCategories << cat;

			cat = new PrimitiveCategory (line);
		}
		elif (cat != null)
		{
			QString cmd = line.left (colon);
			RegexType type = EFilenameRegex;

			if (cmd == "f")
				type = EFilenameRegex;
			elif (cmd == "t")
				type = ETitleRegex;
			else
			{
				print (tr ("Warning: unknown command \"%1\" on line \"%2\""), cmd, line);
				continue;
			}

			QRegExp regex (line.mid (colon + 1));
			RegexEntry entry = { regex, type };
			cat->regexes << entry;
		}
		else
			print ("Warning: Rules given before the first category name");
	}

	if (cat->isValidToInclude())
		g_PrimitiveCategories << cat;

	// Add a category for unmatched primitives.
	// Note: if this function is called the second time, g_unmatched has been
	// deleted at the beginning of the function and is dangling at this point.
	g_unmatched = new PrimitiveCategory (tr ("Other"));
	g_PrimitiveCategories << g_unmatched;
	f.close();
}

// =============================================================================
//
bool PrimitiveCategory::isValidToInclude()
{
	if (regexes.isEmpty())
	{
		print (tr ("Warning: category \"%1\" left without patterns"), name());
		deleteLater();
		return false;
	}

	return true;
}

// =============================================================================
//
bool isPrimitiveLoaderBusy()
{
	return g_activeScanner != null;
}

// =============================================================================
//
static double radialPoint (int i, int divs, double (*func) (double))
{
	return (*func) ((i * 2 * pi) / divs);
}

// =============================================================================
//
void makeCircle (int segs, int divs, double radius, QList<QLineF>& lines)
{
	for (int i = 0; i < segs; ++i)
	{
		double x0 = radius * radialPoint (i, divs, cos),
			x1 = radius * radialPoint (i + 1, divs, cos),
			z0 = radius * radialPoint (i, divs, sin),
			z1 = radius * radialPoint (i + 1, divs, sin);

		lines << QLineF (QPointF (x0, z0), QPointF (x1, z1));
	}
}

// =============================================================================
//
LDObjectList makePrimitive (PrimitiveType type, int segs, int divs, int num)
{
	LDObjectList objs;
	QList<int> condLineSegs;
	QList<QLineF> circle;

	makeCircle (segs, divs, 1, circle);

	for (int i = 0; i < segs; ++i)
	{
		double x0 = circle[i].x1(),
				   x1 = circle[i].x2(),
				   z0 = circle[i].y1(),
				   z1 = circle[i].y2();

		switch (type)
		{
			case Circle:
			{
				Vertex v0 (x0, 0.0f, z0),
				  v1 (x1, 0.0f, z1);

				LDLinePtr line (spawn<LDLine>());
				line->setVertex (0, v0);
				line->setVertex (1, v1);
				line->setColor (edgecolor());
				objs << line;
			} break;

			case Cylinder:
			case Ring:
			case Cone:
			{
				double x2, x3, z2, z3;
				double y0, y1, y2, y3;

				if (type == Cylinder)
				{
					x2 = x1;
					x3 = x0;
					z2 = z1;
					z3 = z0;

					y0 = y1 = 0.0f;
					y2 = y3 = 1.0f;
				}
				else
				{
					x2 = x1 * (num + 1);
					x3 = x0 * (num + 1);
					z2 = z1 * (num + 1);
					z3 = z0 * (num + 1);

					x0 *= num;
					x1 *= num;
					z0 *= num;
					z1 *= num;

					if (type == Ring)
						y0 = y1 = y2 = y3 = 0.0f;
					else
					{
						y0 = y1 = 1.0f;
						y2 = y3 = 0.0f;
					}
				}

				Vertex v0 (x0, y0, z0),
					   v1 (x1, y1, z1),
					   v2 (x2, y2, z2),
					   v3 (x3, y3, z3);

				LDQuadPtr quad (spawn<LDQuad> (v0, v1, v2, v3));
				quad->setColor (maincolor());

				if (type == Cylinder)
					quad->invert();

				objs << quad;

				if (type == Cylinder || type == Cone)
					condLineSegs << i;
			} break;

			case Disc:
			case DiscNeg:
			{
				double x2, z2;

				if (type == Disc)
					x2 = z2 = 0.0f;
				else
				{
					x2 = (x0 >= 0.0f) ? 1.0f : -1.0f;
					z2 = (z0 >= 0.0f) ? 1.0f : -1.0f;
				}

				Vertex v0 (x0, 0.0f, z0),
					   v1 (x1, 0.0f, z1),
					   v2 (x2, 0.0f, z2);

				// Disc negatives need to go the other way around, otherwise
				// they'll end up upside-down.
				LDTrianglePtr seg (spawn<LDTriangle>());
				seg->setColor (maincolor());
				seg->setVertex (type == Disc ? 0 : 2, v0);
				seg->setVertex (1, v1);
				seg->setVertex (type == Disc ? 2 : 0, v2);
				objs << seg;
			} break;
		}
	}

	// If this is not a full circle, we need a conditional line at the other
	// end, too.
	if (segs < divs && condLineSegs.size() != 0)
		condLineSegs << segs;

	for (int i : condLineSegs)
	{
		Vertex v0 (radialPoint (i, divs, cos), 0.0f, radialPoint (i, divs, sin)),
		  v1,
		  v2 (radialPoint (i + 1, divs, cos), 0.0f, radialPoint (i + 1, divs, sin)),
		  v3 (radialPoint (i - 1, divs, cos), 0.0f, radialPoint (i - 1, divs, sin));

		if (type == Cylinder)
		{
			v1 = Vertex (v0[X], 1.0f, v0[Z]);
		}
		elif (type == Cone)
		{
			v1 = Vertex (v0[X] * (num + 1), 0.0f, v0[Z] * (num + 1));
			v0.setX (v0.x() * num);
			v0.setY (1.0);
			v0.setZ (v0.z() * num);
		}

		LDCondLinePtr line = (spawn<LDCondLine>());
		line->setColor (edgecolor());
		line->setVertex (0, v0);
		line->setVertex (1, v1);
		line->setVertex (2, v2);
		line->setVertex (3, v3);
		objs << line;
	}

	return objs;
}

// =============================================================================
//
static QString primitiveTypeName (PrimitiveType type)
{
	// Not translated as primitives are in English.
	return type == Circle   ? "Circle" :
		   type == Cylinder ? "Cylinder" :
		   type == Disc     ? "Disc" :
		   type == DiscNeg  ? "Disc Negative" :
		   type == Ring     ? "Ring" : "Cone";
}

// =============================================================================
//
QString radialFileName (PrimitiveType type, int segs, int divs, int num)
{
	int numer = segs,
			denom = divs;

	// Simplify the fractional part, but the denominator must be at least 4.
	simplify (numer, denom);

	if (denom < 4)
	{
		const int factor = 4 / denom;
		numer *= factor;
		denom *= factor;
	}

	// Compose some general information: prefix, fraction, root, ring number
	QString prefix = (divs == g_lores) ? "" : format ("%1/", divs);
	QString frac = format ("%1-%2", numer, denom);
	QString root = g_radialNameRoots[type];
	QString numstr = (type == Ring || type == Cone) ? format ("%1", num) : "";

	// Truncate the root if necessary (7-16rin4.dat for instance).
	// However, always keep the root at least 2 characters.
	int extra = (frac.length() + numstr.length() + root.length()) - 8;
	root.chop (clamp (extra, 0, 2));

	// Stick them all together and return the result.
	return prefix + frac + root + numstr + ".dat";
}

// =============================================================================
//
LDDocumentPtr generatePrimitive (PrimitiveType type, int segs, int divs, int num)
{
	// Make the description
	QString frac = QString::number ((float) segs / divs);
	QString name = radialFileName (type, segs, divs, num);
	QString descr;

	// Ensure that there's decimals, even if they're 0.
	if (frac.indexOf (".") == -1)
		frac += ".0";

	if (type == Ring || type == Cone)
	{
		QString spacing =
			(num < 10) ? "  " :
			(num < 100) ? " "  : "";

		descr = format ("%1 %2%3 x %4", primitiveTypeName (type), spacing, num, frac);
	}
	else
		descr = format ("%1 %2", primitiveTypeName (type), frac);

	// Prepend "Hi-Res" if 48/ primitive.
	if (divs == g_hires)
		descr.insert (0, "Hi-Res ");

	LDDocumentPtr f = LDDocument::createNew();
	f->setDefaultName (name);

	QString author = APPNAME;
	QString license = "";

	if (not cfg::defaultName.isEmpty())
	{
		license = getLicenseText (cfg::defaultLicense);
		author = format ("%1 [%2]", cfg::defaultName, cfg::defaultUser);
	}

	LDObjectList objs;

	objs << spawn<LDComment> (descr)
		 << spawn<LDComment> (format ("Name: %1", name))
		 << spawn<LDComment> (format ("Author: %1", author))
		 << spawn<LDComment> (format ("!LDRAW_ORG Unofficial_%1Primitive", divs == g_hires ? "48_" : ""))
		 << spawn<LDComment> (license)
		 << spawn<LDEmpty>()
		 << spawn<LDBFC> (LDBFC::CertifyCCW)
		 << spawn<LDEmpty>();

	f->addObjects (objs);
	f->addObjects (makePrimitive (type, segs, divs, num));
	return f;
}

// =============================================================================
//
LDDocumentPtr getPrimitive (PrimitiveType type, int segs, int divs, int num)
{
	QString name = radialFileName (type, segs, divs, num);
	LDDocumentPtr f = getDocument (name);

	if (f != null)
		return f;

	return generatePrimitive (type, segs, divs, num);
}

// =============================================================================
//
PrimitivePrompt::PrimitivePrompt (QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f)
{
	ui = new Ui_MakePrimUI;
	ui->setupUi (this);
	connect (ui->cb_hires, SIGNAL (toggled (bool)), this, SLOT (hiResToggled (bool)));
}

// =============================================================================
//
PrimitivePrompt::~PrimitivePrompt()
{
	delete ui;
}

// =============================================================================
//
void PrimitivePrompt::hiResToggled (bool on)
{
	ui->sb_segs->setMaximum (on ? g_hires : g_lores);

	// If the current value is 16 and we switch to hi-res, default the
	// spinbox to 48.
	if (on && ui->sb_segs->value() == g_lores)
		ui->sb_segs->setValue (g_hires);
}

// =============================================================================
//
DEFINE_ACTION (MakePrimitive, 0)
{
	PrimitivePrompt* dlg = new PrimitivePrompt (g_win);

	if (not dlg->exec())
		return;

	int segs = dlg->ui->sb_segs->value();
	int divs = dlg->ui->cb_hires->isChecked() ? g_hires : g_lores;
	int num = dlg->ui->sb_ringnum->value();
	PrimitiveType type =
		dlg->ui->rb_circle->isChecked()   ? Circle :
		dlg->ui->rb_cylinder->isChecked() ? Cylinder :
		dlg->ui->rb_disc->isChecked()     ? Disc :
		dlg->ui->rb_ndisc->isChecked()    ? DiscNeg :
		dlg->ui->rb_ring->isChecked()     ? Ring : Cone;

	LDDocumentPtr f = generatePrimitive (type, segs, divs, num);
	f->setImplicit (false);
	g_win->save (f, false);
}
