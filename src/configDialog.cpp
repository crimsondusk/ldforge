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
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QCheckBox>

#include "common.h"
#include "configDialog.h"
#include "file.h"
#include "config.h"
#include "misc.h"
#include "colors.h"
#include "colorSelectDialog.h"
#include "gldraw.h"
#include "ui_config.h"
#include "build/moc_configDialog.cpp"

extern_cfg (str, gl_bgcolor);
extern_cfg (str, gl_maincolor);
extern_cfg (bool, lv_colorize);
extern_cfg (bool, gl_colorbfc);
extern_cfg (float, gl_maincolor_alpha);
extern_cfg (int, gl_linethickness);
extern_cfg (str, gui_colortoolbar);
extern_cfg (bool, edit_schemanticinline);
extern_cfg (bool, gl_blackedges);
extern_cfg (bool, gui_implicitfiles);
extern_cfg (str, net_downloadpath);
extern_cfg (bool, net_guesspaths);
extern_cfg (bool, net_autoclose);
extern_cfg (bool, gl_logostuds);

extern_cfg (str, prog_ytruder);
extern_cfg (str, prog_rectifier);
extern_cfg (str, prog_intersector);
extern_cfg (str, prog_coverer);
extern_cfg (str, prog_isecalc);
extern_cfg (str, prog_edger2);
extern_cfg (bool, prog_ytruder_wine);
extern_cfg (bool, prog_rectifier_wine);
extern_cfg (bool, prog_intersector_wine);
extern_cfg (bool, prog_coverer_wine);
extern_cfg (bool, prog_isecalc_wine);
extern_cfg (bool, prog_edger2_wine);

const char* g_extProgPathFilter =
#ifdef _WIN32
	"Applications (*.exe)(*.exe);;All files (*.*)(*.*)";
#else
	"";
#endif

#define act(N) extern_cfg (keyseq, key_##N);
#include "actions.h"

// =============================================================================
// -----------------------------------------------------------------------------
ConfigDialog::ConfigDialog (ForgeWindow* parent) : QDialog (parent) {
	ui = new Ui_ConfigUI;
	ui->setupUi (this);
	
	initMainTab();
	initShortcutsTab();
	initQuickColorTab();
	initGridTab();
	initExtProgTab();
	
	ui->downloadPath->setText (net_downloadpath);
	ui->guessNetPaths->setChecked (net_guesspaths);
	ui->autoCloseNetPrompt->setChecked (net_autoclose);
	connect (ui->findDownloadPath, SIGNAL (clicked(bool)), this, SLOT (slot_findDownloadFolder()));
}

