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

#include "download.h"
#include "ui_downloadfrom.h"
#include "types.h"
#include "gui.h"
#include "build/moc_download.cpp"

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
str PartDownloadPrompt::getURL() {
	str fname;
	Source src = (Source) ui->source->currentIndex();
	
	switch (src) {
	case OfficialLibrary:
	case PartsTracker:
		{
			fname = ui->fname->text();
		
			// Ensure .dat extension
			if (fname.right (4) != ".dat") {
				// Remove the existing extension, if any. It may be we're here over a
				// typo in the .dat extension.
				if (fname.lastIndexOf (".") >= fname.length() - 4)
					fname.chop (fname.length() - fname.lastIndexOf ("."));
				
				fname += ".dat";
			}
			
			/* These sources have stuff only in parts/, parts/s/, p/, and p/48/. If
			* we haven't already specified either parts/ or p/, we need to add it
			* automatically. Part files are numbers which can be followed by:
			* - c** (composites)
			* - d** (formed stickers)
			* - a lowercase alphabetic letter for variants
			*
			* Subfiles have an s** prefix, in which case we use parts/s/. Note that
			* the regex starts with a '^' so it won't catch already fully given part
			* file names.
			*/
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
		
		if (src == OfficialLibrary)
			return str (PartDownloader::k_OfficialURL) + fname;
		
		return str (PartDownloader::k_UnofficialURL) + fname;
	
	case CustomURL:
		return ui->fname->text();
	}
	
	// Shouldn't happen
	return "";
}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadPrompt::sourceChanged (int i) {
	if (i == CustomURL)
		ui->fileNameLabel->setText (tr ("URL:"));
	else
		ui->fileNameLabel->setText (tr ("File name:"));
}

void PartDownloadPrompt::startDownload() {
	int row = ui->progress->rowCount();
	ui->progress->setEnabled (true);
	ui->buttonBox->setEnabled (false);
	ui->progress->insertRow (row);
	
	PartDownloadRequest* req = new PartDownloadRequest (getURL(), this);
	req->setTableRow (row);
	req->updateToTable();
}

// =============================================================================
// -----------------------------------------------------------------------------
PartDownloadRequest::PartDownloadRequest (str url, PartDownloadPrompt* parent) :
	QObject (parent),
	m_prompt (parent),
	m_url (url) {}

// =============================================================================
// -----------------------------------------------------------------------------
void PartDownloadRequest::updateToTable() {
	QTableWidget* table = m_prompt->ui->progress;
	QTableWidgetItem* urlItem = table->item (tableRow(), 0),
		*progressItem = table->item (tableRow(), 1);
	
	if (!urlItem || !progressItem) {
		urlItem = new QTableWidgetItem;
		progressItem = new QTableWidgetItem;
		
		table->setItem (tableRow(), 0, urlItem);
		table->setItem (tableRow(), 1, progressItem);
	}
	
	urlItem->setText (m_url);
	progressItem->setText ("---");
}

// =============================================================================
// -----------------------------------------------------------------------------
DEFINE_ACTION (DownloadFrom, 0) {
	g_PartDownloader.download();
}