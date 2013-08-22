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
#include "moc_download.cxx"
#include "file.h"
#include "gldraw.h"

cfg (String, net_downloadpath, "");
cfg (Bool, net_guesspaths, true);
cfg (Bool, net_autoclose, false);

constexpr const char* PartDownloader::k_OfficialURL,
	*PartDownloader::k_UnofficialURL;

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloader::k_download() {
	str path = getDownloadPath();
	if (path == "" || QDir (path).exists() == false) {
		critical (PartDownloader::tr ("You need to specify a valid path for "
			"downloaded files in the configuration to download paths."));
		return;
	}
	
	PartDownloader* dlg = new PartDownloader;
	dlg->exec();
}

// =============================================================================
// -----------------------------------------------------------------------------
str PartDownloader::getDownloadPath() {
	str path = net_downloadpath;
	
#if DIRSLASH_CHAR != '/'
	path.replace (DIRSLASH, "/");
#endif
	
	return path;
}

// =============================================================================
// -----------------------------------------------------------------------------
PartDownloader::PartDownloader (QWidget* parent) : QDialog (parent) {
	ui = new Ui_DownloadFrom;
	ui->setupUi (this);
	ui->fname->setFocus();
	ui->progress->horizontalHeader()->setResizeMode (PartLabelColumn, QHeaderView::Stretch);
	
	m_downloadButton = new QPushButton (tr ("Download"));
	ui->buttonBox->addButton (m_downloadButton, QDialogButtonBox::ActionRole);
	getButton (Abort)->setEnabled (false);
	
	connect (ui->source, SIGNAL (currentIndexChanged (int)), this, SLOT (sourceChanged (int)));
	connect (ui->buttonBox, SIGNAL (clicked (QAbstractButton*)), this, SLOT (buttonClicked (QAbstractButton*)));
}

// =============================================================================
// -----------------------------------------------------------------------------
PartDownloader::~PartDownloader() {
	delete ui;
}

