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

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDir>
#include <QProgressBar>
#include <QPushButton>
#include "download.h"
#include "ui_downloadfrom.h"
#include "types.h"
#include "gui.h"
#include "file.h"
#include "gldraw.h"
#include "configDialog.h"
#include "moc_download.cpp"

cfg (String,	net_downloadpath,	"");
cfg (Bool,		net_guesspaths,	true);
cfg (Bool,		net_autoclose,		false);

const str g_unofficialLibraryURL ("http://ldraw.org/library/unofficial/");

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloader::staticBegin()
{	str path = getDownloadPath();

	if (path == "" || QDir (path).exists() == false)
	{	critical (PartDownloader::tr ("You need to specify a valid path for "
			"downloaded files in the configuration to download paths."));

		(new ConfigDialog (ConfigDialog::DownloadTab, null))->exec();
		return;
	}

	PartDownloader* dlg = new PartDownloader;
	dlg->exec();
}

// =============================================================================
// -----------------------------------------------------------------------------
str PartDownloader::getDownloadPath()
{	str path = net_downloadpath;

#if DIRSLASH_CHAR != '/'
	path.replace (DIRSLASH, "/");
#endif

	return path;
}

// =============================================================================
// -----------------------------------------------------------------------------
PartDownloader::PartDownloader (QWidget* parent) : QDialog (parent)
{	setInterface (new Ui_DownloadFrom);
	getInterface()->setupUi (this);
	getInterface()->fname->setFocus();
	getInterface()->progress->horizontalHeader()->setResizeMode (PartLabelColumn, QHeaderView::Stretch);

	setDownloadButton (new QPushButton (tr ("Download")));
	getInterface()->buttonBox->addButton (getDownloadButton(), QDialogButtonBox::ActionRole);
	getButton (Abort)->setEnabled (false);

	connect (getInterface()->source, SIGNAL (currentIndexChanged (int)),
		this, SLOT (sourceChanged (int)));
	connect (getInterface()->buttonBox, SIGNAL (clicked (QAbstractButton*)),
		this, SLOT (buttonClicked (QAbstractButton*)));
}

// =============================================================================
// -----------------------------------------------------------------------------
PartDownloader::~PartDownloader()
{	delete getInterface();
}

