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

class QCheckBox;
class QProgressBar;
class QGroupBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QPushButton;
class QLineEdit;
class QSpinBox;
class RadioBox;
class CheckBoxGroup;
class QLabel;
class QAbstractButton;

class OverlayDialog : public QDialog {
	Q_OBJECT
	
public:
	explicit OverlayDialog (QWidget* parent = null, Qt::WindowFlags f = 0);
	
	str			fpath		() const;
	ushort		ofsx		() const;
	ushort		ofsy		() const;
	double		lwidth		() const;
	double		lheight		() const;
	int			 camera		() const;
	
private:
	RadioBox* rb_camera;
	QPushButton* btn_fpath;
	QLineEdit* le_fpath;
	QSpinBox* sb_ofsx, *sb_ofsy;
	QDoubleSpinBox* dsb_lwidth, *dsb_lheight;
	QDialogButtonBox* dbb_buttons;
	
private slots:
	void slot_fpath ();
	void slot_help ();
	void slot_dimensionsChanged ();
	void fillDefaults (int newcam);
};

// =============================================================================
// SetContentsDialog
//
// Performs the Set Contents dialog on the given LDObject. Object's contents
// are exposed to the user and is reinterpreted if the user accepts the new
// contents.
// =============================================================================
class SetContentsDialog : public QDialog {
public:
	explicit SetContentsDialog (QWidget* parent = null, Qt::WindowFlags f = 0);
	str text () const;
	void setObject (LDObject* obj);
	
private:
	QLabel* lb_contents, *lb_errorIcon, *lb_error;
	QLineEdit* le_contents;
};

// =============================================================================
class LDrawPathDialog : public QDialog {
	Q_OBJECT
	
public:
	explicit LDrawPathDialog (const bool validDefault, QWidget* parent = null, Qt::WindowFlags f = 0);
	str filename () const;
	void setPath (str path);
	
private:
	Q_DISABLE_COPY (LDrawPathDialog)
	
	QLabel* lb_resolution;
	QLineEdit* le_path;
	QPushButton* btn_findPath, *btn_cancel;
	QDialogButtonBox* dbb_buttons;
	const bool m_validDefault;
	
	QPushButton* okButton ();
	
private slots:
	void slot_findPath ();
	void slot_tryConfigure ();
	void slot_exit ();
};

// =============================================================================
class RotationPointDialog : public QDialog {
	Q_OBJECT
	
public:
	explicit RotationPointDialog (QWidget* parent = null, Qt::WindowFlags f = 0);
	
	vertex customPos () const;
	bool custom () const;
	void setCustom (bool custom);
	void setCustomPos (const vertex& pos);
	
private:
	QDoubleSpinBox* dsb_customX, *dsb_customY, *dsb_customZ;
	RadioBox* rb_rotpoint;
	QGroupBox* gb_customPos;
	
private slots:
	void radioBoxChanged ();
};

// =============================================================================
class OpenProgressDialog : public QDialog {
	Q_OBJECT
	READ_PROPERTY (ulong, progress, setProgress)
	DECLARE_PROPERTY (ulong, numLines, setNumLines)
	
public:
	explicit OpenProgressDialog (QWidget* parent = null, Qt::WindowFlags f = 0);
	
public slots:
	void updateProgress (int progress);
	
private:
	QProgressBar* progressBar;
	QLabel* progressText;
	
	void updateValues ();
};

#endif // DIALOGS_H