// =============================================================================
// -----------------------------------------------------------------------------
str PartDownloader::getURL() const {
	const Source src = getSource();
	str dest;
	
	switch (src) {
	case PartsTracker:
		dest = ui->fname->text();
		modifyDest (dest);
		return str (k_UnofficialURL) + dest;
	
	case CustomURL:
		return ui->fname->text();
	}
	
	// Shouldn't happen
	return "";
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloader::modifyDest (str& dest) const {
	dest = dest.simplified();
	
	// If the user doesn't want us to guess, stop right here.
	if (net_guesspaths == false)
		return;
	
	// Ensure .dat extension
	if (dest.right (4) != ".dat") {
		/* Remove the existing extension, if any. It may be we're here over a
		   typo in the .dat extension. */
		const int dotpos = dest.lastIndexOf (".");
		if (dotpos != -1 && dotpos >= dest.length() - 4)
			dest.chop (dest.length() - dotpos);
		
		dest += ".dat";
	}
	
	/* If the part starts with s\ or s/, then use parts/s/. Same goes with
	   48\ and p/48/. */
	if (dest.left (2) == "s\\" || dest.left (2) == "s/") {
		dest.remove (0, 2);
		dest.prepend ("parts/s/");
	} elif (dest.left (3) == "48\\" || dest.left (3) == "48/") {
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
PartDownloader::Source PartDownloader::getSource() const {
	return (Source) ui->source->currentIndex();
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloader::sourceChanged (int i) {
	if (i == CustomURL)
		ui->fileNameLabel->setText (tr ("URL:"));
	else
		ui->fileNameLabel->setText (tr ("File name:"));
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloader::buttonClicked (QAbstractButton* btn) {
	if (btn == getButton (Close)) {
		reject();
	} elif (btn == getButton (Abort)) {
		setAborted (true);
		
		for (PartDownloadRequest* req : m_requests)
			req->abort();
	} elif (btn == getButton (Download)) {
		str dest = ui->fname->text();
		setPrimaryFile (null);
		setAborted (false);
		
		if (getSource() == CustomURL)
			dest = basename (getURL());
		
		modifyDest (dest);
		
		if (QFile::exists (PartDownloader::getDownloadPath() + dest)) {
			const str overwritemsg = fmt (tr ("%1 already exists in download directory. Overwrite?"), dest);
			
			if (!confirm (tr ("Overwrite?"), overwritemsg))
				return;
		}
		
		m_downloadButton->setEnabled (false);
		ui->progress->setEnabled (true);
		ui->fname->setEnabled (false);
		ui->source->setEnabled (false);
		downloadFile (dest, getURL(), true);
		getButton (Close)->setEnabled (false);
		getButton (Abort)->setEnabled (true);
		getButton (Download)->setEnabled (false);
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloader::downloadFile (str dest, str url, bool primary) {
	const int row = ui->progress->rowCount();
	
	// Don't download files repeadetly.
	if (m_filesToDownload.find (dest) != -1u)
		return;
	
	modifyDest (dest);
	print ("DOWNLOAD: %1 -> %2\n", url, PartDownloader::getDownloadPath() + dest);
	PartDownloadRequest* req = new PartDownloadRequest (url, dest, primary, this);
	
	m_filesToDownload << dest;
	m_requests << req;
	ui->progress->insertRow (row);
	req->setTableRow (row);
	req->updateToTable();
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloader::checkIfFinished() {
	bool failed = aborted();
	
	// If there is some download still working, we're not finished.
	for (PartDownloadRequest* req : m_requests) {
		if (!req->isFinished())
			return;
		
		if (req->state() == PartDownloadRequest::Failed)
			failed = true;
	}
	
	for (PartDownloadRequest* req : m_requests)
		delete req;
	
	m_requests.clear();
	
	// Update everything now
	if (primaryFile()) {
		LDFile::setCurrent (primaryFile());
		reloadAllSubfiles();
		g_win->fullRefresh();
		g_win->R()->resetAngles();
	}
	
	if (net_autoclose && !failed) {
		// Close automatically if desired.
		accept();
	} else {
		// Allow the prompt be closed now.
		getButton (Abort)->setEnabled (false);
		getButton (Close)->setEnabled (true);
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
QPushButton* PartDownloader::getButton (PartDownloader::Button i) {
	typedef QDialogButtonBox QDBB;
	alias btnbox = ui->buttonBox;
	
	switch (i) {
	case Download:
		return m_downloadButton;
	
	case Abort:
		return qobject_cast<QPushButton*> (btnbox->button (QDBB::Abort));
	
	case Close:
		return qobject_cast<QPushButton*> (btnbox->button (QDBB::Close));
	}
	
	return null;
}

// =============================================================================
// -----------------------------------------------------------------------------
PartDownloadRequest::PartDownloadRequest (str url, str dest, bool primary, PartDownloader* parent) :
	QObject (parent),
	m_prompt (parent),
	m_url (url),
	m_dest (dest),
	m_fpath (PartDownloader::getDownloadPath() + dest),
	m_nam (new QNetworkAccessManager),
	m_firstUpdate (true),
	m_state (Requesting),
	m_primary (primary),
	m_fp (null)
{
	// Make sure that we have a valid destination.
	str dirpath = dirname (m_fpath);
	
	QDir dir (dirpath);
	if (!dir.exists()) {
		print ("Creating %1...\n", dirpath);
		if (!dir.mkpath (dirpath))
			critical (fmt (tr ("Couldn't create the directory %1!"), dirpath));
	}
	
	m_reply = m_nam->get (QNetworkRequest (QUrl (url)));
	connect (m_reply, SIGNAL (finished()), this, SLOT (downloadFinished()));
	connect (m_reply, SIGNAL (readyRead()), this, SLOT (readyRead()));
	connect (m_reply, SIGNAL (downloadProgress (qint64, qint64)), this, SLOT (downloadProgress (qint64, qint64)));
}

// =============================================================================
// -----------------------------------------------------------------------------
PartDownloadRequest::~PartDownloadRequest() {}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadRequest::updateToTable() {
	const int labelcol = PartDownloader::PartLabelColumn,
		progcol = PartDownloader::ProgressColumn;
	QTableWidget* table = m_prompt->ui->progress;
	QProgressBar* prog;
	
	switch (m_state) {
	case Requesting:
	case Downloading:
		prog = qobject_cast<QProgressBar*> (table->cellWidget (tableRow(), progcol));
		
		if (!prog) {
			prog = new QProgressBar;
			table->setCellWidget (tableRow(), progcol, prog);
		}
		
		prog->setRange (0, m_bytesTotal);
		prog->setValue (m_bytesRead);
		break;
	
	case Finished:
	case Failed:
		{
			QLabel* lb = new QLabel ((m_state == Finished) ? "<b><span style=\"color: #080\">FINISHED</span></b>" :
				"<b><span style=\"color: #800\">FAILED</span></b>");
			lb->setAlignment (Qt::AlignCenter);
			table->setCellWidget (tableRow(), progcol, lb);
		}
		break;
	}
	
	QLabel* lb = qobject_cast<QLabel*> (table->cellWidget (tableRow(), labelcol));
	if (m_firstUpdate) {
		lb = new QLabel (fmt ("<b>%1</b>", m_dest), table);
		table->setCellWidget (tableRow(), labelcol, lb);
	}
	
	// Make sure that the cell is big enough to contain the label
	if (table->columnWidth (labelcol) < lb->width())
		table->setColumnWidth (labelcol, lb->width());
	
	m_firstUpdate = false;
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadRequest::downloadFinished() {
	if (m_reply->error() != QNetworkReply::NoError) {
		if (m_primary && !m_prompt->aborted())
			critical (m_reply->errorString());
		
		m_state = Failed;
	} elif (state() != Failed)
		m_state = Finished;
	
	m_bytesRead = m_bytesTotal;
	updateToTable();
	
	if (m_fp) {
		m_fp->close();
		delete m_fp;
		m_fp = null;
		
		if (m_state == Failed)
			QFile::remove (m_fpath);
	}
	
	if (m_state != Finished) {
		m_prompt->checkIfFinished();
		return;
	}
	
	// Try to load this file now.
	LDFile* f = openDATFile (m_fpath, false);
	if (!f)
		return;
	
	f->setImplicit (!m_primary);
	
	// Iterate through this file and check for errors. If there's any that stems
	// from unknown file references, try resolve that by downloading the reference.
	// This is why downloading a part may end up downloading multiple files, as
	// it resolves dependencies.
	for (LDObject* obj : *f) {
		LDError* err = dynamic_cast<LDError*> (obj);
		if (!err || err->fileRef().isEmpty())
			continue;
		
		str dest = err->fileRef();
		m_prompt->modifyDest (dest);
		m_prompt->downloadFile (dest, str (PartDownloader::k_UnofficialURL) + dest, false);
	}
	
	if (m_primary) {
		addRecentFile (m_fpath);
		m_prompt->setPrimaryFile (f);
	}
	
	m_prompt->checkIfFinished();
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadRequest::downloadProgress (int64 recv, int64 total) {
	m_bytesRead = recv;
	m_bytesTotal = total;
	m_state = Downloading;
	updateToTable();
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadRequest::readyRead() {
	if (state() == Failed)
		return;
	
	if (m_fp == null) {
		m_fpath.replace ("\\", "/");
		
		// We have already asked the user whether we can overwrite so we're good
		// to go here.
		m_fp = new QFile (m_fpath.toLocal8Bit());
		if (!m_fp->open (QIODevice::WriteOnly)) {
			critical (fmt (tr ("Couldn't open %1 for writing: %2"), m_fpath, strerror (errno)));
			m_state = Failed;
			m_reply->abort();
			updateToTable();
			m_prompt->checkIfFinished();
			return;
		}
	}
	
	m_fp->write (m_reply->readAll());
}

// =============================================================================
// -----------------------------------------------------------------------------
bool PartDownloadRequest::isFinished() const {
	return m_state == Finished || m_state == Failed;
}

// =============================================================================
// -----------------------------------------------------------------------------
const PartDownloadRequest::State& PartDownloadRequest::state() const {
	return m_state;
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadRequest::abort() {
	m_reply->abort();
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (DownloadFrom, 0) {
	PartDownloader::k_download();
}