// =============================================================================
// -----------------------------------------------------------------------------
str PartDownloader::getURL() const
{	const Source src = getSource();
	str dest;

	switch (src)
	{	case PartsTracker:
			dest = getInterface()->fname->text();
			modifyDestination (dest);
			return g_unofficialLibraryURL + dest;

		case CustomURL:
			return getInterface()->fname->text();
	}

	// Shouldn't happen
	return "";
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloader::modifyDestination (str& dest) const
{	dest = dest.simplified();

	// If the user doesn't want us to guess, stop right here.
	if (net_guesspaths == false)
		return;

	// Ensure .dat extension
	if (dest.right (4) != ".dat")
	{	// Remove the existing extension, if any. It may be we're here over a
		// typo in the .dat extension.
		const int dotpos = dest.lastIndexOf (".");

		if (dotpos != -1 && dotpos >= dest.length() - 4)
			dest.chop (dest.length() - dotpos);

		dest += ".dat";
	}

	// If the part starts with s\ or s/, then use parts/s/. Same goes with
	// 48\ and p/48/.
	if (dest.left (2) == "s\\" || dest.left (2) == "s/")
	{	dest.remove (0, 2);
		dest.prepend ("parts/s/");
	} elif (dest.left (3) == "48\\" || dest.left (3) == "48/")
	{	dest.remove (0, 3);
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
	str partRegex = "^u?[0-9]+(c[0-9][0-9]+)*(d[0-9][0-9]+)*[a-z]?(p[0-9a-z][0-9a-z]+)*";
	str subpartRegex = partRegex + "s[0-9][0-9]+";

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
// -----------------------------------------------------------------------------
PartDownloader::Source PartDownloader::getSource() const
{	return (Source) getInterface()->source->currentIndex();
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloader::sourceChanged (int i)
{	if (i == CustomURL)
		getInterface()->fileNameLabel->setText (tr ("URL:"));
	else
		getInterface()->fileNameLabel->setText (tr ("File name:"));
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloader::buttonClicked (QAbstractButton* btn)
{	if (btn == getButton (Close))
	{	reject();
	}
	elif (btn == getButton (Abort))
	{	setAborted (true);

		for (PartDownloadRequest* req : getRequests())
			req->abort();
	}
	elif (btn == getButton (Download))
	{	str dest = getInterface()->fname->text();
		setPrimaryFile (null);
		setAborted (false);

		if (getSource() == CustomURL)
			dest = basename (getURL());

		modifyDestination (dest);

		if (QFile::exists (PartDownloader::getDownloadPath() + DIRSLASH + dest))
		{	const str overwritemsg = fmt (tr ("%1 already exists in download directory. Overwrite?"), dest);
			if (!confirm (tr ("Overwrite?"), overwritemsg))
				return;
		}

		getDownloadButton()->setEnabled (false);
		getInterface()->progress->setEnabled (true);
		getInterface()->fname->setEnabled (false);
		getInterface()->source->setEnabled (false);
		downloadFile (dest, getURL(), true);
		getButton (Close)->setEnabled (false);
		getButton (Abort)->setEnabled (true);
		getButton (Download)->setEnabled (false);
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloader::downloadFile (str dest, str url, bool primary)
{	const int row = getInterface()->progress->rowCount();

	// Don't download files repeadetly.
	if (getFilesToDownload().indexOf (dest) != -1)
		return;

	modifyDestination (dest);
	log ("DOWNLOAD: %1 -> %2\n", url, PartDownloader::getDownloadPath() + DIRSLASH + dest);
	PartDownloadRequest* req = new PartDownloadRequest (url, dest, primary, this);

	pushToFilesToDownload (dest);
	pushToRequests (req);
	getInterface()->progress->insertRow (row);
	req->setTableRow (row);
	req->updateToTable();
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloader::checkIfFinished()
{	bool failed = isAborted();

	// If there is some download still working, we're not finished.
	for (PartDownloadRequest* req : getRequests())
	{	if (!req->isFinished())
			return;

		if (req->getState() == PartDownloadRequest::EFailed)
			failed = true;
	}

	for (PartDownloadRequest* req : getRequests())
		delete req;

	clearRequests();

	// Update everything now
	if (getPrimaryFile())
	{	LDFile::setCurrent (getPrimaryFile());
		reloadAllSubfiles();
		g_win->doFullRefresh();
		g_win->R()->resetAngles();
	}

	if (net_autoclose && !failed)
	{	// Close automatically if desired.
		accept();
	}
	else
	{	// Allow the prompt be closed now.
		getButton (Abort)->setEnabled (false);
		getButton (Close)->setEnabled (true);
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
QPushButton* PartDownloader::getButton (PartDownloader::Button i)
{	switch (i)
	{	case Download:
			return getDownloadButton();

		case Abort:
			return qobject_cast<QPushButton*> (getInterface()->buttonBox->button (QDialogButtonBox::Abort));

		case Close:
			return qobject_cast<QPushButton*> (getInterface()->buttonBox->button (QDialogButtonBox::Close));
	}

	return null;
}

// =============================================================================
// -----------------------------------------------------------------------------
PartDownloadRequest::PartDownloadRequest (str url, str dest, bool primary, PartDownloader* parent) :
	QObject (parent),
	m_State (ERequesting),
	m_Prompt (parent),
	m_URL (url),
	m_Destinaton (dest),
	m_FilePath (PartDownloader::getDownloadPath() + DIRSLASH + dest),
	m_NAM (new QNetworkAccessManager),
	m_FirstUpdate (true),
	m_Primary (primary),
	m_FilePointer (null)
{
	// Make sure that we have a valid destination.
	str dirpath = dirname (getFilePath());

	QDir dir (dirpath);

	if (!dir.exists())
	{	log ("Creating %1...\n", dirpath);

		if (!dir.mkpath (dirpath))
			critical (fmt (tr ("Couldn't create the directory %1!"), dirpath));
	}

	setReply (getNAM()->get (QNetworkRequest (QUrl (url))));
	connect (getReply(), SIGNAL (finished()), this, SLOT (downloadFinished()));
	connect (getReply(), SIGNAL (readyRead()), this, SLOT (readyRead()));
	connect (getReply(), SIGNAL (downloadProgress (qint64, qint64)),
		this, SLOT (downloadProgress (qint64, qint64)));
}

// =============================================================================
// -----------------------------------------------------------------------------
PartDownloadRequest::~PartDownloadRequest() {}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadRequest::updateToTable()
{	const int		labelcol = PartDownloader::PartLabelColumn,
						progcol = PartDownloader::ProgressColumn;
	QTableWidget*	table = getPrompt()->getInterface()->progress;
	QProgressBar*	prog;

	switch (getState())
	{	case ERequesting:
		case EDownloading:
		{	prog = qobject_cast<QProgressBar*> (table->cellWidget (getTableRow(), progcol));

			if (!prog)
			{	prog = new QProgressBar;
				table->setCellWidget (getTableRow(), progcol, prog);
			}

			prog->setRange (0, getBytesTotal());
			prog->setValue (getBytesRead());
		} break;

		case EFinished:
		case EFailed:
		{	const str text = (getState() == EFinished)
				? "<b><span style=\"color: #080\">FINISHED</span></b>"
				: "<b><span style=\"color: #800\">FAILED</span></b>";

			QLabel* lb = new QLabel (text);
			lb->setAlignment (Qt::AlignCenter);
			table->setCellWidget (getTableRow(), progcol, lb);
		} break;
	}

	QLabel* lb = qobject_cast<QLabel*> (table->cellWidget (getTableRow(), labelcol));

	if (isFirstUpdate())
	{	lb = new QLabel (fmt ("<b>%1</b>", getDestinaton()), table);
		table->setCellWidget (getTableRow(), labelcol, lb);
	}

	// Make sure that the cell is big enough to contain the label
	if (table->columnWidth (labelcol) < lb->width())
		table->setColumnWidth (labelcol, lb->width());

	setFirstUpdate (true);
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadRequest::downloadFinished()
{	if (getReply()->error() != QNetworkReply::NoError)
	{	if (isPrimary() && !getPrompt()->isAborted())
			critical (getReply()->errorString());

		setState (EFailed);
	}
	elif (getState() != EFailed)
		setState (EFinished);

	setBytesRead (getBytesTotal());
	updateToTable();

	if (getFilePointer())
	{	getFilePointer()->close();
		delete getFilePointer();
		setFilePointer (null);

		if (getState() == EFailed)
			QFile::remove (getFilePath());
	}

	if (getState() != EFinished)
	{	getPrompt()->checkIfFinished();
		return;
	}

	// Try to load this file now.
	LDFile* f = openDATFile (getFilePath(), false);

	if (!f)
		return;

	f->setImplicit (!isPrimary());

	// Iterate through this file and check for errors. If there's any that stems
	// from unknown file references, try resolve that by downloading the reference.
	// This is why downloading a part may end up downloading multiple files, as
	// it resolves dependencies.
	for (LDObject* obj : f->getObjects())
	{	LDError* err = dynamic_cast<LDError*> (obj);

		if (!err || err->getFileReferenced().isEmpty())
			continue;

		str dest = err->getFileReferenced();
		getPrompt()->modifyDestination (dest);
		getPrompt()->downloadFile (dest, g_unofficialLibraryURL + dest, false);
	}

	if (isPrimary())
	{	addRecentFile (getFilePath());
		getPrompt()->setPrimaryFile (f);
	}

	getPrompt()->checkIfFinished();
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadRequest::downloadProgress (int64 recv, int64 total)
{	setBytesRead (recv);
	setBytesTotal (total);
	setState (EDownloading);
	updateToTable();
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadRequest::readyRead()
{	if (getState() == EFailed)
		return;

	if (getFilePointer() == null)
	{	replaceInFilePath ("\\", "/");

		// We have already asked the user whether we can overwrite so we're good
		// to go here.
		setFilePointer (new QFile (getFilePath().toLocal8Bit()));

		if (!getFilePointer()->open (QIODevice::WriteOnly))
		{	critical (fmt (tr ("Couldn't open %1 for writing: %2"), getFilePath(), strerror (errno)));
			setState (EFailed);
			getReply()->abort();
			updateToTable();
			getPrompt()->checkIfFinished();
			return;
		}
	}

	getFilePointer()->write (getReply()->readAll());
}

// =============================================================================
// -----------------------------------------------------------------------------
bool PartDownloadRequest::isFinished() const
{	return getState() == EFinished || getState() == EFailed;
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadRequest::abort()
{	getReply()->abort();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (DownloadFrom, 0)
{	PartDownloader::staticBegin();
}