// =============================================================================
// -----------------------------------------------------------------------------
ConfigDialog::~ConfigDialog() {
	delete ui;
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::initMainTab() {
	// Init color stuff
	setButtonBackground (ui->backgroundColorButton, gl_bgcolor);
	connect (ui->backgroundColorButton, SIGNAL (clicked()),
		this, SLOT (slot_setGLBackground()));
	
	setButtonBackground (ui->mainColorButton, gl_maincolor.value);
	connect (ui->mainColorButton, SIGNAL (clicked()),
		this, SLOT (slot_setGLForeground()));
	
	// Sliders
	ui->mainColorAlpha->setValue (gl_maincolor_alpha * 10.0f);
	ui->lineThickness->setValue (gl_linethickness);
	
	// Checkboxes
	ui->colorizeObjects->setChecked (lv_colorize);
	ui->colorBFC->setChecked (gl_colorbfc);
	ui->blackEdges->setChecked (gl_blackedges);
	// ui->scemanticInlining->setChecked (edit_schemanticinline);
	ui->implicitFiles->setChecked (gui_implicitfiles);
	ui->m_logostuds->setChecked (gl_logostuds);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::initShortcutsTab() {
	ulong i = 0;
	
#define act(N) addShortcut (key_##N, ACTION(N), i);
#include "actions.h"
	
	ui->shortcutsList->setSortingEnabled (true);
	ui->shortcutsList->sortItems();
	
	connect (ui->shortcut_set, SIGNAL (clicked()), this, SLOT (slot_setShortcut()));
	connect (ui->shortcut_reset, SIGNAL (clicked()), this, SLOT (slot_resetShortcut()));
	connect (ui->shortcut_clear, SIGNAL (clicked()), this, SLOT (slot_clearShortcut()));
}

void ConfigDialog::addShortcut (keyseqconfig& cfg, QAction* act, ulong& i) {
	ShortcutListItem* item = new ShortcutListItem;
	item->setIcon (act->icon());
	item->setKeyConfig (&cfg);
	item->setAction (act);
	setShortcutText (item);
	
	// If the action doesn't have a valid icon, use an empty one
	// so that the list is kept aligned.
	if (act->icon().isNull())
		item->setIcon (getIcon ("empty"));
	
	ui->shortcutsList->insertItem (i++, item);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::initQuickColorTab() {
	quickColors = quickColorsFromConfig();
	updateQuickColorList();
	
	connect (ui->quickColor_add, SIGNAL (clicked()), this, SLOT (slot_setColor()));
	connect (ui->quickColor_remove, SIGNAL (clicked()), this, SLOT (slot_delColor()));
	connect (ui->quickColor_edit, SIGNAL (clicked()), this, SLOT (slot_setColor()));
	connect (ui->quickColor_addSep, SIGNAL (clicked()), this, SLOT (slot_addColorSeparator()));
	connect (ui->quickColor_moveUp, SIGNAL (clicked()), this, SLOT (slot_moveColor()));
	connect (ui->quickColor_moveDown, SIGNAL (clicked()), this, SLOT (slot_moveColor()));
	connect (ui->quickColor_clear, SIGNAL (clicked()), this, SLOT (slot_clearColors()));
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::initGridTab() {
	QGridLayout* gridlayout = new QGridLayout;
	QLabel* xlabel = new QLabel ("X"),
		*ylabel = new QLabel ("Y"),
		*zlabel = new QLabel ("Z"),
		*anglabel = new QLabel ("Angle");
	short i = 1;
	
	for (QLabel* label : initlist<QLabel*> ({ xlabel, ylabel, zlabel, anglabel })) {
		label->setAlignment (Qt::AlignCenter);
		gridlayout->addWidget (label, 0, i++);
	}
	
	for (int i = 0; i < g_NumGrids; ++i) {
		// Icon
		lb_gridIcons[i] = new QLabel;
		lb_gridIcons[i]->setPixmap (getIcon (fmt ("grid-%1", str (g_GridInfo[i].name).toLower())));
		
		// Text label
		lb_gridLabels[i] = new QLabel (fmt ("%1:", g_GridInfo[i].name));
		
		QHBoxLayout* labellayout = new QHBoxLayout;
		labellayout->addWidget (lb_gridIcons[i]);
		labellayout->addWidget (lb_gridLabels[i]);
		gridlayout->addLayout (labellayout, i + 1, 0);
		
		// Add the widgets
		for (int j = 0; j < 4; ++j) {
			dsb_gridData[i][j] = new QDoubleSpinBox;
			dsb_gridData[i][j]->setValue (g_GridInfo[i].confs[j]->value);
			gridlayout->addWidget (dsb_gridData[i][j], i + 1, j + 1);
		}
	}

	ui->grids->setLayout (gridlayout);
}

// =============================================================================
// -----------------------------------------------------------------------------
static const struct extProgInfo {
	const str name, iconname;
	strconfig* const path;
	mutable QLineEdit* input;
	mutable QPushButton* setPathButton;
#ifndef _WIN32
	boolconfig* const wine;
	mutable QCheckBox* wineBox;
#endif // _WIN32
} g_extProgInfo[] = {
#ifndef _WIN32
# define EXTPROG(NAME, LOWNAME) { #NAME, #LOWNAME, &prog_##LOWNAME, null, null, &prog_##LOWNAME##_wine, null },
#else
# define EXTPROG(NAME, LOWNAME) { #NAME, #LOWNAME, &prog_##LOWNAME, null, null },
#endif
	EXTPROG (Ytruder, ytruder)
	EXTPROG (Rectifier, rectifier)
	EXTPROG (Intersector, intersector)
	EXTPROG (Isecalc, isecalc)
	EXTPROG (Coverer, coverer)
	EXTPROG (Edger2, edger2)
#undef EXTPROG
};

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::initExtProgTab() {
	QGridLayout* pathsLayout = new QGridLayout;
	ulong row = 0;
	
	for (const extProgInfo & info : g_extProgInfo) {
		QLabel* icon = new QLabel,
		*progLabel = new QLabel (info.name);
		QLineEdit* input = new QLineEdit;
		QPushButton* setPathButton = new QPushButton;
		
		icon->setPixmap (getIcon (info.iconname));
		input->setText (info.path->value);
		setPathButton->setIcon (getIcon ("folder"));
		info.input = input;
		info.setPathButton = setPathButton;
		
		connect (setPathButton, SIGNAL (clicked()), this, SLOT (slot_setExtProgPath()));
		
		pathsLayout->addWidget (icon, row, 0);
		pathsLayout->addWidget (progLabel, row, 1);
		pathsLayout->addWidget (input, row, 2);
		pathsLayout->addWidget (setPathButton, row, 3);
		
#ifndef _WIN32
		QCheckBox* wineBox = new QCheckBox ("Wine");
		wineBox->setChecked (*info.wine);
		info.wineBox = wineBox;
		pathsLayout->addWidget (wineBox, row, 4);
#endif
		
		++row;
	}

	ui->extProgs->setLayout (pathsLayout);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::updateQuickColorList (LDQuickColor* sel) {
	for (QListWidgetItem* item : quickColorItems)
		delete item;
	
	quickColorItems.clear();
	
	// Init table items
	for (LDQuickColor& entry : quickColors) {
		QListWidgetItem* item = new QListWidgetItem;
		
		if (entry.isSeparator) {
			item->setText ("--------");
			item->setIcon (getIcon ("empty"));
		} else {
			LDColor* col = entry.col;
			
			if (col == null) {
				item->setText ("[[unknown color]]");
				item->setIcon (getIcon ("error"));
			} else {
				item->setText (col->name);
				item->setIcon (makeColorIcon (col, 16));
			}
		}
		
		ui->quickColorList->addItem (item);
		quickColorItems << item;
		
		if (sel && &entry == sel) {
			ui->quickColorList->setCurrentItem (item);
			ui->quickColorList->scrollToItem (item);
		}
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::slot_setColor() {
	LDQuickColor* entry = null;
	QListWidgetItem* item = null;
	const bool isNew = static_cast<QPushButton*> (sender()) == ui->quickColor_add;
	
	if (isNew == false) {
		item = getSelectedQuickColor();
		
		if (!item)
			return;
		
		ulong i = getItemRow (item, quickColorItems);
		entry = &quickColors[i];
		
		if (entry->isSeparator == true)
			return; // don't color separators
	}
	
	short defval = entry ? entry->col->index : -1;
	short val;
	
	if (ColorSelector::getColor (val, defval, this) == false)
		return;
	
	if (entry)
		entry->col = getColor (val);
	else {
		LDQuickColor entry = {getColor (val), null, false};
		
		item = getSelectedQuickColor();
		ulong idx;
		
		if (item)
			idx = getItemRow (item, quickColorItems) + 1;
		else
			idx = quickColorItems.size();
		
		quickColors.insert (idx, entry);
		entry = quickColors[idx];
	}

	updateQuickColorList (entry);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::slot_delColor() {
	if (ui->quickColorList->selectedItems().size() == 0)
		return;
	
	QListWidgetItem* item = ui->quickColorList->selectedItems() [0];
	quickColors.erase (getItemRow (item, quickColorItems));
	updateQuickColorList();
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::slot_moveColor() {
	const bool up = (static_cast<QPushButton*> (sender()) == ui->quickColor_moveUp);
	
	if (ui->quickColorList->selectedItems().size() == 0)
		return;
	
	QListWidgetItem* item = ui->quickColorList->selectedItems() [0];
	int idx = getItemRow (item, quickColorItems);
	int dest = up ? (idx - 1) : (idx + 1);
	
	if (dest < 0 || (ulong) dest >= quickColorItems.size())
		return; // destination out of bounds
	
	LDQuickColor tmp = quickColors[dest];
	quickColors[dest] = quickColors[idx];
	quickColors[idx] = tmp;
	
	updateQuickColorList (&quickColors[dest]);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::slot_addColorSeparator() {
	quickColors << LDQuickColor ({null, null, true});
	updateQuickColorList (&quickColors[quickColors.size() - 1]);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::slot_clearColors() {
	quickColors.clear();
	updateQuickColorList();
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::pickColor (strconfig& conf, QPushButton* button) {
	QColor col = QColorDialog::getColor (QColor (conf));
	
	if (col.isValid()) {
		uchar r = col.red(),
			  g = col.green(),
			  b = col.blue();
		conf.value.sprintf ("#%.2X%.2X%.2X", r, g, b);
		setButtonBackground (button, conf.value);
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::slot_setGLBackground() {
	pickColor (gl_bgcolor, ui->backgroundColorButton);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::slot_setGLForeground() {
	pickColor (gl_maincolor, ui->mainColorButton);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::setButtonBackground (QPushButton* button, str value) {
	button->setIcon (getIcon ("colorselect"));
	button->setAutoFillBackground (true);
	button->setStyleSheet (fmt ("background-color: %1", value));
}

// =============================================================================
// -----------------------------------------------------------------------------
int ConfigDialog::getItemRow (QListWidgetItem* item, List<QListWidgetItem*>& haystack) {
	int i = 0;
	
	for (QListWidgetItem* it : haystack) {
		if (it == item)
			return i;
		
		++i;
	}
	return -1;
}

// =============================================================================
// -----------------------------------------------------------------------------
QListWidgetItem* ConfigDialog::getSelectedQuickColor() {
	if (ui->quickColorList->selectedItems().size() == 0)
		return null;
	
	return ui->quickColorList->selectedItems() [0];
}

// =============================================================================
// -----------------------------------------------------------------------------
QList<ShortcutListItem*> ConfigDialog::getShortcutSelection() {
	QList<ShortcutListItem*> out;
	
	for (QListWidgetItem* entry : ui->shortcutsList->selectedItems())
		out << static_cast<ShortcutListItem*> (entry);
	
	return out;
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::slot_setShortcut() {
	QList<ShortcutListItem*> sel = getShortcutSelection();
	
	if (sel.size() < 1)
		return;
	
	ShortcutListItem* item = sel[0];
	
	if (KeySequenceDialog::staticDialog (item->keyConfig(), this))
		setShortcutText (item);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::slot_resetShortcut() {
	QList<ShortcutListItem*> sel = getShortcutSelection();
	
	for (ShortcutListItem* item : sel) {
		item->keyConfig()->reset();
		setShortcutText (item);
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::slot_clearShortcut() {
	QList<ShortcutListItem*> sel = getShortcutSelection();
	
	for (ShortcutListItem* item : sel) {
		item->keyConfig()->value = QKeySequence();
		setShortcutText (item);
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::slot_setExtProgPath() {
	const extProgInfo* info = null;
	
	for (const extProgInfo& it : g_extProgInfo) {
		if (it.setPathButton == sender()) {
			info = &it;
			break;
		}
	}
	
	assert (info != null);
	str fpath = QFileDialog::getOpenFileName (this, fmt ("Path to %1", info->name), *info->path, g_extProgPathFilter);
	
	if (fpath.isEmpty())
		return;
	
	info->input->setText (fpath);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::slot_findDownloadFolder() {
	str dpath = QFileDialog::getExistingDirectory();
	ui->downloadPath->setText (dpath);
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::setShortcutText (ShortcutListItem* item) {
	QAction* act = item->action();
	str label = act->iconText();
	str keybind = item->keyConfig()->value.toString();
	item->setText (fmt ("%1 (%2)", label, keybind));
}

// =============================================================================
// -----------------------------------------------------------------------------
str ConfigDialog::quickColorString() {
	str val;
	
	for (LDQuickColor entry : quickColors) {
		if (val.length() > 0)
			val += ':';
		
		if (entry.isSeparator)
			val += '|';
		else
			val += fmt ("%1", entry.col->index);
	}
	
	return val;
}

// =============================================================================
// -----------------------------------------------------------------------------
const Ui_ConfigUI* ConfigDialog::getUI() const {
	return ui;
}

// =============================================================================
// -----------------------------------------------------------------------------
float ConfigDialog::getGridValue (int i, int j) const {
	return dsb_gridData[i][j]->value();
}

// =============================================================================
// -----------------------------------------------------------------------------
void ConfigDialog::staticDialog() {
	ConfigDialog dlg (g_win);
	
	if (dlg.exec()) {
		// Apply configuration
		lv_colorize = dlg.getUI()->colorizeObjects->isChecked();
		gl_colorbfc = dlg.getUI()->colorBFC->isChecked();
		// edit_schemanticinline = dlg.getUI()->scemanticInlining->isChecked();
		gl_blackedges = dlg.getUI()->blackEdges->isChecked();
		gl_maincolor_alpha = ((double) dlg.getUI()->mainColorAlpha->value()) / 10.0f;
		gl_linethickness = dlg.getUI()->lineThickness->value();
		gui_implicitfiles = dlg.getUI()->implicitFiles->isChecked();
		net_downloadpath = dlg.getUI()->downloadPath->text();
		net_guesspaths = dlg.getUI()->guessNetPaths->isChecked();
		net_autoclose = dlg.getUI()->autoCloseNetPrompt->isChecked();
		gl_logostuds = dlg.getUI()->m_logostuds->isChecked();
		
		if (net_downloadpath.value.right (1) != DIRSLASH)
			net_downloadpath += DIRSLASH;
		
		// Rebuild the quick color toolbar
		g_win->setQuickColors (dlg.quickColors);
		gui_colortoolbar = dlg.quickColorString();
		
		// Set the grid settings
		for (int i = 0; i < g_NumGrids; ++i)
			for (int j = 0; j < 4; ++j)
				g_GridInfo[i].confs[j]->value = dlg.getGridValue (i, j);
		
		// Apply key shortcuts
#define act(N) ACTION(N)->setShortcut (key_##N);
#include "actions.h"
		
		// Ext program settings
		for (const extProgInfo & info : g_extProgInfo) {
			*info.path = info.input->text();
			
#ifndef _WIN32
			*info.wine = info.wineBox->isChecked();
#endif // _WIN32
		}
		
		config::save();
		reloadAllSubfiles();
		loadLogoedStuds();
		g_win->R()->setBackground();
		g_win->fullRefresh();
		g_win->updateToolBars();
		g_win->updateFileList();
	}
}

// =========================================================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =========================================================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =========================================================================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =========================================================================================================================
KeySequenceDialog::KeySequenceDialog (QKeySequence seq, QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f), seq (seq) {
	lb_output = new QLabel;
	IMPLEMENT_DIALOG_BUTTONS
	
	setWhatsThis ("Into this dialog you can input a key sequence for use as a "
		"shortcut in LDForge. Use OK to confirm the new shortcut and Cancel to "
		"dismiss.");
	
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget (lb_output);
	layout->addWidget (bbx_buttons);
	setLayout (layout);

	updateOutput();
}

// =============================================================================
// -----------------------------------------------------------------------------
bool KeySequenceDialog::staticDialog (keyseqconfig* cfg, QWidget* parent) {
	KeySequenceDialog dlg (cfg->value, parent);
	
	if (dlg.exec() == false)
		return false;
	
	cfg->value = dlg.seq;
	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
void KeySequenceDialog::updateOutput() {
	str shortcut = seq.toString();
	
	if (seq == QKeySequence())
		shortcut = "&lt;empty&gt;";
	
	str text = fmt ("<center><b>%1</b></center>", shortcut);
	lb_output->setText (text);
}

// =============================================================================
// -----------------------------------------------------------------------------
void KeySequenceDialog::keyPressEvent (QKeyEvent* ev) {
	seq = ev->key() + ev->modifiers();
	updateOutput();
}