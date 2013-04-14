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

#ifndef ZZ_HISTORYDIALOG_H
#define ZZ_HISTORYDIALOG_H

#include <qdialog.h>
#include <qdialogbuttonbox.h>
#include <qlistwidget.h>
#include <qpushbutton.h>
#include "gui.h"

class HistoryDialog : public QDialog {
	Q_OBJECT
	
public:
	explicit HistoryDialog (QWidget* parent = null, Qt::WindowFlags f = 0);
	void populateList ();
	
	QListWidget* qHistoryList;
	QPushButton* qUndoButton, *qRedoButton, *qClearButton;
	QDialogButtonBox* qButtons;
	
private:
	void updateButtons ();
	void updateSelection ();
	
private slots:
	void slot_undo ();
	void slot_redo ();
	void slot_clear ();
	void slot_selChanged ();
};

#endif // ZZ_HISTORYDIALOG_H