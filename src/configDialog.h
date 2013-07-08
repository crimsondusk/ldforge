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
#include <QDialog>

class Ui_ConfigUI;
class QLabel;
class QDoubleSpinBox;

// =============================================================================
class ShortcutListItem : public QListWidgetItem
{
public:
	explicit ShortcutListItem( QListWidget* view = null, int type = Type ) :
		QListWidgetItem( view, type ) {}
	
	actionmeta* getActionInfo() const
	{
		return m_info;
	}
	
	void setActionInfo( actionmeta* info )
	{
		m_info = info;
	}
	
private:
	actionmeta* m_info;
};

// =============================================================================
class ConfigDialog : public QDialog
{
	Q_OBJECT
	
public:
	ConfigDialog( ForgeWindow* parent );
	virtual ~ConfigDialog();
	static void staticDialog();
	const Ui_ConfigUI* getUI() const;
	float getGridValue( int i, int j ) const;
	
	vector<quickColor> quickColorMeta;
	QDoubleSpinBox* dsb_gridData[3][4];

private:
	Ui_ConfigUI* ui;
	QLabel* lb_gridLabels[3];
	QLabel* lb_gridIcons[3];
	vector<QListWidgetItem*> quickColorItems;
	
	void initMainTab();
	void initShortcutsTab();
	void initQuickColorTab();
	void initGridTab();
	void initExtProgTab();
	void setButtonBackground( QPushButton* button, str value );
	void pickColor( strconfig& cfg, QPushButton* button );
	void updateQuickColorList( quickColor* sel = null );
	void setShortcutText( QListWidgetItem* qItem, actionmeta meta );
	int getItemRow( QListWidgetItem* item, vector<QListWidgetItem*>& haystack );
	str makeColorToolBarString();
	QListWidgetItem* getSelectedQuickColor();
	QList<ShortcutListItem*> getShortcutSelection();

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
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class KeySequenceDialog : public QDialog
{
	Q_OBJECT

public:
	explicit KeySequenceDialog( QKeySequence seq, QWidget* parent = null, Qt::WindowFlags f = 0 );
	static bool staticDialog( actionmeta& meta, QWidget* parent = null );

	QLabel* lb_output;
	QDialogButtonBox* bbx_buttons;
	QKeySequence seq;

private:
	void updateOutput();

private slots:
	virtual void keyPressEvent( QKeyEvent* ev ) override;
};

#endif // CONFIGDIALOG_H
