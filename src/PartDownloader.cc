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

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDir>
#include <QProgressBar>
#include <QPushButton>
#include "PartDownloader.h"
#include "ui_downloadfrom.h"
#include "Types.h"
#include "MainWindow.h"
#include "Document.h"
#include "GLRenderer.h"
#include "ConfigurationDialog.h"

cfg (String,	net_downloadpath,	"");
cfg (Bool,		net_guesspaths,	true);
cfg (Bool,		net_autoclose,		true);

const QString g_unofficialLibraryURL ("http://ldraw.org/library/unofficial/");

// =============================================================================
//
void PartDownloader::staticBegin()
{
	QString path = getDownloadPath();

	if (path == "" || QDir (path).exists() == false)
	{
		critical (PartDownloader::tr ("You need to specify a valid path for "
			"downloaded files in the configuration to download paths."));

		(new ConfigDialog (ConfigDialog::DownloadTab, null))->exec();
		return;
	}

	PartDownloader* dlg = new PartDownloader;
	dlg->exec();
}

// =============================================================================
//
QString PartDownloader::getDownloadPath()
{
	QString path = net_downloadpath;

#if DIRSLASH_CHAR != '/'
	path.replace (DIRSLASH, "/");
#endif

	return path;
}

// =============================================================================
//
PartDownloader::PartDownloader (QWidget* parent) : QDialog (parent)
{
	setInterface (new Ui_DownloadFrom);
	interface()->setupUi (this);
	interface()->fname->setFocus();
	interface()->progress->horizontalHeader()->setResizeMode (PartLabelColumn, QHeaderView::Stretch);

	setDownloadButton (new QPushButton (tr ("Download")));
	interface()->buttonBox->addButton (downloadButton(), QDialogButtonBox::ActionRole);
	getButton (Abort)->setEnabled (false);

	connect (interface()->source, SIGNAL (currentIndexChanged (int)),
		this, SLOT (sourceChanged (int)));
	connect (interface()->buttonBox, SIGNAL (clicked (QAbstractButton*)),
		this, SLOT (buttonClicked (QAbstractButton*)));
}

// =============================================================================
//
PartDownloader::~PartDownloader()
{
	delete interface();
}

// =============================================================================
//
QString PartDownloader::getURL() const
{
	const Source src = getSource();
	QString dest;

	switch (src)
	{
		case PartsTracker:
			dest = interface()->fname->text();
			modifyDestination (dest);
			return g_unofficialLibraryURL + dest;

		case CustomURL:
			return interface()->fname->text();
	}

	// Shouldn't happen
	return "";
}

// =============================================================================
//
void PartDownloader::modifyDestination (QString& dest) const
{
	dest = dest.simplified();

	// If the user doesn't want us to guess, stop right here.
	if (net_guesspaths == false)
		return;

	// Ensure .dat extension
	if (dest.right (4) != ".dat")
	{
		// Remove the existing extension, if any. It may be we're here over a
		// typo in the .dat extension.
		const int dotpos = dest.lastIndexOf (".");

		if (dotpos != -1 && dotpos >= dest.length() - 4)
			dest.chop (dest.length() - dotpos);

		dest += ".dat";
	}

	// If the part starts with s\ or s/, then use parts/s/. Same goes with
	// 48\ and p/48/.
	if (dest.left (2) == "s\\" || dest.left (2) == "s/")
	{
		dest.remove (0, 2);
		dest.prepend ("parts/s/");
	} elif (dest.left (3) == "48\\" || dest.left (3) == "48/")
	{
		dest.remove (0, 3);
		dest.prepend ("p/48/");
	}

	/* Try determine where to put this part. We have four directories:
	   parts/, parts/s/, p/, and p/48/. If we haven't already specified
	   either parts/ or p/, we need to add it automatically. Part files
	   are numbers wit a possible u prefix for parts with unknown number
	   which can be followed by any of:
	   - c** (composites)
	   - d** (formed stickers)
	   - p** (patterns)
	   - a lowercase alphabetic letter for variants

	   Subfiles (usually) have an s** prefix, in which case we use parts/s/.
	   Note that the regex starts with a '^' so it won't catch already fully
	   given part file names. */
	QString partRegex = "^u?[0-9]+(c[0-9][0-9]+)*(d[0-9][0-9]+)*[a-z]?(p[0-9a-z][0-9a-z]+)*";
	QString subpartRegex = partRegex + "s[0-9][0-9]+";

	partRegex += "\\.dat$";
	subpartRegex += "\\.dat$";

	if (QRegExp (subpartRegex).exactMatch (dest))
		dest.prepend ("parts/s/");
	elif (QRegExp (partRegex).exactMatch (dest))
		dest.prepend ("parts/");
	elif (dest.left (6) != "parts/" && dest.left (2) != "p/")
		dest.prepend ("p/");
}

