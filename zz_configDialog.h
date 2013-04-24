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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include "gui.h"
#include <qdialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlistwidget.h>
#include <qspinbox.h>

class ConfigDialog : public QDialog {
	Q_OBJECT
	
public:
	QTabWidget* tabs;
	QWidget* mainTab, *shortcutsTab, *quickColorTab;
	
	// =========================================================================
	// Main tab widgets
	QLabel* lb_LDrawPath;
	QLabel* lb_viewBg, *lb_viewFg, *lb_viewFgAlpha;
	QLabel* lb_lineThickness, *lb_iconSize;
	QLineEdit* le_LDrawPath;
	QPushButton* pb_findLDrawPath;
	QPushButton* pb_viewBg, *pb_viewFg;
	QCheckBox* cb_colorize, *cb_colorBFC, *cb_selFlash;
	QSlider* sl_viewFgAlpha, *sl_lineThickness, *sl_iconSize;
	
	// =========================================================================
	// Shortcuts tab
	QListWidget* lw_shortcutList;
	QPushButton* pb_setShortcut, *pb_resetShortcut, *pb_clearShortcut;
	std::vector<QListWidgetItem*> shortcutItems;
	
	// =========================================================================
	// Quick color toolbar tab
	QListWidget* lw_quickColors;
	QPushButton* pb_addColor, *pb_delColor, *pb_changeColor, *pb_addColorSeparator,
		*pb_moveColorUp, *pb_moveColorDown, *pb_clearColors;
	std::vector<QListWidgetItem*> quickColorItems;
	std::vector<quickColorMetaEntry> quickColorMeta;
	
	// =========================================================================
	// Grid tab
	QLabel* lb_gridLabels[3];
	QLabel* lb_gridIcons[3];
	QDoubleSpinBox* dsb_gridData[3][4];
	
	// =========================================================================
	QDialogButtonBox* bbx_buttons;
	
	ConfigDialog (ForgeWindow* parent);
	~ConfigDialog ();
	static void staticDialog ();
	
private:
	void initMainTab ();
	void initShortcutsTab ();
	void initQuickColorTab ();
	void initGridTab ();
	
	void makeSlider (QSlider*& qSlider, short int dMin, short int dMax, short int dDefault);
	void setButtonBackground (QPushButton* qButton, str zValue);
	void pickColor (strconfig& cfg, QPushButton* qButton);
	void updateQuickColorList (quickColorMetaEntry* pSel = null);
	void setShortcutText (QListWidgetItem* qItem, actionmeta meta);
	long getItemRow (QListWidgetItem* qItem, std::vector<QListWidgetItem*>& haystack);
	str makeColorToolBarString ();
	QListWidgetItem* getSelectedQuickColor ();
	
private slots:
	void slot_findLDrawPath ();
	void slot_setGLBackground ();
	void slot_setGLForeground ();
	
	void slot_setShortcut ();
	void slot_resetShortcut ();
	void slot_clearShortcut ();
	
	void slot_setColor ();
	void slot_delColor ();
	void slot_addColorSeparator ();
	void slot_moveColor ();
	void slot_clearColors ();
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class KeySequenceDialog : public QDialog {
	Q_OBJECT
	
public:
	explicit KeySequenceDialog (QKeySequence seq, QWidget* parent = null, Qt::WindowFlags f = 0);
	static bool staticDialog (actionmeta& meta, QWidget* parent = null);
	
	QLabel* lb_output;
	QDialogButtonBox* bbx_buttons;
	QKeySequence seq;
	
private:
	void updateOutput ();
	
private slots:
	virtual void keyPressEvent (QKeyEvent* ev);
};

#endif // CONFIGDIALOG_H