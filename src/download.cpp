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

constexpr const char* PartDownloader::k_OfficialURL,
	*PartDownloader::k_UnofficialURL;

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloader::download() {
	PartDownloadPrompt* dlg = new PartDownloadPrompt;
	dlg->exec();
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
	
	switch (src) {
/*	case OfficialLibrary:
		return str (PartDownloader::k_OfficialURL) + getDest();
*/
	case PartsTracker:
		return str (PartDownloader::k_UnofficialURL) + getDest();
	
	case CustomURL:
		return ui->fname->text();
	}
	
	// Shouldn't happen
	return "";
}

// =============================================================================
// -----------------------------------------------------------------------------
str PartDownloadPrompt::getDest() const {
	str fname = ui->fname->text();
	
	if (getSource() == CustomURL)
		fname = basename (fname);
	
	// Ensure .dat extension
	if (fname.right (4) != ".dat") {
		// Remove the existing extension, if any. It may be we're here over a
		// typo in the .dat extension.
		if (fname.lastIndexOf (".") >= fname.length() - 4)
			fname.chop (fname.length() - fname.lastIndexOf ("."));
		
		fname += ".dat";
	}
	
	// If the part starts with s\ or s/, then use parts/s/. Same goes with
	// 48\ and p/48/.
	if (fname.left (2) == "s\\" || fname.left (2) == "s/") {
		fname.remove (0, 2);
		fname.prepend ("parts/s/");
	} elif (fname.left (3) == "48\\" || fname.left (3) == "48/") {
		fname.remove (0, 3);
		fname.prepend ("p/48/");
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
		str partRegex = "^[0-9]+(c[0-9][0-9]+)*(d[0-9][0-9]+)*[a-z]?";
		str subpartRegex = partRegex + "s[0-9][0-9]+";
		
		partRegex += "\\.dat$";
		subpartRegex += "\\.dat$";
		
		if (QRegExp (subpartRegex).exactMatch (fname))
			fname.prepend ("parts/s/");
		elif (QRegExp (partRegex).exactMatch (fname))
			fname.prepend ("parts/");
		elif (fname.left (6) != "parts/" && fname.left (2) != "p/")
			fname.prepend ("p/");
	}
	
	return fname;
}

// =============================================================================
// -----------------------------------------------------------------------------
str PartDownloadPrompt::fullFilePath() const {
	return "/home/arezey/ldraw/downloads/" + getDest(); // FIXME: hehe
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
	downloadFile (getDest(), true);
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadPrompt::downloadFile (const str& path, bool primary) {
	const int row = ui->progress->rowCount();
	
	// Don't download files repeadetly.
	if (m_filesToDownload.find (path) != -1u)
		return;
	
	print ("%1: row: %2\n", path, row);
	PartDownloadRequest* req = new PartDownloadRequest (getURL(), path, primary, this);
	
	m_filesToDownload << path;
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
	g_win->fullRefresh();
	g_win->R()->resetAngles();
	
	// Close the dialog
	accept();
}

// =============================================================================
// -----------------------------------------------------------------------------
PartDownloadRequest::PartDownloadRequest (str url, str dest, bool primary, PartDownloadPrompt* parent) :
	QObject (parent),
	m_prompt (parent),
	m_url (url),
	m_dest (dest),
	m_fpath (m_prompt->fullFilePath()),
	m_nam (new QNetworkAccessManager),
	m_firstUpdate (true),
	m_state (Requesting),
	m_primary (primary)
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
		lb = new QLabel (fmt ("<b>%1</b><br>%2", m_dest, m_url), table);
		table->setCellWidget (tableRow(), PartLabelColumn, lb);
		table->setRowHeight (tableRow(), lb->height());
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
	
	// Check for any errors which stemmed from unresolved references.
	// Try to solve these by downloading them.
	for (LDObject* obj : *f) {
		if (obj->getType() != LDObject::Error)
			continue;
		
		LDErrorObject* err = static_cast<LDErrorObject*> (obj);
		if (err->fileRef() == "")
			continue;
		
		m_prompt->downloadFile (err->fileRef(), false);
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