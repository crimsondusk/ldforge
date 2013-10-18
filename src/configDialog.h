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

#ifndef LDFORGE_CONFIGDIALOG_H
#define LDFORGE_CONFIGDIALOG_H

#include "gui.h"
#include <QDialog>

class Ui_ConfigUI;
class QLabel;
class QDoubleSpinBox;

// =============================================================================
class ShortcutListItem : public QListWidgetItem
{	PROPERTY (KeySequenceConfig*, keyConfig, setKeyConfig)
	PROPERTY (QAction*, action, setAction)

	public:
		explicit ShortcutListItem (QListWidget* view = null, int type = Type) :
			QListWidgetItem (view, type) {}
};

// =============================================================================
class ConfigDialog : public QDialog
{	Q_OBJECT

	public:
		enum Tab
		{	InterfaceTab,
			ProfileTab,
			ShortcutsTab,
			QuickColorsTab,
			GridsTab,
			ExtProgsTab,
			DownloadTab
		};

		explicit ConfigDialog (Tab deftab = InterfaceTab, QWidget* parent = null, Qt::WindowFlags f = 0);
		virtual ~ConfigDialog();
		float getGridValue (int i, int j) const;

		QList<LDQuickColor> quickColors;
		QDoubleSpinBox* dsb_gridData[3][4];

	private:
		Ui_ConfigUI* ui;
		QLabel* lb_gridLabels[3];
		QLabel* lb_gridIcons[3];
		QList<QListWidgetItem*> quickColorItems;

		void applySettings();
		void addShortcut (KeySequenceConfig& cfg, QAction* act, int& i);
		void setButtonBackground (QPushButton* button, str value);
		void pickColor (StringConfig& cfg, QPushButton* button);
		void updateQuickColorList (LDQuickColor* sel = null);
		void setShortcutText (ShortcutListItem* item);
		int getItemRow (QListWidgetItem* item, QList<QListWidgetItem*>& haystack);
		str quickColorString();
		QListWidgetItem* getSelectedQuickColor();
		QList<ShortcutListItem*> getShortcutSelection();
		void initGrids();
		void initExtProgs();

	private slots:
		void slot_setGLBackground();
		void slot_setGLForeground();
		void slot_setShortcut();
		void slot_resetShortcut();
		void slot_clearShortcut();
		void slot_setColor();
		void slot_delColor();
		void slot_addColorSeparator();
		void slot_moveColor();
		void slot_clearColors();
		void slot_setExtProgPath();
		void slot_findDownloadFolder();
		void buttonClicked (QAbstractButton* button);
		void selectPage (int row);
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class KeySequenceDialog : public QDialog
{	Q_OBJECT

	public:
		explicit KeySequenceDialog (QKeySequence seq, QWidget* parent = null, Qt::WindowFlags f = 0);
		static bool staticDialog (KeySequenceConfig* cfg, QWidget* parent = null);

		QLabel* lb_output;
		QDialogButtonBox* bbx_buttons;
		QKeySequence seq;

	private:
		void updateOutput();

	private slots:
		virtual void keyPressEvent (QKeyEvent* ev) override;
};

#endif // LDFORGE_CONFIGDIALOG_H
