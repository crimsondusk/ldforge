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

#include "common.h"
#include "zz_configDialog.h"
#include "file.h"
#include "config.h"
#include "misc.h"
#include "colors.h"
#include "zz_colorSelectDialog.h"
#include <qgridlayout.h>
#include <qfiledialog.h>
#include <qcolordialog.h>
#include <qboxlayout.h>
#include <qevent.h>

extern_cfg (str, io_ldpath);
extern_cfg (str, gl_bgcolor);
extern_cfg (str, gl_maincolor);
extern_cfg (bool, lv_colorize);
extern_cfg (bool, gl_colorbfc);
extern_cfg (float, gl_maincolor_alpha);
extern_cfg (int, gl_linethickness);
extern_cfg (int, gui_toolbar_iconsize);
extern_cfg (str, gui_colortoolbar);
extern_cfg (bool, gl_selflash);
extern_cfg (bool, edit_schemanticinline);
extern_cfg (bool, gl_blackedges);

ConfigDialog* g_ConfigDialog = null;

#define INIT_CHECKBOX(BOX, CFG) \
	BOX->setCheckState (CFG ? Qt::Checked : Qt::Unchecked);

#define APPLY_CHECKBOX(BTN, CFG) \
	CFG = BTN->checkState() == Qt::Checked;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ConfigDialog::ConfigDialog (ForgeWindow* parent) : QDialog (parent) {
	g_ConfigDialog = this;
	tabs = new QTabWidget;
	
	initMainTab ();
	initShortcutsTab ();
	initQuickColorTab ();
	initGridTab ();
	
	IMPLEMENT_DIALOG_BUTTONS
	
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget (tabs);
	layout->addWidget (bbx_buttons);
	setLayout (layout);
	
	setWindowTitle (APPNAME_DISPLAY " - Settings");
	setWindowIcon (getIcon ("settings"));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::initMainTab () {
	mainTab = new QWidget;
	
	// =========================================================================
	// LDraw path
	lb_LDrawPath = new QLabel ("LDraw path:");
	
	le_LDrawPath = new QLineEdit;
	le_LDrawPath->setText (io_ldpath.value.chars());
	
	pb_findLDrawPath = new QPushButton;
	pb_findLDrawPath->setIcon (getIcon ("folder"));
	connect (pb_findLDrawPath, SIGNAL (clicked ()),
		this, SLOT (slot_findLDrawPath ()));
	
	QHBoxLayout* qLDrawPathLayout = new QHBoxLayout;
	qLDrawPathLayout->addWidget (le_LDrawPath);
	qLDrawPathLayout->addWidget (pb_findLDrawPath);
	
	// =========================================================================
	// Background and foreground colors
	lb_viewBg = new QLabel ("Background color:");
	pb_viewBg = new QPushButton;
	setButtonBackground (pb_viewBg, gl_bgcolor.value);
	connect (pb_viewBg, SIGNAL (clicked ()),
		this, SLOT (slot_setGLBackground ()));
	
	lb_viewFg = new QLabel ("Foreground color:");
	pb_viewFg = new QPushButton;
	setButtonBackground (pb_viewFg, gl_maincolor.value);
	connect (pb_viewFg, SIGNAL (clicked ()),
		this, SLOT (slot_setGLForeground ()));
	
	// =========================================================================
	// Alpha and line thickness sliders
	lb_viewFgAlpha = new QLabel ("Alpha:");
	makeSlider (sl_viewFgAlpha, 1, 10, (gl_maincolor_alpha * 10.0f));
	
	lb_lineThickness = new QLabel ("Line thickness:");
	makeSlider (sl_lineThickness, 1, 8, gl_linethickness);
	
	// =========================================================================
	// Tool bar icon size slider
	lb_iconSize = new QLabel ("Toolbar icon size:");
	makeSlider (sl_iconSize, 1, 5, (gui_toolbar_iconsize - 12) / 4);
	
	// =========================================================================
	// List view colorizer and BFC red/green view checkboxes
	cb_colorize = new QCheckBox ("Colorize polygons in list view");
	INIT_CHECKBOX (cb_colorize, lv_colorize)
	
	cb_colorBFC = new QCheckBox ("Red/green BFC view");
	INIT_CHECKBOX (cb_colorBFC, gl_colorbfc)
	
	cb_selFlash = new QCheckBox ("Selection flash");
	INIT_CHECKBOX (cb_selFlash, gl_selflash)
	
	cb_blackEdges = new QCheckBox ("Black edges");
	cb_blackEdges->setWhatsThis ("If this is set, all edgelines appear black. If this is "
		"not set, edge lines take their color as defined in LDConfig.ldr");
	INIT_CHECKBOX (cb_blackEdges, gl_blackedges)
	
	cb_schemanticInline = new QCheckBox ("Only insert schemantic objects from a file");
	INIT_CHECKBOX (cb_schemanticInline, edit_schemanticinline)
	
	QGridLayout* layout = new QGridLayout;
	layout->addWidget (lb_LDrawPath, 0, 0);
	layout->addLayout (qLDrawPathLayout, 0, 1, 1, 3);
	
	layout->addWidget (lb_viewBg, 1, 0);
	layout->addWidget (pb_viewBg, 1, 1);
	layout->addWidget (lb_viewFg, 1, 2);
	layout->addWidget (pb_viewFg, 1, 3);
	
	layout->addWidget (lb_lineThickness, 2, 0);
	layout->addWidget (sl_lineThickness, 2, 1);
	layout->addWidget (lb_viewFgAlpha, 2, 2);
	layout->addWidget (sl_viewFgAlpha, 2, 3);
	
	layout->addWidget (lb_iconSize, 3, 0);
	layout->addWidget (sl_iconSize, 3, 1);
	
	layout->addWidget (cb_colorize, 4, 0, 1, 4);
	layout->addWidget (cb_colorBFC, 5, 0, 1, 4);
	layout->addWidget (cb_selFlash, 6, 0, 1, 4);
	layout->addWidget (cb_blackEdges, 7, 0, 1, 4);
	layout->addWidget (cb_schemanticInline, 8, 0, 1, 4);
	mainTab->setLayout (layout);
	
	// Add the tab to the manager
	tabs->addTab (mainTab, "Main settings");
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::initShortcutsTab () {
	QGridLayout* qLayout;
	
	shortcutsTab = new QWidget;
	lw_shortcutList = new QListWidget;
	qLayout = new QGridLayout;
	
	shortcutsTab->setWhatsThis ("Here you can alter keyboard shortcuts for "
		"almost all LDForge actions. Only exceptions are the controls for the "
		"viewport. Use the set button to set a key shortcut, clear to remove it "
		"and reset to restore the shortcut to its default value.\n"
		"\tShortcut changes apply immediately after closing this dialog." );
	
	// Init table items
	ulong i = 0;
	for (actionmeta meta : g_ActionMeta) {
		QAction* const qAct = *meta.qAct;
		
		QListWidgetItem* qItem = new QListWidgetItem;
		setShortcutText (qItem, meta);
		qItem->setIcon (qAct->icon ());
		
		shortcutItems.push_back (qItem);
		lw_shortcutList->insertItem (i, qItem);
		++i;
	}
	
	pb_setShortcut = new QPushButton ("Set");
	pb_resetShortcut = new QPushButton ("Reset");
	pb_clearShortcut = new QPushButton ("Clear");
	
	connect (pb_setShortcut, SIGNAL (clicked ()), this, SLOT (slot_setShortcut ()));
	connect (pb_resetShortcut, SIGNAL (clicked ()), this, SLOT (slot_resetShortcut ()));
	connect (pb_clearShortcut, SIGNAL (clicked ()), this, SLOT (slot_clearShortcut ()));
	
	QVBoxLayout* qButtonLayout = new QVBoxLayout;
	qButtonLayout->addWidget (pb_setShortcut);
	qButtonLayout->addWidget (pb_resetShortcut);
	qButtonLayout->addWidget (pb_clearShortcut);
	qButtonLayout->addStretch (10);
	
	qLayout->addWidget (lw_shortcutList, 0, 0);
	qLayout->addLayout (qButtonLayout, 0, 1);
	shortcutsTab->setLayout (qLayout);
	tabs->addTab (shortcutsTab, "Shortcuts");
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::initQuickColorTab () {
	quickColorTab = new QWidget;
	
	pb_addColor = new QPushButton (getIcon ("palette"), "Add");
	pb_delColor = new QPushButton (getIcon ("delete"), "Remove");
	pb_changeColor = new QPushButton (getIcon ("palette"), "Set");
	pb_addColorSeparator = new QPushButton ("Add Separator");
	pb_moveColorUp = new QPushButton (getIcon ("arrow-up"), "Move Up");
	pb_moveColorDown = new QPushButton (getIcon ("arrow-down"), "Move Down");
	pb_clearColors = new QPushButton (getIcon ("delete-all"), "Clear");
	
	lw_quickColors = new QListWidget;
	
	quickColorMeta = parseQuickColorMeta ();
	updateQuickColorList ();
	
	QVBoxLayout* qButtonLayout = new QVBoxLayout;
	qButtonLayout->addWidget (pb_addColor);
	qButtonLayout->addWidget (pb_delColor);
	qButtonLayout->addWidget (pb_changeColor);
	qButtonLayout->addWidget (pb_addColorSeparator);
	qButtonLayout->addWidget (pb_moveColorUp);
	qButtonLayout->addWidget (pb_moveColorDown);
	qButtonLayout->addWidget (pb_clearColors);
	qButtonLayout->addStretch (1);
	
	connect (pb_addColor, SIGNAL (clicked ()), this, SLOT (slot_setColor ()));
	connect (pb_delColor, SIGNAL (clicked ()), this, SLOT (slot_delColor ()));
	connect (pb_changeColor, SIGNAL (clicked ()), this, SLOT (slot_setColor ()));
	connect (pb_addColorSeparator, SIGNAL (clicked ()), this, SLOT (slot_addColorSeparator ()));
	connect (pb_moveColorUp, SIGNAL (clicked ()), this, SLOT (slot_moveColor ()));
	connect (pb_moveColorDown, SIGNAL (clicked ()), this, SLOT (slot_moveColor ()));
	connect (pb_clearColors, SIGNAL (clicked ()), this, SLOT (slot_clearColors ()));
	
	QGridLayout* qLayout = new QGridLayout;
	qLayout->addWidget (lw_quickColors, 0, 0);
	qLayout->addLayout (qButtonLayout, 0, 1);
	
	quickColorTab->setLayout (qLayout);
	tabs->addTab (quickColorTab, "Quick Colors");
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::initGridTab () {
	QWidget* tab = new QWidget;
	QGridLayout* layout = new QGridLayout;
	QVBoxLayout* l2 = new QVBoxLayout;
	
	QLabel* xlabel = new QLabel ("X"),
		*ylabel = new QLabel ("Y"),
		*zlabel = new QLabel ("Z"),
		*anglabel = new QLabel ("Angle");
	
	short i = 1;
	for (QLabel* label : std::initializer_list<QLabel*> ({xlabel, ylabel, zlabel, anglabel})) {
		label->setAlignment (Qt::AlignCenter);
		layout->addWidget (label, 0, i++);
	}
	
	for (int i = 0; i < g_NumGrids; ++i) {
		// Icon
		lb_gridIcons[i] = new QLabel;
		lb_gridIcons[i]->setPixmap (getIcon (format ("grid-%s", str (g_GridInfo[i].name).tolower ().chars ())));
		
		// Text label
		lb_gridLabels[i] = new QLabel (format ("%s:", g_GridInfo[i].name));
		
		QHBoxLayout* labellayout = new QHBoxLayout;
		labellayout->addWidget (lb_gridIcons[i]);
		labellayout->addWidget (lb_gridLabels[i]);
		layout->addLayout (labellayout, i + 1, 0);
		
		// Add the widgets
		for (int j = 0; j < 4; ++j) {
			dsb_gridData[i][j] = new QDoubleSpinBox;
			dsb_gridData[i][j]->setValue (g_GridInfo[i].confs[j]->value);
			layout->addWidget (dsb_gridData[i][j], i + 1, j + 1);
		}
	}
	
	l2->addLayout (layout);
	l2->addStretch (1);
	
	tab->setLayout (l2);
	tabs->addTab (tab, "Grids");
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::updateQuickColorList (quickColorMetaEntry* pSel) {
	for (QListWidgetItem* qItem : quickColorItems)
		delete qItem;
	
	quickColorItems.clear ();
	
	// Init table items
	for (quickColorMetaEntry& entry : quickColorMeta) {
		QListWidgetItem* qItem = new QListWidgetItem;
		
		if (entry.bSeparator) {
			qItem->setText ("--------");
			qItem->setIcon (getIcon ("empty"));
		} else {
			color* col = entry.col;
			
			if (col == null) {
				qItem->setText ("[[unknown color]]");
				qItem->setIcon (getIcon ("error"));
			} else {
				qItem->setText (col->zName);
				qItem->setIcon (getIcon ("palette"));
			}
		}
		
		lw_quickColors->addItem (qItem);
		quickColorItems.push_back (qItem);
		
		if (pSel && &entry == pSel) {
			lw_quickColors->setCurrentItem (qItem);
			lw_quickColors->scrollToItem (qItem);
		}
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::slot_setColor () {
	quickColorMetaEntry* pEntry = null;
	QListWidgetItem* qItem = null;
	const bool bNew = static_cast<QPushButton*> (sender ()) == pb_addColor;
	
	if (bNew == false) {
		qItem = getSelectedQuickColor ();
		if (!qItem)
			return;
		
		ulong ulIdx = getItemRow (qItem, quickColorItems);
		pEntry = &quickColorMeta[ulIdx];
		
		if (pEntry->bSeparator == true)
			return; // don't color separators
	}
	
	short dDefault = pEntry ? pEntry->col->index () : -1;
	short dValue;
	
	if (ColorSelectDialog::staticDialog (dValue, dDefault, this) == false)
		return;
	
	if (pEntry)
		pEntry->col = getColor (dValue);
	else {
		quickColorMetaEntry entry = {getColor (dValue), null, false};
		
		qItem = getSelectedQuickColor ();
		ulong idx;
		
		if (qItem)
			idx = getItemRow (qItem, quickColorItems) + 1;
		else
			idx = quickColorItems.size();
		
		quickColorMeta.insert (quickColorMeta.begin() + idx, entry);
		pEntry = &quickColorMeta[idx];
	}
	
	updateQuickColorList (pEntry);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::slot_delColor () {
	if (lw_quickColors->selectedItems().size() == 0)
		return;
	
	QListWidgetItem* qItem = lw_quickColors->selectedItems ()[0];
	ulong ulIdx = getItemRow (qItem, quickColorItems);
	quickColorMeta.erase (quickColorMeta.begin () + ulIdx);
	updateQuickColorList ();
}

// =============================================================================
void ConfigDialog::slot_moveColor () {
	const bool bUp = (static_cast<QPushButton*> (sender()) == pb_moveColorUp);
	
	if (lw_quickColors->selectedItems().size() == 0)
		return;
	
	QListWidgetItem* qItem = lw_quickColors->selectedItems ()[0];
	ulong ulIdx = getItemRow (qItem, quickColorItems);
	
	long lDest = bUp ? (ulIdx - 1) : (ulIdx + 1);
	
	if (lDest < 0 || (ulong)lDest >= quickColorItems.size ())
		return; // destination out of bounds
	
	quickColorMetaEntry tmp = quickColorMeta[lDest];
	quickColorMeta[lDest] = quickColorMeta[ulIdx];
	quickColorMeta[ulIdx] = tmp;
	
	updateQuickColorList (&quickColorMeta[lDest]);
}

// =============================================================================
void ConfigDialog::slot_addColorSeparator() {
	quickColorMeta.push_back ({null, null, true});
	updateQuickColorList (&quickColorMeta[quickColorMeta.size () - 1]);
}

// =============================================================================
void ConfigDialog::slot_clearColors () {
	quickColorMeta.clear ();
	updateQuickColorList ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::makeSlider (QSlider*& qSlider, short int dMin, short int dMax,
	short dDefault)
{
	qSlider = new QSlider (Qt::Horizontal);
	qSlider->setRange (dMin, dMax);
	qSlider->setSliderPosition (dDefault);
	qSlider->setTickPosition (QSlider::TicksAbove);
	qSlider->setTickInterval (1);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
ConfigDialog::~ConfigDialog () {
	g_ConfigDialog = null;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::slot_findLDrawPath () {
	str zDir = QFileDialog::getExistingDirectory (this, "Choose LDraw directory",
		le_LDrawPath->text());
	
	if (~zDir)
		le_LDrawPath->setText (zDir.chars());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::pickColor (strconfig& cfg, QPushButton* qButton) {
	QColorDialog dlg (QColor (cfg.value.chars()));
	dlg.setWindowIcon (getIcon ("colorselect"));
	
	if (dlg.exec ()) {
		uchar r = dlg.currentColor ().red (),
			g = dlg.currentColor ().green (),
			b = dlg.currentColor ().blue ();
		cfg.value.format ("#%.2X%.2X%.2X", r, g, b);
		setButtonBackground (qButton, cfg.value);
	}
}

void ConfigDialog::slot_setGLBackground () {
	pickColor (gl_bgcolor, pb_viewBg);
}

void ConfigDialog::slot_setGLForeground () {
	pickColor (gl_maincolor, pb_viewFg);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::setButtonBackground (QPushButton* qButton, str zValue) {
	qButton->setIcon (getIcon ("colorselect"));
	qButton->setAutoFillBackground (true);
	qButton->setStyleSheet (
		format ("background-color: %s", zValue.chars()).chars()
	);
}

// =============================================================================
long ConfigDialog::getItemRow (QListWidgetItem* qItem, std::vector<QListWidgetItem*>& haystack) {
	long i = 0;
	
	for (QListWidgetItem* it : haystack) {
		if (it == qItem)
			return i;
		++i;
	}
	
	return -1;
}

// =============================================================================
QListWidgetItem* ConfigDialog::getSelectedQuickColor () {
	if (lw_quickColors->selectedItems().size() == 0)
		return null;
	
	return lw_quickColors->selectedItems ()[0];
}

// =============================================================================
void ConfigDialog::slot_setShortcut () {
	QList<QListWidgetItem*> qaSel = lw_shortcutList->selectedItems ();
	
	if (qaSel.size() < 1)
		return;
	
	QListWidgetItem* qItem = qaSel[0];
	
	// Find the row this object is on.
	long idx = getItemRow (qItem, shortcutItems);
	
	if (KeySequenceDialog::staticDialog (g_ActionMeta[idx], this))
		setShortcutText (qItem, g_ActionMeta[idx]);
}

// =============================================================================
void ConfigDialog::slot_resetShortcut () {
	QList<QListWidgetItem*> qaSel = lw_shortcutList->selectedItems ();
	
	for (QListWidgetItem* qItem : qaSel) {
		long idx = getItemRow (qItem, shortcutItems);
		
		actionmeta meta = g_ActionMeta[idx];
		keyseqconfig* conf = g_ActionMeta[idx].conf;
		
		conf->reset ();
		(*meta.qAct)->setShortcut (*conf);
		
		setShortcutText (qItem, meta);
	}
}

// =============================================================================
void ConfigDialog::slot_clearShortcut () {
	QList<QListWidgetItem*> qaSel = lw_shortcutList->selectedItems ();
	QKeySequence qDummySeq;
	
	for (QListWidgetItem* qItem : qaSel) {
		long idx = getItemRow (qItem, shortcutItems);
		
		actionmeta meta = g_ActionMeta[idx];
		keyseqconfig* conf = g_ActionMeta[idx].conf;
		conf->value = qDummySeq;
		
		(*meta.qAct)->setShortcut (*conf);
		setShortcutText (qItem, meta);
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::setShortcutText (QListWidgetItem* qItem, actionmeta meta) {
	QAction* const qAct = *meta.qAct;
	str zLabel = qAct->iconText ();
	str zKeybind = qAct->shortcut ().toString ();
	
	qItem->setText (format ("%s (%s)", zLabel.chars () ,zKeybind.chars ()).chars());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str ConfigDialog::makeColorToolBarString () {
	str zVal;
	
	for (quickColorMetaEntry entry : quickColorMeta) {
		if (~zVal > 0)
			zVal += ':';
		
		if (entry.bSeparator)
			zVal += '|';
		else
			zVal.appendformat ("%d", entry.col->index ());
	}
	
	return zVal;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::staticDialog () {
	ConfigDialog dlg (g_ForgeWindow);
	
	if (dlg.exec ()) {
		io_ldpath = dlg.le_LDrawPath->text();
		
		APPLY_CHECKBOX (dlg.cb_colorize, lv_colorize)
		APPLY_CHECKBOX (dlg.cb_colorBFC, gl_colorbfc)
		APPLY_CHECKBOX (dlg.cb_selFlash, gl_selflash)
		APPLY_CHECKBOX (dlg.cb_schemanticInline, edit_schemanticinline)
		APPLY_CHECKBOX (dlg.cb_blackEdges, gl_blackedges)
		
		gl_maincolor_alpha = ((double)dlg.sl_viewFgAlpha->value ()) / 10.0f;
		gl_linethickness = dlg.sl_lineThickness->value ();
		gui_toolbar_iconsize = (dlg.sl_iconSize->value () * 4) + 12;
		
		// Manage the quick color toolbar
		g_ForgeWindow->quickColorMeta = dlg.quickColorMeta;
		gui_colortoolbar = dlg.makeColorToolBarString ();
		
		// Set the grid settings
		for (int i = 0; i < g_NumGrids; ++i)
			for (int j = 0; j < 4; ++j)
				g_GridInfo[i].confs[j]->value = dlg.dsb_gridData[i][j]->value ();
		
		// Save the config
		config::save ();
		
		// Reload all subfiles
		reloadAllSubfiles ();
		
		g_ForgeWindow->R->setBackground ();
		g_ForgeWindow->refresh ();
		g_ForgeWindow->updateToolBars ();
	}
}

// =========================================================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =========================================================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =========================================================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =========================================================================================================================
KeySequenceDialog::KeySequenceDialog (QKeySequence seq, QWidget* parent,
	Qt::WindowFlags f) : QDialog (parent, f), seq (seq)
{
	lb_output = new QLabel;
	IMPLEMENT_DIALOG_BUTTONS
	
	setWhatsThis ("Into this dialog you can input a key sequence for use as a "
		"shortcut in LDForge. Use OK to confirm the new shortcut and Cancel to "
		"dismiss.");
	
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget (lb_output);
	layout->addWidget (bbx_buttons);
	setLayout (layout);
	
	updateOutput ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool KeySequenceDialog::staticDialog (actionmeta& meta, QWidget* parent) {
	KeySequenceDialog dlg (*meta.conf, parent);
	
	if (dlg.exec () == false)
		return false;
	
	*meta.conf = dlg.seq;
	(*meta.qAct)->setShortcut (*meta.conf);
	return true;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void KeySequenceDialog::updateOutput () {
	str zShortcut = seq.toString ();
	
	str zText = format ("<center><b>%s</b></center>", zShortcut.chars ());
	
	lb_output->setText (zText);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void KeySequenceDialog::keyPressEvent (QKeyEvent* ev) {
	seq = ev->key ();
	
	switch (seq) {
	case Qt::Key_Shift:
	case Qt::Key_Control:
	case Qt::Key_Alt:
	case Qt::Key_Meta:
		seq = 0;
		break;
	
	default:
		break;
	}
	
	seq = (seq | ev->modifiers ());
	
	updateOutput ();
}