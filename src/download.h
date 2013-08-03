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
#include "types.h"

class LDFile;
class QFile;
class PartDownloadRequest;
class Ui_DownloadFrom;
class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;
class QAbstractButton;

// =============================================================================
// -----------------------------------------------------------------------------
extern class PartDownloader {
public:
	constexpr static const char* k_OfficialURL = "http://ldraw.org/library/official/",
		*k_UnofficialURL = "http://ldraw.org/library/unofficial/";
	
	PartDownloader() {}
	void download();
	static str getDownloadPath();
	void operator()() { download(); }
} g_PartDownloader;

// =============================================================================
// -----------------------------------------------------------------------------
class PartDownloadPrompt : public QDialog {
	Q_OBJECT
	PROPERTY (LDFile*, primaryFile, setPrimaryFile)
	PROPERTY (bool, aborted, setAborted)
	
public:
	enum Source {
		PartsTracker,
		CustomURL,
	};
	
	enum Button {
		Download,
		Abort,
		Close
	};
	
	explicit PartDownloadPrompt (QWidget* parent = null);
	virtual ~PartDownloadPrompt();
	str getURL() const;
	Source getSource() const;
	void downloadFile (str dest, str url, bool primary);
	void modifyDest (str& dest) const;
	QPushButton* getButton (Button i);
	
public slots:
	void sourceChanged (int i);
	void checkIfFinished();
	void buttonClicked (QAbstractButton* btn);
	
protected:
	Ui_DownloadFrom* ui;
	friend class PartDownloadRequest;
	
private:
	List<str> m_filesToDownload;
	List<PartDownloadRequest*> m_requests;
	QPushButton* m_downloadButton;
};

// =============================================================================
// -----------------------------------------------------------------------------
class PartDownloadRequest : public QObject {
	Q_OBJECT
	PROPERTY (int, tableRow, setTableRow)
	
public:
	enum TableColumn {
		PartLabelColumn,
		ProgressColumn,
	};
	
	enum State {
		Requesting,
		Downloading,
		Finished,
		Failed,
	};
	
	explicit PartDownloadRequest (str url, str dest, bool primary, PartDownloadPrompt* parent);
	         PartDownloadRequest (const PartDownloadRequest&) = delete;
	virtual ~PartDownloadRequest();
	void updateToTable();
	bool isFinished() const;
	const State& state() const;
	
	void operator= (const PartDownloadRequest&) = delete;
	
public slots:
	void downloadFinished();
	void readyRead();
	void downloadProgress (qint64 recv, qint64 total);
	void abort();
	
private:
	PartDownloadPrompt* m_prompt;
	str m_url, m_dest, m_fpath;
	QNetworkAccessManager* m_nam;
	QNetworkReply* m_reply;
	bool m_firstUpdate;
	State m_state;
	int64 m_bytesRead, m_bytesTotal;
	bool m_primary;
	QFile* m_fp;
};

#endif // LDFORGE_DOWNLOAD_H