// =============================================================================
//
PartDownloader::Source PartDownloader::getSource() const
{
	return (Source) interface()->source->currentIndex();
}

// =============================================================================
//
void PartDownloader::sourceChanged (int i)
{
	if (i == CustomURL)
		interface()->fileNameLabel->setText (tr ("URL:"));
	else
		interface()->fileNameLabel->setText (tr ("File name:"));
}

// =============================================================================
//
void PartDownloader::buttonClicked (QAbstractButton* btn)
{
	if (btn == getButton (Close))
	{
		reject();
	}
	elif (btn == getButton (Abort))
	{
		setAborted (true);

		for (PartDownloadRequest* req : requests())
			req->abort();
	}
	elif (btn == getButton (Download))
	{
		QString dest = interface()->fname->text();
		setPrimaryFile (null);
		setAborted (false);

		if (getSource() == CustomURL)
			dest = basename (getURL());

		modifyDestination (dest);

		if (QFile::exists (PartDownloader::getDownloadPath() + DIRSLASH + dest))
		{
			const QString overwritemsg = format (tr ("%1 already exists in download directory. Overwrite?"), dest);
			if (!confirm (tr ("Overwrite?"), overwritemsg))
				return;
		}

		downloadButton()->setEnabled (false);
		interface()->progress->setEnabled (true);
		interface()->fname->setEnabled (false);
		interface()->source->setEnabled (false);
		downloadFile (dest, getURL(), true);
		getButton (Close)->setEnabled (false);
		getButton (Abort)->setEnabled (true);
		getButton (Download)->setEnabled (false);
	}
}

// =============================================================================
//
void PartDownloader::downloadFile (QString dest, QString url, bool primary)
{
	const int row = interface()->progress->rowCount();

	// Don't download files repeadetly.
	if (filesToDownload().indexOf (dest) != -1)
		return;

	modifyDestination (dest);
	PartDownloadRequest* req = new PartDownloadRequest (url, dest, primary, this);
	m_filesToDownload << dest;
	m_requests << req;
	interface()->progress->insertRow (row);
	req->setTableRow (row);
	req->updateToTable();
}

// =============================================================================
//
void PartDownloader::checkIfFinished()
{
	bool failed = isAborted();

	// If there is some download still working, we're not finished.
	for (PartDownloadRequest* req : requests())
	{
		if (!req->isFinished())
			return;

		if (req->state() == PartDownloadRequest::EFailed)
			failed = true;
	}

	for (PartDownloadRequest* req : requests())
		delete req;

	m_requests.clear();

	// Update everything now
	if (primaryFile() != null)
	{
		LDDocument::setCurrent (primaryFile());
		reloadAllSubfiles();
		g_win->doFullRefresh();
		g_win->R()->resetAngles();
	}

	if (net_autoclose && !failed)
	{
		// Close automatically if desired.
		accept();
	}
	else
	{
		// Allow the prompt be closed now.
		getButton (Abort)->setEnabled (false);
		getButton (Close)->setEnabled (true);
	}
}

// =============================================================================
//
QPushButton* PartDownloader::getButton (PartDownloader::Button i)
{
	switch (i)
	{
		case Download:
			return downloadButton();

		case Abort:
			return qobject_cast<QPushButton*> (interface()->buttonBox->button (QDialogButtonBox::Abort));

		case Close:
			return qobject_cast<QPushButton*> (interface()->buttonBox->button (QDialogButtonBox::Close));
	}

	return null;
}

// =============================================================================
//
PartDownloadRequest::PartDownloadRequest (QString url, QString dest, bool primary, PartDownloader* parent) :
	QObject (parent),
	m_state (ERequesting),
	m_prompt (parent),
	m_url (url),
	m_destinaton (dest),
	m_filePath (PartDownloader::getDownloadPath() + DIRSLASH + dest),
	m_networkManager (new QNetworkAccessManager),
	m_isFirstUpdate (true),
	m_isPrimary (primary),
	m_filePointer (null)
{
	// Make sure that we have a valid destination.
	QString dirpath = dirname (filePath());

	QDir dir (dirpath);

	if (dir.exists() == false)
	{
		print ("Creating %1...\n", dirpath);

		if (!dir.mkpath (dirpath))
			critical (format (tr ("Couldn't create the directory %1!"), dirpath));
	}

	setNetworkReply (networkManager()->get (QNetworkRequest (QUrl (url))));
	connect (networkReply(), SIGNAL (finished()), this, SLOT (downloadFinished()));
	connect (networkReply(), SIGNAL (readyRead()), this, SLOT (readyRead()));
	connect (networkReply(), SIGNAL (downloadProgress (qint64, qint64)),
		this, SLOT (downloadProgress (qint64, qint64)));
}

