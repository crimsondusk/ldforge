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
#include "download.h"
#include "ui_downloadfrom.h"
#include "types.h"
#include "gui.h"
#include "build/moc_download.cpp"
#include "file.h"
#include "gldraw.h"

PartDownloader g_PartDownloader;

cfg (str, net_downloadpath, "");

constexpr const char* PartDownloader::k_OfficialURL,
	*PartDownloader::k_UnofficialURL;

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloader::download() {
	str path = getDownloadPath();
	if (path == "" || QDir (path).exists() == false) {
		critical (PartDownloadPrompt::tr ("You need to specify a valid path for "
			"downloaded files in the configuration to download paths."));
		return;
	}
	
	PartDownloadPrompt* dlg = new PartDownloadPrompt;
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
PartDownloadPrompt::PartDownloadPrompt (QWidget* parent) : QDialog (parent) {
	ui = new Ui_DownloadFrom;
	ui->setupUi (this);
	ui->fname->setFocus();
	
	connect (ui->source, SIGNAL (currentIndexChanged (int)), this, SLOT (sourceChanged (int)));
	connect (ui->buttonBox, SIGNAL (accepted()), this, SLOT (startDownload()));
}

// =============================================================================
// -----------------------------------------------------------------------------
PartDownloadPrompt::~PartDownloadPrompt() {
	delete ui;
}

// =============================================================================
// -----------------------------------------------------------------------------
str PartDownloadPrompt::getURL() const {
	const Source src = getSource();
	str dest;
	
	switch (src) {
	case PartsTracker:
		dest = ui->fname->text();
		modifyDest (dest);
		return str (PartDownloader::k_UnofficialURL) + dest;
	
	case CustomURL:
		return ui->fname->text();
	}
	
	// Shouldn't happen
	return "";
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadPrompt::modifyDest (str& dest) const {
	dest = dest.simplified();
	
	if (getSource() == CustomURL)
		dest = basename (dest);
	
	// Ensure .dat extension
	if (dest.right (4) != ".dat") {
		// Remove the existing extension, if any. It may be we're here over a
		// typo in the .dat extension.
		const int dotpos = dest.lastIndexOf (".");
		if (dotpos != -1 && dotpos >= dest.length() - 4)
			dest.chop (dest.length() - dotpos);
		
		dest += ".dat";
	}
	
	// If the part starts with s\ or s/, then use parts/s/. Same goes with
	// 48\ and p/48/.
	if (dest.left (2) == "s\\" || dest.left (2) == "s/") {
		dest.remove (0, 2);
		dest.prepend ("parts/s/");
	} elif (dest.left (3) == "48\\" || dest.left (3) == "48/") {
		dest.remove (0, 3);
		dest.prepend ("p/48/");
	}
	
	/* Try determine where to put this part. We have four directories:
	 * parts/, parts/s/, p/, and p/48/. If we haven't already specified
	 * either parts/ or p/, we need to add it automatically. Part files
	 * are numbers which can be followed by:
	 * - c** (composites)
	 * - d** (formed stickers)
	 * - a lowercase alphabetic letter for variants
	 *
	 * Subfiles have an s** prefix, in which case we use parts/s/. Note that
	 * the regex starts with a '^' so it won't catch already fully given part
	 * file names.
	 */
	{
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
}

// =============================================================================
// -----------------------------------------------------------------------------
PartDownloadPrompt::Source PartDownloadPrompt::getSource() const {
	return (Source) ui->source->currentIndex();
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadPrompt::sourceChanged (int i) {
	if (i == CustomURL)
		ui->fileNameLabel->setText (tr ("URL:"));
	else
		ui->fileNameLabel->setText (tr ("File name:"));
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadPrompt::startDownload() {
	ui->progress->setEnabled (true);
	ui->buttonBox->setEnabled (false);
	ui->fname->setEnabled (false);
	ui->source->setEnabled (false);
	
	str dest = ui->fname->text();
	modifyDest (dest);
	downloadFile (dest, getURL(), true);
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadPrompt::downloadFile (str dest, str url, bool primary) {
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
void PartDownloadPrompt::checkIfFinished() {
	// If there is some download still working, we're not finished.
	for (PartDownloadRequest* req : m_requests)
		if (!req->isFinished())
			return;
	
	// Update everything now
	reloadAllSubfiles();
	g_win->fullRefresh();
	g_win->R()->resetAngles();
	
	// Allow the prompt be closed now.
	ui->buttonBox->disconnect (SIGNAL (accepted()));
	connect (ui->buttonBox, SIGNAL (accepted()), this, SLOT (accept()));
	ui->buttonBox->setEnabled (true);
	ui->progress->setEnabled (false);
	
	// accept();
}

// =============================================================================
// -----------------------------------------------------------------------------
PartDownloadRequest::PartDownloadRequest (str url, str dest, bool primary, PartDownloadPrompt* parent) :
	QObject (parent),
	m_prompt (parent),
	m_url (url),
	m_dest (dest),
	m_fpath (PartDownloader::getDownloadPath() + dest),
	m_nam (new QNetworkAccessManager),
	m_firstUpdate (true),
	m_state (Requesting),
	m_primary (primary)
{
	m_fpath.replace ("\\", "/");
	QFile::remove (m_fpath);
	
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
	connect (m_reply, SIGNAL (error (QNetworkReply::NetworkError)), this, SLOT (downloadError()));
}

// =============================================================================
// -----------------------------------------------------------------------------
PartDownloadRequest::~PartDownloadRequest() {}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadRequest::updateToTable() {
	QTableWidget* table = m_prompt->ui->progress;
	QProgressBar* prog;
	
	switch (m_state) {
	case Requesting:
	case Downloading:
		prog = qobject_cast<QProgressBar*> (table->cellWidget (tableRow(), ProgressColumn));
	
		if (!prog) {
			prog = new QProgressBar;
			table->setCellWidget (tableRow(), ProgressColumn, prog);
		}
		
		prog->setRange (0, m_bytesTotal);
		prog->setValue (m_bytesRead);
		break;
	
	case Finished:
	case Failed:
	case Aborted:
		{
			QLabel* lb = new QLabel ((m_state == Finished) ? "<b><span style=\"color: #080\">FINISHED</span></b>" :
				"<b><span style=\"color: #800\">FAILED</span></b>");
			lb->setAlignment (Qt::AlignCenter);
			table->setCellWidget (tableRow(), ProgressColumn, lb);
		}
		break;
	}
	
	QLabel* lb = qobject_cast<QLabel*> (table->cellWidget (tableRow(), PartLabelColumn));
	if (m_firstUpdate) {
		lb = new QLabel (fmt ("<b>%1</b>", m_dest), table);
		table->setCellWidget (tableRow(), PartLabelColumn, lb);
	}
	
	// Make sure that the cell is big enough to contain the label
	if (table->columnWidth (PartLabelColumn) < lb->width())
		table->setColumnWidth (PartLabelColumn, lb->width());
	
	m_firstUpdate = false;
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadRequest::downloadFinished() {
	m_state = m_reply->error() == QNetworkReply::NoError ? Finished : Failed;
	m_bytesRead = m_bytesTotal;
	updateToTable();
	
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
		if (obj->getType() != LDObject::Error)
			continue;
		
		LDErrorObject* err = static_cast<LDErrorObject*> (obj);
		if (err->fileRef() == "")
			continue;
		
		str dest = err->fileRef();
		m_prompt->modifyDest (dest);
		m_prompt->downloadFile (dest, str (PartDownloader::k_UnofficialURL) + dest, false);
	}
	
	if (m_primary)
		LDFile::setCurrent (f);
	
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
	QFile f (m_fpath);
	if (!f.open (QIODevice::Append))
		return;
	
	f.write (m_reply->readAll());
	f.close();
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadRequest::downloadError() {
	if (m_primary)
		critical (m_reply->errorString());
	
	QFile::remove (m_fpath);
	m_state = Failed;
	updateToTable();
}

// =============================================================================
// -----------------------------------------------------------------------------
bool PartDownloadRequest::isFinished() const {
	return m_state == Finished || m_state == Failed || m_state == Aborted;
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (DownloadFrom, 0) {
	g_PartDownloader.download();
}