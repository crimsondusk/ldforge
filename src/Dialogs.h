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

#pragma once
#include <QDialog>
#include "Main.h"
#include "Types.h"

class Ui_ExtProgPath;
class QRadioButton;
class QCheckBox;
class QProgressBar;
class QGroupBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QPushButton;
class QLineEdit;
class QSpinBox;
class RadioGroup;
class QLabel;
class QAbstractButton;
class Ui_OverlayUI;
class Ui_LDPathUI;
class Ui_OpenProgressUI;

class OverlayDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit OverlayDialog (QWidget* parent = null, Qt::WindowFlags f = 0);
		virtual ~OverlayDialog();

		QString         fpath() const;
		int         ofsx() const;
		int         ofsy() const;
		double      lwidth() const;
		double      lheight() const;
		int         camera() const;

	private:
		Ui_OverlayUI* ui;
		QList<Pair<QRadioButton*, int>> m_cameraArgs;

	private slots:
		void slot_fpath();
		void slot_help();
		void slot_dimensionsChanged();
		void fillDefaults (int newcam);
};

// =============================================================================
class LDrawPathDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit LDrawPathDialog (const bool validDefault, QWidget* parent = null, Qt::WindowFlags f = 0);
		virtual ~LDrawPathDialog();
		QString filename() const;
		void setPath (QString path);

	private:
		Q_DISABLE_COPY (LDrawPathDialog)
		const bool m_validDefault;
		Ui_LDPathUI* ui;
		QPushButton* okButton();
		QPushButton* cancelButton();

	private slots:
		void slot_findPath();
		void slot_tryConfigure();
		void slot_exit();
		void slot_accept();
};

// =============================================================================
class OpenProgressDialog : public QDialog
{
	Q_OBJECT
	PROPERTY (public,	int, progress,	setProgress,	STOCK_WRITE)
	PROPERTY (public,	int, numLines,	setNumLines,	CUSTOM_WRITE)

	public:
		explicit OpenProgressDialog (QWidget* parent = null, Qt::WindowFlags f = 0);
		virtual ~OpenProgressDialog();

	public slots:
		void updateProgress (int progress);

	private:
		Ui_OpenProgressUI* ui;

		void updateValues();
};

// =============================================================================
class ExtProgPathPrompt : public QDialog
{
	Q_OBJECT

	public:
		explicit ExtProgPathPrompt (QString progName, QWidget* parent = 0, Qt::WindowFlags f = 0);
		virtual ~ExtProgPathPrompt();
		QString getPath() const;

	public slots:
		void findPath();

	private:
		Ui_ExtProgPath* ui;
};

// =============================================================================
class AboutDialog : public QDialog
{
	Q_OBJECT

	public:
		AboutDialog (QWidget* parent = null, Qt::WindowFlags f = 0);

	private slots:
		void slot_mail();
};

void bombBox (const QString& message);
