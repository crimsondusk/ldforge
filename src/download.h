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

#ifndef LDFORGE_DOWNLOAD_H
#define LDFORGE_DOWNLOAD_H

#include <QDialog>
#include "common.h"

class Ui_DownloadFrom;
class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;

// =============================================================================
// -----------------------------------------------------------------------------
extern class PartDownloader {
public:
	constexpr static const char* k_OfficialURL = "http://ldraw.org/library/official/",
		*k_UnofficialURL = "http://ldraw.org/library/unofficial/";
	
	PartDownloader() {}
	void download();
	void operator()() { download(); }
} g_PartDownloader;

// =============================================================================
// -----------------------------------------------------------------------------
class PartDownloadPrompt : public QDialog {
	Q_OBJECT
	
public:
	enum Source {
		OfficialLibrary,
		PartsTracker,
		CustomURL,
	};
	
	explicit PartDownloadPrompt (QWidget* parent = null);
	virtual ~PartDownloadPrompt();
	str getURL();
	
public slots:
	void sourceChanged (int i);
	void startDownload();
	
protected:
	Ui_DownloadFrom* ui;
	friend class PartDownloadRequest;
};

// =============================================================================
// -----------------------------------------------------------------------------
class PartDownloadRequest : public QObject {
	PROPERTY (int, tableRow, setTableRow)
	
public:
	explicit PartDownloadRequest (str url, PartDownloadPrompt* parent);
	void updateToTable();
	
private:
	PartDownloadPrompt* m_prompt;
	str m_url;
	QNetworkAccessManager* m_nam;
	QNetworkReply* m_reply;
};

#endif // LDFORGE_DOWNLOAD_H