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

#ifndef DIALOGS_H
#define DIALOGS_H

#include <QDialog>
#include "common.h"
#include "types.h"

class QRadioButton;
class QCheckBox;
class QProgressBar;
class QGroupBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QPushButton;
class QLineEdit;
class QSpinBox;
class RadioBox;
class QLabel;
class QAbstractButton;
class Ui_OverlayUI;
class Ui_LDPathUI;
class Ui_OpenProgressUI;

class OverlayDialog : public QDialog
{
	Q_OBJECT

public:
	explicit OverlayDialog( QWidget* parent = null, Qt::WindowFlags f = 0 );
	virtual ~OverlayDialog();
	
	str         fpath() const;
	ushort      ofsx() const;
	ushort      ofsy() const;
	double      lwidth() const;
	double      lheight() const;
	int         camera() const;

private:
	Ui_OverlayUI* ui;
	vector<pair<QRadioButton*, int>> m_cameraArgs;

private slots:
	void slot_fpath();
	void slot_help();
	void slot_dimensionsChanged();
	void fillDefaults( int newcam );
};

// =============================================================================
class LDrawPathDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit LDrawPathDialog( const bool validDefault, QWidget* parent = null, Qt::WindowFlags f = 0 );
	virtual ~LDrawPathDialog();
	str filename() const;
	void setPath( str path );
	
private:
	Q_DISABLE_COPY( LDrawPathDialog )
	const bool m_validDefault;
	Ui_LDPathUI* ui;
	QPushButton* okButton();
	QPushButton* cancelButton();
	
private slots:
	void slot_findPath();
	void slot_tryConfigure();
	void slot_exit();
};

// =============================================================================
class OpenProgressDialog : public QDialog
{
	Q_OBJECT
	READ_PROPERTY( ulong, progress, setProgress )
	DECLARE_PROPERTY( ulong, numLines, setNumLines )
	
public:
	explicit OpenProgressDialog( QWidget* parent = null, Qt::WindowFlags f = 0 );
	virtual ~OpenProgressDialog();
	
public slots:
	void updateProgress (int progress);
	
private:
	Ui_OpenProgressUI* ui;
	
	void updateValues ();
};

#endif // DIALOGS_H