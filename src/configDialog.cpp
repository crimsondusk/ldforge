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

#include <QGridLayout>
#include <QFileDialog>
#include <QColorDialog>
#include <QBoxLayout>
#include <QKeyEvent>
#include <QGroupBox>

#include "common.h"
#include "configDialog.h"
#include "file.h"
#include "config.h"
#include "misc.h"
#include "colors.h"
#include "colorSelectDialog.h"
#include "gldraw.h"

extern_cfg (str, gl_bgcolor);
extern_cfg (str, gl_maincolor);
extern_cfg (bool, lv_colorize);
extern_cfg (bool, gl_colorbfc);
extern_cfg (float, gl_maincolor_alpha);
extern_cfg (int, gl_linethickness);
extern_cfg (int, gui_toolbar_iconsize);
extern_cfg (str, gui_colortoolbar);
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
	initExtProgTab ();
	
	IMPLEMENT_DIALOG_BUTTONS
	
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget (tabs);
	layout->addWidget (bbx_buttons);
	setLayout (layout);
	
	setWindowTitle ("Settings");
	setWindowIcon (getIcon ("settings"));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::initMainTab () {
	mainTab = new QWidget;
	
	// =========================================================================
	// Background and foreground colors
	lb_viewBg = new QLabel ("Background color:");
	pb_viewBg = new QPushButton;
	setButtonBackground (pb_viewBg, gl_bgcolor.value);
	connect (pb_viewBg, SIGNAL (clicked ()),
		this, SLOT (slot_setGLBackground ()));
	pb_viewBg->setWhatsThis ("This is the background color for the viewport.");
	
	lb_viewFg = new QLabel ("Foreground color:");
	pb_viewFg = new QPushButton;
	setButtonBackground (pb_viewFg, gl_maincolor.value);
	connect (pb_viewFg, SIGNAL (clicked ()),
		this, SLOT (slot_setGLForeground ()));
	pb_viewFg->setWhatsThis ("This color is used for the main color.");
	
	// =========================================================================
	// Alpha and line thickness sliders
	lb_viewFgAlpha = new QLabel ("Alpha:");
	makeSlider (sl_viewFgAlpha, 1, 10, (gl_maincolor_alpha * 10.0f));
	sl_viewFgAlpha->setWhatsThis ("Opacity of main color in the viewport.");
	
	lb_lineThickness = new QLabel ("Line thickness:");
	makeSlider (sl_lineThickness, 1, 8, gl_linethickness);
	sl_lineThickness->setWhatsThis ("How thick lines should be drawn in the viewport.");
	
	// =========================================================================
	// Tool bar icon size slider
	lb_iconSize = new QLabel ("Toolbar icon size:");
	makeSlider (sl_iconSize, 1, 5, (gui_toolbar_iconsize - 12) / 4);
	
	// =========================================================================
	// List view colorizer and BFC red/green view checkboxes
	cb_colorize = new QCheckBox ("Colorize polygons in object list");
	cb_colorize->setChecked (lv_colorize);
	cb_colorize->setWhatsThis ("Makes colored objects (non-16 and 24) appear "
		"colored in the object list. A red polygon will have its description "
		"written in red text.");
	
	cb_colorBFC = new QCheckBox ("Red/green BFC view");
	cb_colorBFC->setChecked (gl_colorbfc);
	cb_colorBFC->setWhatsThis ("Polygons' front sides become green and back "
		"sides red. Not implemented yet.");
	
	cb_blackEdges = new QCheckBox ("Black edges");
	cb_blackEdges->setWhatsThis ("Makes all edgelines appear black. If this is "
		"not set, edge lines take their color as defined in LDConfig.ldr");
	cb_blackEdges->setChecked (gl_blackedges);
	
	cb_schemanticInline = new QCheckBox ("Schemantic insertion only");
	cb_schemanticInline->setChecked (edit_schemanticinline);
	cb_colorBFC->setWhatsThis ("When inserting objects through inlining, file "
		"inserting or through external programs, all non-schemantics (those without "
		"actual meaning in the part file like comments and such) are filtered out.");
	
	cb_schemanticInline->setEnabled (false);
	
	QGridLayout* layout = new QGridLayout;
	layout->addWidget (lb_viewBg, 0, 0);
	layout->addWidget (pb_viewBg, 0, 1);
	layout->addWidget (lb_viewFg, 0, 2);
	layout->addWidget (pb_viewFg, 0, 3);
	
	layout->addWidget (lb_lineThickness, 1, 0);
	layout->addWidget (sl_lineThickness, 1, 1);
	layout->addWidget (lb_viewFgAlpha, 1, 2);
	layout->addWidget (sl_viewFgAlpha, 1, 3);
	
	layout->addWidget (lb_iconSize, 2, 0);
	layout->addWidget (sl_iconSize, 2, 1);
	
	layout->addWidget (cb_colorize, 3, 0, 1, 4);
	layout->addWidget (cb_colorBFC, 4, 0, 1, 4);
	layout->addWidget (cb_blackEdges, 5, 0, 1, 4);
	layout->addWidget (cb_schemanticInline, 6, 0, 1, 4);
	mainTab->setLayout (layout);
	
	// Add the tab to the manager
	tabs->addTab (mainTab, "Main settings");
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::initShortcutsTab () {
	shortcutsTab = new QWidget;
	lw_shortcutList = new QListWidget;
	lw_shortcutList->setAlternatingRowColors (true);
	
	shortcutsTab->setWhatsThis ("Here you can alter keyboard shortcuts for "
		"almost all LDForge actions. Only exceptions are the controls for the "
		"viewport. Use the set button to set a key shortcut, clear to remove it "
		"and reset to restore the shortcut to its default value.\n"
		"\tShortcut changes apply immediately after closing this dialog." );
	
	// Init table items
	ulong i = 0;
	for (actionmeta& info : g_ActionMeta) {
		QAction* const act = *info.qAct;
		
		ShortcutListItem* item = new ShortcutListItem;
		setShortcutText (item, info);
		item->setIcon (act->icon ());
		item->setActionInfo (&info);
		
		// If the action doesn't have a valid icon, use an empty one
		// so that the list is kept aligned.
		if (act->icon ().isNull ())
			item->setIcon (getIcon ("empty"));
		
		lw_shortcutList->insertItem (i++, item);
	}
	
	lw_shortcutList->setSortingEnabled (true);
	lw_shortcutList->sortItems ();
	
	pb_setShortcut = new QPushButton ("Set");
	pb_resetShortcut = new QPushButton ("Reset");
	pb_clearShortcut = new QPushButton ("Clear");
	
	connect (pb_setShortcut, SIGNAL (clicked ()), this, SLOT (slot_setShortcut ()));
	connect (pb_resetShortcut, SIGNAL (clicked ()), this, SLOT (slot_resetShortcut ()));
	connect (pb_clearShortcut, SIGNAL (clicked ()), this, SLOT (slot_clearShortcut ()));
	
	QVBoxLayout* buttonLayout = new QVBoxLayout;
	buttonLayout->addWidget (pb_setShortcut);
	buttonLayout->addWidget (pb_resetShortcut);
	buttonLayout->addWidget (pb_clearShortcut);
	buttonLayout->addStretch (10);
	
	QGridLayout* layout = new QGridLayout;
	layout->addWidget (lw_shortcutList, 0, 0);
	layout->addLayout (buttonLayout, 0, 1);
	shortcutsTab->setLayout (layout);
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
	
	QVBoxLayout* buttonLayout = new QVBoxLayout;
	buttonLayout->addWidget (pb_addColor);
	buttonLayout->addWidget (pb_delColor);
	buttonLayout->addWidget (pb_changeColor);
	buttonLayout->addWidget (pb_addColorSeparator);
	buttonLayout->addWidget (pb_moveColorUp);
	buttonLayout->addWidget (pb_moveColorDown);
	buttonLayout->addWidget (pb_clearColors);
	buttonLayout->addStretch (1);
	
	connect (pb_addColor, SIGNAL (clicked ()), this, SLOT (slot_setColor ()));
	connect (pb_delColor, SIGNAL (clicked ()), this, SLOT (slot_delColor ()));
	connect (pb_changeColor, SIGNAL (clicked ()), this, SLOT (slot_setColor ()));
	connect (pb_addColorSeparator, SIGNAL (clicked ()), this, SLOT (slot_addColorSeparator ()));
	connect (pb_moveColorUp, SIGNAL (clicked ()), this, SLOT (slot_moveColor ()));
	connect (pb_moveColorDown, SIGNAL (clicked ()), this, SLOT (slot_moveColor ()));
	connect (pb_clearColors, SIGNAL (clicked ()), this, SLOT (slot_clearColors ()));
	
	QGridLayout* layout = new QGridLayout;
	layout->addWidget (lw_quickColors, 0, 0);
	layout->addLayout (buttonLayout, 0, 1);
	
	quickColorTab->setLayout (layout);
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
		lb_gridIcons[i]->setPixmap (getIcon (fmt ("grid-%s", str (g_GridInfo[i].name).lower ().chars ())));
		
		// Text label
		lb_gridLabels[i] = new QLabel (fmt ("%s:", g_GridInfo[i].name));
		
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
extern_cfg (str, prog_ytruder);
extern_cfg (str, prog_rectifier);
extern_cfg (str, prog_intersector);
extern_cfg (str, prog_coverer);
extern_cfg (str, prog_isecalc);
static const struct extProgInfo {
	const char* const name, *iconname;
	strconfig* const path;
	mutable QLineEdit* input;
	mutable QPushButton* setPathButton;
} g_extProgInfo[] = {
	{ "Ytruder", "ytruder", &prog_ytruder, null, null },
	{ "Rectifier", "rectifier", &prog_rectifier, null, null },
	{ "Intersector", "intersector", &prog_intersector, null, null },
	{ "Isecalc", "isecalc", &prog_isecalc, null, null },
	{ "Coverer", "coverer", &prog_coverer, null, null },
};

void ConfigDialog::initExtProgTab () {
	QWidget* tab = new QWidget;
	QGridLayout* pathsLayout = new QGridLayout;
	QGroupBox* pathsBox = new QGroupBox ("Paths", this);
	QVBoxLayout* layout = new QVBoxLayout (this);
	
	ulong row = 0;
	for (const extProgInfo& info : g_extProgInfo) {
		QLabel* icon = new QLabel,
			*progLabel = new QLabel (info.name);
		QLineEdit* input = new QLineEdit;
		QPushButton* setPathButton = new QPushButton ();
		
		icon->setPixmap (getIcon (info.iconname));
		input->setText (info.path->value);
		setPathButton->setIcon (getIcon ("folder"));
		info.input = input;
		info.setPathButton = setPathButton;
		
		connect (setPathButton, SIGNAL (clicked ()), this, SLOT (slot_setExtProgPath ()));
		
		pathsLayout->addWidget (icon, row, 0);
		pathsLayout->addWidget (progLabel, row, 1);
		pathsLayout->addWidget (input, row, 2);
		pathsLayout->addWidget (setPathButton, row, 3);
		++row;
	}
	
	pathsBox->setLayout (pathsLayout);
	layout->addWidget (pathsBox);
	layout->addSpacing (10);
	
	tab->setLayout (layout);
	tabs->addTab (tab, "Ext. Programs");
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::updateQuickColorList (quickColorMetaEntry* sel) {
	for (QListWidgetItem* item : quickColorItems)
		delete item;
	
	quickColorItems.clear ();
	
	// Init table items
	for (quickColorMetaEntry& entry : quickColorMeta) {
		QListWidgetItem* item = new QListWidgetItem;
		
		if (entry.bSeparator) {
			item->setText ("--------");
			item->setIcon (getIcon ("empty"));
		} else {
			color* col = entry.col;
			
			if (col == null) {
				item->setText ("[[unknown color]]");
				item->setIcon (getIcon ("error"));
			} else {
				item->setText (col->name);
				item->setIcon (makeColorIcon (col, 16));
			}
		}
		
		lw_quickColors->addItem (item);
		quickColorItems.push_back (item);
		
		if (sel && &entry == sel) {
			lw_quickColors->setCurrentItem (item);
			lw_quickColors->scrollToItem (item);
		}
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::slot_setColor () {
	quickColorMetaEntry* entry = null;
	QListWidgetItem* item = null;
	const bool isNew = static_cast<QPushButton*> (sender ()) == pb_addColor;
	
	if (isNew == false) {
		item = getSelectedQuickColor ();
		if (!item)
			return;
		
		ulong ulIdx = getItemRow (item, quickColorItems);
		entry = &quickColorMeta[ulIdx];
		
		if (entry->bSeparator == true)
			return; // don't color separators
	}
	
	short dDefault = entry ? entry->col->index : -1;
	short dValue;
	
	if (ColorSelectDialog::staticDialog (dValue, dDefault, this) == false)
		return;
	
	if (entry)
		entry->col = getColor (dValue);
	else {
		quickColorMetaEntry entry = {getColor (dValue), null, false};
		
		item = getSelectedQuickColor ();
		ulong idx;
		
		if (item)
			idx = getItemRow (item, quickColorItems) + 1;
		else
			idx = quickColorItems.size();
		
		quickColorMeta.insert (quickColorMeta.begin() + idx, entry);
		entry = quickColorMeta[idx];
	}
	
	updateQuickColorList (entry);
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
void ConfigDialog::makeSlider (QSlider*& slider, short min, short max, short defval) {
	slider = new QSlider (Qt::Horizontal);
	slider->setRange (min, max);
	slider->setSliderPosition (defval);
	slider->setTickPosition (QSlider::TicksAbove);
	slider->setTickInterval (1);
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
		fmt ("background-color: %s", zValue.chars()).chars()
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
QList<ShortcutListItem*> ConfigDialog::getShortcutSelection () {
	QList<ShortcutListItem*> out;
	
	for (QListWidgetItem* entry : lw_shortcutList->selectedItems ())
		out << static_cast<ShortcutListItem*> (entry);
	
	return out;
}

// =============================================================================
void ConfigDialog::slot_setShortcut () {
	QList<ShortcutListItem*> sel = getShortcutSelection ();
	
	if (sel.size() < 1)
		return;
	
	ShortcutListItem* item = sel[0];
	if (KeySequenceDialog::staticDialog (*(item->getActionInfo ()), this))
		setShortcutText (item, *(item->getActionInfo ()));
}

// =============================================================================
void ConfigDialog::slot_resetShortcut () {
	QList<ShortcutListItem*> sel = getShortcutSelection ();
	
	for (ShortcutListItem* item : sel) {
		actionmeta* info = item->getActionInfo ();
		keyseqconfig* conf = info->conf;
		
		conf->reset ();
		(*info->qAct)->setShortcut (*conf);
		
		setShortcutText (item, *info);
	}
}

// =============================================================================
void ConfigDialog::slot_clearShortcut () {
	QList<ShortcutListItem*> sel = getShortcutSelection ();
	QKeySequence dummy;
	
	for (ShortcutListItem* item : sel) {
		actionmeta* info = item->getActionInfo ();
		keyseqconfig* conf = info->conf;
		conf->value = dummy;
		
		(*info->qAct)->setShortcut (*conf);
		setShortcutText (item, *info);
	}
}

// =============================================================================
void ConfigDialog::slot_setExtProgPath () {
	const extProgInfo* info = null;
	for (const extProgInfo& it : g_extProgInfo) {
		if (it.setPathButton == sender ()) {
			info = &it;
			break;
		}
	}
	
	assert (info != null);
	
	str filter;
#ifdef _WIN32
	filter = "Applications (*.exe)(*.exe);;All files (*.*)(*.*)";
#endif // WIN32
	
	str fpath = QFileDialog::getOpenFileName (this, fmt ("Path to %s", info->name), "", filter);
	if (!~fpath)
		return;
	
	info->input->setText (fpath);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::setShortcutText (QListWidgetItem* qItem, actionmeta meta) {
	QAction* const act = *meta.qAct;
	str zLabel = act->iconText ();
	str zKeybind = act->shortcut ().toString ();
	
	qItem->setText (fmt ("%s (%s)", zLabel.chars () ,zKeybind.chars ()).chars());
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str ConfigDialog::makeColorToolBarString () {
	str val;
	
	for (quickColorMetaEntry entry : quickColorMeta) {
		if (~val > 0)
			val += ':';
		
		if (entry.bSeparator)
			val += '|';
		else
			val += fmt ("%d", entry.col->index);
	}
	
	return val;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void ConfigDialog::staticDialog () {
	ConfigDialog dlg (g_win);
	
	if (dlg.exec ()) {
		lv_colorize = dlg.cb_colorize->isChecked ();
		gl_colorbfc = dlg.cb_colorBFC->isChecked ();
		edit_schemanticinline = dlg.cb_schemanticInline->isChecked ();
		gl_blackedges = dlg.cb_blackEdges->isChecked ();
		
		gl_maincolor_alpha = ((double)dlg.sl_viewFgAlpha->value ()) / 10.0f;
		gl_linethickness = dlg.sl_lineThickness->value ();
		gui_toolbar_iconsize = (dlg.sl_iconSize->value () * 4) + 12;
		
		// Manage the quick color toolbar
		g_win->setQuickColorMeta (dlg.quickColorMeta);
		gui_colortoolbar = dlg.makeColorToolBarString ();
		
		// Set the grid settings
		for (int i = 0; i < g_NumGrids; ++i)
			for (int j = 0; j < 4; ++j)
				g_GridInfo[i].confs[j]->value = dlg.dsb_gridData[i][j]->value ();
		
		// Ext program settings
		for (const extProgInfo& info : g_extProgInfo)
			*info.path = info.input->text ();
		
		// Save the config
		config::save ();
		
		// Reload all subfiles as the ldraw path potentially changed.
		reloadAllSubfiles ();
		
		g_win->R ()->setBackground ();
		g_win->fullRefresh ();
		g_win->updateToolBars ();
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
	str shortcut = seq.toString ();
	
	if (seq == QKeySequence ())
		shortcut = "&lt;empty&gt;";
	
	str text = fmt ("<center><b>%s</b></center>", shortcut.chars ());
	
	lb_output->setText (text);
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