// =============================================================================
//
PartDownloadRequest::~PartDownloadRequest() {}

// =============================================================================
//
void PartDownloadRequest::updateToTable()
{
	const int		labelcol = PartDownloader::PartLabelColumn,
					progcol = PartDownloader::ProgressColumn;
	QTableWidget*	table = prompt()->interface()->progress;
	QProgressBar*	prog;

	switch (state())
	{
		case ERequesting:
		case EDownloading:
		{
			prog = qobject_cast<QProgressBar*> (table->cellWidget (tableRow(), progcol));

			if (!prog)
			{
				prog = new QProgressBar;
				table->setCellWidget (tableRow(), progcol, prog);
			}

			prog->setRange (0, numBytesTotal());
			prog->setValue (numBytesRead());
		} break;

		case EFinished:
		case EFailed:
		{
			const QString text = (state() == EFinished)
				? "<b><span style=\"color: #080\">FINISHED</span></b>"
				: "<b><span style=\"color: #800\">FAILED</span></b>";

			QLabel* lb = new QLabel (text);
			lb->setAlignment (Qt::AlignCenter);
			table->setCellWidget (tableRow(), progcol, lb);
		} break;
	}

	QLabel* lb = qobject_cast<QLabel*> (table->cellWidget (tableRow(), labelcol));

	if (isFirstUpdate())
	{
		lb = new QLabel (format ("<b>%1</b>", destinaton()), table);
		table->setCellWidget (tableRow(), labelcol, lb);
	}

	// Make sure that the cell is big enough to contain the label
	if (table->columnWidth (labelcol) < lb->width())
		table->setColumnWidth (labelcol, lb->width());

	setFirstUpdate (true);
}

// =============================================================================
//
void PartDownloadRequest::downloadFinished()
{
	if (networkReply()->error() != QNetworkReply::NoError)
	{
		if (isPrimary() && !prompt()->isAborted())
			critical (networkReply()->errorString());

		setState (EFailed);
	}
	elif (state() != EFailed)
		setState (EFinished);

	setNumBytesRead (numBytesTotal());
	updateToTable();

	if (filePointer())
	{
		filePointer()->close();
		delete filePointer();
		setFilePointer (null);

		if (state() == EFailed)
			QFile::remove (filePath());
	}

	if (state() != EFinished)
	{
		prompt()->checkIfFinished();
		return;
	}

	// Try to load this file now.
	LDDocument* f = openDocument (filePath(), false);

	if (!f)
		return;

	f->setImplicit (!isPrimary());

	// Iterate through this file and check for errors. If there's any that stems
	// from unknown file references, try resolve that by downloading the reference.
	// This is why downloading a part may end up downloading multiple files, as
	// it resolves dependencies.
	for (LDObject* obj : f->objects())
	{
		LDError* err = dynamic_cast<LDError*> (obj);

		if (err == null || err->fileReferenced().isEmpty())
			continue;

		QString dest = err->fileReferenced();
		prompt()->modifyDestination (dest);
		prompt()->downloadFile (dest, g_unofficialLibraryURL + dest, false);
	}

	if (isPrimary())
	{
		addRecentFile (filePath());
		prompt()->setPrimaryFile (f);
	}

	prompt()->checkIfFinished();
}

// =============================================================================
//
void PartDownloadRequest::downloadProgress (int64 recv, int64 total)
{
	setNumBytesRead (recv);
	setNumBytesTotal (total);
	setState (EDownloading);
	updateToTable();
}

// =============================================================================
//
void PartDownloadRequest::readyRead()
{
	if (state() == EFailed)
		return;

	if (filePointer() == null)
	{
		m_filePath.replace ("\\", "/");

		// We have already asked the user whether we can overwrite so we're good
		// to go here.
		setFilePointer (new QFile (filePath().toLocal8Bit()));

		if (!filePointer()->open (QIODevice::WriteOnly))
		{
			critical (format (tr ("Couldn't open %1 for writing: %2"), filePath(), strerror (errno)));
			setState (EFailed);
			networkReply()->abort();
			updateToTable();
			prompt()->checkIfFinished();
			return;
		}
	}

	filePointer()->write (networkReply()->readAll());
}

// =============================================================================
//
bool PartDownloadRequest::isFinished() const
{
	return state() == EFinished || state() == EFailed;
}

// =============================================================================
//
void PartDownloadRequest::abort()
{
	networkReply()->abort();
}

// =============================================================================
//
DEFINE_ACTION (DownloadFrom, 0)
{
	PartDownloader::staticBegin();
}
