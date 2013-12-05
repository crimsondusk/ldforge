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
class PartDownloader : public QDialog
{	Q_OBJECT
	PROPERTY (public,	LDFile*, PrimaryFile,	NO_OPS,		NO_CB)
	PROPERTY (public,	bool,		Aborted,			BOOL_OPS,	NO_CB)

	public:
		constexpr static const char* k_UnofficialURL = "http://ldraw.org/library/unofficial/";

		enum Source
		{	PartsTracker,
			CustomURL,
		};

		enum Button
		{	Download,
			Abort,
			Close
		};

		enum TableColumn
		{	PartLabelColumn,
			ProgressColumn,
		};

		explicit PartDownloader (QWidget* parent = null);
		virtual ~PartDownloader();
		str getURL() const;
		static str getDownloadPath();
		Source getSource() const;
		void downloadFile (str dest, str url, bool primary);
		void modifyDestination (str& dest) const;
		QPushButton* getButton (Button i);
		static void k_download();

	public slots:
		void sourceChanged (int i);
		void checkIfFinished();
		void buttonClicked (QAbstractButton* btn);

	protected:
		Ui_DownloadFrom* ui;
		friend class PartDownloadRequest;

	private:
		QStringList m_filesToDownload;
		QList<PartDownloadRequest*> m_requests;
		QPushButton* m_downloadButton;
};

// =============================================================================
// -----------------------------------------------------------------------------
class PartDownloadRequest : public QObject
{	Q_OBJECT
	PROPERTY (public,	int, TableRow,	NUM_OPS,	NO_CB)

	public:
		enum State
		{	Requesting,
			Downloading,
			Finished,
			Failed,
		};

		explicit PartDownloadRequest (str url, str dest, bool primary, PartDownloader* parent);
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
		PartDownloader* m_prompt;
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
